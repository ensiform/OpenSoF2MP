/*
===========================================================================
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

// cl_uiapi.c  -- client system interaction with client game
#include "client.h"
#include "cl_lan.h"
#include "botlib/botlib.h"
#include "snd_ambient.h"
#include "FXExport.h"
#include "FxUtil.h"

#include "ghoul2/ghoul2_shared.h"

#include "qcommon/GenericParser2.h"

extern IHeapAllocator *G2VertSpaceClient;
extern botlib_export_t *botlib_export;

// ui interface
static vm_t *uivm; // ui vm, valid for legacy and new api

//
// ui vmMain calls
//

int UIVM_GetAPIVersion( void ) {
	return (int)VM_Call( uivm, UI_GETAPIVERSION );
}

void UIVM_Init( qboolean inGameLoad ) {
	VM_Call( uivm, UI_INIT, inGameLoad );
}

void UIVM_Shutdown( void ) {
	VM_Call( uivm, UI_SHUTDOWN );
}

void UIVM_KeyEvent( int key, qboolean down ) {
	VM_Call( uivm, UI_KEY_EVENT, key, down );
}

void UIVM_MouseEvent( int dx, int dy ) {
	VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
}

void UIVM_Refresh( int realtime ) {
	VM_Call( uivm, UI_REFRESH, realtime );
}

qboolean UIVM_IsFullscreen( void ) {
	return (qboolean)VM_Call( uivm, UI_IS_FULLSCREEN );
}

void UIVM_SetActiveMenu( uiMenuCommand_t menu ) {
	VM_Call( uivm, UI_SET_ACTIVE_MENU, menu );
}

void UIVM_CloseAll( void ) {
	VM_Call( uivm, UI_CLOSEALL );
}

qboolean UIVM_ConsoleCommand( int realTime ) {
	return (qboolean)VM_Call( uivm, UI_CONSOLE_COMMAND, realTime );
}

void UIVM_DrawConnectScreen( qboolean overlay ) {
	VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, overlay );
}

void UIVM_DrawLoadingScreen( void ) {
	VM_Call( uivm, UI_DRAW_LOADING_SCREEN );
}

//
// ui syscalls
//

// wrappers and such

static void CL_GetClientState( uiClientState_t *state ) {
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz( state->servername, cls.servername, sizeof( state->servername ) );
	Q_strncpyz( state->updateInfoString, cls.updateInfoString, sizeof( state->updateInfoString ) );
	Q_strncpyz( state->messageString, clc.serverMessage, sizeof( state->messageString ) );
	state->clientNum = cl.snap.ps.clientNum;
}

static void CL_GetGlconfig( glconfig_t *config ) {
	*config = cls.glconfig;
}

static void GetClipboardData( char *buf, int buflen ) {
	char	*cbd, *c;

	c = cbd = Sys_GetClipboardData();
	if ( !cbd ) {
		*buf = 0;
		return;
	}

	for ( int i = 0, end = buflen - 1; *c && i < end; i++ )
	{
		uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
		buf[i] = ConvertUTF32ToExpectedCharset( utf32 );
	}

	Z_Free( cbd );
}

static int GetConfigString(int index, char *buf, int size)
{
	int		offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return qfalse;

	offset = cl.gameState.stringOffsets[index];
	if (!offset) {
		if( size ) {
			buf[0] = 0;
		}
		return qfalse;
	}

	Q_strncpyz( buf, cl.gameState.stringData+offset, size);

	return qtrue;
}

static void Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	char	*value;

	value = Key_GetBinding( keynum );
	if ( value ) {
		Q_strncpyz( buf, value, buflen );
	}
	else {
		*buf = 0;
	}
}

static void Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
	const char *psKeyName = Key_KeynumToString( keynum/*, qtrue */);
	Q_strncpyz( buf, psKeyName, buflen );
}

