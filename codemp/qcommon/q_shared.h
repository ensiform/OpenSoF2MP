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

#pragma once

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#define PRODUCT_NAME			"sof2mp"

#define CLIENT_WINDOW_TITLE "SoF2 MP"
#define CLIENT_CONSOLE_TITLE "Soldier of Fortune 2 Console"
#define HOMEPATH_NAME_UNIX "sof2mp"
#define HOMEPATH_NAME_WIN "OpenSoF2MP"
#define HOMEPATH_NAME_MACOSX HOMEPATH_NAME_WIN

#define	BASEGAME "base"

//NOTENOTE: Only change this to re-point ICARUS to a new script directory
#define Q3_SCRIPT_DIR	"scripts"

#define MAX_TEAMNAME 32
#define MAX_MASTER_SERVERS      5	// number of supported master servers

#define BASE_COMPAT // some unused and leftover code has been stripped out, but this breaks compatibility
					//	between base<->modbase clients and servers (mismatching events, powerups, etc)
					// leave this defined to ensure compatibility

#include "qcommon/q_math.h"
#include "qcommon/q_color.h"
#include "qcommon/q_string.h"
#include "qcommon/disablewarnings.h"

#include "game/teams.h" //npc team stuff

#define MAX_WORLD_COORD		( 64 * 1024 )
#define MIN_WORLD_COORD		( -64 * 1024 )
#define WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

//Pointer safety utilities
#define VALID( a )		( a != NULL )
#define	VALIDATE( a )	( assert( a ) )

#define	VALIDATEV( a )	if ( a == NULL ) {	assert(0);	return;			}
#define	VALIDATEB( a )	if ( a == NULL ) {	assert(0);	return qfalse;	}
#define VALIDATEP( a )	if ( a == NULL ) {	assert(0);	return NULL;	}

#define VALIDSTRING( a )	( ( a != NULL ) && ( a[0] != '\0' ) )
#define VALIDENT( e )		( ( e != NULL ) && ( (e)->inuse ) )

#define ARRAY_LEN( x ) ( sizeof( x ) / sizeof( *(x) ) )
#define STRING( a ) #a
#define XSTRING( a ) STRING( a )
/*
#define G2_EHNANCEMENTS

#ifdef G2_EHNANCEMENTS
//these two will probably explode if they're defined independent of one another.
//rww - RAGDOLL_BEGIN
#define JK2_RAGDOLL
//rww - RAGDOLL_END
//rww - Bone cache for multiplayer base.
#define MP_BONECACHE
#endif
*/

#ifndef FINAL_BUILD
	// may want to enable timing and leak checking again. requires G2API changes.
//	#define G2_PERFORMANCE_ANALYSIS
//	#define _FULL_G2_LEAK_CHECKING
//	extern int g_Ghoul2Allocations;
//	extern int g_G2ServerAlloc;
//	extern int g_G2ClientAlloc;
//	extern int g_G2AllocServer;
#endif

#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>

//Ignore __attribute__ on non-gcc platforms
#if !defined(__GNUC__) && !defined(__attribute__)
	#define __attribute__(x)
#endif

#if defined(__GNUC__)
	#define UNUSED_VAR __attribute__((unused))
#else
	#define UNUSED_VAR
#endif

#if (defined _MSC_VER)
	#define Q_EXPORT __declspec(dllexport)
#elif (defined __SUNPRO_C)
	#define Q_EXPORT __global
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
	#define Q_EXPORT __attribute__((visibility("default")))
#else
	#define Q_EXPORT
#endif

#if defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#endif

// this is the define for determining if we have an asm version of a C function
#if (defined(_M_IX86) || defined(__i386__)) && !defined(__sun__)
	#define id386	1
#else
	#define id386	0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
	#define idppc	1
#else
	#define idppc	0
#endif

#include "qcommon/q_platform.h"

typedef union fileBuffer_u {
	void *v;
	char *c;
	byte *b;
} fileBuffer_t;

typedef int32_t qhandle_t, thandle_t, fxHandle_t, sfxHandle_t, fileHandle_t, clipHandle_t;

#define NULL_HANDLE ((qhandle_t)0)
#define NULL_SOUND ((sfxHandle_t)0)
#define NULL_FX ((fxHandle_t)0)
#define NULL_SFX ((sfxHandle_t)0)
#define NULL_FILE ((fileHandle_t)0)
#define NULL_CLIP ((clipHandle_t)0)

#define PAD(base, alignment)	(((base)+(alignment)-1) & ~((alignment)-1))
#define PADLEN(base, alignment)	(PAD((base), (alignment)) - (base))

#define PADP(base, alignment)	((void *) PAD((intptr_t) (base), (alignment)))

#ifdef __GNUC__
#define QALIGN(x) __attribute__((aligned(x)))
#else
#define QALIGN(x)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define INT_ID( a, b, c, d ) (uint32_t)((((a) & 0xff) << 24) | (((b) & 0xff) << 16) | (((c) & 0xff) << 8) | ((d) & 0xff))

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		  8192
#define	BIG_INFO_VALUE		8192

