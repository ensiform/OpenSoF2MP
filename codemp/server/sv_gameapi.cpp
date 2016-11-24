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

// sv_gameapi.cpp  -- interface to the game dll
//Anything above this #include will be ignored by the compiler

#include "server.h"
#include "botlib/botlib.h"
#include "ghoul2/ghoul2_shared.h"
#include "qcommon/cm_public.h"
#include "qcommon/timing.h"

#include "qcommon/GenericParser2.h"

botlib_export_t	*botlib_export;

// game interface
static vm_t *gvm; // game vm, valid for legacy and new api

//
// game vmMain calls
//

void GVM_InitGame( int levelTime, int randomSeed, int restart ) {
	VM_Call( gvm, GAME_INIT, levelTime, randomSeed, restart );
}

void GVM_ShutdownGame( int restart ) {
	VM_Call( gvm, GAME_SHUTDOWN, restart );
}

char *GVM_ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	intptr_t denied = VM_Call( gvm, GAME_CLIENT_CONNECT, clientNum, firstTime, isBot );
	if ( denied ) {
		return (char *)VM_ExplicitArgPtr( gvm, denied );
	}
	return nullptr;
}

void GVM_ClientBegin( int clientNum ) {
	VM_Call( gvm, GAME_CLIENT_BEGIN, clientNum );
}

qboolean GVM_ClientUserinfoChanged( int clientNum ) {
	return (qboolean)VM_Call( gvm, GAME_CLIENT_USERINFO_CHANGED, clientNum );
}

void GVM_ClientDisconnect( int clientNum ) {
	VM_Call( gvm, GAME_CLIENT_DISCONNECT, clientNum );
}

void GVM_ClientCommand( int clientNum ) {
	VM_Call( gvm, GAME_CLIENT_COMMAND, clientNum );
}

void GVM_ClientThink( int clientNum, usercmd_t *ucmd ) {
	VM_Call( gvm, GAME_CLIENT_THINK, clientNum, reinterpret_cast< intptr_t >( ucmd ) );
}

void GVM_RunFrame( int levelTime ) {
	VM_Call( gvm, GAME_RUN_FRAME, levelTime );
}

qboolean GVM_ConsoleCommand( void ) {
	return (qboolean)VM_Call( gvm, GAME_CONSOLE_COMMAND );
}

int GVM_BotAIStartFrame( int time ) {
	return VM_Call( gvm, BOTAI_START_FRAME, time );
}

void GVM_SpawnRMGEntity( void ) {
	VM_Call( gvm, GAME_SPAWN_RMG_ENTITY );
}

void GVM_GametypeCommand( int cmd, int arg0, int arg1, int arg2, int arg3, int arg4 ) {
	VM_Call( gvm, GAME_GAMETYPE_COMMAND, cmd, arg0, arg1, arg2, arg3, arg4 );
}


//
// game syscalls
//	only used by legacy mods!
//

// legacy syscall

extern float g_svCullDist;
int CM_ModelContents( clipHandle_t model, int subBSPIndex );
int CM_LoadSubBSP( const char *name, qboolean clientload );
int CM_FindSubBSP( int modelIndex );
char *CM_SubBSPEntityString( int index );

static void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}

static void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );
}

static void SV_GameSendServerCommand( int clientNum, const char *text ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", text );
	}
}

static qboolean SV_EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, int capsule ) {
	const float	*origin, *angles;
	clipHandle_t	ch;
	trace_t			trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace ( &trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule );

	return (qboolean)trace.startsolid;
}

static void SV_SetBrushModel( sharedEntity_t *ent, const char *name ) {
	clipHandle_t	h;
	vec3_t			mins, maxs;

	if (!name)
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}

	if (name[0] == '*')
	{
		ent->s.modelindex = atoi( name + 1 );

		if (sv.mLocalSubBSPIndex != -1)
		{
			ent->s.modelindex += sv.mLocalSubBSPModelOffset;
		}

		h = CM_InlineModel( ent->s.modelindex );

		CM_ModelBounds(h, mins, maxs);

		VectorCopy (mins, ent->r.mins);
		VectorCopy (maxs, ent->r.maxs);
		ent->r.bmodel = qtrue;
		ent->r.contents = CM_ModelContents( h, -1 );
	}
	else if (name[0] == '#')
	{
		ent->s.modelindex = CM_LoadSubBSP(va("maps/%s.bsp", name + 1), qfalse);
		CM_ModelBounds( ent->s.modelindex, mins, maxs );

		VectorCopy (mins, ent->r.mins);
		VectorCopy (maxs, ent->r.maxs);
		ent->r.bmodel = qtrue;

		//rwwNOTE: We don't ever want to set contents -1, it includes CONTENTS_LIGHTSABER.
		//Lots of stuff will explode if there's a brush with CONTENTS_LIGHTSABER that isn't attached to a client owner.
		//ent->contents = -1;		// we don't know exactly what is in the brushes
		h = CM_InlineModel( ent->s.modelindex );
		ent->r.contents = CM_ModelContents( h, CM_FindSubBSP(ent->s.modelindex) );
	}
	else
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}
}

static qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	int		leafnum, cluster;
//	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
//	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
//	area2 = CM_LeafArea( leafnum );

	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;

	return qtrue;
}

static void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
		return;
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

static void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open ) {
	svEntity_t	*svEnt;

	svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 )
		return;

	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}

static void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
		return;
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

static const char *SV_SetActiveSubBSP( int index ) {
	if ( index >= 0 ) {
		sv.mLocalSubBSPIndex = CM_FindSubBSP( index );
		sv.mLocalSubBSPModelOffset = index;
		sv.mLocalSubBSPEntityParsePoint = CM_SubBSPEntityString( sv.mLocalSubBSPIndex );
		return sv.mLocalSubBSPEntityParsePoint;
	}

	sv.mLocalSubBSPIndex = -1;
	return NULL;
}

static qboolean SV_GetEntityToken( char *buffer, int bufferSize ) {
	char *s;

	if ( sv.mLocalSubBSPIndex == -1 ) {
		s = COM_Parse( (const char **)&sv.entityParsePoint );
		Q_strncpyz( buffer, s, bufferSize );
		if ( !sv.entityParsePoint && !s[0] )
			return qfalse;
		else
			return qtrue;
	}
	else {
		s = COM_Parse( (const char **)&sv.mLocalSubBSPEntityParsePoint);
		Q_strncpyz( buffer, s, bufferSize );
		if ( !sv.mLocalSubBSPEntityParsePoint && !s[0] )
			return qfalse;
		else
			return qtrue;
	}
}

