/*
===========================================================================
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

#include "tr_local.h"

/*
============================================================================

SKINS

============================================================================
*/

static char *CommaParse( char **data_p );
//can't be dec'd here since we need it for non-dedicated builds now as well.

/*
===============
RE_RegisterSkin

===============
*/

bool gServerSkinHack = false;


shader_t *R_FindServerShader( const char *name, const int *lightmapIndex, const byte *styles, qboolean mipRawImage );
static char *CommaParse( char **data_p );

qhandle_t RE_RegisterSkin( const char *name, int numPairs, char *skinPairs ) {
	qhandle_t	hSkin;
	skin_t		*skin;

	if ( !name || !name[0] ) {
		ri->Printf( PRINT_ALL, "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		ri->Printf( PRINT_ALL, "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}

	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri->Printf( PRINT_ALL, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = (struct skin_s *)Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	char surfName[MAX_QPATH];
	char *token;
	for (int i = 0; i < numPairs; ++i) {
		token = CommaParse(&skinPairs);
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		token = CommaParse(&skinPairs);
		skinSurface_t *surf = (skinSurface_t *) Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		skin->surfaces[skin->numSurfaces] = (_skinSurface_t *)surf;

		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );

		if (gServerSkinHack)	surf->shader = R_FindServerShader( token, lightmapsNone, stylesDefault, qtrue );
		else					surf->shader = R_FindShader( token, lightmapsNone, stylesDefault, qtrue );
		skin->numSurfaces++;
	}

	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return(hSkin);
}



/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatible with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *(const unsigned char* /*eurofix*/)data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' )
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				data++;
			}
			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
===============
RE_RegisterServerSkin

Mangled version of the above function to load .skin files on the server.
===============
*/
qhandle_t RE_RegisterServerSkin( const char *name ) {
	qhandle_t r;

	if (ri->Cvar_VariableIntegerValue( "cl_running" ) &&
		ri->Com_TheHunkMarkHasBeenMade() &&
		ShaderHashTableExists())
	{ //If the client is running then we can go straight into the normal registerskin func
		//SOF2 TODO
		return RE_RegisterSkin(name, 0, NULL);
	}

	gServerSkinHack = true;
	//SOF2 TODO
	r = RE_RegisterSkin(name, 0, NULL);
	gServerSkinHack = false;

	return r;
}

/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (struct skin_s *)ri->Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = (_skinSurface_t *)ri->Hunk_Alloc( sizeof( skinSurface_t ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	ri->Printf( PRINT_ALL,  "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri->Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri->Printf( PRINT_ALL, "       %s = %s\n",
				skin->surfaces[j]->name, ((shader_t* )skin->surfaces[j]->shader)->name );
		}
	}
	ri->Printf( PRINT_ALL,  "------------------\n");
}
