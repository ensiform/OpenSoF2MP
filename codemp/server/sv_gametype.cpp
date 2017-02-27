/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

// sv_gametype.c -- interface to the gametype dll
#include "server.h"
#include "sv_gameapi.h"
#include "sv_gametypeapi.h"

//==============================================

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs( void ) {
	if ( !svs.gameTypeStarted ) {
		return;
	}
	SV_UnbindGameType();
}

/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/

void SV_InitGameProgs( void ) {
	//FIXME these are temp while I make bots run in vm
	extern int	bot_enable;

	cvar_t *var = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	bot_enable = var ? var->integer : 0;

	svs.gameStarted = qtrue;
	SV_BindGameType();

	SV_InitGameType( qfalse );
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
/*qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return qfalse;
	}

	return GVM_ConsoleCommand();
}*/
