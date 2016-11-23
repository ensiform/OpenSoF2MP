/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "qcommon/qcommon.h"
#include "vm_local.h"

vm_t	*currentVM = NULL;
vm_t	*lastVM    = NULL;
int		vm_debugLevel;

// used by Com_Error to get rid of running vm's before longjmp
static int forced_unload;

static const char *vmNames[MAX_VM] = {
	"sof2mp_game",
	"sof2mp_cgame",
	"sof2mp_ui",
	"sof2mp_gametype" //fixme this should be based on the gametype string etc
};

const char *vmStrs[MAX_VM] = {
	"GameVM",
	"CGameVM",
	"UIVM",
	"GametypeVM"
};

// VM slots are automatically allocated by VM_Create, and freed by VM_Free
// The VM table should never be directly accessed from other files.
// Example usage:
//	cgvm = VM_Create( VM_CGAME );	// vmTable[VM_CGAME] is allocated
//	CGVM_Init( foo, bar );			// internally may use VM_Call( cgvm, CGAME_INIT, foo, bar ) for legacy cgame modules
//	cgvm = VM_Restart( cgvm );		// vmTable[VM_CGAME] is recreated, we update the cgvm pointer
//	VM_Free( cgvm );				// vmTable[VM_CGAME] is deallocated and set to NULL
//	cgvm = NULL;					// ...so we update the cgvm pointer

static vm_t vmTable[MAX_VM];

#if 0 // 64bit!
// converts a VM pointer to a C pointer and
// checks to make sure that the range is acceptable
void	*VM_VM2C( vmptr_t p, int length ) {
	return (void *)p;
}
#endif

void VM_Debug( int level ) {
	vm_debugLevel = level;
}

void VM_VmInfo_f( void );
void VM_VmProfile_f( void );

void VM_Init( void ) {
	Cvar_Get( "vm_cgame", "2", CVAR_ARCHIVE );	// !@# SHIP WITH SET TO 2
	Cvar_Get( "vm_game", "2", CVAR_ARCHIVE );	// !@# SHIP WITH SET TO 2
	Cvar_Get( "vm_gametype", "2", CVAR_ARCHIVE );	// !@# SHIP WITH SET TO 2
	Cvar_Get( "vm_ui", "2", CVAR_ARCHIVE );		// !@# SHIP WITH SET TO 2

	Cmd_AddCommand ("vmprofile", VM_VmProfile_f );
	Cmd_AddCommand ("vminfo", VM_VmInfo_f );

	Com_Memset( vmTable, 0, sizeof( vmTable ) );
}


/*
===============
VM_ValueToSymbol

Assumes a program counter value
===============
*/
const char *VM_ValueToSymbol( vm_t *vm, int value ) {
	vmSymbol_t	*sym;
	static char		text[MAX_TOKEN_CHARS];

	sym = vm->symbols;
	if ( !sym ) {
		return "NO SYMBOLS";
	}

	// find the symbol
	while ( sym->next && sym->next->symValue <= value ) {
		sym = sym->next;
	}

	if ( value == sym->symValue ) {
		return sym->symName;
	}

	Com_sprintf( text, sizeof( text ), "%s+%i", sym->symName, value - sym->symValue );

	return text;
}

/*
===============
VM_ValueToFunctionSymbol

For profiling, find the symbol behind this value
===============
*/
vmSymbol_t *VM_ValueToFunctionSymbol( vm_t *vm, int value ) {
	vmSymbol_t	*sym;
	static vmSymbol_t	nullSym;

	sym = vm->symbols;
	if ( !sym ) {
		return &nullSym;
	}

	while ( sym->next && sym->next->symValue <= value ) {
		sym = sym->next;
	}

	return sym;
}