#define NET_ADDRSTRMAXLEN 48 // maximum length of an IPv6 address string including trailing '\0'

// moved these from ui_local.h so we can access them everywhere
#define MAX_ADDRESSLENGTH		256//64
#define MAX_HOSTNAMELENGTH		256//22
#define MAX_MAPNAMELENGTH		256//16
#define MAX_STATUSLENGTH		256//64

#define	MAX_QPATH			64		// max length of a quake game pathname
#ifdef PATH_MAX
#define MAX_OSPATH			PATH_MAX
#else
#define	MAX_OSPATH			256		// max length of a filesystem pathname
#endif

#define	MAX_NAME_LENGTH		32		// max length of a client name
#define MAX_NETNAME			36

#define	MAX_SAY_TEXT	150

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
} cbufExec_t;


//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility


#define LS_STYLES_START			0
#define LS_NUM_STYLES			32
#define	LS_SWITCH_START			(LS_STYLES_START+LS_NUM_STYLES)
#define LS_NUM_SWITCH			32
#if !defined MAX_LIGHT_STYLES
#define MAX_LIGHT_STYLES		64
#endif

//For system-wide prints
enum WL_e {
	WL_ERROR=1,
	WL_WARNING,
	WL_VERBOSE,
	WL_DEBUG
};

extern float forceSpeedLevels[4];

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER,		// only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;


#ifdef ERR_FATAL
#undef ERR_FATAL			// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_NEED_CD					// pop up the need-cd dialog
} errorParm_t;

#if defined(_DEBUG) && !defined(BSPC)
	#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

void *Hunk_Alloc( int size, ha_pref preference );

#define Com_Memset memset
#define Com_Memcpy memcpy

#define CIN_system	1
#define CIN_loop	2
#define	CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16

//rww - a C-ified structure version of the class which fires off callbacks and gives arguments to update ragdoll status.
enum sharedERagPhase
{
	RP_START_DEATH_ANIM,
	RP_END_DEATH_ANIM,
	RP_DEATH_COLLISION,
	RP_CORPSE_SHOT,
	RP_GET_PELVIS_OFFSET,  // this actually does nothing but set the pelvisAnglesOffset, and pelvisPositionOffset
	RP_SET_PELVIS_OFFSET,  // this actually does nothing but set the pelvisAnglesOffset, and pelvisPositionOffset
	RP_DISABLE_EFFECTORS  // this removes effectors given by the effectorsToTurnOff member
};

enum sharedERagEffector
{
	RE_MODEL_ROOT=			0x00000001, //"model_root"
	RE_PELVIS=				0x00000002, //"pelvis"
	RE_LOWER_LUMBAR=		0x00000004, //"lower_lumbar"
	RE_UPPER_LUMBAR=		0x00000008, //"upper_lumbar"
	RE_THORACIC=			0x00000010, //"thoracic"
	RE_CRANIUM=				0x00000020, //"cranium"
	RE_RHUMEROUS=			0x00000040, //"rhumerus"
	RE_LHUMEROUS=			0x00000080, //"lhumerus"
	RE_RRADIUS=				0x00000100, //"rradius"
	RE_LRADIUS=				0x00000200, //"lradius"
	RE_RFEMURYZ=			0x00000400, //"rfemurYZ"
	RE_LFEMURYZ=			0x00000800, //"lfemurYZ"
	RE_RTIBIA=				0x00001000, //"rtibia"
	RE_LTIBIA=				0x00002000, //"ltibia"
	RE_RHAND=				0x00004000, //"rhand"
	RE_LHAND=				0x00008000, //"lhand"
	RE_RTARSAL=				0x00010000, //"rtarsal"
	RE_LTARSAL=				0x00020000, //"ltarsal"
	RE_RTALUS=				0x00040000, //"rtalus"
	RE_LTALUS=				0x00080000, //"ltalus"
	RE_RRADIUSX=			0x00100000, //"rradiusX"
	RE_LRADIUSX=			0x00200000, //"lradiusX"
	RE_RFEMURX=				0x00400000, //"rfemurX"
	RE_LFEMURX=				0x00800000, //"lfemurX"
	RE_CEYEBROW=			0x01000000 //"ceyebrow"
};

typedef struct sharedRagDollParams_s {
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t pelvisAnglesOffset;    // always set on return, an argument for RP_SET_PELVIS_OFFSET
	vec3_t pelvisPositionOffset; // always set on return, an argument for RP_SET_PELVIS_OFFSET

	float fImpactStrength; //should be applicable when RagPhase is RP_DEATH_COLLISION
	float fShotStrength; //should be applicable for setting velocity of corpse on shot (probably only on RP_CORPSE_SHOT)
	int me; //index of entity giving this update

	//rww - we have convenient animation/frame access in the game, so just send this info over from there.
	int startFrame;
	int endFrame;

	int collisionType; // 1 = from a fall, 0 from effectors, this will be going away soon, hence no enum

	qboolean CallRagDollBegin; // a return value, means that we are now begininng ragdoll and the NPC stuff needs to happen

	int RagPhase;

// effector control, used for RP_DISABLE_EFFECTORS call

	int effectorsToTurnOff;  // set this to an | of the above flags for a RP_DISABLE_EFFECTORS

} sharedRagDollParams_t;

