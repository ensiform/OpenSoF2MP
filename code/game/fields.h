/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// Filename:-	fields.h
//

#ifndef FIELDS_H
#define FIELDS_H

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP		1
#define MAX_GHOULINST_SIZE	16384

#ifndef FOFS
#define	FOFS(x) ((int)&(((gentity_t *)0)->x))	// usually already defined in qshared.h
#endif
#define	STOFS(x) ((int)&(((spawn_temp_t *)0)->x))
#define	LLOFS(x) ((int)&(((level_locals_t *)0)->x))
#define	CLOFS(x) ((int)&(((gclient_t *)0)->x))
#define NPCOFS(x) ((int)&(((gNPC_t *)0)->x)) 
#define VHOFS(x) ((int)&(((Vehicle_t *)0)->x)) 

//
#define strFOFS(x)	 #x,FOFS(x)
#define	strSTOFS(x)  #x,STOFS(x)
#define	strLLOFS(x)	 #x,LLOFS(x)	
#define	strCLOFS(x)  #x,CLOFS(x)
#define strNPCOFS(x) #x,NPCOFS(x)
#define strVHICOFS(x) #x,VHOFS(x)

typedef enum
{
//	F_INT, 
//	F_SHORT,
//	F_FLOAT,
	F_STRING,		// string
//	F_VECTOR,
	F_NULL,				// A ptr to null out
	F_ITEM,				// Item pointer handling
//	F_MMOVE,			// Mmove pointer handling
	F_GCLIENT,			// Client pointer handling
	F_GENTITY,			// gentity_t ptr handling
	F_BOOLPTR,			// Generic pointer that is recreated later, could be left alone, but clearer if only 0/1 rather than 0/alloc

	F_BEHAVIORSET,		// special scripting string ptr array handler	
	F_ALERTEVENT,		// special handler for alertevent struct in level_locals_t
	F_AIGROUPS,			// some AI grouping stuff of Mike's
	F_ANIMFILESETS,		// animfileset animevent strings

	F_GROUP,
	F_VEHINFO,
	F_IGNORE
} fieldtypeSAVE_t;

typedef struct
{
	char	*psName;
	int		iOffset;
	fieldtypeSAVE_t	eFieldType;
} save_field_t;

#endif	// #ifndef FIELDS_H

//////////////////////// eof //////////////////////////