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

#include "client.h"
#include "cl_cgameapi.h"
#include "cl_uiapi.h"
/*

key up events are sent even if in console mode

*/

// console
field_t		g_consoleField;
int			nextHistoryLine;	// the last line in the history buffer, not masked
int			historyLine;		// the line being displayed from history buffer will be <= nextHistoryLine
field_t		historyEditLines[COMMAND_HISTORY];

// chat
field_t		chatField;
qboolean	chat_team;
int			chat_playerNum;

keyGlobals_t	kg;										

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
keyname_t keynames[K_LAST_KEY] =							
{			
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},

	{"BACKSPACE", K_BACKSPACE},

	{"COMMAND", K_COMMAND},
	{"CAPSLOCK", K_CAPSLOCK},
	{"POWER", K_POWER},
	{"PAUSE", K_PAUSE},

	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},
	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},
	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"SCROLL", K_SCROLL},

	{"KP_HOME", K_KP_HOME },
	{"KP_UPARROW", K_KP_UPARROW },
	{"KP_PGUP", K_KP_PGUP },
	{"KP_LEFTARROW", K_KP_LEFTARROW },
	{"KP_5", K_KP_5 },
	{"KP_RIGHTARROW", K_KP_RIGHTARROW },
	{"KP_END", K_KP_END },
	{"KP_DOWNARROW", K_KP_DOWNARROW },
	{"KP_PGDN", K_KP_PGDN },
	{"KP_ENTER",  K_KP_ENTER },
	{"KP_INS", K_KP_INS },
	{"KP_DEL", K_KP_DEL },
	{"KP_SLASH", K_KP_SLASH },
	{"KP_MINUS", K_KP_MINUS },
	{"KP_PLUS", K_KP_PLUS },
	{"KP_NUMLOCK", K_KP_NUMLOCK },
	{"KP_STAR", K_KP_STAR },
	{"KP_EQUALS", K_KP_EQUALS },
	
	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"MWHEELUP",    K_MWHEELUP },
	{"MWHEELDOWN",  K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"SEMICOLON", ';'},     // because a raw semicolon seperates commands

	{NULL, 0}
};
static const size_t numKeynames = ARRAY_LEN( keynames );



/*
=============================================================================

EDIT FIELDS

=============================================================================
*/


/*
===================
Field_Draw

Handles horizontal scrolling and cursor blinking
x, y, amd width are in pixels
===================
*/
void Field_VariableSizeDraw( field_t *edit, int x, int y, int width, int size, qboolean showCursor, qboolean noColorEscape ) {
	int		len;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_STRING_CHARS];
	int		i;

	drawLen = edit->widthInChars - 1; // - 1 so there is always a space for the cursor
	len = strlen( edit->buffer );

	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else {
		if ( edit->scroll + drawLen > len ) {
			edit->scroll = len - drawLen;
			if ( edit->scroll < 0 ) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	if ( drawLen < 0 )
		return;

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_STRING_CHARS ) {
		Com_Error( ERR_DROP, "drawLen >= MAX_STRING_CHARS" );
	}

	Com_Memcpy( str, edit->buffer + prestep, drawLen );
	str[ drawLen ] = 0;

	// draw it
	if ( size == SMALLCHAR_WIDTH ) {
		float	color[4];

		color[0] = color[1] = color[2] = color[3] = 1.0;
		SCR_DrawSmallStringExt( x, y, str, color, qfalse, noColorEscape );
	} else {
		// draw big string with drop shadow
		SCR_DrawBigString( x, y, str, 1.0, noColorEscape );
	}

	// draw the cursor
	if ( showCursor ) {
		if ( (int)( cls.realtime >> 8 ) & 1 ) {
			return;		// off blink
		}

		if ( kg.key_overstrikeMode ) {
			cursorChar = 11;
		} else {
			cursorChar = 10;
		}

		i = drawLen - strlen( str );

		if ( size == SMALLCHAR_WIDTH ) {
			SCR_DrawSmallChar( x + ( edit->cursor - prestep - i ) * size, y, cursorChar );
		} else {
			str[0] = cursorChar;
			str[1] = 0;
			SCR_DrawBigString( x + ( edit->cursor - prestep - i ) * size, y, str, 1.0, qfalse );
		}
	}
}