//And one for updating during model animation.
typedef struct sharedRagDollUpdateParams_s {
	vec3_t angles;
	vec3_t position;
	vec3_t scale;
	vec3_t velocity;
	int	me;
	int settleFrame;
} sharedRagDollUpdateParams_t;

//rww - update parms for ik bone stuff
typedef struct sharedIKMoveParams_s {
	char boneName[512]; //name of bone
	vec3_t desiredOrigin; //world coordinate that this bone should be attempting to reach
	vec3_t origin; //world coordinate of the entity who owns the g2 instance that owns the bone
	float movementSpeed; //how fast the bone should move toward the destination
} sharedIKMoveParams_t;


typedef struct sharedSetBoneIKStateParams_s {
	vec3_t pcjMins; //ik joint limit
	vec3_t pcjMaxs; //ik joint limit
	vec3_t origin; //origin of caller
	vec3_t angles; //angles of caller
	vec3_t scale; //scale of caller
	float radius; //bone rad
	int blendTime; //bone blend time
	int pcjOverrides; //override ik bone flags
	int startFrame; //base pose start
	int endFrame; //base pose end
	qboolean forceAnimOnBone; //normally if the bone has specified start/end frames already it will leave it alone.. if this is true, then the animation will be restarted on the bone with the specified frames anyway.
} sharedSetBoneIKStateParams_t;

enum sharedEIKMoveState
{
	IKS_NONE = 0,
	IKS_DYNAMIC
};

//rww - bot stuff that needs to be shared
#define MAX_WPARRAY_SIZE 4096
#define MAX_NEIGHBOR_SIZE 32

#define MAX_NEIGHBOR_LINK_DISTANCE 128
#define MAX_NEIGHBOR_FORCEJUMP_LINK_DISTANCE 400

#define DEFAULT_GRID_SPACING 400

typedef struct wpneighbor_s
{
	int num;
	int forceJumpTo;
} wpneighbor_t;

typedef struct wpobject_s
{
	vec3_t origin;
	int inuse;
	int index;
	float weight;
	float disttonext;
	int flags;
	int associated_entity;

	int forceJumpTo;

	int neighbornum;
	//int neighbors[MAX_NEIGHBOR_SIZE];
	wpneighbor_t neighbors[MAX_NEIGHBOR_SIZE];
} wpobject_t;

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define	SCREEN_WIDTH		640
#define	SCREEN_HEIGHT		480

#define TINYCHAR_WIDTH		(SMALLCHAR_WIDTH)
#define TINYCHAR_HEIGHT		(SMALLCHAR_HEIGHT/2)

#define SMALLCHAR_WIDTH		8
#define SMALLCHAR_HEIGHT	16

#define BIGCHAR_WIDTH		12	// 16
#define BIGCHAR_HEIGHT		13	// 16

#define	GIANTCHAR_WIDTH		32
#define	GIANTCHAR_HEIGHT	48

//=============================================

char	*COM_SkipPath( char *pathname );
const char	*COM_GetExtension( const char *name );
void	COM_StripExtension( const char *in, char *out, int destsize );
qboolean COM_CompareExtension(const char *in, const char *ext);
void	COM_DefaultExtension( char *path, int maxSize, const char *extension );

void	COM_BeginParseSession( const char *name );
int		COM_GetCurrentParseLine( void );
const char	*SkipWhitespace( const char *data, qboolean *hasNewLines );
char	*COM_Parse( const char **data_p );
char	*COM_ParseExt( const char **data_p, qboolean allowLineBreak );
int		COM_Compress( char *data_p );
void	COM_ParseError( char *format, ... );
void	COM_ParseWarning( char *format, ... );
qboolean COM_ParseString( const char **data, const char **s );
qboolean COM_ParseInt( const char **data, int *i );
qboolean COM_ParseFloat( const char **data, float *f );
qboolean COM_ParseVec4( const char **buffer, vec4_t *c);
//int		COM_ParseInfos( char *buf, int max, char infos[][MAX_INFO_STRING] );

#define MAX_TOKENLENGTH		1024

#ifndef TT_STRING
//token types
#define TT_STRING					1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER					3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation
#endif

typedef struct pc_token_s
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void	COM_MatchToken( const char**buf_p, char *match );

qboolean SkipBracedSection (const char **program, int depth);
void SkipRestOfLine ( const char **data );

void Parse1DMatrix (const char **buf_p, int x, float *m);
void Parse2DMatrix (const char **buf_p, int y, int x, float *m);
void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m);
int Com_HexStrToInt( const char *str );

int	QDECL Com_sprintf (char *dest, int size, const char *fmt, ...);

char *Com_SkipTokens( char *s, int numTokens, char *sep );
char *Com_SkipCharset( char *s, char *sep );

void Com_RandomBytes( byte *string, int len );