static void CL_Key_SetCatcher( int catcher ) {
	// Don't allow the ui module to close the console
	Key_SetCatcher( catcher | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
}

// legacy syscall

intptr_t CL_UISystemCalls( intptr_t *args ) {
	switch ( args[0] ) {
		//rww - alright, DO NOT EVER add a GAME/CGAME/UI generic call without adding a trap to match, and
		//all of these traps must be shared and have cases in sv_game, cl_cgame, and cl_ui. They must also
		//all be in the same order, and start at 100.
	case TRAP_MEMSET:
		Com_Memset( VMA(1), args[2], args[3] );
		return 0;

	case TRAP_MEMCPY:
		Com_Memcpy( VMA(1), VMA(2), args[3] );
		return 0;

	case TRAP_STRNCPY:
		strncpy( (char *)VMA(1), (const char *)VMA(2), args[3] );
		return args[1];

	case TRAP_SIN:
		return FloatAsInt( sin( VMF(1) ) );

	case TRAP_COS:
		return FloatAsInt( cos( VMF(1) ) );

	case TRAP_ATAN2:
		return FloatAsInt( atan2( VMF(1), VMF(2) ) );

	case TRAP_SQRT:
		return FloatAsInt( sqrt( VMF(1) ) );

	case TRAP_MATRIXMULTIPLY:
		MatrixMultiply( (vec3_t *)VMA(1), (vec3_t *)VMA(2), (vec3_t *)VMA(3) );
		return 0;

	case TRAP_ANGLEVECTORS:
		AngleVectors( (const float *)VMA(1), (float *)VMA(2), (float *)VMA(3), (float *)VMA(4) );
		return 0;

	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector( (float *)VMA(1), (const float *)VMA(2) );
		return 0;

	case TRAP_FLOOR:
		return FloatAsInt( floor( VMF(1) ) );

	case TRAP_CEIL:
		return FloatAsInt( ceil( VMF(1) ) );

	case TRAP_TESTPRINTINT:
		return 0;

	case TRAP_TESTPRINTFLOAT:
		return 0;

	case TRAP_ACOS:
		return FloatAsInt( Q_acos( VMF(1) ) );

	case TRAP_ASIN:
		return FloatAsInt( Q_asin( VMF(1) ) );

	case UI_ERROR:
		Com_Error( ERR_DROP, "%s", VMA(1) );
		return 0;

	case UI_PRINT:
		Com_Printf( "%s", VMA(1) );
		return 0;

	case UI_MILLISECONDS:
		return Sys_Milliseconds();

	case UI_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *)VMA(1), (const char *)VMA(2), (const char *)VMA(3), args[4], VMF(5), VMF(6) );
		return 0;

	case UI_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *)VMA(1) );
		return 0;

	case UI_CVAR_SET:
		Cvar_VM_Set( (const char *)VMA(1), (const char *)VMA(2), VM_UI );
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt( Cvar_VariableValue( (const char *)VMA(1) ) );

	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( (const char *)VMA(1), (char *)VMA(2), args[3] );
		return 0;

	case UI_CVAR_SETVALUE:
		Cvar_VM_SetValue( (const char *)VMA(1), VMF(2), VM_UI );
		return 0;

	case UI_CVAR_RESET:
		Cvar_Reset( (const char *)VMA(1) );
		return 0;

	case UI_CVAR_CREATE:
		Cvar_Get( (const char *)VMA(1), (const char *)VMA(2), args[3] );
		return 0;

	case UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_ARGC:
		return Cmd_Argc();

	case UI_ARGV:
		Cmd_ArgvBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText( args[1], (const char *)VMA(2) );
		return 0;

	case UI_FS_FOPENFILE:
		return FS_FOpenFileByMode( (const char *)VMA(1), (int *)VMA(2), (fsMode_t)args[3] );

	case UI_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;

	case UI_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;

	case UI_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;

	case UI_FS_GETFILELIST:
		return FS_GetFileList( (const char *)VMA(1), (const char *)VMA(2), (char *)VMA(3), args[4] );

	case UI_R_REGISTERMODEL:
		return re->RegisterModel( (const char *)VMA(1) );

	case UI_R_REGISTERSKIN:
		//SOF2 TODO
		return re->RegisterSkin( (const char *)VMA(1), 0, NULL );

	case UI_R_REGISTERSHADERNOMIP:
		return re->RegisterShaderNoMip( (const char *)VMA(1) );

	case UI_R_CLEARSCENE:
		re->ClearScene();
		return 0;

	case UI_R_ADDREFENTITYTOSCENE:
		re->AddRefEntityToScene( (const refEntity_t *)VMA(1) );
		return 0;

	case UI_R_ADDPOLYTOSCENE:
		//SOF2 TODO
		re->AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), 1 );
		return 0;

	case UI_R_ADDLIGHTTOSCENE:
		re->AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;

	case UI_R_RENDERSCENE:
		re->RenderScene( (const refdef_t *)VMA(1) );
		return 0;

	case UI_R_SETCOLOR:
		re->SetColor( (const float *)VMA(1) );
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		re->DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[10] );
		return 0;

	case UI_R_MODELBOUNDS:
		re->ModelBounds( args[1], (float *)VMA(2), (float *)VMA(3) );
		return 0;

	case UI_UPDATESCREEN:
		if ( args[1] )
		{
			// draw loading screen
			SCR_RenderLoadingScreen();
		}
		else
		{
			SCR_UpdateScreen();
		}
		return 0;

	case UI_CM_LERPTAG:
		re->LerpTag( (orientation_t *)VMA(1), args[2], args[3], args[4], VMF(5), (const char *)VMA(6) );
		return 0;

	case UI_S_REGISTERSOUND:
		return S_RegisterSound( (const char *)VMA(1) );

	case UI_S_STARTLOCALSOUND:
		S_StartLocalSound( args[1], args[2] );
		return 0;

	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_KEY_SETBINDING:
		Key_SetBinding( args[1], (const char *)VMA(2) );
		return 0;

	case UI_KEY_ISDOWN:
		return Key_IsDown( args[1] );

	case UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode( (qboolean)args[1] );
		return 0;

	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case UI_KEY_SETCATCHER:
		CL_Key_SetCatcher( args[1] );
		return 0;

	case UI_GETCLIPBOARDDATA:
		GetClipboardData( (char *)VMA(1), args[2] );
		return 0;

	case UI_GETCLIENTSTATE:
		CL_GetClientState( (uiClientState_t *)VMA(1) );
		return 0;

	case UI_GETGLCONFIG:
		CL_GetGlconfig( (glconfig_t *)VMA(1) );
		return 0;

	case UI_GETCONFIGSTRING:
		return GetConfigString( args[1], (char *)VMA(2), args[3] );

	case UI_NET_AVAILABLE:
		return qtrue;

	case UI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case UI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case UI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (const char *)VMA(2), (const char *)VMA(3));

	case UI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (const char *)VMA(2));
		return 0;

	case UI_LAN_GETPINGQUEUECOUNT:
		return LAN_GetPingQueueCount();

	case UI_LAN_CLEARPING:
		LAN_ClearPing( args[1] );
		return 0;

	case UI_LAN_GETPING:
		LAN_GetPing( args[1], (char *)VMA(2), args[3], (int *)VMA(4) );
		return 0;

	case UI_LAN_GETPINGINFO:
		LAN_GetPingInfo( args[1], (char *)VMA(2), args[3] );
		return 0;

	case UI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case UI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString( args[1], args[2], (char *)VMA(3), args[4] );
		return 0;

	case UI_LAN_GETSERVERINFO:
		LAN_GetServerInfo( args[1], args[2], (char *)VMA(3), args[4] );
		return 0;

	case UI_LAN_GETSERVERPING:
		return LAN_GetServerPing( args[1], args[2] );

	case UI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible( args[1], args[2], (qboolean)args[3] );
		return 0;

	case UI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible( args[1], args[2] );

	case UI_LAN_UPDATEVISIBLEPINGS:
		return LAN_UpdateVisiblePings( args[1] );

	case UI_LAN_RESETPINGS:
		LAN_ResetPings( args[1] );
		return 0;

	case UI_LAN_SERVERSTATUS:
		return LAN_GetServerStatus( (char *)VMA(1), (char *)VMA(2), args[3] );

	case UI_LAN_COMPARESERVERS:
		return LAN_CompareServers( args[1], args[2], args[3], args[4], args[5] );

	case UI_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();

	case UI_R_REGISTERFONT:
		return re->RegisterFont( (const char *)VMA(1) );

	case UI_R_GETTEXTWIDTH:
		return re->Font_StrLenPixels( (const char *)VMA(1), args[2], VMF(3) );


	case UI_R_GETTEXTHEIGHT:
		return re->Font_HeightPixels( args[2], VMF(3) );
		
	case UI_R_DRAWTEXTWITHCURSOR:
		re->Font_DrawString(args[1], args[2], args[3], VMF(4), (vec_t *)VMA(5), (const char *)VMA(6), args[7], args[8], args[9], args[10]);
		return 0;

	case UI_R_DRAWTEXT:
		re->Font_DrawString(args[1], args[2], args[3], VMF(4), (vec_t *)VMA(5), (const char *)VMA(6), args[7], args[8], 0, 0);
		return 0;

	case UI_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( (char *)VMA(1) );

	case UI_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( (const char *)VMA(1) );

	case UI_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );

	case UI_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], (struct pc_token_s *)VMA(2) );

	case UI_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], (char *)VMA(2), (int *)VMA(3) );

	case UI_PC_LOAD_GLOBAL_DEFINES:
		return botlib_export->PC_LoadGlobalDefines ( (char *)VMA(1) );

	case UI_PC_REMOVE_ALL_GLOBAL_DEFINES:
		botlib_export->PC_RemoveAllGlobalDefines ( );
		return 0;

	case UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( (const char *)VMA(1), (const char *)VMA(2), qfalse );
		return 0;

	case UI_REAL_TIME:
		return Com_RealTime( (struct qtime_s *)VMA(1) );

	case UI_CIN_PLAYCINEMATIC:
		Com_DPrintf("UI_CIN_PlayCinematic\n");
		return CIN_PlayCinematic((const char *)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case UI_R_REMAP_SHADER:
		re->RemapShader( (const char *)VMA(1), (const char *)VMA(2), (const char *)VMA(3) );
		return 0;

/*
Ghoul2 Insert Start
*/
		
	case UI_G2_LISTSURFACES:
		re->G2API_ListSurfaces( (CGhoul2Info *) args[1] );
		return 0;

	case UI_G2_LISTBONES:
		re->G2API_ListBones( (CGhoul2Info *) args[1], args[2]);
		return 0;

	case UI_G2_HAVEWEGHOULMODELS:
		return re->G2API_HaveWeGhoul2Models( *(GhoulHandle(args[1])) );

	case UI_G2_SETMODELS:
		re->G2API_SetGhoul2ModelIndexes( *(GhoulHandle(args[1])), (qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

	case UI_G2_GETBOLT:
		return re->G2API_GetBoltMatrix( *(GhoulHandle(args[1])), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	/*case UI_G2_GETBOLT_NOREC:
		re->G2API_BoltMatrixReconstruction( qfalse );//gG2_GBMNoReconstruct = qtrue;
		return re->G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case UI_G2_GETBOLT_NOREC_NOROT:
		//RAZFIXME: cgame reconstructs bolt matrix, why is this different?
		re->G2API_BoltMatrixReconstruction( qfalse );//gG2_GBMNoReconstruct = qtrue;
		re->G2API_BoltMatrixSPMethod( qtrue );//gG2_GBMUseSPMethod = qtrue;
		return re->G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));*/

	case UI_G2_INITGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
#if id386
		return re->G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4],
									  (qhandle_t) args[5], args[6], args[7]);
#else
		return re->G2API_VM_InitGhoul2Model((qhandle_t *)VMA(1), (const char *)VMA(2), args[3], (qhandle_t)args[4],
			(qhandle_t)args[5], args[6], args[7]);
#endif
		//return	re->G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4], (qhandle_t) args[5], args[6], args[7]);

	/*case UI_G2_COLLISIONDETECT:
	case UI_G2_COLLISIONDETECTCACHE:
		return 0; //not supported for ui*/

	case UI_G2_ANGLEOVERRIDE:
		return re->G2API_SetBoneAngles(*(GhoulHandle(args[1])), args[2], (const char *)VMA(3), (float *)VMA(4), args[5], (const Eorientations) args[6], (const Eorientations) args[7], (const Eorientations) args[8], (qhandle_t *)VMA(9), args[10], args[11] );

	case UI_G2_CLEANMODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
#if id386
		re->G2API_CleanGhoul2Models((CGhoul2Info_v **)VMA(1));
#else
		re->G2API_VM_CleanGhoul2Models((qhandle_t *)VMA(1));
#endif
		//re->G2API_CleanGhoul2Models((CGhoul2Info_v **)VMA(1));
		//	re->G2API_CleanGhoul2Models((CGhoul2Info_v **)args[1]);
		return 0;

	case UI_G2_PLAYANIM:
		return re->G2API_SetBoneAnim(*(GhoulHandle(args[1])), args[2], (const char *)VMA(3), args[4], args[5], args[6], VMF(7), args[8], VMF(9), args[10]);

	/*case UI_G2_GETBONEANIM:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[10];

			return re->G2API_GetBoneAnim(g2, modelIndex, (const char*)VMA(2), args[3], (float *)VMA(4), (int *)VMA(5), (int *)VMA(6), (int *)VMA(7), (float *)VMA(8), (int *)VMA(9));
		}

	case UI_G2_GETBONEFRAME:
		{ //rwwFIXMEFIXME: Just make a G2API_GetBoneFrame func too. This is dirty.
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[6];
			int iDontCare1 = 0, iDontCare2 = 0, iDontCare3 = 0;
			float fDontCare1 = 0;

			return re->G2API_GetBoneAnim(g2, modelIndex, (const char*)VMA(2), args[3], (float *)VMA(4), &iDontCare1, &iDontCare2, &iDontCare3, &fDontCare1, (int *)VMA(5));
		}*/

	case UI_G2_GETGLANAME:
		return (intptr_t)re->G2API_GetGLAName(*(GhoulHandle(args[1])), args[2]);
		/*{
			char *point = ((char *)VMA(3));
			char *local;
			local = re->G2API_GetGLAName(*((CGhoul2Info_v *)args[1]), args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}
		return 0;*/

	case UI_G2_COPYGHOUL2INSTANCE:
		return (int)re->G2API_CopyGhoul2Instance(GhoulHandle(args[1]), GhoulHandle(args[2]), args[3]);

	case UI_G2_COPYSPECIFICGHOUL2MODEL:
		re->G2API_CopySpecificG2Model(*(GhoulHandle(args[1])), args[2], *(GhoulHandle(args[3])), args[4]);
		return 0;

	case UI_G2_DUPLICATEGHOUL2INSTANCE:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
#if id386
		re->G2API_DuplicateGhoul2Instance(GhoulHandle(args[1]), (CGhoul2Info_v **)VMA(2));
#else
		re->G2API_VM_DuplicateGhoul2Instance(GhoulHandle(args[1]), (qhandle_t *)VMA(2));
#endif
		//re->G2API_DuplicateGhoul2Instance(*((CGhoul2Info_v *)args[1]), (CGhoul2Info_v **)VMA(2));
		return 0;

	case UI_G2_REMOVEGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
#if id386
		return (int)re->G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);
#else
		return (int)re->G2API_VM_RemoveGhoul2Model((qhandle_t *)VMA(1), args[2]);
#endif
		//return (int)re->G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);
		//return (int)G2API_RemoveGhoul2Model((CGhoul2Info_v **)args[1], args[2]);

	case UI_G2_ADDBOLT:
		return re->G2API_AddBolt(*(GhoulHandle(args[1])), args[2], (const char *)VMA(3));

	case UI_G2_REMOVEBOLT:
		{
			CGhoul2Info_v &g2 = *(GhoulHandle(args[1]));//*((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(g2, (qhandle_t)args[2]);
			return re->G2API_RemoveBolt(ghlinfo, args[3]);
		}

	/*case UI_G2_SETBOLTON:
		re.G2API_SetBoltInfo(*((CGhoul2Info_v *)args[1]), args[2], args[3]);
		return 0;*/

#ifdef _SOF2	
	case UI_G2_ADDSKINGORE:
		re->G2API_AddSkinGore(GhoulHandle(args[1]), *(SSkinGoreData *)VMA(2));
		return 0;
#endif // _SOF2
	/*case UI_G2_SETROOTSURFACE:
		return re.G2API_SetRootSurface(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));*/

	case UI_G2_SETSURFACEONOFF:
		{
			CGhoul2Info_v &g2 = *(GhoulHandle(args[1]));//*((CGhoul2Info_v *)args[1]);
			return re->G2API_SetSurfaceOnOff(g2, args[2], (const char *)VMA(3), args[4]);
		}

		return 0;
	case UI_G2_SETSKIN:
		{
			CGhoul2Info_v &g2 = *(GhoulHandle(args[1]));//*((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(g2, (qhandle_t)args[2]);

			return re->G2API_SetSkin(ghlinfo, args[3], 0);
		}

	case UI_G2_ATTACHG2MODEL:
		{
			CGhoul2Info_v *g2From = GhoulHandle(args[1]);//((CGhoul2Info_v *)args[1]);
			CGhoul2Info_v *g2To = GhoulHandle(args[3]);//((CGhoul2Info_v *)args[3]);

			return re->G2API_AttachG2Model(*g2From, args[2], *g2To, args[4], args[5]);
		}
	case UI_G2_GETANIMFILENAMEINDEX:
		{
			CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(ghoul2, (qhandle_t)args[2]);
			char * srcFilename;
			qboolean retval = re->G2API_GetAnimFileName(ghlinfo, &srcFilename);
			strncpy((char *) VMA(3), srcFilename, MAX_QPATH);
			return (int) retval;
		}

	case UI_G2_REGISTERSKIN:
		return re->RegisterSkin((const char *)VMA(1), args[2], (char *)VMA(3) );
/*
Ghoul2 Insert End
*/

	case UI_GP_PARSE:
		return GP_VM_Parse( (char **) VMA(1), (bool) args[2], (bool) args[3] );
		//return (intptr_t)GP_Parse((char **) VMA(1), (bool) args[2], (bool) args[3]);
	case UI_GP_PARSE_FILE:
		{
			char * data;
			FS_ReadFile((char *) VMA(1), (void **) &data);
			return GP_VM_Parse( &data, (bool) args[2], (bool) args[3] );
			//return (intptr_t)GP_Parse(&data, (bool) args[2], (bool) args[3]);
		}
	case UI_GP_CLEAN:
		GP_VM_Clean( (qhandle_t *)VMA(1) );
		//GP_Clean((TGenericParser2) VMA(1));
		return 0;
	case UI_GP_DELETE:
		GP_VM_Delete( (qhandle_t *)VMA(1) );
		//GP_Delete((TGenericParser2 *) VMA(1));
		return 0;
	case UI_GP_GET_BASE_PARSE_GROUP:
		return (intptr_t)GP_GetBaseParseGroup((TGenericParser2) VMA(1));

	case UI_VM_LOCALALLOC:
		return (intptr_t)VM_Local_Alloc(args[1]);
	case UI_VM_LOCALALLOCUNALIGNED:
		return (intptr_t)VM_Local_AllocUnaligned(args[1]);
	case UI_VM_LOCALTEMPALLOC:
		return (intptr_t)VM_Local_TempAlloc(args[1]);
	case UI_VM_LOCALTEMPFREE:
		VM_Local_TempFree(args[1]);
		return 0;
	case UI_VM_LOCALSTRINGALLOC:
		return (intptr_t)VM_Local_StringAlloc((char *) VMA(1));

	case UI_GET_CDKEY:
		return 0;
	case UI_SET_CDKEY:
		return 0;
	case UI_VERIFY_CDKEY:
		return 1;

	case UI_GPG_GET_NAME:
		return (intptr_t)GPG_GetName((TGPGroup) VMA(1), (char *) VMA(2));
	case UI_GPG_GET_NEXT:
		return (intptr_t)GPG_GetNext((TGPGroup) VMA(1));
	case UI_GPG_GET_INORDER_NEXT:
		return (intptr_t)GPG_GetInOrderNext((TGPGroup) VMA(1));
	case UI_GPG_GET_INORDER_PREVIOUS:
		return (intptr_t)GPG_GetInOrderPrevious((TGPGroup) VMA(1));
	case UI_GPG_GET_PAIRS:
		return (intptr_t)GPG_GetPairs((TGPGroup) VMA(1));
	case UI_GPG_GET_INORDER_PAIRS:
		return (intptr_t)GPG_GetInOrderPairs((TGPGroup) VMA(1));
	case UI_GPG_GET_SUBGROUPS:
		return (intptr_t)GPG_GetSubGroups((TGPGroup) VMA(1));
	case UI_GPG_GET_INORDER_SUBGROUPS:
		return (intptr_t)GPG_GetInOrderSubGroups((TGPGroup) VMA(1));
	case UI_GPG_FIND_SUBGROUP:
		return (intptr_t)GPG_FindSubGroup((TGPGroup) VMA(1), (char *) VMA(2));
	case UI_GPG_FIND_PAIR:
		return (intptr_t)GPG_FindPair((TGPGroup) VMA(1), (const char *) VMA(2));
	case UI_GPG_FIND_PAIRVALUE:
		return (intptr_t)GPG_FindPairValue((TGPGroup) VMA(1), (const char *) VMA(2), (const char *) VMA(3), (char *) VMA(4));
		
	case UI_GPV_GET_NAME:
		return (intptr_t)GPV_GetName((TGPValue) VMA(1), (char *) VMA(2));
	case UI_GPV_GET_NEXT:
		return (intptr_t)GPV_GetNext((TGPValue) VMA(1));
	case UI_GPV_GET_INORDER_NEXT:
		return (intptr_t)GPV_GetInOrderNext((TGPValue) VMA(1));
	case UI_GPV_GET_INORDER_PREVIOUS:
		return (intptr_t)GPV_GetInOrderPrevious((TGPValue) VMA(1));

	case UI_GPV_IS_LIST:
		return (intptr_t)GPV_IsList((TGPValue) VMA(1));
	case UI_GPV_GET_TOP_VALUE:
		{
			const char * topValue = GPV_GetTopValue((TGPValue) VMA(1));
			if (topValue)
			{
				strcpy((char *) VMA(2), topValue);
			}
			return 0;
		}
	case UI_GPV_GET_LIST:
		return (intptr_t)GPV_GetList((TGPValue) VMA(1));

	case UI_PB_ISENABLED:
	case UI_PB_ENABLE:
	case UI_PB_DISABLE:
		return qfalse;

	case UI_GET_TEAM_COUNT:
		//arg1 = int team
		//TODO SOF2
		return 1;
	case UI_GET_TEAM_SCORE:
		//arg1 = int team
		//TODO SOF2
		return 1;

	default:
		Com_Error( ERR_DROP, "Bad UI system trap: %ld", (long int) args[0] );

	}

	return 0;
}

void CL_BindUI( void ) {
	uivm = VM_Create( VM_UI, CL_UISystemCalls, VMI_COMPILED );
	if ( !uivm ) {
		cls.uiStarted = qfalse;
		Com_Error( ERR_DROP, "VM_Create on ui failed" );
	}
}

void CL_UnbindUI( void ) {
	UIVM_Shutdown();
	VM_Free( uivm );
	uivm = NULL;
}