void Field_Draw( field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape )
{
	Field_VariableSizeDraw( edit, x, y, width, SMALLCHAR_WIDTH, showCursor, noColorEscape );
}

void Field_BigDraw( field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape )
{
	Field_VariableSizeDraw( edit, x, y, width, GIANTCHAR_HEIGHT/2, showCursor, noColorEscape );
}

/*
================
Field_Paste
================
*/
void Field_Paste( field_t *edit ) {
	char	*cbd, *c;

	c = cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		return;
	}

	// send as if typed, so insert / overstrike works properly
	while( *c )
	{
		uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
		Field_CharEvent( edit, ConvertUTF32ToExpectedCharset( utf32 ) );
	}

	Z_Free( cbd );
}

/*
=================
Field_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void Field_KeyDownEvent( field_t *edit, int key ) {
	int		len;

	// shift-insert is paste
	if ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && kg.keys[K_SHIFT].down ) {
		Field_Paste( edit );
		return;
	}

	//key = tolower( key );
	len = strlen( edit->buffer );

	switch ( key ) {
		case K_DEL:
			if ( edit->cursor < len ) {
				memmove( edit->buffer + edit->cursor,
					edit->buffer + edit->cursor + 1, len - edit->cursor );
			}
			break;

		case K_RIGHTARROW:
			if ( edit->cursor < len ) {
				edit->cursor++;
			}
			break;

		case K_LEFTARROW:
			if ( edit->cursor > 0 ) {
				edit->cursor--;
			}
			break;

		case K_HOME:
			edit->cursor = 0;
			break;

		case K_END:
			edit->cursor = len;
			break;

		case K_INS:
			kg.key_overstrikeMode = (qboolean)!kg.key_overstrikeMode;
			break;

		default:
			break;
 	}

	// Change scroll if cursor is no longer visible
	if ( edit->cursor < edit->scroll ) {
		edit->scroll = edit->cursor;
	} else if ( edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len ) {
		edit->scroll = edit->cursor - edit->widthInChars + 1;
 	}
}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent( field_t *edit, int ch ) {
	int		len;

	if ( ch == 'v' - 'a' + 1 ) {	// ctrl-v is paste
		Field_Paste( edit );
		return;
	}

	if ( ch == 'c' - 'a' + 1 ) {	// ctrl-c clears the field
		Field_Clear( edit );
		return;
	}

	len = strlen( edit->buffer );

	if ( ch == 'h' - 'a' + 1 )	{	// ctrl-h is backspace
		if ( edit->cursor > 0 ) {
			memmove( edit->buffer + edit->cursor - 1,
				edit->buffer + edit->cursor, len + 1 - edit->cursor );
			edit->cursor--;
			if ( edit->cursor < edit->scroll )
			{
				edit->scroll--;
			}
		}
		return;
	}

	if ( ch == 'a' - 'a' + 1 ) {	// ctrl-a is home
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( ch == 'e' - 'a' + 1 ) {	// ctrl-e is end
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( ch < 32 ) {
		return;
	}

	if ( kg.key_overstrikeMode ) {
		// - 2 to leave room for the leading slash and trailing \0
		if ( edit->cursor == MAX_EDIT_LINE - 2 )
			return;
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	} else {	// insert mode
		// - 2 to leave room for the leading slash and trailing \0
		if ( len == MAX_EDIT_LINE - 2 ) {
			return; // all full
		}
		memmove( edit->buffer + edit->cursor + 1,
			edit->buffer + edit->cursor, len + 1 - edit->cursor );
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}

	if ( edit->cursor >= edit->widthInChars ) {
		edit->scroll++;
	}

	if ( edit->cursor == len + 1) {
		edit->buffer[edit->cursor] = 0;
	}
}

/*
====================
Console_Key

Handles history and console scrollback
====================
*/
void Console_Key( int key ) {
	// ctrl-L clears screen
	if ( tolower(key) == 'l' && kg.keys[K_CTRL].down ) {
		Cbuf_AddText( "clear\n" );
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER ) {
		// legacy hack: strip any prepended slashes. they're not necessary anymore
		if ( g_consoleField.buffer[0] &&
			(g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/') ) {
			char temp[MAX_EDIT_LINE-1];

			Q_strncpyz( temp, g_consoleField.buffer+1, sizeof( temp ) );
			Com_sprintf( g_consoleField.buffer, sizeof( g_consoleField.buffer ), "%s", temp );
			g_consoleField.cursor--;
		}
	//	else
	//		Field_AutoComplete( &g_consoleField );

		// print executed command
		Com_Printf( "%c%s\n", CONSOLE_PROMPT_CHAR, g_consoleField.buffer );

		// valid command
		Cbuf_AddText( g_consoleField.buffer );
		Cbuf_AddText( "\n" );

		if (!g_consoleField.buffer[0])
		{
			return; // empty lines just scroll the console without adding to history
		}

		// copy line to history buffer
		historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
		nextHistoryLine++;
		historyLine = nextHistoryLine;

		Field_Clear( &g_consoleField );

		g_consoleField.widthInChars = g_console_field_width;

	//	CL_SaveConsoleHistory();

		if ( cls.state == CA_DISCONNECTED )
			SCR_UpdateScreen ();	// force an update, because the command may take some time

		return;
	}

	// tab completion
	if ( key == K_TAB ) {
		Field_AutoComplete( &g_consoleField );
		return;
	}

	// history scrolling
	if ( key == K_UPARROW || key == K_KP_UPARROW
		|| (kg.keys[K_SHIFT].down && key == K_MWHEELUP)
		|| (kg.keys[K_CTRL].down && tolower(key) == 'p') )
	{// scroll up: arrow-up, numpad-up, shift + mwheelup, ctrl + p
		if ( nextHistoryLine - historyLine < COMMAND_HISTORY && historyLine > 0 )
			historyLine--;
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];

		return;
	}

	if ( key == K_DOWNARROW || key == K_KP_DOWNARROW
		|| (kg.keys[K_SHIFT].down && key == K_MWHEELDOWN)
		|| (kg.keys[K_CTRL].down && tolower(key) == 'n') )
	{// scroll down: arrow-down, numpad-down, shift + mwheeldown, ctrl + n
		historyLine++;
		if (historyLine >= nextHistoryLine) {
			historyLine = nextHistoryLine;
			Field_Clear( &g_consoleField );
			g_consoleField.widthInChars = g_console_field_width;
			return;
		}
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
		return;
	}

	// console scrolling (ctrl to scroll fast)
	if ( key == K_PGUP || key == K_MWHEELUP ) {
		int count = kg.keys[K_CTRL].down ? 5 : 1;
		for ( int i=0; i<count; i++ )
			Con_PageUp();
		return;
	}

	if ( key == K_PGDN || key == K_MWHEELDOWN ) {
		int count = kg.keys[K_CTRL].down ? 5 : 1;
		for ( int i=0; i<count; i++ )
			Con_PageDown();
		return;
	}

	// ctrl-home = top of console
	if ( key == K_HOME && kg.keys[K_CTRL].down ) {
		Con_Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( key == K_END && kg.keys[K_CTRL].down ) {
		Con_Bottom();
		return;
	}

	// pass to the normal editline routine
	Field_KeyDownEvent( &g_consoleField, key );
}

//============================================================================


/*
================
Message_Key

In game talk message
================
*/
void Message_Key( int key ) {
	char buffer[MAX_STRING_CHARS] = {0};

	if ( key == K_ESCAPE ) {
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MESSAGE );
		Field_Clear( &chatField );
		return;
	}

	if ( key == K_ENTER || key == K_KP_ENTER ) {
		if ( chatField.buffer[0] && cls.state == CA_ACTIVE ) {
				 if ( chat_playerNum != -1 )	Com_sprintf( buffer, sizeof( buffer ), "tell %i \"%s\"\n", chat_playerNum, chatField.buffer );
			else if ( chat_team )				Com_sprintf( buffer, sizeof( buffer ), "say_team \"%s\"\n", chatField.buffer );
			else								Com_sprintf( buffer, sizeof( buffer ), "say \"%s\"\n", chatField.buffer );

			CL_AddReliableCommand( buffer, qfalse );
		}
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MESSAGE );
		Field_Clear( &chatField );
		return;
	}

	Field_KeyDownEvent( &chatField, key );
}