// mode parm for FS_FOpenFile
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC,
	FS_READ_TEXT,
	FS_WRITE_TEXT,
	FS_APPEND_TEXT,
	FS_APPEND_SYNC_TEXT
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct qint64_s {
	byte	b0;
	byte	b1;
	byte	b2;
	byte	b3;
	byte	b4;
	byte	b5;
	byte	b6;
	byte	b7;
} qint64;

int FloatAsInt( float f );

char	* QDECL va(const char *format, ...);

#define TRUNCATE_LENGTH	64
void Com_TruncateLongString( char *buffer, const char *s );

//=============================================

//
// key / value info strings
//
char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_RemoveKey_Big( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
void Info_SetValueForKey_Big( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );
qboolean Info_NextPair( const char **s, char *key, char *value );

// this is only here so the functions in q_shared.c and bg_*.c can link
#if defined( _GAME ) || defined( _CGAME ) || defined( UI_BUILD )
	void (*Com_Error)( int level, const char *error, ... );
	void (*Com_Printf)( const char *msg, ... );
#else
	void NORETURN QDECL Com_Error( int level, const char *error, ... );
	void QDECL Com_Printf( const char *msg, ... );
#endif


/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when cheats is zero,
	force all unspecified variables to their cefault values.

==========================================================
*/

#define	CVAR_NONE			(0x00000000u)
#define	CVAR_ARCHIVE		(0x00000001u)	// set to cause it to be saved to configuration file. used for system variables,
											//	not for player specific configurations
#define	CVAR_USERINFO		(0x00000002u)	// sent to server on connect or change
#define	CVAR_SERVERINFO		(0x00000004u)	// sent in response to front end requests
#define	CVAR_SYSTEMINFO		(0x00000008u)	// these cvars will be duplicated on all clients
#define	CVAR_INIT			(0x00000010u)	// don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH			(0x00000020u)	// will only change when C code next does a Cvar_Get(), so it can't be changed
											//	without proper initialization. modified will be set, even though the value
											//	hasn't changed yet
#define	CVAR_ROM			(0x00000040u)	// display only, cannot be set by user at all (can be set by code)
#define	CVAR_USER_CREATED	(0x00000080u)	// created by a set command
#define	CVAR_TEMP			(0x00000100u)	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			(0x00000200u)	// can not be changed if cheats are disabled
#define CVAR_NORESTART		(0x00000400u)	// do not clear when a cvar_restart is issued
#define CVAR_INTERNAL		(0x00000800u)	// cvar won't be displayed, ever (for passwords and such)
#define	CVAR_PARENTAL		(0x00001000u)	// lets cvar system know that parental stuff needs to be updated
#define CVAR_SERVER_CREATED	(0x00002000u)	// cvar was created by a server the client connected to.
#define CVAR_VM_CREATED		(0x00004000u)	// cvar was created exclusively in one of the VMs.
#define CVAR_PROTECTED		(0x00008000u)	// prevent modifying this var from VMs or the server
#define CVAR_LOCK_RANGE		(0x00010000u)	// enforces the mins / maxs
// These flags are only returned by the Cvar_Flags() function
#define CVAR_MODIFIED		(0x40000000u)	// Cvar was modified
#define CVAR_NONEXISTENT	(0x80000000u)	// Cvar doesn't exist.

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char			*name;
	char			*description;
	char			*string;
	char			*resetString;		// cvar_restart will reset to this value
	char			*latchedString;		// for CVAR_LATCH vars
	uint32_t		flags;
	qboolean		modified;			// set each time the cvar is changed
	int				modificationCount;	// incremented each time the cvar is changed
	float			value;				// atof( string )
	int				integer;			// atoi( string )
	qboolean		validate;
	qboolean		integral;
	float			min, max;

	struct cvar_s	*next, *prev;
	struct cvar_s	*hashNext, *hashPrev;
	int				hashIndex;
} cvar_t;

#define	MAX_CVAR_VALUE_STRING	256