/*static void SV_RegisterSharedMemory( char *memory ) {
	sv.mSharedMemory = memory;
}

static qhandle_t SV_RE_RegisterSkin( const char *name ) {
	return re->RegisterServerSkin( name );
}*/

intptr_t SV_GameSystemCalls( intptr_t *args ) {
	switch( args[0] ) {

		//rww - alright, DO NOT EVER add a game/cgame/ui generic call without adding a trap to match, and
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

	case G_PRINT:
		Com_Printf( "%s", VMA(1) );
		return 0;

	case G_ERROR:
		Com_Error( ERR_DROP, "%s", VMA(1) );
		return 0;

	case G_MILLISECONDS:
		return Sys_Milliseconds();

	case G_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *)VMA(1), (const char *)VMA(2), (const char *)VMA(3), args[4] );
		return 0;

	case G_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *)VMA(1) );
		return 0;

	case G_CVAR_SET:
		Cvar_VM_Set( (const char *)VMA(1), (const char *)VMA(2), VM_GAME );
		return 0;

	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue( (const char *)VMA(1) );

	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer( (const char *)VMA(1), (char *)VMA(2), args[3] );
		return 0;

	case G_ARGC:
		return Cmd_Argc();

	case G_ARGV:
		Cmd_ArgvBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;

	case G_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText( args[1], (const char *)VMA(2) );
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode( (const char *)VMA(1), (int *)VMA(2), (fsMode_t)args[3] );

	case G_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;

	case G_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;

	case G_FS_FCLOSE_FILE:
		FS_FCloseFile( args[1] );
		return 0;

	case G_FS_GETFILELIST:
		return FS_GetFileList( (const char *)VMA(1), (const char *)VMA(2), (char *)VMA(3), args[4] );

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData( (sharedEntity_t *)VMA(1), args[2], args[3], (struct playerState_s *)VMA(4), args[5] );
		return 0;

	case G_DROP_CLIENT:
		SV_GameDropClient( args[1], (const char *)VMA(2) );
		return 0;

	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand( args[1], (const char *)VMA(2) );
		return 0;

	case G_LINKENTITY:
		SV_LinkEntity( (sharedEntity_t *)VMA(1) );
		return 0;

	case G_UNLINKENTITY:
		SV_UnlinkEntity( (sharedEntity_t *)VMA(1) );
		return 0;

	case G_ENTITIES_IN_BOX:
		return SV_AreaEntities( (const float *)VMA(1), (const float *)VMA(2), (int *)VMA(3), args[4] );

	case G_ENTITY_CONTACT:
		return SV_EntityContact( (const float *)VMA(1), (const float *)VMA(2), (const sharedEntity_t *)VMA(3), /*int capsule*/ qfalse );

	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact( (const float *)VMA(1), (const float *)VMA(2), (const sharedEntity_t *)VMA(3), /*int capsule*/ qtrue );

	case G_TRACE:
		SV_Trace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], /*int capsule*/ qfalse, /*args[8]*/0, args[9] );
		return 0;
	case G_TRACECAPSULE:
		SV_Trace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], /*int capsule*/ qtrue, args[8], args[9]  );
		return 0;
	case G_POINT_CONTENTS:
		return SV_PointContents( (const float *)VMA(1), args[2] );
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel( (sharedEntity_t *)VMA(1), (const char *)VMA(2) );
		return 0;
	case G_IN_PVS:
		return SV_inPVS( (const float *)VMA(1), (const float *)VMA(2) );
	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals( (const float *)VMA(1), (const float *)VMA(2) );

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring( args[1], (const char *)VMA(2) );
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring( args[1], (char *)VMA(2), args[3] );
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo( args[1], (const char *)VMA(2) );
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo( args[1], (char *)VMA(2), args[3] );
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo( (char *)VMA(1), args[2] );
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState( (sharedEntity_t *)VMA(1), (qboolean)args[2] );
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected( args[1], args[2] );

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient( args[1] );
		return 0;

	case G_GET_USERCMD:
		SV_GetUsercmd( args[1], (struct usercmd_s *)VMA(2) );
		return 0;

		//rwwRMG - see below
		/*
	case G_GET_ENTITY_TOKEN:
		{
			const char	*s;

			s = COM_Parse( (const char **) &sv.entityParsePoint );
			Q_strncpyz( (char *)VMA(1), s, args[2] );
			if ( !sv.entityParsePoint && !s[0] ) {
				return qfalse;
			} else {
				return qtrue;
			}
		}
		*/

		/*
	case G_BOT_GET_MEMORY:
		void *ptr;
		ptr = Bot_GetMemoryGame(args[1]);
		return (int)ptr;
	case G_BOT_FREE_MEMORY:
		Bot_FreeMemoryGame((void *)VMA(1));
		return 0;
		*/
	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate( args[1], args[2], (float (*)[3])VMA(3) );
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete( args[1] );
		return 0;
	case G_REAL_TIME:
		return Com_RealTime( (struct qtime_s *)VMA(1) );
	case G_SNAPVECTOR:
		Sys_SnapVector( (float *)VMA(1) );
		return 0;

	/*case G_SET_SHARED_BUFFER:
		SV_RegisterSharedMemory( (char *)VMA(1) );
		return 0;*/
		//====================================

	case BOTLIB_SETUP:
		return botlib_export->BotLibSetup();
	case BOTLIB_SHUTDOWN:
		return botlib_export->BotLibShutdown();
	case BOTLIB_LIBVAR_SET:
		return botlib_export->BotLibVarSet( (char *)VMA(1), (char *)VMA(2) );
	case BOTLIB_LIBVAR_GET:
		return botlib_export->BotLibVarGet( (char *)VMA(1), (char *)VMA(2), args[3] );

	case BOTLIB_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( (char *)VMA(1) );
	case BOTLIB_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( (const char *)VMA(1) );
	case BOTLIB_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case BOTLIB_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], (struct pc_token_s *)VMA(2) );
	case BOTLIB_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], (char *)VMA(2), (int *)VMA(3) );

	case BOTLIB_START_FRAME:
		return botlib_export->BotLibStartFrame( VMF(1) );
	case BOTLIB_LOAD_MAP:
		return botlib_export->BotLibLoadMap( (const char *)VMA(1) );
	case BOTLIB_UPDATENTITY:
		return botlib_export->BotLibUpdateEntity( args[1], (struct bot_entitystate_s *)VMA(2) );
	case BOTLIB_TEST:
		return botlib_export->Test( args[1], (char *)VMA(2), (float *)VMA(3), (float *)VMA(4) );

	case BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity( args[1], args[2] );
	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage( args[1], (char *)VMA(2), args[3] );
	case BOTLIB_USER_COMMAND:
		SV_ClientThink( &svs.clients[args[1]], (struct usercmd_s *)VMA(2) );
		return 0;

	case BOTLIB_AAS_BBOX_AREAS:
		return botlib_export->aas.AAS_BBoxAreas( (float *)VMA(1), (float *)VMA(2), (int *)VMA(3), args[4] );
	case BOTLIB_AAS_AREA_INFO:
		return botlib_export->aas.AAS_AreaInfo( args[1], (struct aas_areainfo_s *)VMA(2) );
	case BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL:
		return botlib_export->aas.AAS_AlternativeRouteGoals( (float *)VMA(1), args[2], (float *)VMA(3), args[4], args[5], (struct aas_altroutegoal_s *)VMA(6), args[7], args[8] );
	case BOTLIB_AAS_ENTITY_INFO:
		botlib_export->aas.AAS_EntityInfo( args[1], (struct aas_entityinfo_s *)VMA(2) );
		return 0;

	case BOTLIB_AAS_INITIALIZED:
		return botlib_export->aas.AAS_Initialized();
	case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		botlib_export->aas.AAS_PresenceTypeBoundingBox( args[1], (float *)VMA(2), (float *)VMA(3) );
		return 0;
	case BOTLIB_AAS_TIME:
		return FloatAsInt( botlib_export->aas.AAS_Time() );

	case BOTLIB_AAS_POINT_AREA_NUM:
		return botlib_export->aas.AAS_PointAreaNum( (float *)VMA(1) );
	case BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX:
		return botlib_export->aas.AAS_PointReachabilityAreaIndex( (float *)VMA(1) );
	case BOTLIB_AAS_TRACE_AREAS:
		return botlib_export->aas.AAS_TraceAreas( (float *)VMA(1), (float *)VMA(2), (int *)VMA(3), (float (*)[3])VMA(4), args[5] );

	case BOTLIB_AAS_POINT_CONTENTS:
		return botlib_export->aas.AAS_PointContents( (float *)VMA(1) );
	case BOTLIB_AAS_NEXT_BSP_ENTITY:
		return botlib_export->aas.AAS_NextBSPEntity( args[1] );
	case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_ValueForBSPEpairKey( args[1], (char *)VMA(2), (char *)VMA(3), args[4] );
	case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_VectorForBSPEpairKey( args[1], (char *)VMA(2), (float *)VMA(3) );
	case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_FloatForBSPEpairKey( args[1], (char *)VMA(2), (float *)VMA(3) );
	case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_IntForBSPEpairKey( args[1], (char *)VMA(2), (int *)VMA(3) );

	case BOTLIB_AAS_AREA_REACHABILITY:
		return botlib_export->aas.AAS_AreaReachability( args[1] );

	case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return botlib_export->aas.AAS_AreaTravelTimeToGoalArea( args[1], (float *)VMA(2), args[3], args[4] );
	case BOTLIB_AAS_ENABLE_ROUTING_AREA:
		return botlib_export->aas.AAS_EnableRoutingArea( args[1], args[2] );
	case BOTLIB_AAS_PREDICT_ROUTE:
		return botlib_export->aas.AAS_PredictRoute( (struct aas_predictroute_s *)VMA(1), args[2], (float *)VMA(3), args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11] );

	case BOTLIB_AAS_SWIMMING:
		return botlib_export->aas.AAS_Swimming( (float *)VMA(1) );
	case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return botlib_export->aas.AAS_PredictClientMovement( (struct aas_clientmove_s *)VMA(1), args[2], (float *)VMA(3), args[4], args[5],
			(float *)VMA(6), (float *)VMA(7), args[8], args[9], VMF(10), args[11], args[12], args[13] );

	case BOTLIB_EA_SAY:
		botlib_export->ea.EA_Say( args[1], (char *)VMA(2) );
		return 0;
	case BOTLIB_EA_SAY_TEAM:
		botlib_export->ea.EA_SayTeam( args[1], (char *)VMA(2) );
		return 0;
	case BOTLIB_EA_COMMAND:
		botlib_export->ea.EA_Command( args[1], (char *)VMA(2) );
		return 0;

	case BOTLIB_EA_ACTION:
		botlib_export->ea.EA_Action( args[1], args[2] );
		break;
	case BOTLIB_EA_GESTURE:
		botlib_export->ea.EA_Gesture( args[1] );
		return 0;
	case BOTLIB_EA_TALK:
		botlib_export->ea.EA_Talk( args[1] );
		return 0;
	case BOTLIB_EA_ATTACK:
		botlib_export->ea.EA_Attack( args[1] );
		return 0;
	case BOTLIB_EA_ALT_ATTACK:
		botlib_export->ea.EA_Alt_Attack( args[1] );
		return 0;
	case BOTLIB_EA_FORCEPOWER:
		botlib_export->ea.EA_ForcePower( args[1] );
		return 0;
	case BOTLIB_EA_USE:
		botlib_export->ea.EA_Use( args[1] );
		return 0;
	case BOTLIB_EA_RESPAWN:
		botlib_export->ea.EA_Respawn( args[1] );
		return 0;
	case BOTLIB_EA_CROUCH:
		botlib_export->ea.EA_Crouch( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_UP:
		botlib_export->ea.EA_MoveUp( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_DOWN:
		botlib_export->ea.EA_MoveDown( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_FORWARD:
		botlib_export->ea.EA_MoveForward( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_BACK:
		botlib_export->ea.EA_MoveBack( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_LEFT:
		botlib_export->ea.EA_MoveLeft( args[1] );
		return 0;
	case BOTLIB_EA_MOVE_RIGHT:
		botlib_export->ea.EA_MoveRight( args[1] );
		return 0;

	case BOTLIB_EA_SELECT_WEAPON:
		botlib_export->ea.EA_SelectWeapon( args[1], args[2] );
		return 0;
	case BOTLIB_EA_JUMP:
		botlib_export->ea.EA_Jump( args[1] );
		return 0;
	case BOTLIB_EA_DELAYED_JUMP:
		botlib_export->ea.EA_DelayedJump( args[1] );
		return 0;
	case BOTLIB_EA_MOVE:
		botlib_export->ea.EA_Move( args[1], (float *)VMA(2), VMF(3) );
		return 0;
	case BOTLIB_EA_VIEW:
		botlib_export->ea.EA_View( args[1], (float *)VMA(2) );
		return 0;

	case BOTLIB_EA_END_REGULAR:
		botlib_export->ea.EA_EndRegular( args[1], VMF(2) );
		return 0;
	case BOTLIB_EA_GET_INPUT:
		botlib_export->ea.EA_GetInput( args[1], VMF(2), (struct bot_input_s *)VMA(3) );
		return 0;
	case BOTLIB_EA_RESET_INPUT:
		botlib_export->ea.EA_ResetInput( args[1] );
		return 0;

	case BOTLIB_AI_LOAD_CHARACTER:
		return botlib_export->ai.BotLoadCharacter( (char *)VMA(1), VMF(2) );
	case BOTLIB_AI_FREE_CHARACTER:
		botlib_export->ai.BotFreeCharacter( args[1] );
		return 0;
	case BOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt( botlib_export->ai.Characteristic_Float( args[1], args[2] ) );
	case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt( botlib_export->ai.Characteristic_BFloat( args[1], args[2], VMF(3), VMF(4) ) );
	case BOTLIB_AI_CHARACTERISTIC_INTEGER:
		return botlib_export->ai.Characteristic_Integer( args[1], args[2] );
	case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return botlib_export->ai.Characteristic_BInteger( args[1], args[2], args[3], args[4] );
	case BOTLIB_AI_CHARACTERISTIC_STRING:
		botlib_export->ai.Characteristic_String( args[1], args[2], (char *)VMA(3), args[4] );
		return 0;

	case BOTLIB_AI_ALLOC_CHAT_STATE:
		return botlib_export->ai.BotAllocChatState();
	case BOTLIB_AI_FREE_CHAT_STATE:
		botlib_export->ai.BotFreeChatState( args[1] );
		return 0;
	case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		botlib_export->ai.BotQueueConsoleMessage( args[1], args[2], (char *)VMA(3) );
		return 0;
	case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		botlib_export->ai.BotRemoveConsoleMessage( args[1], args[2] );
		return 0;
	case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNextConsoleMessage( args[1], (struct bot_consolemessage_s *)VMA(2) );
	case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNumConsoleMessages( args[1] );
	case BOTLIB_AI_INITIAL_CHAT:
		botlib_export->ai.BotInitialChat( args[1], (char *)VMA(2), args[3], (char *)VMA(4), (char *)VMA(5), (char *)VMA(6), (char *)VMA(7), (char *)VMA(8), (char *)VMA(9), (char *)VMA(10), (char *)VMA(11) );
		return 0;
	case BOTLIB_AI_NUM_INITIAL_CHATS:
		return botlib_export->ai.BotNumInitialChats( args[1], (char *)VMA(2) );
	case BOTLIB_AI_REPLY_CHAT:
		return botlib_export->ai.BotReplyChat( args[1], (char *)VMA(2), args[3], args[4], (char *)VMA(5), (char *)VMA(6), (char *)VMA(7), (char *)VMA(8), (char *)VMA(9), (char *)VMA(10), (char *)VMA(11), (char *)VMA(12) );
	case BOTLIB_AI_CHAT_LENGTH:
		return botlib_export->ai.BotChatLength( args[1] );
	case BOTLIB_AI_ENTER_CHAT:
		botlib_export->ai.BotEnterChat( args[1], args[2], args[3] );
		return 0;
	case BOTLIB_AI_GET_CHAT_MESSAGE:
		botlib_export->ai.BotGetChatMessage( args[1], (char *)VMA(2), args[3] );
		return 0;
	case BOTLIB_AI_STRING_CONTAINS:
		return botlib_export->ai.StringContains( (char *)VMA(1), (char *)VMA(2), args[3] );
	case BOTLIB_AI_FIND_MATCH:
		return botlib_export->ai.BotFindMatch( (char *)VMA(1), (struct bot_match_s *)VMA(2), args[3] );
	case BOTLIB_AI_MATCH_VARIABLE:
		botlib_export->ai.BotMatchVariable( (struct bot_match_s *)VMA(1), args[2], (char *)VMA(3), args[4] );
		return 0;
	case BOTLIB_AI_UNIFY_WHITE_SPACES:
		botlib_export->ai.UnifyWhiteSpaces( (char *)VMA(1) );
		return 0;
	case BOTLIB_AI_REPLACE_SYNONYMS:
		botlib_export->ai.BotReplaceSynonyms( (char *)VMA(1), args[2] );
		return 0;
	case BOTLIB_AI_LOAD_CHAT_FILE:
		return botlib_export->ai.BotLoadChatFile( args[1], (char *)VMA(2), (char *)VMA(3) );
	case BOTLIB_AI_SET_CHAT_GENDER:
		botlib_export->ai.BotSetChatGender( args[1], args[2] );
		return 0;
	case BOTLIB_AI_SET_CHAT_NAME:
		botlib_export->ai.BotSetChatName( args[1], (char *)VMA(2), args[3] );
		return 0;

	case BOTLIB_AI_RESET_GOAL_STATE:
		botlib_export->ai.BotResetGoalState( args[1] );
		return 0;
	case BOTLIB_AI_RESET_AVOID_GOALS:
		botlib_export->ai.BotResetAvoidGoals( args[1] );
		return 0;
	case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		botlib_export->ai.BotRemoveFromAvoidGoals( args[1], args[2] );
		return 0;
	case BOTLIB_AI_PUSH_GOAL:
		botlib_export->ai.BotPushGoal( args[1], (struct bot_goal_s *)VMA(2) );
		return 0;
	case BOTLIB_AI_POP_GOAL:
		botlib_export->ai.BotPopGoal( args[1] );
		return 0;
	case BOTLIB_AI_EMPTY_GOAL_STACK:
		botlib_export->ai.BotEmptyGoalStack( args[1] );
		return 0;
	case BOTLIB_AI_DUMP_AVOID_GOALS:
		botlib_export->ai.BotDumpAvoidGoals( args[1] );
		return 0;
	case BOTLIB_AI_DUMP_GOAL_STACK:
		botlib_export->ai.BotDumpGoalStack( args[1] );
		return 0;
	case BOTLIB_AI_GOAL_NAME:
		botlib_export->ai.BotGoalName( args[1], (char *)VMA(2), args[3] );
		return 0;
	case BOTLIB_AI_GET_TOP_GOAL:
		return botlib_export->ai.BotGetTopGoal( args[1], (struct bot_goal_s *)VMA(2) );
	case BOTLIB_AI_GET_SECOND_GOAL:
		return botlib_export->ai.BotGetSecondGoal( args[1], (struct bot_goal_s *)VMA(2) );
	case BOTLIB_AI_CHOOSE_LTG_ITEM:
		return botlib_export->ai.BotChooseLTGItem( args[1], (float *)VMA(2), (int *)VMA(3), args[4] );
	case BOTLIB_AI_CHOOSE_NBG_ITEM:
		return botlib_export->ai.BotChooseNBGItem( args[1], (float *)VMA(2), (int *)VMA(3), args[4], (struct bot_goal_s *)VMA(5), VMF(6) );
	case BOTLIB_AI_TOUCHING_GOAL:
		return botlib_export->ai.BotTouchingGoal( (float *)VMA(1), (struct bot_goal_s *)VMA(2) );
	case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return botlib_export->ai.BotItemGoalInVisButNotVisible( args[1], (float *)VMA(2), (float *)VMA(3), (struct bot_goal_s *)VMA(4) );
	case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return botlib_export->ai.BotGetLevelItemGoal( args[1], (char *)VMA(2), (struct bot_goal_s *)VMA(3) );
	case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return botlib_export->ai.BotGetNextCampSpotGoal( args[1], (struct bot_goal_s *)VMA(2) );
	case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return botlib_export->ai.BotGetMapLocationGoal( (char *)VMA(1), (struct bot_goal_s *)VMA(2) );
	case BOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt( botlib_export->ai.BotAvoidGoalTime( args[1], args[2] ) );
	case BOTLIB_AI_SET_AVOID_GOAL_TIME:
		botlib_export->ai.BotSetAvoidGoalTime( args[1], args[2], VMF(3));
		return 0;
	case BOTLIB_AI_INIT_LEVEL_ITEMS:
		botlib_export->ai.BotInitLevelItems();
		return 0;
	case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
		botlib_export->ai.BotUpdateEntityItems();
		return 0;
	case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return botlib_export->ai.BotLoadItemWeights( args[1], (char *)VMA(2) );
	case BOTLIB_AI_FREE_ITEM_WEIGHTS:
		botlib_export->ai.BotFreeItemWeights( args[1] );
		return 0;
	case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotInterbreedGoalFuzzyLogic( args[1], args[2], args[3] );
		return 0;
	case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotSaveGoalFuzzyLogic( args[1], (char *)VMA(2) );
		return 0;
	case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		botlib_export->ai.BotMutateGoalFuzzyLogic( args[1], VMF(2) );
		return 0;
	case BOTLIB_AI_ALLOC_GOAL_STATE:
		return botlib_export->ai.BotAllocGoalState( args[1] );
	case BOTLIB_AI_FREE_GOAL_STATE:
		botlib_export->ai.BotFreeGoalState( args[1] );
		return 0;

	case BOTLIB_AI_RESET_MOVE_STATE:
		botlib_export->ai.BotResetMoveState( args[1] );
		return 0;
	case BOTLIB_AI_ADD_AVOID_SPOT:
		botlib_export->ai.BotAddAvoidSpot( args[1], (float *)VMA(2), VMF(3), args[4] );
		return 0;
	case BOTLIB_AI_MOVE_TO_GOAL:
		botlib_export->ai.BotMoveToGoal( (struct bot_moveresult_s *)VMA(1), args[2], (struct bot_goal_s *)VMA(3), args[4] );
		return 0;
	case BOTLIB_AI_MOVE_IN_DIRECTION:
		return botlib_export->ai.BotMoveInDirection( args[1], (float *)VMA(2), VMF(3), args[4] );
	case BOTLIB_AI_RESET_AVOID_REACH:
		botlib_export->ai.BotResetAvoidReach( args[1] );
		return 0;
	case BOTLIB_AI_RESET_LAST_AVOID_REACH:
		botlib_export->ai.BotResetLastAvoidReach( args[1] );
		return 0;
	case BOTLIB_AI_REACHABILITY_AREA:
		return botlib_export->ai.BotReachabilityArea( (float *)VMA(1), args[2] );
	case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return botlib_export->ai.BotMovementViewTarget( args[1], (struct bot_goal_s *)VMA(2), args[3], VMF(4), (float *)VMA(5) );
	case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return botlib_export->ai.BotPredictVisiblePosition( (float *)VMA(1), args[2], (struct bot_goal_s *)VMA(3), args[4], (float *)VMA(5) );
	case BOTLIB_AI_ALLOC_MOVE_STATE:
		return botlib_export->ai.BotAllocMoveState();
	case BOTLIB_AI_FREE_MOVE_STATE:
		botlib_export->ai.BotFreeMoveState( args[1] );
		return 0;
	case BOTLIB_AI_INIT_MOVE_STATE:
		botlib_export->ai.BotInitMoveState( args[1], (struct bot_initmove_s *)VMA(2) );
		return 0;

	case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return botlib_export->ai.BotChooseBestFightWeapon( args[1], (int *)VMA(2) );
	case BOTLIB_AI_GET_WEAPON_INFO:
		botlib_export->ai.BotGetWeaponInfo( args[1], args[2], (struct weaponinfo_s *)VMA(3) );
		return 0;
	case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return botlib_export->ai.BotLoadWeaponWeights( args[1], (char *)VMA(2) );
	case BOTLIB_AI_ALLOC_WEAPON_STATE:
		return botlib_export->ai.BotAllocWeaponState();
	case BOTLIB_AI_FREE_WEAPON_STATE:
		botlib_export->ai.BotFreeWeaponState( args[1] );
		return 0;
	case BOTLIB_AI_RESET_WEAPON_STATE:
		botlib_export->ai.BotResetWeaponState( args[1] );
		return 0;

	case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return botlib_export->ai.GeneticParentsAndChildSelection(args[1], (float *)VMA(2), (int *)VMA(3), (int *)VMA(4), (int *)VMA(5));

	/*case G_R_REGISTERSKIN:
		return re->RegisterServerSkin((const char *)VMA(1));*/

	case G_G2_LISTBONES:
		re->G2API_ListBones( (CGhoul2Info *) VMA(1), args[2]);
		return 0;

	case G_G2_LISTSURFACES:
		re->G2API_ListSurfaces( (CGhoul2Info *) args[1] );
		return 0;

	case G_G2_HAVEWEGHOULMODELS:
		return re->G2API_HaveWeGhoul2Models( *((CGhoul2Info_v *)args[1]) );

	case G_G2_SETMODELS:
		re->G2API_SetGhoul2ModelIndexes( *((CGhoul2Info_v *)args[1]),(qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

	case G_G2_GETBOLT:
		return re->G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	/*case G_G2_GETBOLT_NOREC:
		re->G2API_BoltMatrixReconstruction( qfalse );//gG2_GBMNoReconstruct = qtrue;
		return re->G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case G_G2_GETBOLT_NOREC_NOROT:
		re->G2API_BoltMatrixReconstruction( qfalse );//gG2_GBMNoReconstruct = qtrue;
		re->G2API_BoltMatrixSPMethod( qtrue );//gG2_GBMUseSPMethod = qtrue;
		return re->G2API_GetBoltMatrix(*((CGhoul2Info_v *)args[1]), args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));*/

	case G_G2_INITGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		return	re->G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4],
									  (qhandle_t) args[5], args[6], args[7]);

	case G_G2_SETSKIN:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo( g2, (int)args[2] );

			return re->G2API_SetSkin(ghlinfo, args[3], args[4]);
		}

	/*case G_G2_SIZE:
		return re->G2API_Ghoul2Size ( *((CGhoul2Info_v *)args[1]) );
		break;*/

	case G_G2_ADDBOLT:
		return	re->G2API_AddBolt(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));

	case G_G2_SETBOLTINFO:
		re->G2API_SetBoltInfo(*((CGhoul2Info_v *)args[1]), args[2], args[3]);
		return 0;

	case G_G2_ANGLEOVERRIDE:
		return re->G2API_SetBoneAngles(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), (float *)VMA(4), args[5],
							 (const Eorientations) args[6], (const Eorientations) args[7], (const Eorientations) args[8],
							 (qhandle_t *)VMA(9), args[10], args[11] );

	case G_G2_PLAYANIM:
		return re->G2API_SetBoneAnim(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), args[4], args[5],
								args[6], VMF(7), args[8], VMF(9), args[10]);

/*	case G_G2_GETBONEANIM:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			int modelIndex = args[10];

			return re->G2API_GetBoneAnim(g2, modelIndex, (const char*)VMA(2), args[3], (float *)VMA(4), (int *)VMA(5),
								(int *)VMA(6), (int *)VMA(7), (float *)VMA(8), (int *)VMA(9));
		}*/

	case G_G2_GETGLANAME:
		//return (int)G2API_GetGLAName(*((CGhoul2Info_v *)args[1]), args[2]);
		{ //Since returning a pointer in such a way to a VM seems to cause MASSIVE FAILURE<tm>, we will shove data into the pointer the vm passes instead
			char *point = ((char *)VMA(3));
			char *local;
			local = re->G2API_GetGLAName(*((CGhoul2Info_v *)args[1]), args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}

		return 0;

	case G_G2_COPYGHOUL2INSTANCE:
		return (int)re->G2API_CopyGhoul2Instance(*((CGhoul2Info_v *)args[1]), *((CGhoul2Info_v *)args[2]), args[3]);

	case G_G2_COPYSPECIFICGHOUL2MODEL:
		re->G2API_CopySpecificG2Model(*((CGhoul2Info_v *)args[1]), args[2], *((CGhoul2Info_v *)args[3]), args[4]);
		return 0;

	case G_G2_DUPLICATEGHOUL2INSTANCE:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		re->G2API_DuplicateGhoul2Instance(*((CGhoul2Info_v *)args[1]), (CGhoul2Info_v **)VMA(2));
		return 0;

/*	case G_G2_HASGHOUL2MODELONINDEX:
		//return (int)G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)args[1], args[2]);
		return (int)re->G2API_HasGhoul2ModelOnIndex((CGhoul2Info_v **)VMA(1), args[2]);*/

	case G_G2_REMOVEGHOUL2MODEL:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		//return (int)G2API_RemoveGhoul2Model((CGhoul2Info_v **)args[1], args[2]);
		return (int)re->G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);

	/*case G_G2_REMOVEGHOUL2MODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		//return (int)G2API_RemoveGhoul2Models((CGhoul2Info_v **)args[1]);
		return (int)re->G2API_RemoveGhoul2Models((CGhoul2Info_v **)VMA(1));*/

	case G_G2_CLEANMODELS:
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 1;
#endif
		re->G2API_CleanGhoul2Models((CGhoul2Info_v **)VMA(1));
	//	re->G2API_CleanGhoul2Models((CGhoul2Info_v **)args[1]);
		return 0;

	case G_G2_COLLISIONDETECT:
		re->G2API_CollisionDetect ( (CollisionRecord_t*)VMA(1), *((CGhoul2Info_v *)args[2]), (const float*)VMA(3), (const float*)VMA(4), args[5], args[6], (float*)VMA(7), (float*)VMA(8), (float*)VMA(9), G2VertSpaceServer, args[10], args[11], VMF(12) );
		return 0;

/*	case G_G2_COLLISIONDETECTCACHE:
		re->G2API_CollisionDetectCache ( (CollisionRecord_t*)VMA(1), *((CGhoul2Info_v *)args[2]), (const float*)VMA(3), (const float*)VMA(4), args[5], args[6], (float*)VMA(7), (float*)VMA(8), (float*)VMA(9), G2VertSpaceServer, args[10], args[11], VMF(12) );
		return 0;*/

#if 0
	case G_G2_SETROOTSURFACE:
		return re->G2API_SetRootSurface(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3));

	case G_G2_SETSURFACEONOFF:
		return re->G2API_SetSurfaceOnOff(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), /*(const int)VMA(3)*/args[3]);

	case G_G2_SETNEWORIGIN:
		return re->G2API_SetNewOrigin(*((CGhoul2Info_v *)args[1]), /*(const int)VMA(2)*/args[2]);

	case G_G2_DOESBONEEXIST:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			return re->G2API_DoesBoneExist(g2, args[2], (const char *)VMA(3));
		}

	case G_G2_GETSURFACERENDERSTATUS:
	{
		CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

		return re->G2API_GetSurfaceRenderStatus(g2, args[2], (const char *)VMA(3));
	}

	case G_G2_ABSURDSMOOTHING:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			re->G2API_AbsurdSmoothing(g2, (qboolean)args[2]);
		}
		return 0;

	case G_G2_SETRAGDOLL:
		{
			//Convert the info in the shared structure over to the class-based version.
			sharedRagDollParams_t *rdParamst = (sharedRagDollParams_t *)VMA(2);
			CRagDollParams rdParams;

			if (!rdParamst)
			{
				re->G2API_ResetRagDoll(*((CGhoul2Info_v *)args[1]));
				return 0;
			}

			VectorCopy(rdParamst->angles, rdParams.angles);
			VectorCopy(rdParamst->position, rdParams.position);
			VectorCopy(rdParamst->scale, rdParams.scale);
			VectorCopy(rdParamst->pelvisAnglesOffset, rdParams.pelvisAnglesOffset);
			VectorCopy(rdParamst->pelvisPositionOffset, rdParams.pelvisPositionOffset);

			rdParams.fImpactStrength = rdParamst->fImpactStrength;
			rdParams.fShotStrength = rdParamst->fShotStrength;
			rdParams.me = rdParamst->me;

			rdParams.startFrame = rdParamst->startFrame;
			rdParams.endFrame = rdParamst->endFrame;

			rdParams.collisionType = rdParamst->collisionType;
			rdParams.CallRagDollBegin = rdParamst->CallRagDollBegin;

			rdParams.RagPhase = (CRagDollParams::ERagPhase)rdParamst->RagPhase;
			rdParams.effectorsToTurnOff = (CRagDollParams::ERagEffector)rdParamst->effectorsToTurnOff;

			re->G2API_SetRagDoll(*((CGhoul2Info_v *)args[1]), &rdParams);
		}
		return 0;
		break;
	case G_G2_ANIMATEG2MODELS:
		{
			sharedRagDollUpdateParams_t *rduParamst = (sharedRagDollUpdateParams_t *)VMA(3);
			CRagDollUpdateParams rduParams;

			if (!rduParamst)
			{
				return 0;
			}

			VectorCopy(rduParamst->angles, rduParams.angles);
			VectorCopy(rduParamst->position, rduParams.position);
			VectorCopy(rduParamst->scale, rduParams.scale);
			VectorCopy(rduParamst->velocity, rduParams.velocity);

			rduParams.me = rduParamst->me;
			rduParams.settleFrame = rduParamst->settleFrame;

			re->G2API_AnimateG2ModelsRag(*((CGhoul2Info_v *)args[1]), args[2], &rduParams);
		}
		return 0;
		break;

	//additional ragdoll options -rww
	case G_G2_RAGPCJCONSTRAINT:
		return re->G2API_RagPCJConstraint(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3), (float *)VMA(4));
	case G_G2_RAGPCJGRADIENTSPEED:
		return re->G2API_RagPCJGradientSpeed(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), VMF(3));
	case G_G2_RAGEFFECTORGOAL:
		return re->G2API_RagEffectorGoal(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3));
	case G_G2_GETRAGBONEPOS:
		return re->G2API_GetRagBonePos(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3), (float *)VMA(4), (float *)VMA(5), (float *)VMA(6));
	case G_G2_RAGEFFECTORKICK:
		return re->G2API_RagEffectorKick(*((CGhoul2Info_v *)args[1]), (const char *)VMA(2), (float *)VMA(3));
	case G_G2_RAGFORCESOLVE:
		return re->G2API_RagForceSolve(*((CGhoul2Info_v *)args[1]), (qboolean)args[2]);

	case G_G2_SETBONEIKSTATE:
		return re->G2API_SetBoneIKState(*((CGhoul2Info_v *)args[1]), args[2], (const char *)VMA(3), args[4], (sharedSetBoneIKStateParams_t *)VMA(5));
	case G_G2_IKMOVE:
		return re->G2API_IKMove(*((CGhoul2Info_v *)args[1]), args[2], (sharedIKMoveParams_t *)VMA(3));

	case G_G2_REMOVEBONE:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			return re->G2API_RemoveBone(g2, args[3], (const char *)VMA(2));
		}

	case G_G2_ATTACHINSTANCETOENTNUM:
		{
			re->G2API_AttachInstanceToEntNum(*((CGhoul2Info_v *)args[1]), args[2], (qboolean)args[3]);
		}
		return 0;
	case G_G2_CLEARATTACHEDINSTANCE:
		re->G2API_ClearAttachedInstance(args[1]);
		return 0;
	case G_G2_CLEANENTATTACHMENTS:
		re->G2API_CleanEntAttachments();
		return 0;
	case G_G2_OVERRIDESERVER:
		{
			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);
			return re->G2API_OverrideServerWithClientData(g2, 0);
		}

	case G_G2_GETSURFACENAME:
		{ //Since returning a pointer in such a way to a VM seems to cause MASSIVE FAILURE<tm>, we will shove data into the pointer the vm passes instead
			char *point = ((char *)VMA(4));
			char *local;
			int modelindex = args[3];

			CGhoul2Info_v &g2 = *((CGhoul2Info_v *)args[1]);

			local = re->G2API_GetSurfaceName(g2, modelindex, args[2]);
			if (local)
			{
				strcpy(point, local);
			}
		}
		return 0;