/*
===============
VM_SymbolToValue
===============
*/
int VM_SymbolToValue( vm_t *vm, const char *symbol ) {
	vmSymbol_t	*sym;

	for ( sym = vm->symbols ; sym ; sym = sym->next ) {
		if ( !strcmp( symbol, sym->symName ) ) {
			return sym->symValue;
		}
	}
	return 0;
}


/*
=====================
VM_SymbolForCompiledPointer
=====================
*/
#if 0 // 64bit!
const char *VM_SymbolForCompiledPointer( vm_t *vm, void *code ) {
	int			i;

	if ( code < (void *)vm->codeBase ) {
		return "Before code block";
	}
	if ( code >= (void *)(vm->codeBase + vm->codeLength) ) {
		return "After code block";
	}

	// find which original instruction it is after
	for ( i = 0 ; i < vm->codeLength ; i++ ) {
		if ( (void *)vm->instructionPointers[i] > code ) {
			break;
		}
	}
	i--;

	// now look up the bytecode instruction pointer
	return VM_ValueToSymbol( vm, i );
}
#endif



/*
===============
ParseHex
===============
*/
int	ParseHex( const char *text ) {
	int		value;
	int		c;

	value = 0;
	while ( ( c = *text++ ) != 0 ) {
		if ( c >= '0' && c <= '9' ) {
			value = value * 16 + c - '0';
			continue;
		}
		if ( c >= 'a' && c <= 'f' ) {
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if ( c >= 'A' && c <= 'F' ) {
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

/*
===============
VM_LoadSymbols
===============
*/
void VM_LoadSymbols( vm_t *vm ) {
	union {
		char	*c;
		void	*v;
	} mapfile;
	const char *text_p, *token;
	char	name[MAX_QPATH];
	char	symbols[MAX_QPATH];
	vmSymbol_t	**prev, *sym;
	int		count;
	int		value;
	int		chars;
	int		segment;
	int		numInstructions;

	// don't load symbols if not developer
	if ( !com_developer->integer ) {
		return;
	}

	COM_StripExtension(vm->name, name, sizeof(name));
	Com_sprintf( symbols, sizeof( symbols ), "vm/%s.map", name );
	FS_ReadFile( symbols, &mapfile.v );
	if ( !mapfile.c ) {
		Com_Printf( "Couldn't load symbol file: %s\n", symbols );
		return;
	}

	numInstructions = vm->instructionCount;

	// parse the symbols
	text_p = mapfile.c;
	prev = &vm->symbols;
	count = 0;

	while ( 1 ) {
		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		segment = ParseHex( token );
		if ( segment ) {
			COM_Parse( &text_p );
			COM_Parse( &text_p );
			continue;		// only load code segment values
		}

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			Com_Printf( "WARNING: incomplete line at end of file\n" );
			break;
		}
		value = ParseHex( token );

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			Com_Printf( "WARNING: incomplete line at end of file\n" );
			break;
		}
		chars = strlen( token );
		sym = (vmSymbol_t *)Hunk_Alloc( sizeof( *sym ) + chars, h_high );
		*prev = sym;
		prev = &sym->next;
		sym->next = NULL;

		// convert value from an instruction number to a code offset
		if ( value >= 0 && value < numInstructions ) {
			value = vm->instructionPointers[value];
		}

		sym->symValue = value;
		Q_strncpyz( sym->symName, token, chars + 1 );

		count++;
	}

	vm->numSymbols = count;
	Com_Printf( "%i symbols parsed from %s\n", count, symbols );
	FS_FreeFile( mapfile.v );
}
// The syscall mechanism relies on stack manipulation to get it's args.
// This is likely due to C's inability to pass "..." parameters to a function in one clean chunk.
// On PowerPC Linux, these parameters are not necessarily passed on the stack, so while (&arg[0] == arg) is true,
//	(&arg[1] == 2nd function parameter) is not necessarily accurate, as arg's value might have been stored to the stack
//	or other piece of scratch memory to give it a valid address, but the next parameter might still be sitting in a
//	register.
// QVM's syscall system also assumes that the stack grows downward, and that any needed types can be squeezed, safely,
//	into a signed int.
// This hack below copies all needed values for an argument to an array in memory, so that QVM can get the correct values.
// This can also be used on systems where the stack grows upwards, as the presumably standard and safe stdargs.h macros
//	are used.
// The original code, while probably still inherently dangerous, seems to work well enough for the platforms it already
//	works on. Rather than add the performance hit for those platforms, the original code is still in use there.
// For speed, we just grab 15 arguments, and don't worry about exactly how many the syscall actually needs; the extra is
//	thrown away.
intptr_t QDECL VM_DllSyscall( intptr_t arg, ... ) {
#if !id386 || defined __clang__ || defined MACOS_X
	// rcg010206 - see commentary above
	intptr_t args[MAX_VMSYSCALL_ARGS];
	va_list ap;

	args[0] = arg;

	va_start( ap, arg );
	for (size_t i = 1; i < ARRAY_LEN (args); i++)
		args[i] = va_arg( ap, intptr_t );
	va_end( ap );

	return currentVM->syscall( args );
#else // original id code
	return currentVM->syscall( &arg );
#endif
}

qboolean FS_Which(const char *filename, void *searchPath);
int FS_FindVM(void **startSearch, char *found, int foundlen, const char *name, int enableDll);
long FS_ReadFileDir(const char *qpath, void *searchPath, qboolean unpure, void **buffer);

#define LOCAL_POOL_SIZE 2048000

vmHeader_t *VM_LoadQVM( vm_t *vm, qboolean alloc, qboolean unpure)
{
	int					dataLength;
	int					i;
	char				filename[MAX_QPATH];
	union {
		vmHeader_t	*h;
		void				*v;
	} header;

	// load the image
	Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", vm->name );
	Com_Printf( "Loading vm file %s...\n", filename );

	FS_ReadFileDir(filename, vm->searchPath, unpure, &header.v);

	if ( !header.h ) {
		Com_Printf( "Failed.\n" );
		VM_Free( vm );

		Com_Printf(S_COLOR_YELLOW "Warning: Couldn't open VM file %s\n", filename);

		return NULL;
	}

	// show where the qvm was loaded from
	FS_Which(filename, vm->searchPath);

	if( LittleLong( header.h->vmMagic ) == VM_MAGIC_VER2 ) {
		Com_Printf( "...which has vmMagic VM_MAGIC_VER2\n" );

		// byte swap the header
		for ( i = 0 ; i < sizeof( vmHeader_t ) / 4 ; i++ ) {
			((int *)header.h)[i] = LittleLong( ((int *)header.h)[i] );
		}

		// validate
		if ( header.h->jtrgLength < 0
			|| header.h->bssLength < 0
			|| header.h->dataLength < 0
			|| header.h->litLength < 0
			|| header.h->codeLength <= 0 )
		{
			VM_Free(vm);
			FS_FreeFile(header.v);
			
			Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
			return NULL;
		}
	} else if( LittleLong( header.h->vmMagic ) == VM_MAGIC ) {
		// byte swap the header
		// sizeof( vmHeader_t ) - sizeof( int ) is the 1.32b vm header size
		for ( i = 0 ; i < ( sizeof( vmHeader_t ) - sizeof( int ) ) / 4 ; i++ ) {
			((int *)header.h)[i] = LittleLong( ((int *)header.h)[i] );
		}

		// validate
		if ( header.h->bssLength < 0
			|| header.h->dataLength < 0
			|| header.h->litLength < 0
			|| header.h->codeLength <= 0 )
		{
			VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
			return NULL;
		}
	} else {
		VM_Free( vm );
		FS_FreeFile(header.v);

		Com_Printf(S_COLOR_YELLOW "Warning: %s does not have a recognisable "
				"magic number in its header\n", filename);
		return NULL;
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	vm->localPoolStart = header.h->dataLength + header.h->litLength + header.h->bssLength;
	vm->localPoolSize = 0;
	vm->localPoolTail = LOCAL_POOL_SIZE;
	dataLength = vm->localPoolStart + LOCAL_POOL_SIZE;
	for ( i = 0 ; dataLength > ( 1 << i ) ; i++ ) {
	}
	dataLength = 1 << i;

	if(alloc)
	{
		// allocate zero filled space for initialized and uninitialized data
		vm->dataBase = (byte *)Hunk_Alloc(dataLength, h_high);
		vm->dataMask = dataLength - 1;
	}
	else
	{
		// clear the data, but make sure we're not clearing more than allocated
		if(vm->dataMask + 1 != dataLength)
		{
			VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: Data region size of %s not matching after "
					"VM_Restart()\n", filename);
			return NULL;
		}
		
		Com_Memset(vm->dataBase, 0, dataLength);
	}

	// copy the intialized data
	Com_Memcpy( vm->dataBase, (byte *)header.h + header.h->dataOffset,
		header.h->dataLength + header.h->litLength );

	// byte swap the longs
	for ( i = 0 ; i < header.h->dataLength ; i += 4 ) {
		*(int *)(vm->dataBase + i) = LittleLong( *(int *)(vm->dataBase + i ) );
	}

	if(header.h->vmMagic == VM_MAGIC_VER2)
	{
		int previousNumJumpTableTargets = vm->numJumpTableTargets;

		header.h->jtrgLength &= ~0x03;

		vm->numJumpTableTargets = header.h->jtrgLength >> 2;
		Com_Printf("Loading %d jump table targets\n", vm->numJumpTableTargets);

		if(alloc)
		{
			vm->jumpTableTargets = (byte *)Hunk_Alloc(header.h->jtrgLength, h_high);
		}
		else
		{
			if(vm->numJumpTableTargets != previousNumJumpTableTargets)
			{
				VM_Free(vm);
				FS_FreeFile(header.v);

				Com_Printf(S_COLOR_YELLOW "Warning: Jump table size of %s not matching after "
						"VM_Restart()\n", filename);
				return NULL;
			}

			Com_Memset(vm->jumpTableTargets, 0, header.h->jtrgLength);
		}

		Com_Memcpy(vm->jumpTableTargets, (byte *) header.h + header.h->dataOffset +
				header.h->dataLength + header.h->litLength, header.h->jtrgLength);

		// byte swap the longs
		for ( i = 0 ; i < header.h->jtrgLength ; i += 4 ) {
			*(int *)(vm->jumpTableTargets + i) = LittleLong( *(int *)(vm->jumpTableTargets + i ) );
		}
	}

	return header.h;
}

// Reload the data, but leave everything else in place
// This allows a server to do a map_restart without changing memory allocation

// We need to make sure that servers can access unpure QVMs (not contained in any pak)
// even if the client is pure, so take "unpure" as argument.
vm_t *VM_Restart( vm_t *vm/*, qboolean unpure=qfalse*/ ) {
	// DLL's can't be restarted in place
	if ( vm->dllHandle ) {
		const vm_t saved = *vm;

		VM_Free( vm );

		return VM_Create( saved.slot, saved.syscall, VMI_NATIVE );
	}
	vmHeader_t	*header;
	// load the image
	Com_Printf("VM_Restart()\n");

	if(!(header = VM_LoadQVM(vm, qfalse, qfalse/*unpure*/)))
	{
		Com_Error(ERR_DROP, "VM_Restart failed");
		return NULL;
	}

	// free the original file
	FS_FreeFile(header);

	return vm;
}

static byte * dllLocalPool = 0;

vm_t *VM_Create( vmSlots_t vmSlot, intptr_t( *systemCalls )(intptr_t *), vmInterpret_t interpret ) {
	vm_t *vm = NULL;
	vmHeader_t	*header;
	int			remaining, retval;
	char filename[MAX_OSPATH];
	void *startSearch = NULL;

	if ( !systemCalls ) {
		Com_Error( ERR_FATAL, "VM_Create: bad parms" );
	}
	
	remaining = (Z_MemSize(TAG_HUNK_MARK1)+Z_MemSize(TAG_HUNK_MARK2));

	// see if we already have the VM
	if ( VALIDSTRING( vmTable[vmSlot].name ) )
		return &vmTable[vmSlot];

	// find a free vm
	vm = &vmTable[vmSlot];

	// initialise it
	vm->slot = vmSlot;
	if ( vm->slot != VM_GAMETYPE )
		Q_strncpyz( vm->name, vmNames[vmSlot], sizeof(vm->name) );
	else
		Com_sprintf( vm->name, sizeof(vm->name), "gt_%s", Cvar_VariableString("g_gametype") );

	do
	{
		retval = FS_FindVM(&startSearch, filename, sizeof(filename), vm->name, (interpret == VMI_NATIVE));
		
		if(retval == VMI_NATIVE)
		{
			Com_Printf("Try loading dll file %s\n", filename);

			vm->dllHandle = Sys_LoadGameDll(filename, &vm->main, VM_DllSyscall);
			
			if(vm->dllHandle)
			{
				vm->syscall = systemCalls;
				// allocate memory for local allocs
				vm->localPoolStart = 0;
				vm->localPoolSize = 0;
				vm->localPoolTail = LOCAL_POOL_SIZE;
				dllLocalPool = (unsigned char *)Hunk_Alloc(LOCAL_POOL_SIZE, h_high);
				return vm;
			}
			
			Com_Printf("Failed loading dll, trying next\n");
		}
		else if(retval == VMI_COMPILED)
		{
			vm->searchPath = startSearch;
			if((header = VM_LoadQVM(vm, qtrue, qfalse)))
				break;

			// VM_Free overwrites the name on failed load
			if ( vm->slot != VM_GAMETYPE )
				Q_strncpyz( vm->name, vmNames[vmSlot], sizeof(vm->name) );
			else
				Com_sprintf( vm->name, sizeof(vm->name), "gt_%s", Cvar_VariableString("g_gametype") );
		}
	} while(retval >= 0);
	
	if(retval < 0)
		return NULL;

	vm->syscall = systemCalls;

	// allocate space for the jump targets, which will be filled in by the compile/prep functions
	vm->instructionCount = header->instructionCount;
	vm->instructionPointers = (intptr_t *)Hunk_Alloc(vm->instructionCount * sizeof(*vm->instructionPointers), h_high);

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	vm->compiled = qfalse;

#ifdef NO_VM_COMPILED
	if(interpret >= VMI_COMPILED) {
		Com_Printf("Architecture doesn't have a bytecode compiler, using interpreter\n");
		interpret = VMI_BYTECODE;
	}
#else
	if(interpret != VMI_BYTECODE)
	{
		vm->compiled = qtrue;
		VM_Compile( vm, header );
	}
#endif
	// VM_Compile may have reset vm->compiled if compilation failed
	if (!vm->compiled)
	{
		VM_PrepareInterpreter( vm, header );
	}

	// free the original file
	FS_FreeFile( header );

	// load the map file
	VM_LoadSymbols( vm );

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom = vm->programStack - PROGRAM_STACK_SIZE;

	Com_Printf("%s loaded in %d bytes on the hunk\n", vmNames[vmSlot], (Z_MemSize(TAG_HUNK_MARK1)+Z_MemSize(TAG_HUNK_MARK2)) - remaining);

	return vm;
}

void VM_Free( vm_t *vm ) {
	if ( !vm )
		return;

	if(vm->callLevel) {
		if(!forced_unload) {
			Com_Error( ERR_FATAL, "VM_Free(%s) on running vm", vm->name );
			return;
		} else {
			Com_Printf( "forcefully unloading %s vm\n", vm->name );
		}
	}

	if(vm->destroy)
		vm->destroy(vm);

	if ( vm->dllHandle ) {
		Sys_UnloadDll( vm->dllHandle );
		Com_Memset( vm, 0, sizeof( *vm ) );
	}
	Com_Memset( vm, 0, sizeof( *vm ) );

	currentVM = NULL;
	lastVM = NULL;
}

void VM_Clear( void ) {
	for ( int i = 0; i < MAX_VM; i++ )
		VM_Free( &vmTable[i] );

	currentVM = NULL;
}

void *VM_Shift ( void * mem )
{
	//Alright, subtract the database from the memory pointer to get a memory address relative to the VM.
	//When the VM modifies it it should be modifying the same chunk of memory we have allocated in the engine.
	return (void*)((intptr_t)mem - (intptr_t)currentVM->dataBase);
}

/// Local pool allocation mirrored from BG_Local_Alloc and such
void *VM_Local_Alloc ( int size )
{
	if (!currentVM)
	{
		assert(0);
		return NULL;
	}

	currentVM->localPoolSize = ((currentVM->localPoolSize + 0x00000003) & 0xfffffffc);

	if (currentVM->localPoolSize + size > currentVM->localPoolTail)
	{
		Com_Error( ERR_DROP, "VM_Local_Alloc: buffer exceeded tail (%d > %d)", currentVM->localPoolSize + size, currentVM->localPoolTail);
		return 0;
	}

	currentVM->localPoolSize += size;
	
	byte * pool = currentVM->dataBase;
	if (!currentVM->dataBase) {
		pool = dllLocalPool;
	}
	return VM_Shift(&pool[currentVM->localPoolStart + currentVM->localPoolSize - size]);
}

void *VM_Local_AllocUnaligned ( int size )
{
	if (!currentVM)
	{
		assert(0);
		return NULL;
	}

	if (currentVM->localPoolSize + size > currentVM->localPoolTail)
	{
		Com_Error( ERR_DROP, "VM_Local_AllocUnaligned: buffer exceeded tail (%d > %d)", currentVM->localPoolSize + size, currentVM->localPoolTail);
		return 0;
	}

	currentVM->localPoolSize += size;

	byte * pool = currentVM->dataBase;
	if (!currentVM->dataBase) {
		pool = dllLocalPool;
	}
	return VM_Shift(&pool[currentVM->localPoolStart + currentVM->localPoolSize-size]);
}

void *VM_Local_TempAlloc( int size )
{
	if (!currentVM)
	{
		assert(0);
		return NULL;
	}

	size = ((size + 0x00000003) & 0xfffffffc);

	if (currentVM->localPoolTail - size < currentVM->localPoolSize)
	{
		Com_Error( ERR_DROP, "VM_Local_TempAlloc: buffer exceeded head (%d > %d)", currentVM->localPoolTail - size, currentVM->localPoolSize);
		return 0;
	}

	currentVM->localPoolTail -= size;

	byte * pool = currentVM->dataBase;
	if (!currentVM->dataBase) {
		pool = dllLocalPool;
	}
	return VM_Shift(&pool[currentVM->localPoolStart + currentVM->localPoolTail]);
}

void VM_Local_TempFree( int size )
{
	size = ((size + 0x00000003) & 0xfffffffc);

	if (currentVM->localPoolTail+size > LOCAL_POOL_SIZE)
	{
		Com_Error( ERR_DROP, "BG_TempFree: tail greater than size (%d > %d)", currentVM->localPoolTail+size, LOCAL_POOL_SIZE );
	}

	currentVM->localPoolTail += size;
}

char *VM_Local_StringAlloc ( const char *source )
{
	char *dest = (char*)VM_Local_Alloc( strlen ( source ) + 1 );
	char *localDest = (char*)VM_ArgPtr((intptr_t)dest);
	strcpy( localDest, source );
	return dest;
}

void VM_Shifted_Alloc( void **ptr, int size ) {
	void *mem = NULL;

	if ( !currentVM ) {
		assert( 0 );
		*ptr = NULL;
		return;
	}

	mem = Z_Malloc( size + 1, TAG_VM_ALLOCATED, qfalse );
	if ( !mem ) {
		assert( 0 );
		*ptr = NULL;
		return;
	}

	memset( mem, 0, size + 1 );

	//This can happen.. if a free chunk of memory is found before the vm alloc pointer, commonly happens
	//when allocating like 4 bytes or whatever. However it seems to actually be handled which I didn't
	//think it would be.. so hey.
#if 0
	if ((int)mem < (int)currentVM->dataBase)
	{
		assert(!"Unspeakably bad thing has occured (mem ptr < vm base ptr)");
		*ptr = NULL;
		return;
	}
#endif

	//Alright, subtract the database from the memory pointer to get a memory address relative to the VM.
	//When the VM modifies it it should be modifying the same chunk of memory we have allocated in the engine.
	*ptr = (void *)((intptr_t)mem - (intptr_t)currentVM->dataBase);
}

void VM_Shifted_Free( void **ptr ) {
	void *mem = NULL;

	if ( !currentVM ) {
		assert( 0 );
		return;
	}

	//Shift the VM memory pointer back to get the same pointer we initially allocated in real memory space.
	mem = (void *)((intptr_t)currentVM->dataBase + (intptr_t)*ptr);
	if ( !mem ) {
		assert( 0 );
		return;
	}

	Z_Free( mem );
	*ptr = NULL; //go ahead and clear the pointer for the game.
}

void VM_Forced_Unload_Start(void) {
	forced_unload = 1;
}

void VM_Forced_Unload_Done(void) {
	forced_unload = 0;
}

void *VM_ArgPtr( intptr_t intValue ) {
	if ( !intValue )
		return NULL;

	// currentVM is missing on reconnect
	if ( !currentVM )
		return NULL;

	if ( currentVM->main ) {
		return (void *)(currentVM->dataBase + intValue);
	}
	else {
		return (void *)(currentVM->dataBase + (intValue & currentVM->dataMask));
	}
}

void *VM_ExplicitArgPtr( vm_t *vm, intptr_t intValue ) {
	if ( !intValue )
		return NULL;

	// currentVM is missing on reconnect here as well?
	if ( !currentVM )
		return NULL;

	if ( vm->main ) {
		return (void *)(vm->dataBase + intValue);
	}
	else {
		return (void *)(vm->dataBase + (intValue & vm->dataMask));
	}
}

intptr_t QDECL VM_Call( vm_t *vm, int callnum, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 ) {
	if ( !vm || !vm->name[0] ) {
		Com_Error( ERR_FATAL, "VM_Call with NULL vm" );
		return 0;
	}

	VMSwap v( vm );
	lastVM = vm;

	if ( vm_debugLevel ) {
		Com_Printf( "VM_Call( %d )\n", callnum );
	}

	intptr_t r;

	++vm->callLevel;
	// if we have a dll loaded, call it directly
	if ( vm->main ) {
		return vm->main( callnum, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8,
			arg9, arg10, arg11 );
	}
	else
	{
		struct {
			int callnum;
			int args[MAX_VMMAIN_ARGS-1];
		} a;
		a.callnum = callnum;
		{
			a.args[0] = arg0;
			a.args[1] = arg1;
			a.args[2] = arg2;
			a.args[3] = arg3;
			a.args[4] = arg4;
			a.args[5] = arg5;
			a.args[6] = arg6;
			a.args[7] = arg7;
			a.args[8] = arg8;
			a.args[9] = arg9;
			a.args[10] = arg10;
			a.args[11] = arg11;
		}

#ifndef NO_VM_COMPILED
		if ( vm->compiled )
			r = VM_CallCompiled( vm, &a.callnum );
		else
#endif
			r = VM_CallInterpreted( vm, &a.callnum );
	}
	--vm->callLevel;
	return r;
}

//=================================================================

static int QDECL VM_ProfileSort( const void *a, const void *b ) {
	vmSymbol_t	*sa, *sb;

	sa = *(vmSymbol_t **)a;
	sb = *(vmSymbol_t **)b;

	if ( sa->profileCount < sb->profileCount ) {
		return -1;
	}
	if ( sa->profileCount > sb->profileCount ) {
		return 1;
	}
	return 0;
}

/*
==============
VM_VmProfile_f

==============
*/
void VM_VmProfile_f( void ) {
	vm_t		*vm;
	vmSymbol_t	**sorted, *sym;
	int			i;
	double		total;

	if ( !lastVM ) {
		return;
	}

	vm = lastVM;

	if ( !vm->numSymbols ) {
		return;
	}

	sorted = (vmSymbol_t **)Z_Malloc( vm->numSymbols * sizeof( *sorted ), qfalse );
	sorted[0] = vm->symbols;
	total = sorted[0]->profileCount;
	for ( i = 1 ; i < vm->numSymbols ; i++ ) {
		sorted[i] = sorted[i-1]->next;
		total += sorted[i]->profileCount;
	}

	qsort( sorted, vm->numSymbols, sizeof( *sorted ), VM_ProfileSort );

	for ( i = 0 ; i < vm->numSymbols ; i++ ) {
		int		perc;

		sym = sorted[i];

		perc = 100 * (float) sym->profileCount / total;
		Com_Printf( "%2i%% %9i %s\n", perc, sym->profileCount, sym->symName );
		sym->profileCount = 0;
	}

	Com_Printf("    %9.0f total\n", total );

	Z_Free( sorted );
}

/*
==============
VM_VmInfo_f

==============
*/
void VM_VmInfo_f( void ) {
	Com_Printf( "Registered virtual machines:\n" );
	for ( size_t i = 0 ; i < MAX_VM ; i++ ) {
		const vm_t *vm = &vmTable[i];
		if ( !vm->name[0] ) {
			continue;
		}
		Com_Printf( "%s : ", vm->name );
		if ( vm->dllHandle ) {
			Com_Printf( "native\n" );
			continue;
		}
		if ( vm->compiled ) {
			Com_Printf( "compiled on load\n" );
		} else {
			Com_Printf( "interpreted\n" );
		}
		Com_Printf( "    code length : %7i\n", vm->codeLength );
		Com_Printf( "    table length: %7i\n", vm->instructionCount*4 );
		Com_Printf( "    data length : %7i\n", vm->dataMask + 1 );
	}
}

/*
===============
VM_LogSyscalls

Insert calls to this while debugging the vm compiler
===============
*/
void VM_LogSyscalls( int *args ) {
	static	int		callnum;
	static	FILE	*f;

	if ( !f ) {
		f = fopen("syscalls.log", "w" );
	}
	callnum++;
	fprintf(f, "%i: %p (%i) = %i %i %i %i\n", callnum, (void*)(args - (int *)currentVM->dataBase),
		args[0], args[1], args[2], args[3], args[4] );
}

/*
=================
VM_BlockCopy
Executes a block copy operation within currentVM data space
=================
*/

void VM_BlockCopy(unsigned int dest, unsigned int src, size_t n)
{
	unsigned int dataMask = currentVM->dataMask;

	if ((dest & dataMask) != dest
	|| (src & dataMask) != src
	|| ((dest + n) & dataMask) != dest + n
	|| ((src + n) & dataMask) != src + n)
	{
		Com_Error(ERR_DROP, "OP_BLOCK_COPY out of range!");
	}

	Com_Memcpy(currentVM->dataBase + dest, currentVM->dataBase + src, n);
}