typedef int	cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct vmCvar_s {
	cvarHandle_t	handle;
	int			modificationCount;
	float		value;
	int			integer;
	char		string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "game/surfaceflags.h"			// shared with the q3map utility

/*
Ghoul2 Insert Start
*/
typedef struct CollisionRecord_s {
	float		mDistance;
	int			mEntityNum;
	int			mModelIndex;
	int			mPolyIndex;
	int			mSurfaceIndex;
	vec3_t		mCollisionPosition;
	vec3_t		mCollisionNormal;
	int			mFlags;
	int			mMaterial;
	int			mLocation;
	float		mBarycentricI; // two barycentic coodinates for the hit point
	float		mBarycentricJ; // K = 1-I-J
} CollisionRecord_t;

#define MAX_G2_COLLISIONS 16

typedef CollisionRecord_t G2Trace_t[MAX_G2_COLLISIONS];	// map that describes all of the parts of ghoul2 models that got hit

/*
Ghoul2 Insert End
*/
// a trace is returned when a box is swept through the world
typedef struct trace_s {
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area

	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact, transformed to world space
	int			surfaceFlags;	// surface hit
	int			contents;	// contents on other side of surface hit
	int			entityNum;	// entity the contacted sirface is a part of
/*
Ghoul2 Insert Start
*/
	//rww - removed this for now, it's just wasting space in the trace structure.
//	CollisionRecord_t G2CollisionMap[MAX_G2_COLLISIONS];	// map that describes all of the parts of ghoul2 models that got hit
/*
Ghoul2 Insert End
*/
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD


// markfragments are returned by CM_MarkFragments()
typedef struct markFragment_s {
	int		firstPoint;
	int		numPoints;
} markFragment_t;



typedef struct orientation_s {
	vec3_t		origin;
	matrix3_t	axis;
} orientation_t;

//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE		0x0001
#define	KEYCATCH_UI					0x0002
#define	KEYCATCH_MESSAGE		0x0004
#define	KEYCATCH_CGAME			0x0008
#define KEYCATCH_NUMBERSONLY	0x0010


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum {
	CHAN_AUTO,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # Auto-picks an empty channel to play sound on
	CHAN_LOCAL,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # menu sounds, etc
	CHAN_WEAPON,//## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_VOICE, //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Voice sounds cause mouth animation
	//CHAN_VOICE_ATTEN, //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Causes mouth animation but still use normal sound falloff 
	CHAN_ITEM,  //## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_BODY,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3"
	CHAN_LOCAL_SOUND,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #chat messages, etc
	CHAN_ANNOUNCER,		//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #announcer voices, etc
	CHAN_AMBIENT,//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" # added for ambient sounds
	//CHAN_LESS_ATTEN,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #attenuates similar to chan_voice, but uses empty channel auto-pick behaviour
	//CHAN_MENU1,		//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #menu stuff, etc
	//CHAN_VOICE_GLOBAL,  //## %s !!"W:\game\base\!!sound\voice\*.wav;*.mp3" # Causes mouth animation and is broadcast, like announcer
	//CHAN_MUSIC,	//## %s !!"W:\game\base\!!sound\*.wav;*.mp3" #music played as a looping sound - added by BTO (VV)
} soundChannel_t;


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

#define	SNAPFLAG_RATE_DELAYED	1
#define	SNAPFLAG_NOT_ACTIVE		2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT	4	// toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define	MAX_CLIENTS			64		// absolute limit
//#define MAX_RADAR_ENTITIES	MAX_GENTITIES
#define MAX_TERRAINS		32 //rwwRMG: inserted
#define MAX_LOCATIONS		64
#define MAX_LADDERS			64

#define MAX_INSTANCE_TYPES		16

#define	GENTITYNUM_BITS	10		// don't need to send any more
#define	MAX_GENTITIES	(1<<GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


// these are also in be_aas_def.h - argh (rjr)
#define	MAX_MODELS				256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS				256		// so they cannot be blindly increased
#define MAX_AMBIENT_SOUNDSETS	64
#define MAX_FX					64		// max effects strings, I'm hoping that 64 will be plenty
#define MAX_SUB_BSP				32
#define MAX_ICONS				32
#define	MAX_CHARSKINS			64		// character skins
#define	MAX_HUDICONS			16		// icons on hud

#define	MAX_CONFIGSTRINGS	1400 //this is getting pretty high. Try not to raise it anymore than it already is.

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define	CS_SERVERINFO		0		// an info string with all the serverinfo cvars
#define	CS_SYSTEMINFO		1		// an info string for server system to client system configuration (timescale, etc)
#define CS_PLAYERS			2		// info string for player user info
#define CS_CUSTOM			(CS_PLAYERS + MAX_CLIENTS )

#define	RESERVED_CONFIGSTRINGS	2	// game can't modify below this, only the system can

#define	MAX_GAMESTATE_CHARS	16000
typedef struct gameState_s {
	int			stringOffsets[MAX_CONFIGSTRINGS];
	char		stringData[MAX_GAMESTATE_CHARS];
	int			dataCount;
} gameState_t;

//=========================================================

// bit field limits
#define	MAX_STATS				16
#define	MAX_PERSISTANT			16
#define	MAX_AMMO				16
#define	MAX_WEAPONS				32		
#define MAX_GAMETYPE_ITEMS		5

#define	MAX_PS_EVENTS			4

#define PS_PMOVEFRAMECOUNTBITS	6

typedef enum
{
	ATTACK_NORMAL,
	ATTACK_ALTERNATE,
	ATTACK_MAX

} attackType_t;

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.

typedef struct playerState_s {
	int			commandTime;	// cmd->serverTime of last executed command
	int			pm_type;
	int			bobCycle;		// for view bobbing and footstep generation
	int			pm_flags;		// ducked, etc
	int			pm_debounce;	// debounce buttons
	int			pm_time;

	vec3_t		origin;
	vec3_t		velocity;

	int			weaponTime;
	int			weaponFireBurstCount;
	int			weaponAnimId;
	int			weaponAnimIdChoice;
	int			weaponAnimTime;
	int			weaponCallbackTime;
	int			weaponCallbackStep;

	int			gravity;
	int			speed;
	int			delta_angles[3];				// add to command angles to get view direction
												// changed by spawns, rotating objects, and teleporters
	int			groundEntityNum;				// ENTITYNUM_NONE = in air
												
	int			legsAnim;						// mask off ANIM_TOGGLEBIT
												
	int			torsoTimer;						// don't change low priority animations until this runs out
	int			torsoAnim;						// mask off ANIM_TOGGLEBIT
												
	int			movementDir;					// a number 0 to 7 that represents the reletive angle
												// of movement to the view angle (axial and diagonals)
												// when at rest, the value will remain unchanged
												// used to twist the legs during strafing
												
	int			eFlags;							// copied to entityState_t->eFlags
												
	int			eventSequence;					// pmove generated events
	int			events[MAX_PS_EVENTS];			
	int			eventParms[MAX_PS_EVENTS];		
												
	int			externalEvent;					// events set on player from another source
	int			externalEventParm;				
	int			externalEventTime;				
												
	int			clientNum;						// ranges from 0 to MAX_CLIENTS-1
	int			weapon;							// copied to entityState_t->weapon
	int			weaponstate;					
												
	vec3_t		viewangles;						// for fixed views
	int			viewheight;						
												
	// damage feedback							
	int			damageEvent;					// when it changes, latch the other parms
	int			damageYaw;						
	int			damagePitch;					
	int			damageCount;					
												
	int			painTime;						// used for both game and client side to process the pain twitch - NOT sent across the network
	int			painDirection;					// NOT sent across the network
										
	int			stats[MAX_STATS];				
	int			persistant[MAX_PERSISTANT];		// stats that aren't cleared on death
	int			ammo[MAX_AMMO];
	int			clip[ATTACK_MAX][MAX_WEAPONS];
	int			firemode[MAX_WEAPONS];

	int			generic1;
	int			loopSound;

	// Incaccuracy values for firing
	int			inaccuracy;
	int			inaccuracyTime;
	int			kickPitch;

	// not communicated over the net at all
	int			ping;							// server to game info for scoreboard
	int			pmove_framecount;				// FIXME: don't transmit over the network
	int			jumppad_frame;
	int			entityEventSequence;
	vec3_t		pvsOrigin;						// view origin used to calculate PVS (also the lean origin)
												// THIS VARIABLE MUST AT LEAST BE THE PLAYERS ORIGIN ALL OF THE 
												// TIME OR THE PVS CALCULATIONS WILL NOT WORK.

	// Zooming
	int			zoomTime;
	int			zoomFov;

	// LAdders
	int			ladder;
	int			leanTime;

	// Timers 
	int			grenadeTimer;
	int			respawnTimer;
} playerState_t;


typedef enum 
{
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS

} team_t;

//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define	BUTTON_ATTACK		(1<<0)
#define	BUTTON_TALK			(1<<1)			// displays talk balloon and disables actions	
#define BUTTON_GOGGLES		(1<<2)			// turns nv or therm goggles on/off
#define BUTTON_LEAN			(1<<3)			// lean modifier, when held strafe left and right will lean
#define	BUTTON_WALKING		(1<<4)			// walking can't just be infered from MOVE_RUN
											// because a key pressed late in the frame will
											// only generate a small move value for that frame
											// walking will use different animations and
											// won't generate footsteps
#define	BUTTON_USE			(1<<5)			// the ol' use key returns!
#define	BUTTON_RELOAD		(1<<6)			// reloads current weapon
#define BUTTON_ALT_ATTACK	(1<<7)
#define	BUTTON_ANY			(1<<8)			// any key whatsoever
#define BUTTON_ZOOMIN		(1<<9)
#define BUTTON_ZOOMOUT		(1<<10)
#define BUTTON_FIREMODE		(1<<11)

#define BUTTON_LEAN_RIGHT	(1<<12)
#define BUTTON_LEAN_LEFT	(1<<13)

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int				serverTime;
	int				angles[3];
	int 			buttons;
	byte			weapon;
	signed char		forwardmove;
	signed char		rightmove;
	signed char		upmove;
} usercmd_t;

//===================================================================

//rww - unsightly hack to allow us to make an FX call that takes a horrible amount of args
typedef struct addpolyArgStruct_s {
	vec3_t				p[4];
	vec2_t				ev[4];
	int					numVerts;
	vec3_t				vel;
	vec3_t				accel;
	float				alpha1;
	float				alpha2;
	float				alphaParm;
	vec3_t				rgb1;
	vec3_t				rgb2;
	float				rgbParm;
	vec3_t				rotationDelta;
	float				bounce;
	int					motionDelay;
	int					killTime;
	qhandle_t			shader;
	int					flags;
} addpolyArgStruct_t;

typedef struct addbezierArgStruct_s {
	vec3_t start;
	vec3_t end;
	vec3_t control1;
	vec3_t control1Vel;
	vec3_t control2;
	vec3_t control2Vel;
	float size1;
	float size2;
	float sizeParm;
	float alpha1;
	float alpha2;
	float alphaParm;
	vec3_t sRGB;
	vec3_t eRGB;
	float rgbParm;
	int killTime;
	qhandle_t shader;
	int flags;
} addbezierArgStruct_t;

typedef struct addspriteArgStruct_s
{
	vec3_t origin;
	vec3_t vel;
	vec3_t accel;
	float scale;
	float dscale;
	float sAlpha;
	float eAlpha;
	float rotation;
	float bounce;
	int life;
	qhandle_t shader;
	int flags;
} addspriteArgStruct_t;

typedef struct effectTrailVertStruct_s {
	vec3_t	origin;

	// very specifc case, we can modulate the color and the alpha
	vec3_t	rgb;
	vec3_t	destrgb;
	vec3_t	curRGB;

	float	alpha;
	float	destAlpha;
	float	curAlpha;

	// this is a very specific case thing...allow interpolating the st coords so we can map the texture
	//	properly as this segement progresses through it's life
	float	ST[2];
	float	destST[2];
	float	curST[2];
} effectTrailVertStruct_t;

typedef struct effectTrailArgStruct_s {
	effectTrailVertStruct_t		mVerts[4];
	qhandle_t					mShader;
	int							mSetFlags;
	int							mKillTime;
} effectTrailArgStruct_t;

typedef struct addElectricityArgStruct_s {
	vec3_t start;
	vec3_t end;
	float size1;
	float size2;
	float sizeParm;
	float alpha1;
	float alpha2;
	float alphaParm;
	vec3_t sRGB;
	vec3_t eRGB;
	float rgbParm;
	float chaos;
	int killTime;
	qhandle_t shader;
	int flags;
} addElectricityArgStruct_t;

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE,					// value = base + sin( time / duration ) * delta
	TR_GRAVITY,
	TR_HEAVYGRAVITY,
	TR_LIGHTGRAVITY
} trType_t;