#endif

	case G_SET_ACTIVE_SUBBSP:
		SV_SetActiveSubBSP(args[1]);
		return 0;

	case G_RMG_INIT:
		return 0;

	case G_CM_REGISTER_TERRAIN:
		return 0;

	/*case G_BOT_UPDATEWAYPOINTS:
		SV_BotWaypointReception(args[1], (wpobject_t **)VMA(2));
		return 0;
	case G_BOT_CALCULATEPATHS:
		SV_BotCalculatePaths(args[1]);
		return 0;*/

	case G_GET_ENTITY_TOKEN:
		return SV_GetEntityToken((char *)VMA(1), args[2]);

	case G_VM_LOCALALLOC:
		return (intptr_t)VM_Local_Alloc(args[1]);
	case G_VM_LOCALALLOCUNALIGNED:
		return (intptr_t)VM_Local_AllocUnaligned(args[1]);
	case G_VM_LOCALTEMPALLOC:
		return (intptr_t)VM_Local_TempAlloc(args[1]);
	case G_VM_LOCALTEMPFREE:
		VM_Local_TempFree(args[1]);
		return 0;
	case G_VM_LOCALSTRINGALLOC:
		return (intptr_t)VM_Local_StringAlloc((char *) VMA(1));

	case G_GP_PARSE:
		return GP_VM_Parse( (char **) VMA(1), (bool) args[2], (bool) args[3] );
		//return (intptr_t)GP_Parse((char **) VMA(1), (bool) args[2], (bool) args[3]);
	case G_GP_PARSE_FILE:
		{
			char * data;
			FS_ReadFile((char *) VMA(1), (void **) &data);
			return GP_VM_Parse( &data, (bool) args[2], (bool) args[3] );
			//return (intptr_t)GP_Parse(&data, (bool) args[2], (bool) args[3]);
		}
	case G_GP_CLEAN:
		GP_VM_Clean( (qhandle_t *)VMA(1) );
		//GP_Clean((TGenericParser2) args[1]);
		return 0;
	case G_GP_DELETE:
		GP_VM_Delete( (qhandle_t *)VMA(1) );
		//GP_Delete((TGenericParser2 *) VMA(1));
		return 0;
	case G_GP_GET_BASE_PARSE_GROUP:
		return (intptr_t)GP_GetBaseParseGroup((TGenericParser2) VMA(1));

	case G_GPG_GET_NAME:
		return (intptr_t)GPG_GetName((TGPGroup) VMA(1), (char *) VMA(2));
	case G_GPG_GET_NEXT:
		return (intptr_t)GPG_GetNext((TGPGroup) VMA(1));
	case G_GPG_GET_INORDER_NEXT:
		return (intptr_t)GPG_GetInOrderNext((TGPGroup) VMA(1));
	case G_GPG_GET_INORDER_PREVIOUS:
		return (intptr_t)GPG_GetInOrderPrevious((TGPGroup) VMA(1));
	case G_GPG_GET_PAIRS:
		return (intptr_t)GPG_GetPairs((TGPGroup) VMA(1));
	case G_GPG_GET_INORDER_PAIRS:
		return (intptr_t)GPG_GetInOrderPairs((TGPGroup) VMA(1));
	case G_GPG_GET_SUBGROUPS:
		return (intptr_t)GPG_GetSubGroups((TGPGroup) VMA(1));
	case G_GPG_GET_INORDER_SUBGROUPS:
		return (intptr_t)GPG_GetInOrderSubGroups((TGPGroup) VMA(1));
	case G_GPG_FIND_SUBGROUP:
		return (intptr_t)GPG_FindSubGroup((TGPGroup) VMA(1), (char *) VMA(2));
	case G_GPG_FIND_PAIR:
		return (intptr_t)GPG_FindPair((TGPGroup) VMA(1), (const char *) VMA(2));
	case G_GPG_FIND_PAIRVALUE:
		return (intptr_t)GPG_FindPairValue((TGPGroup) VMA(1), (const char *) VMA(2), (const char *) VMA(3), (char *) VMA(4));

	case G_GET_WORLD_BOUNDS:
		CM_ModelBounds(0, (vec_t *)VMA(1), (vec_t *)VMA(2));
		return 0;

	case G_GPV_GET_NAME:
		return (intptr_t)GPV_GetName((TGPValue) VMA(1), (char *) VMA(2));
	case G_GPV_GET_NEXT:
		return (intptr_t)GPV_GetNext((TGPValue) VMA(1));
	case G_GPV_GET_INORDER_NEXT:
		return (intptr_t)GPV_GetInOrderNext((TGPValue) VMA(1));
	case G_GPV_GET_INORDER_PREVIOUS:
		return (intptr_t)GPV_GetInOrderPrevious((TGPValue) VMA(1));
	case G_GPV_IS_LIST:
		return (intptr_t)GPV_IsList((TGPValue) VMA(1));
	case G_GPV_GET_TOP_VALUE:
		{
			const char * topValue = GPV_GetTopValue((TGPValue) VMA(1));
			if (topValue)
			{
				strcpy((char *) VMA(2), topValue);
			}
			return 0;
		}
	case G_GPV_GET_LIST:
		return (intptr_t)GPV_GetList((TGPValue) VMA(1));

	case G_GT_INIT:
		//SOF2 TODO
		//  const char* gametype, qboolean restart
		return 0;
	case G_GT_RUNFRAME:
		//SOF2 TODO
		// int time
		return 0;
	case G_GT_START:
		//SOF2 TODO
		// int time
		return 0;
	case G_GT_SENDEVENT:
		//SOF2 TODO
		// int event, int time, int arg0, int arg1, int arg2, int arg3, int arg4
		return 0;

	default:
		Com_Error( ERR_DROP, "Bad game system trap: %ld", (long int) args[0] );
	}
	return -1;
}

void SV_InitGame( qboolean restart ) {
	int i=0;
	client_t *cl = NULL;

	// clear level pointers
	sv.entityParsePoint = CM_EntityString();
	for ( i=0, cl=svs.clients; i<sv_maxclients->integer; i++, cl++ )
		cl->gentity = NULL;

	GVM_InitGame( sv.time, Com_Milliseconds(), restart );
}

void SV_BindGame( void ) {
	gvm = VM_Create( VM_GAME, SV_GameSystemCalls, VMI_COMPILED );
	if ( !gvm ) {
		svs.gameStarted = qfalse;
		Com_Error( ERR_DROP, "VM_Create on game failed" );
	}
}

void SV_UnbindGame( void ) {
	GVM_ShutdownGame( qfalse );
	VM_Free( gvm );
	gvm = NULL;
}

void SV_RestartGame( void ) {
	GVM_ShutdownGame( qtrue );

	gvm = VM_Restart( gvm );
	SV_BindGame();
	if ( !gvm ) {
		svs.gameStarted = qfalse;
		Com_Error( ERR_DROP, "VM_Restart on game failed" );
		return;
	}

	SV_InitGame( qtrue );
}