//============================================================================


qboolean Key_GetOverstrikeMode( void ) {
	return kg.key_overstrikeMode;
}


void Key_SetOverstrikeMode( qboolean state ) {
	kg.key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown( int keynum ) {
	if ( keynum < 0 || keynum >= K_LAST_KEY )
		return qfalse;

	return kg.keys[keynum].down;
}


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers
to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum( char *str ) {
	keyname_t	*kn;
	
	if ( !str || !str[0] ) 
	{
		return -1;
	}
	// If single char bind, presume ascii char bind
	if ( !str[1] ) 
	{
		return str[0];
	}

	// scan for a text match
	for ( kn=keynames ; kn->name ; kn++ ) 
	{
		if ( !Q_stricmp( str, kn->name ) )
		{
			return kn->keynum;
		}
	}

	// check for hex code
	if ( strlen( str ) == 4 ) {
		int n = Com_HexStrToInt( str );

		if ( n >= 0 )
			return n;
	}

	return -1;
}


static char tinyString[16];
static const char *Key_KeynumValid( int keynum )
{
	if ( keynum == -1 ) 
	{
		return "<KEY NOT FOUND>";
	}
	if ( keynum < 0 || keynum >= K_LAST_KEY ) 
	{
		return "<OUT OF RANGE>";
	}
	return NULL;
}

static const char *Key_KeyToHex( int keynum ) {
	int i = keynum >> 4;
	int j = keynum & 15;

	tinyString[0] = '0';
	tinyString[1] = 'x';
	tinyString[2] = i > 9 ? (i - 10 + 'A') : (i + '0');
	tinyString[3] = j > 9 ? (j - 10 + 'A') : (j + '0');
	tinyString[4] = '\0';

	return tinyString;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
// Returns a console/config file friendly name for the key
const char *Key_KeynumToString( int keynum ) 
{
	keyname_t   *kn;
	const char	*name;

	name = Key_KeynumValid(keynum);

	// check for printable ascii
	if ( !name && keynum > 32 && keynum < 127) 
	{
		tinyString[0] = keynum;
		tinyString[1] = 0;
		return tinyString;
	}
	// Check for friendly name
	if ( !name )
	{
		for ( kn=keynames ; kn->name ; kn++ ) {
			if (keynum == kn->keynum) {
				name = kn->name;
				break;
			}
		}
	}
	// Fallback to hex number
	if ( !name )
		name = Key_KeyToHex( keynum );

	return name;
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding( int keynum, const char *binding ) {
	if ( keynum < 0 || keynum >= K_LAST_KEY )
		return;

	// free old bindings
	if ( kg.keys[keynum].binding ) {
		Z_Free( kg.keys[keynum].binding );
		kg.keys[keynum].binding = NULL;
	}

	// allocate memory for new binding
	if ( binding )
		kg.keys[keynum].binding = CopyString( binding );

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
===================
Key_GetBinding
===================
*/
char *Key_GetBinding( int keynum ) {
	if ( keynum < 0 || keynum >= K_LAST_KEY )
		return "";

	return kg.keys[keynum].binding;
}

/*
===================
Key_GetKey
===================
*/
int Key_GetKey( const char *binding ) {
	if ( binding ) {
		for ( int i=0; i<K_LAST_KEY; i++ ) {
			if ( kg.keys[i].binding && !Q_stricmp( binding, kg.keys[i].binding ) )
				return i;
		}
	}

	return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f( void ) {
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "unbind <key> : remove commands from a key\n" );
		return;
	}

	int b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if ( b == -1 ) {
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( b, "" );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f( void ) {
	for (int i = 0; i < K_LAST_KEY ; i++) {
		if ( kg.keys[i].binding )
			Key_SetBinding( i, "" );
	}
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f( void ) {
	int c = Cmd_Argc();

	if ( c < 2 ) {
		Com_Printf( "bind <key> [command] : attach a command to a key\n" );
		return;
	}

	int b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if ( b == -1 ) {
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	if ( c == 2 ) {
		if ( kg.keys[b].binding && kg.keys[b].binding[0] )
			Com_Printf( S_COLOR_GREY "Bind " S_COLOR_WHITE "%s = " S_COLOR_GREY "\"" S_COLOR_WHITE "%s" S_COLOR_GREY "\"" S_COLOR_WHITE "\n", Key_KeynumToString( b ), kg.keys[b].binding );
		else
			Com_Printf( "\"%s\" is not bound\n", Key_KeynumToString( b ) );
		return;
	}

	Key_SetBinding( b, Cmd_ArgsFrom( 2 ) );
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( fileHandle_t f ) {
	FS_Printf( f, "unbindall\n" );
	for (int i=0 ; i<K_LAST_KEY ; i++) {
		if ( kg.keys[i].binding && kg.keys[i].binding[0] ) {
			const char *name = Key_KeynumToString( i );

			// handle the escape character nicely
			if ( !strcmp( name, "\\" ) )
				FS_Printf( f, "bind \"\\\" \"%s\"\n", kg.keys[i].binding );
			else
				FS_Printf( f, "bind \"%s\" \"%s\"\n", name, kg.keys[i].binding );
		}
	}
}

/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void ) {
	for ( int i = 0 ; i < K_LAST_KEY ; i++ ) {
		if ( kg.keys[i].binding && kg.keys[i].binding[0] )
			Com_Printf( "Key : %s \"%s\"\n", Key_KeynumToString(i), kg.keys[i].binding );
	}
}

/*
============
Key_KeynameCompletion
============
*/
void Key_KeynameCompletion( callbackFunc_t callback ) {
	for ( size_t i=0; i<numKeynames; i++ ) {
		if ( keynames[i].name )
			callback( keynames[i].name );
	}
}

/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind( char *args, int argNum ) {
	if ( argNum == 2 ) {
		// Skip "unbind "
		char *p = Com_SkipTokens( args, 1, " " );
		if ( p > args )
			Field_CompleteKeyname();
	}
}

/*
====================
Key_CompleteBind
====================
*/
static void Key_CompleteBind( char *args, int argNum ) {
	char *p;

	if ( argNum == 2 ) {
		// Skip "bind "
		p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
			Field_CompleteKeyname();
	}
	else if ( argNum >= 3 ) {
		// Skip "bind <key> "
		p = Com_SkipTokens( args, 2, " " );

		if ( p > args )
			Field_CompleteCommand( p, qtrue, qtrue );
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands( void ) {
	// register our functions
	Cmd_AddCommand( "bind", Key_Bind_f, "Bind a key to a console command" );
	Cmd_SetCommandCompletionFunc( "bind", Key_CompleteBind );
	Cmd_AddCommand( "unbind", Key_Unbind_f, "Unbind a key" );
	Cmd_SetCommandCompletionFunc( "unbind", Key_CompleteUnbind );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f, "Delete all key bindings" );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f, "Show all bindings in the console" );
}

/*
===================
CL_BindUICommand

Returns qtrue if bind command should be executed while user interface is shown
===================
*/
static qboolean CL_BindUICommand( const char *cmd ) {
	if ( Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		return qfalse;

	if ( !Q_stricmp( cmd, "toggleconsole" ) )
		return qtrue;
	if ( !Q_stricmp( cmd, "togglemenu" ) )
		return qtrue;

	return qfalse;
}

/*
===================
CL_ParseBinding

Execute the commands in the bind string
===================
*/
void CL_ParseBinding( int key, qboolean down, unsigned time )
{
	char buf[ MAX_STRING_CHARS ], *p = buf, *end;
	qboolean allCommands, allowUpCmds;

	if( cls.state == CA_DISCONNECTED && Key_GetCatcher( ) == 0 )
		return;
	if( !kg.keys[key].binding || !kg.keys[key].binding[0] )
		return;
	Q_strncpyz( buf, kg.keys[key].binding, sizeof( buf ) );

	// run all bind commands if console, ui, etc aren't reading keys
	allCommands = (qboolean)( Key_GetCatcher( ) == 0 );

	// allow button up commands if in game even if key catcher is set
	allowUpCmds = (qboolean)( cls.state != CA_DISCONNECTED );

	while( 1 )
	{
		while( isspace( *p ) )
			p++;
		end = strchr( p, ';' );
		if( end )
			*end = '\0';
		if( *p == '+' )
		{
			// button commands add keynum and time as parameters
			// so that multiple sources can be discriminated and
			// subframe corrected
			if ( allCommands || ( allowUpCmds && !down ) ) {
				char cmd[1024];
				Com_sprintf( cmd, sizeof( cmd ), "%c%s %d %d\n",
					( down ) ? '+' : '-', p + 1, key, time );
				Cbuf_AddText( cmd );
			}
		}
		else if( down )
		{
			// normal commands only execute on key press
			if ( allCommands || CL_BindUICommand( p ) ) {
				// down-only command
				Cbuf_AddText( p );
				Cbuf_AddText( "\n" );
			}
		}
		if( !end )
			break;
		p = end + 1;
	}
}

/*
===================
CL_KeyDownEvent

Called by CL_KeyEvent to handle a keypress
===================
*/
void CL_KeyDownEvent( int key, unsigned time )
{
	kg.keys[key].down = qtrue;
	kg.keys[key].repeats++;
	if( kg.keys[key].repeats == 1 ) {
		kg.keyDownCount++;
		kg.anykeydown = qtrue;
	}

	if ( cl_allowAltEnter->integer && kg.keys[K_ALT].down && key == K_ENTER )
	{
		Cvar_SetValue( "r_fullscreen", !Cvar_VariableIntegerValue( "r_fullscreen" ) );
		return;
	}

	// console key is hardcoded, so the user can never unbind it
	if ( key == K_CONSOLE || ( kg.keys[K_SHIFT].down && key == K_ESCAPE ) ) {
		Con_ToggleConsole_f();
		Key_ClearStates ();
		return;
	}

	// keys can still be used for bound actions
	if ( cls.state == CA_CINEMATIC && !Key_GetCatcher() ) {
		if ( !com_cameraMode->integer ) {
			Cvar_Set( "nextdemo", "" );
			key = K_ESCAPE;
		}
	}

	// escape is always handled special
	if ( key == K_ESCAPE ) {
		if ( !kg.keys[K_SHIFT].down && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
			Con_ToggleConsole_f();
			Key_ClearStates();
			return;
		}

		if ( Key_GetCatcher() & KEYCATCH_MESSAGE ) {
			// clear message mode
			Message_Key( key );
			return;
		}

		// escape always gets out of CGAME stuff
		if ( Key_GetCatcher() & KEYCATCH_CGAME ) {
			Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
			CGVM_EventHandling( CGAME_EVENT_NONE );
			return;
		}

		if ( !(Key_GetCatcher() & KEYCATCH_UI) ) {
			if ( cls.state == CA_ACTIVE && !clc.demoplaying )
				UIVM_SetActiveMenu( UIMENU_INGAME );
			else {
				CL_Disconnect_f();
				S_StopAllSounds();
				UIVM_SetActiveMenu( UIMENU_MAIN );
			}
			return;
		}

		UIVM_KeyEvent( key, qtrue );
		return;
	}

	// send the bound action
	CL_ParseBinding( key, qtrue, time );

	// distribute the key down event to the appropriate handler
	// console
	if ( Key_GetCatcher() & KEYCATCH_CONSOLE )
		Console_Key( key );
	// ui
	else if ( Key_GetCatcher() & KEYCATCH_UI ) {
		if ( cls.uiStarted )
			UIVM_KeyEvent( key, qtrue );
	}
	// cgame
	else if ( Key_GetCatcher() & KEYCATCH_CGAME ) {
		if ( cls.cgameStarted )
			CGVM_KeyEvent( key, qtrue );
	}
	// chatbox
	else if ( Key_GetCatcher() & KEYCATCH_MESSAGE )
		Message_Key( key );
	// console
	else if ( cls.state == CA_DISCONNECTED )
		Console_Key( key );
}

/*
===================
CL_KeyUpEvent

Called by CL_KeyEvent to handle a keyrelease
===================
*/
void CL_KeyUpEvent( int key, unsigned time )
{
	kg.keys[key].repeats = 0;
	kg.keys[key].down = qfalse;
	kg.keyDownCount--;

	if (kg.keyDownCount <= 0) {
		kg.anykeydown = qfalse;
		kg.keyDownCount = 0;
	}

	// don't process key-up events for the console key
	if ( key == K_CONSOLE || ( key == K_ESCAPE && kg.keys[K_SHIFT].down ) )
		return;

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	CL_ParseBinding( key, qfalse, time );

	if ( Key_GetCatcher( ) & KEYCATCH_UI && cls.uiStarted )
		UIVM_KeyEvent( key, qfalse );
	else if ( Key_GetCatcher( ) & KEYCATCH_CGAME && cls.cgameStarted )
		CGVM_KeyEvent( key, qfalse );
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
void CL_KeyEvent (int key, qboolean down, unsigned time) {
	if( down )
		CL_KeyDownEvent( key, time );
	else
		CL_KeyUpEvent( key, time );
}

/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( int key ) {
	// delete is not a printable character and is otherwise handled by Field_KeyDownEvent
	if ( key == 127 )
		return;

	// distribute the key down event to the appropriate handler
		 if ( Key_GetCatcher() & KEYCATCH_CONSOLE )		Field_CharEvent( &g_consoleField, key );
	else if ( Key_GetCatcher() & KEYCATCH_UI )			UIVM_KeyEvent( key|K_CHAR_FLAG, qtrue );
	else if ( Key_GetCatcher() & KEYCATCH_CGAME )		CGVM_KeyEvent( key|K_CHAR_FLAG, qtrue );
	else if ( Key_GetCatcher() & KEYCATCH_MESSAGE )		Field_CharEvent( &chatField, key );
	else if ( cls.state == CA_DISCONNECTED )			Field_CharEvent( &g_consoleField, key );
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates( void ) {
	kg.anykeydown = qfalse;

	for ( int i=0 ; i < K_LAST_KEY ; i++ ) {
		if ( kg.keys[i].down )
			CL_KeyEvent( i, qfalse, 0 );
		kg.keys[i].down = qfalse;
		kg.keys[i].repeats = 0;
	}
}

static int keyCatchers = 0;

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher( void ) {
	return keyCatchers;
}

/*
====================
Key_SetCatcher
====================
*/
void Key_SetCatcher( int catcher ) {
	// If the catcher state is changing, clear all key states
	if ( catcher != keyCatchers )
		Key_ClearStates();

	keyCatchers = catcher;
}