typedef struct trajectory_s {
	trType_t	trType;
	int		trTime;
	int		trDuration;			// if non 0, trTime + trDuration = stop time
	vec3_t	trBase;
	vec3_t	trDelta;			// velocity, etc
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s 
{
	int				number;			// entity index
	int				eType;			// entityType_t
	int				eFlags;

	trajectory_t	pos;			// for calculating position
	trajectory_t	apos;			// for calculating angles

	int				time;
	int				time2;
					
	vec3_t			origin;
	vec3_t			origin2;
					
	vec3_t			angles;
	vec3_t			angles2;
					
	int				otherEntityNum;	// shotgun sources, etc
	int				otherEntityNum2;
					
	int				groundEntityNum;	// -1 = in air
					
	int				loopSound;		// constantly loop this sound
	int				mSoundSet;
										
	int				modelindex;
	int				modelindex2;
	int				clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	int				frame;
					
	int				solid;			// for client side prediction, trap_linkentity sets this properly
					
	int				event;			// impulse events -- muzzle flashes, footsteps, etc
	int				eventParm;

	int				generic1;

	// for players
	// these fields are only transmitted for client entities!!!!!
	int				gametypeitems;	// bit flags indicating which items are carried
	int				weapon;			// determines weapon and flash model, etc
	int				legsAnim;		// mask off ANIM_TOGGLEBIT
	int				torsoAnim;		// mask off ANIM_TOGGLEBIT
	int				torsoTimer;		// time the animation will play for
	int				leanOffset;		// Lean direction
} entityState_t;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED, 	// not talking to a server
	CA_AUTHORIZING,		// not used any more, was checking cd key
	CA_CONNECTING,		// sending request packets to the server
	CA_CHALLENGING,		// sending challenge packets to the server
	CA_CONNECTED,		// netchan_t established, getting gamestate
	CA_LOADING,			// only during cgame initialization, never during main loop
	CA_PRIMED,			// got gamestate, waiting for first frame
	CA_ACTIVE,			// game views should be displayed
	CA_CINEMATIC		// playing a cinematic or a static pic, not connected to a server
} connstate_t;


#define Square(x) ((x)*(x))

// real time
//=============================================


typedef struct qtime_s {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
} qtime_t;


// server browser sources
#define AS_LOCAL			0
#define AS_GLOBAL			1
#define AS_FAVORITES		2
#define AS_MPLAYER		3 // obsolete


// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,		// play
	FMV_EOF,		// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;
#define	MAX_GLOBAL_SERVERS			2048
#define	MAX_OTHER_SERVERS			128
#define MAX_PINGREQUESTS			32
#define MAX_SERVERSTATUSREQUESTS	16

#define SAY_ALL		0
#define SAY_TEAM	1
#define SAY_TELL	2

/*
Ghoul2 Insert Start
*/

typedef struct mdxaBone_s {
	float		matrix[3][4];
} mdxaBone_t;

// For ghoul2 axis use

typedef enum Eorientations
{
	ORIGIN = 0,
	POSITIVE_X,
	POSITIVE_Z,
	POSITIVE_Y,
	NEGATIVE_X,
	NEGATIVE_Z,
	NEGATIVE_Y
} orientations_t;
/*
Ghoul2 Insert End
*/

// define the new memory tags for the zone, used by all modules now
//
#define TAGDEF(blah) TAG_ ## blah
typedef enum {
	#include "qcommon/tags.h"
} memtag;
typedef unsigned memtag_t;

typedef struct 
{
	int		isValid;	
	void	*ghoul2;
	int		modelNum;
	int		boltNum;
	vec3_t	angles;
	vec3_t	origin;
	vec3_t	scale;
	vec3_t	dir;
	vec3_t	forward;

} CFxBoltInterface;

//rww - conveniently toggle "gore" code, for model decals and stuff.
#define _G2_GORE

// these are the actual shaders
typedef enum {
	PGORE_NONE,
	PGORE_ARMOR,
	PGORE_BULLETBIG,
	PGORE_KNIFESLASH,
	PGORE_PUNCTURE,
	PGORE_SHOTGUN,
	PGORE_SHOTGUNBIG,
	PGORE_IMMOLATE,
	PGORE_BURN,
	PGORE_SPURT,
	PGORE_SPLATTER,
	PGORE_BLOODY_GLASS,
	PGORE_BLOODY_GLASS_B,
	PGORE_BLOODY_ICK,
	PGORE_BLOODY_DROOP,
	PGORE_BLOODY_MAUL,
	PGORE_BLOODY_DROPS,
	PGORE_BULLET_E,
	PGORE_BULLET_F,
	PGORE_BULLET_G,
	PGORE_BULLET_H,
	PGORE_BULLET_I,
	PGORE_BULLET_J,
	PGORE_BULLET_K,
	PGORE_BLOODY_HAND,
	PGORE_POWDER_BURN_DENSE,
	PGORE_POWDER_BURN_CHUNKY,
	PGORE_KNIFESLASH2,
	PGORE_KNIFESLASH3,
	PGORE_CHUNKY_SPLAT,
	PGORE_BIG_SPLATTER,
	PGORE_BLOODY_SPLOTCH,
	PGORE_BLEEDER,
	PGORE_PELLETS,
	PGORE_KNIFE_SOAK,
	PGORE_BLEEDER_DENSE,
	PGORE_BLOODY_SPLOTCH2,
	PGORE_BLOODY_DRIPS,
	PGORE_DRIPPING_DOWN,
	PGORE_GUTSHOT,
	PGORE_SHRAPNEL,
	PGORE_COUNT
} goreEnum_t;

struct goreEnumShader_t
{
	int				maxLODBias;   //if r_lodBias (and the other 3 convars) =x then shaders with this larger than x will not be used
	goreEnum_t		shaderEnum;  // why is this even in here?
	char			shaderName[MAX_QPATH];
};

typedef struct SSkinGoreData_s
{
	vec3_t			angles;
	vec3_t			position;
	int				currentTime;
	int				entNum;
	vec3_t			rayDirection;	// in world space
	vec3_t			hitLocation;	// in world space
	vec3_t			scale;
	float			SSize;			// size of splotch in the S texture direction in world units
	float			TSize;			// size of splotch in the T texture direction in world units
	float			theta;			// angle to rotate the splotch

	// growing stuff
	int				growDuration;			// time over which we want this to scale up, set to -1 for no scaling
	float			goreScaleStartFraction; // fraction of the final size at which we want the gore to initially appear

	qboolean		frontFaces;
	qboolean		backFaces;
	qboolean		baseModelOnly;
	int				lifeTime;				// effect expires after this amount of time
	int				fadeOutTime;			//specify the duration of fading, from the lifeTime (e.g. 3000 will start fading 3 seconds before removal and be faded entirely by removal)
	int				shrinkOutTime;			// unimplemented
	float			alphaModulate;			// unimplemented
	vec3_t			tint;					// unimplemented
	float			impactStrength;			// unimplemented

	goreEnum_t		shaderEnum; // enum that'll get switched over to the shader's actual handle 

	int				myIndex; // used internally
} SSkinGoreData;

/*
========================================================================

String ID Tables

========================================================================
*/
#define ENUM2STRING(arg)   { #arg, arg }
typedef struct stringID_table_s
{
	const char	*name;
	int		id;
} stringID_table_t;

int GetIDForString ( stringID_table_t *table, const char *string );
const char *GetStringForID( stringID_table_t *table, int id );


// stuff to help out during development process, force reloading/uncacheing of certain filetypes...
//
typedef enum
{
	eForceReload_NOTHING,
//	eForceReload_BSP,	// not used in MP codebase
	eForceReload_MODELS,
	eForceReload_ALL

} ForceReload_e;


void NET_AddrToString( char *out, size_t size, void *addr );

qboolean Q_InBitflags( const uint32_t *bits, int index, uint32_t bitsPerByte );
void Q_AddToBitflags( uint32_t *bits, int index, uint32_t bitsPerByte );
void Q_RemoveFromBitflags( uint32_t *bits, int index, uint32_t bitsPerByte );

typedef int( *cmpFunc_t )(const void *a, const void *b);

void *Q_LinearSearch( const void *key, const void *ptr, size_t count,
	size_t size, cmpFunc_t cmp );
