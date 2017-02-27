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

// cl_cgameapi.cpp  -- client system interaction with client game
#include "qcommon/cm_public.h"
#include "client.h"
#include "cl_cgameapi.h"
#include "cl_uiapi.h"
#include "botlib/botlib.h"
#include "snd_ambient.h"
#include "FXExport.h"
#include "FxUtil.h"

#include "ghoul2/ghoul2_shared.h"

#include "qcommon/GenericParser2.h"
#include "materials.h"

extern IHeapAllocator *G2VertSpaceClient;
extern botlib_export_t *botlib_export;

// cgame interface
static vm_t *cgvm; // cgame vm, valid for legacy and new api

qboolean VM_IsNative( const vm_t *vm );

//
// cgame vmMain calls
//
void CGVM_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	VM_Call( cgvm, CG_INIT, serverMessageNum, serverCommandSequence, clientNum );
}

void CGVM_Shutdown( void ) {
	VM_Call( cgvm, CG_SHUTDOWN );
}

qboolean CGVM_ConsoleCommand( void ) {
	return (qboolean)VM_Call( cgvm, CG_CONSOLE_COMMAND );
}

void CGVM_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, serverTime, stereoView, demoPlayback );
}

int CGVM_CrosshairPlayer( void ) {
	return VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
}

int CGVM_LastAttacker( void ) {
	return VM_Call( cgvm, CG_LAST_ATTACKER );
}

void CGVM_KeyEvent( int key, qboolean down ) {
	VM_Call( cgvm, CG_KEY_EVENT, key, down );
}

void CGVM_MouseEvent( int x, int y ) {
	VM_Call( cgvm, CG_MOUSE_EVENT, x, y );
}

void CGVM_EventHandling( int type ) {
	VM_Call( cgvm, CG_EVENT_HANDLING, type );
}

int CGVM_PointContents( void ) {
	return VM_Call( cgvm, CG_POINT_CONTENTS );
}

void CGVM_GetLerpOrigin( void ) {
	VM_Call( cgvm, CG_GET_LERP_ORIGIN );
}

void CGVM_GetLerpAngles( void ) {
	VM_Call( cgvm, CG_GET_LERP_ANGLES );
}

void CGVM_GetModelScale( void ) {
	VM_Call( cgvm, CG_GET_MODEL_SCALE );
}

// Input data with entnum already set
// Output to origin, angles, and modelscale
void CGVM_GetLerpData( TCGVectorData *data, vec3_t *origin, vec3_t *angles, vec3_t *modelscale ) {
	if( origin )
	{
		VM_Call( cgvm, CG_GET_LERP_ORIGIN );
		VectorCopy( data->mPoint, *origin );
	}

	if( angles )
	{
		VM_Call( cgvm, CG_GET_LERP_ANGLES );
		VectorCopy( data->mPoint, *angles );
	}

	if( modelscale )
	{
		VM_Call( cgvm, CG_GET_MODEL_SCALE );
		VectorCopy( data->mPoint, *modelscale );
	}
}

void CGVM_Trace( void ) {
	VM_Call( cgvm, CG_TRACE );
}

int CGVM_RagCallback( int callType ) {
	return 0;
	//return VM_Call( cgvm, CG_RAG_CALLBACK, callType );
}

void CGVM_GetOrigin( int entID, vec3_t out ) {
	VM_Call( cgvm, CG_GET_ORIGIN, entID, reinterpret_cast< intptr_t >( out ) );
}

void CGVM_GetAngles( int entID, vec3_t out ) {
	VM_Call( cgvm, CG_GET_ANGLES, entID, reinterpret_cast< intptr_t >( out ) );
}

trajectory_t *CGVM_GetOriginTrajectory( int entID ) {
	return (trajectory_t *)VM_Call( cgvm, CG_GET_ORIGIN_TRAJECTORY, entID );
}

trajectory_t *CGVM_GetAngleTrajectory( int entID ) {
	return (trajectory_t *)VM_Call( cgvm, CG_GET_ANGLE_TRAJECTORY, entID );
}

void CGVM_MapChange( void ) {
	VM_Call( cgvm, CG_MAP_CHANGE );
}

void CGVM_MiscEnt( void ) {
	VM_Call( cgvm, CG_MISC_ENT );
}

void CGVM_CameraShake( void ) {
	VM_Call( cgvm, CG_FX_CAMERASHAKE );
}


#define GhoulHandleVMCheck( x ) ( VM_IsNative(cgvm) ? (CGhoul2Info_v *)x : GhoulHandle(x) )

//
// cgame syscalls
//

int CM_LoadSubBSP( const char *name, qboolean clientload ); //cm_load.cpp
void FX_FeedTrail( effectTrailArgStruct_t *a ); //FxPrimitives.cpp

// wrappers and such

static void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

static void CL_CM_LoadMap( const char *mapname, qboolean subBSP ) {
	if ( subBSP )	CM_LoadSubBSP( va( "maps/%s.bsp", mapname+1 ), qfalse );
	else			CM_LoadMap( mapname, qtrue, NULL );
}

static void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}

static void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

static void RegisterSharedMemory( char *memory ) {
	cl.mSharedMemory = memory;
}

static int CL_Milliseconds( void ) {
	return Sys_Milliseconds();
}

static int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}

static void CGFX_AddLine( vec3_t start, vec3_t end, float size1, float size2, float sizeParm, float alpha1, float alpha2, float alphaParm, vec3_t sRGB, vec3_t eRGB, float rgbParm, int killTime, qhandle_t shader, int flags ) {
	FX_AddLine( start, end, size1, size2, sizeParm, alpha1, alpha2, alphaParm, sRGB, eRGB, rgbParm, killTime, shader, flags );
}

static void CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*static qboolean CGFX_PlayBoltedEffectID( int id, vec3_t org, void *ghoul2, const int boltNum, const int entNum, const int modelNum, int iLooptime, qboolean isRelative ) {
	if ( !ghoul2 ) return qfalse;

	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	int boltInfo=0;
	if ( re->G2API_AttachEnt( &boltInfo, g2, modelNum, boltNum, entNum, modelNum ) )
	{
		FX_PlayBoltedEffectID(id, org, boltInfo, &g2, iLooptime, isRelative );
		return qtrue;
	}
	return qfalse;
}*/

/*static qboolean CL_SE_GetStringTextString( const char *text, char *buffer, int bufferLength ) {
	const char *str;

	assert( text && buffer );

	str = SE_GetString( text );

	if ( str[0] ) {
		Q_strncpyz( buffer, str, bufferLength );
		return qtrue;
	}

	Com_sprintf( buffer, bufferLength, "??%s", str );
	return qfalse;
}*/

static void CL_G2API_ListModelSurfaces( void *ghlInfo ) {
	re->G2API_ListSurfaces( (CGhoul2Info *)ghlInfo );
}

static void CL_G2API_ListModelBones( void *ghlInfo, int frame ) {
	re->G2API_ListBones( (CGhoul2Info *)ghlInfo, frame );
}

static void CL_G2API_SetGhoul2ModelIndexes( void *ghoul2, qhandle_t *modelList, qhandle_t *skinList ) {
	if ( !ghoul2 ) return;
	re->G2API_SetGhoul2ModelIndexes( *((CGhoul2Info_v *)ghoul2), modelList, skinList );
}

static qboolean CL_G2API_HaveWeGhoul2Models( void *ghoul2) {
	if ( !ghoul2 ) return qfalse;
	return re->G2API_HaveWeGhoul2Models( *((CGhoul2Info_v *)ghoul2) );
}

static qboolean CL_G2API_GetBoltMatrix( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	if ( !ghoul2 ) return qfalse;
	return re->G2API_GetBoltMatrix( *((CGhoul2Info_v *)ghoul2), modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale );
}

/*static qboolean CL_G2API_GetBoltMatrix_NoReconstruct( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	if ( !ghoul2 ) return qfalse;
	re->G2API_BoltMatrixReconstruction( qfalse );
	return re->G2API_GetBoltMatrix( *((CGhoul2Info_v *)ghoul2), modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale );
}

static qboolean CL_G2API_GetBoltMatrix_NoRecNoRot( void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale ) {
	if ( !ghoul2 ) return qfalse;
	re->G2API_BoltMatrixSPMethod( qtrue );
	return re->G2API_GetBoltMatrix( *((CGhoul2Info_v *)ghoul2), modelIndex, boltIndex, matrix, angles, position, frameNum, modelList, scale );
}*/

/*static int CL_G2API_InitGhoul2Model( void **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias ) {
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	return re->G2API_InitGhoul2Model( (CGhoul2Info_v **)ghoul2Ptr, fileName, modelIndex, customSkin, customShader, modelFlags, lodBias );
}*/

/*static qboolean CL_G2API_SetSkin( void *ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin ) {
	if ( !ghoul2 ) return qfalse;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	CGhoul2Info *ghlinfo = re->G2API_GetInfo(g2, modelIndex);
	return re->G2API_SetSkin( ghlinfo, customSkin, renderSkin );
}*/

static void CL_G2API_CollisionDetect( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius ) {
	if ( !ghoul2 ) return;
	re->G2API_CollisionDetect( collRecMap, *((CGhoul2Info_v *)ghoul2), angles, position, frameNumber, entNum, rayStart, rayEnd, scale, G2VertSpaceClient, traceFlags, useLod, fRadius );
}

/*static void CL_G2API_CollisionDetectCache( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, int traceFlags, int useLod, float fRadius ) {
	if ( !ghoul2 ) return;
	re->G2API_CollisionDetectCache( collRecMap, *((CGhoul2Info_v *)ghoul2), angles, position, frameNumber, entNum, rayStart, rayEnd, scale, G2VertSpaceClient, traceFlags, useLod, fRadius );
}*/

static void CL_G2API_CleanGhoul2Models( void **ghoul2Ptr ) {
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	re->G2API_CleanGhoul2Models( (CGhoul2Info_v **)ghoul2Ptr );
}

static qboolean CL_G2API_SetBoneAngles( void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags, const int up, const int right, const int forward, qhandle_t *modelList, int blendTime , int currentTime ) {
	if ( !ghoul2 ) return qfalse;
	return re->G2API_SetBoneAngles( *((CGhoul2Info_v *)ghoul2), modelIndex, boneName, angles, flags, (const Eorientations)up, (const Eorientations)right, (const Eorientations)forward, modelList, blendTime , currentTime );
}

static qboolean CL_G2API_SetBoneAnim( void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime ) {
	if ( !ghoul2 ) return qfalse;
	return re->G2API_SetBoneAnim( *((CGhoul2Info_v *)ghoul2), modelIndex, boneName, startFrame, endFrame, flags, animSpeed, currentTime, setFrame, blendTime );
}

/*static qboolean CL_G2API_GetBoneAnim( void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *startFrame, int *endFrame, int *flags, float *animSpeed, int *modelList, const int modelIndex ) {
	if ( !ghoul2 ) return qfalse;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	return re->G2API_GetBoneAnim( g2, modelIndex, boneName, currentTime, currentFrame, startFrame, endFrame, flags, animSpeed, modelList );
}

static qboolean CL_G2API_GetBoneFrame( void *ghoul2, const char *boneName, const int currentTime, float *currentFrame, int *modelList, const int modelIndex ) {
	if ( !ghoul2 ) return qfalse;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	int iDontCare1 = 0, iDontCare2 = 0, iDontCare3 = 0;
	float fDontCare1 = 0;

	return re->G2API_GetBoneAnim(g2, modelIndex, boneName, currentTime, currentFrame, &iDontCare1, &iDontCare2, &iDontCare3, &fDontCare1, modelList);
}*/

static void CL_G2API_GetGLAName( void *ghoul2, int modelIndex, char *fillBuf ) {
	if ( !ghoul2 )
	{
		fillBuf[0] = '\0';
		return;
	}

	char *tmp = re->G2API_GetGLAName( *((CGhoul2Info_v *)ghoul2), modelIndex );
	if ( tmp )
		strcpy( fillBuf, tmp );
	else
		fillBuf[0] = '\0';
}

/*static int CL_G2API_CopyGhoul2Instance( void *g2From, void *g2To, int modelIndex ) {
	if ( !g2From || !g2To ) return 0;

	return re->G2API_CopyGhoul2Instance( *((CGhoul2Info_v *)g2From), *((CGhoul2Info_v *)g2To), modelIndex );
}

static void CL_G2API_CopySpecificGhoul2Model( void *g2From, int modelFrom, void *g2To, int modelTo ) {
	if ( !g2From || !g2To) return;
	re->G2API_CopySpecificG2Model( *((CGhoul2Info_v *)g2From), modelFrom, *((CGhoul2Info_v *)g2To), modelTo );
}

static void CL_G2API_DuplicateGhoul2Instance( void *g2From, void **g2To ) {
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	if ( !g2From || !g2To ) return;
	re->G2API_DuplicateGhoul2Instance( *((CGhoul2Info_v *)g2From), (CGhoul2Info_v **)g2To );
}*/

/*static qboolean CL_G2API_HasGhoul2ModelOnIndex( void *ghlInfo, int modelIndex ) {
	return re->G2API_HasGhoul2ModelOnIndex( (CGhoul2Info_v **)ghlInfo, modelIndex );
}*/

/*static qboolean CL_G2API_RemoveGhoul2Model( void *ghlInfo, int modelIndex ) {
#ifdef _FULL_G2_LEAK_CHECKING
		g_G2AllocServer = 0;
#endif
	return re->G2API_RemoveGhoul2Model( (CGhoul2Info_v **)ghlInfo, modelIndex );
}*/

/*static int CL_G2API_GetNumGoreMarks( void *ghlInfo, int modelIndex ) {
#ifdef _G2_GORE
	if ( !ghlInfo ) return 0;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghlInfo);
	return re->G2API_GetNumGoreMarks( g2, modelIndex );
#else
	return 0;
#endif
}*/

static void CL_G2API_AddSkinGore( void *ghlInfo, SSkinGoreData *gore ) {
#ifdef _G2_GORE
	if ( !ghlInfo ) return;
	re->G2API_AddSkinGore( *((CGhoul2Info_v *)ghlInfo), *(SSkinGoreData *)gore );
#endif
}

static void CL_G2API_ClearSkinGore( void *ghlInfo ) {
#ifdef _G2_GORE
	if ( !ghlInfo ) return;
	re->G2API_ClearSkinGore( *((CGhoul2Info_v *)ghlInfo) );
#endif
}

static int CL_G2API_AddBolt( void *ghoul2, int modelIndex, const char *boneName ) {
	if ( !ghoul2 ) return 0;
	return re->G2API_AddBolt( *((CGhoul2Info_v *)ghoul2), modelIndex, boneName );
}

/*static qboolean CL_G2API_AttachEnt( int *boltInfo, void *ghlInfoTo, int toBoltIndex, int entNum, int toModelNum ) {
	if ( !ghlInfoTo ) return qfalse;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghlInfoTo);
	return re->G2API_AttachEnt( boltInfo, g2, 0, toBoltIndex, entNum, toModelNum );
}*/

static void CL_G2API_SetBoltInfo( void *ghoul2, int modelIndex, int boltInfo ) {
	if ( !ghoul2 ) return;
	re->G2API_SetBoltInfo( *((CGhoul2Info_v *)ghoul2), modelIndex, boltInfo );
}

static qboolean CL_G2API_SetRootSurface( void *ghoul2, const int modelIndex, const char *surfaceName ) {
	if ( !ghoul2 ) return qfalse;
	return re->G2API_SetRootSurface( *((CGhoul2Info_v *)ghoul2), modelIndex, surfaceName );
}

static qboolean CL_G2API_SetSurfaceOnOff( void *ghoul2, int modelIndex, const char *surfaceName, const int flags ) {
	if ( !ghoul2 ) return qfalse;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	//CGhoul2Info *ghlinfo = re->G2API_GetInfo(g2, modelIndex);
	return re->G2API_SetSurfaceOnOff( g2, modelIndex, surfaceName, flags );
}

static qboolean CL_G2API_SetNewOrigin( void *ghoul2, int modelIndex, const int boltIndex ) {
	if ( !ghoul2 ) return qfalse;
	CGhoul2Info_v &g2 = *((CGhoul2Info_v *)ghoul2);
	//CGhoul2Info *ghlinfo = re->G2API_GetInfo(g2, modelIndex);
	return re->G2API_SetNewOrigin( g2, modelIndex, boltIndex );
}

static void CL_Key_SetCatcher( int catcher ) {
	// Don't allow the cgame module to close the console
	Key_SetCatcher( catcher | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
}

// legacy syscall

intptr_t CL_CgameSystemCalls( intptr_t *args ) {
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

	case CG_PRINT:
		Com_Printf( "%s", (const char*)VMA(1) );
		return 0;

	case CG_ERROR:
		Com_Error( ERR_DROP, "%s", (const char*)VMA(1) );
		return 0;

	case CG_MILLISECONDS:
		return CL_Milliseconds();

	case CG_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *)VMA(1), (const char *)VMA(2), (const char *)VMA(3), args[4], VMF(5), VMF(6) );
		return 0;

	case CG_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *)VMA(1) );
		return 0;

	case CG_CVAR_SET:
		Cvar_VM_Set( (const char *)VMA(1), (const char *)VMA(2), VM_CGAME );
		return 0;

	case CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( (const char *)VMA(1), (char *)VMA(2), args[3] );
		return 0;
	case CG_ARGC:
		return Cmd_Argc();

	case CG_ARGV:
		Cmd_ArgvBuffer( args[1], (char *)VMA(2), args[3] );
		return 0;

	case CG_ARGS:
		Cmd_ArgsBuffer( (char *)VMA(1), args[2] );
		return 0;

	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode( (const char *)VMA(1), (int *)VMA(2), (fsMode_t)args[3] );

	case CG_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;

	case CG_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;

	case CG_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;

	case CG_FS_GETFILELIST:
		return FS_GetFileList( (const char *)VMA(1), (const char *)VMA(2), (char *)VMA(3), args[4] );

	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText( (const char *)VMA(1) );
		return 0;

	case CG_ADDCOMMAND:
		CL_AddCgameCommand( (const char *)VMA(1) );
		return 0;

	case CG_REMOVECOMMAND:
		Cmd_VM_RemoveCommand( (const char *)VMA(1), VM_CGAME );
		return 0;

	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand( (const char *)VMA(1), qfalse );
		return 0;

	case CG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
		//		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
		// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
		// if there is a map change while we are downloading at pk3.
		// ZOID
		SCR_UpdateScreen();
		return 0;

	case CG_CM_LOADMAP:
		CL_CM_LoadMap( (const char *)VMA(1), (qboolean)args[2] );
		return 0;

	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();

	case CG_CM_INLINEMODEL:
		return CM_InlineModel( args[1] );

	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( (const float *)VMA(1), (const float *)VMA(2), /*int capsule*/ qfalse );

	case CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel( (const float *)VMA(1), (const float *)VMA(2), /*int capsule*/ qtrue );

	case CG_CM_POINTCONTENTS:
		return CM_PointContents( (const float *)VMA(1), args[2] );

	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContents( (const float *)VMA(1), args[2], (const float *)VMA(3), (const float *)VMA(4) );

	case CG_CM_BOXTRACE:
		CM_BoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], /*int capsule*/ qfalse );
		return 0;

	case CG_CM_CAPSULETRACE:
		CM_BoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], /*int capsule*/ qtrue );
		return 0;

	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], (const float *)VMA(8), (const float *)VMA(9), /*int capsule*/ qfalse );
		return 0;

	case CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTrace( (trace_t *)VMA(1), (const float *)VMA(2), (const float *)VMA(3), (const float *)VMA(4), (const float *)VMA(5), args[6], args[7], (const float *)VMA(8), (const float *)VMA(9), /*int capsule*/ qtrue );
		return 0;

	case CG_CM_MARKFRAGMENTS:
		return re->MarkFragments( args[1], (const vec3_t *)VMA(2), (const float *)VMA(3), args[4], (float *)VMA(5), args[6], (markFragment_t *)VMA(7) );

	case CG_S_STARTSOUND:
		S_StartSound( (float *)VMA(1), args[2], args[3], args[4], args[5], args[6] );
		return 0;

	case CG_S_STARTLOCALSOUND:
		S_StartLocalSound( args[1], args[2] );
		return 0;

	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds( args[1]?qtrue:qfalse );
		return 0;

	case CG_S_ADDLOOPINGSOUND:
		S_AddLoopingSound( args[1], (const float *)VMA(2), (const float *)VMA(3), VMF(4), args[5] );
		return 0;

	case CG_S_ADDREALLOOPINGSOUND:
		///*S_AddRealLoopingSound*/S_AddLoopingSound( args[1], (const float *)VMA(2), (const float *)VMA(3), args[4] );
		S_AddLoopingSound( args[1], (const float *)VMA(2), (const float *)VMA(3), VMF(4), args[5] );
		return 0;

	case CG_S_STOPLOOPINGSOUND:
		S_StopLoopingSound( args[1] );
		return 0;

	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[1], (const float *)VMA(2) );
		return 0;

	case CG_S_RESPATIALIZE:
		S_Respatialize( args[1], (const float *)VMA(2), (vec3_t *)VMA(3), args[4] );
		return 0;
	case CG_S_REGISTERSOUND:
		return S_RegisterSound( (const char *)VMA(1) );

	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( (const char *)VMA(1), (const char *)VMA(2), args[3]?qtrue:qfalse );
		return 0;

	case CG_AS_PARSESETS:
		AS_ParseSets();
		return 0;

	case CG_AS_ADDPRECACHEENTRY:
		AS_AddPrecacheEntry((const char *)VMA(1));
		return 0;
	case CG_AS_GETBMODELSOUND:
		return AS_GetBModelSound((const char *)VMA(1), args[2]);

	case CG_R_LOADWORLDMAP:
		re->LoadWorld( (const char *)VMA(1) );
		return 0;

	case CG_R_REGISTERMODEL:
		return re->RegisterModel( (const char *)VMA(1) );

	case CG_R_REGISTERSKIN:
		return re->RegisterSkin( (const char *)VMA(1), 0, NULL );

	case CG_R_REGISTERSHADER:
		return re->RegisterShader( (const char *)VMA(1) );

	case CG_R_REGISTERSHADERNOMIP:
		return re->RegisterShaderNoMip( (const char *)VMA(1) );

	case CG_R_REGISTERFONT:
		return re->RegisterFont( (const char *)VMA(1) );

	case CG_R_CLEARSCENE:
		re->ClearScene();
		return 0;

	case CG_R_CLEARDECALS:
		re->ClearDecals();
		return 0;

	case CG_R_ADDREFENTITYTOSCENE:
		re->AddRefEntityToScene( (const refEntity_t *)VMA(1) );
		return 0;

	case CG_R_ADDPOLYTOSCENE:
		re->AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), 1 );
		return 0;

	case CG_R_ADDPOLYSTOSCENE:
		re->AddPolyToScene( args[1], args[2], (const polyVert_t *)VMA(3), args[4] );
		return 0;

	case CG_R_ADDDECALTOSCENE:
		re->AddDecalToScene( (qhandle_t)args[1], (const float*)VMA(2), (const float*)VMA(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), (qboolean)args[9], VMF(10), (qboolean)args[11] );
		return 0;

	case CG_R_LIGHTFORPOINT:
		return re->LightForPoint( (float *)VMA(1), (float *)VMA(2), (float *)VMA(3), (float *)VMA(4) );

	case CG_R_ADDLIGHTTOSCENE:
		re->AddLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;

	case CG_R_ADDADDITIVELIGHTTOSCENE:
		re->AddAdditiveLightToScene( (const float *)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;

	case CG_R_RENDERSCENE:
		re->RenderScene( (const refdef_t *)VMA(1) );
		return 0;

	case CG_R_SETCOLOR:
		re->SetColor( (const float *)VMA(1) );
		return 0;

	case CG_R_DRAWSTRETCHPIC:
		re->DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[10] );
		return 0;

	case CG_R_MODELBOUNDS:
		re->ModelBounds( args[1], (float *)VMA(2), (float *)VMA(3) );
		return 0;

	case CG_R_LERPTAG:
		return re->LerpTag( (orientation_t *)VMA(1), args[2], args[3], args[4], VMF(5), (const char *)VMA(6) );

	case CG_R_DRAWROTATEPIC:
		re->DrawRotatePic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), VMF(9), args[10] );
		return 0;

	case CG_R_DRAWROTATEPIC2:
		re->DrawRotatePic2( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), VMF(9), args[10] );
		return 0;
	
	case CG_GETGLCONFIG:
		CL_GetGlconfig( (glconfig_t *)VMA(1) );
		return 0;

	case CG_GETGAMESTATE:
		CL_GetGameState( (gameState_t *)VMA(1) );
		return 0;

	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber( (int *)VMA(1), (int *)VMA(2) );
		return 0;

	case CG_GETSNAPSHOT:
		return CL_GetSnapshot( args[1], (snapshot_t *)VMA(2) );

	case CG_GETDEFAULTSTATE:
		return CL_GetDefaultState(args[1], (entityState_t *)VMA(2));

	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand( args[1] );

	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();

	case CG_GETUSERCMD:
		return CL_GetUserCmd( args[1], (struct usercmd_s *)VMA(2) );

	case CG_SETUSERCMDVALUE:
		CL_SetUserCmdValue( args[1], VMF(2) );
		return 0;

	case CG_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();

	case CG_KEY_ISDOWN:
		return Key_IsDown( args[1] );

	case CG_KEY_GETCATCHER:
		return Key_GetCatcher();

	case CG_KEY_SETCATCHER:
		CL_Key_SetCatcher( args[1] );
		return 0;

	case CG_KEY_GETKEY:
		return Key_GetKey( (const char *)VMA(1) );

	case CG_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( (char *)VMA(1) );

	case CG_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( (const char *)VMA(1) );

	case CG_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );

	case CG_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], (struct pc_token_s *)VMA(2) );

	case CG_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], (char *)VMA(2), (int *)VMA(3) );

	case CG_PC_LOAD_GLOBAL_DEFINES:
		return botlib_export->PC_LoadGlobalDefines ( (char *)VMA(1) );

	case CG_PC_REMOVE_ALL_GLOBAL_DEFINES:
		botlib_export->PC_RemoveAllGlobalDefines ( );
		return 0;

	case CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case CG_REAL_TIME:
		return Com_RealTime( (struct qtime_s *)VMA(1) );

	case CG_SNAPVECTOR:
		Sys_SnapVector( (float *)VMA(1) );
		return 0;

	case CG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((const char *)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case CG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case CG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case CG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case CG_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case CG_R_REMAP_SHADER:
		re->RemapShader( (const char *)VMA(1), (const char *)VMA(2), (const char *)VMA(3) );
		return 0;

	case CG_R_GET_LIGHT_STYLE:
		re->GetLightStyle(args[1], (unsigned char *)VMA(2));
		return 0;

	case CG_R_SET_LIGHT_STYLE:
		re->SetLightStyle(args[1], args[2]);
		return 0;

	case CG_GET_ENTITY_TOKEN:
		return re->GetEntityToken( (char *)VMA(1), args[2] );

	case CG_R_INPVS:
		//SOF2 TODO
		return re->inPVS( (const float *)VMA(1), (const float *)VMA(2), 0 );
		//return re->inPVS( (const float *)VMA(1), (const float *)VMA(2), (byte *)VMA(3) );

#ifndef DEBUG_DISABLEFXCALLS
	case CG_FX_ADDLINE:
		CGFX_AddLine( (float *)VMA(1), (float *)VMA(2), VMF(3), VMF(4), VMF(5),
			VMF(6), VMF(7), VMF(8),
			(float *)VMA(9), (float *)VMA(10), VMF(11),
			args[12], args[13], args[14]);
		return 0;
	case CG_FX_REGISTER_EFFECT:
		return FX_RegisterEffect((const char *)VMA(1));

	case CG_FX_PLAY_EFFECT:
		FX_PlayEffect((const char *)VMA(1), (float *)VMA(2), (float *)VMA(3), args[4], args[5] );
		return 0;

	case CG_FX_PLAY_ENTITY_EFFECT:
		assert(0);
		return 0;

	case CG_FX_PLAY_EFFECT_ID:
		FX_PlayEffectID(args[1], (float *)VMA(2), (float *)VMA(3), args[4], args[5] );
		return 0;

	case CG_FX_PLAY_ENTITY_EFFECT_ID:
		FX_PlayEntityEffectID(args[1], (float *)VMA(2), (vec3_t *)VMA(3), args[4], args[5], args[6], args[7] );
		return 0;

	case CG_FX_PLAY_BOLTED_EFFECT_ID:
		{
			//SOF2 TODO Temporarily use FX_PlayEffectID here.
			//(int id,CFxBoltInterface *obj, int vol, int rad)
			CFxBoltInterface * boltInterface = (CFxBoltInterface *) VMA(2);
			FX_PlayEffectID(args[1], boltInterface->origin, boltInterface->forward, args[3], args[4]);
			return 0;
		}
		//return CGFX_PlayBoltedEffectID( args[1], (float *)VMA(2), (void *)args[3], args[4], args[5], args[6], args[7], (qboolean)args[8] );

	case CG_FX_ADD_SCHEDULED_EFFECTS:
		FX_AddScheduledEffects(qfalse);
		return 0;

	case CG_FX_DRAW_2D_EFFECTS:
		FX_Draw2DEffects ( VMF(1), VMF(2) );
		return 0;

	case CG_FX_INIT_SYSTEM:
		return FX_InitSystem( (refdef_t*)VMA(1) );

	case CG_FX_FREE_SYSTEM:
		return FX_FreeSystem();

	case CG_FX_ADJUST_TIME:
		FX_AdjustTime(args[1]);
		return 0;

	case CG_FX_RESET:
		FX_Free ( false );
		return 0;

#else
	case CG_FX_REGISTER_EFFECT:
	case CG_FX_PLAY_EFFECT:
	case CG_FX_PLAY_ENTITY_EFFECT:
	case CG_FX_PLAY_EFFECT_ID:
	case CG_FX_PLAY_PORTAL_EFFECT_ID:
	case CG_FX_PLAY_ENTITY_EFFECT_ID:
	case CG_FX_PLAY_BOLTED_EFFECT_ID:
	case CG_FX_ADD_SCHEDULED_EFFECTS:
	case CG_FX_INIT_SYSTEM:
	case CG_FX_FREE_SYSTEM:
	case CG_FX_ADJUST_TIME:
	case CG_FX_ADDPOLY:
	case CG_FX_ADDBEZIER:
	case CG_FX_ADDPRIMITIVE:
	case CG_FX_ADDSPRITE:
	case CG_FX_ADDELECTRICITY:
		return 0;
#endif

	case CG_G2_LISTSURFACES:
		CL_G2API_ListModelSurfaces( (void *)args[1] );
		return 0;

	case CG_G2_LISTBONES:
		CL_G2API_ListModelBones( (void *)args[1], args[2]);
		return 0;

	case CG_G2_HAVEWEGHOULMODELS:
		return CL_G2API_HaveWeGhoul2Models( (void *)args[1] );

	case CG_G2_SETMODELS:
		CL_G2API_SetGhoul2ModelIndexes( (void *)args[1],(qhandle_t *)VMA(2),(qhandle_t *)VMA(3));
		return 0;

	case CG_G2_GETBOLT:
		return CL_G2API_GetBoltMatrix((void *)args[1], args[2], args[3], (mdxaBone_t *)VMA(4), (const float *)VMA(5),(const float *)VMA(6), args[7], (qhandle_t *)VMA(8), (float *)VMA(9));

	case CG_G2_INITGHOUL2MODEL:
#if id386
		return re->G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4],
									  (qhandle_t) args[5], args[6], args[7]);
#else
		if( VM_IsNative( cgvm ) )
			return re->G2API_InitGhoul2Model((CGhoul2Info_v **)VMA(1), (const char *)VMA(2), args[3], (qhandle_t) args[4],
				(qhandle_t) args[5], args[6], args[7]);
		else
			return re->G2API_VM_InitGhoul2Model((qhandle_t *)VMA(1), (const char *)VMA(2), args[3], (qhandle_t)args[4],
				(qhandle_t)args[5], args[6], args[7]);
#endif

	case CG_G2_SETSKIN:
	{
		CGhoul2Info_v &g2 = *(GhoulHandleVMCheck(args[1]));
		CGhoul2Info *ghlinfo = re->G2API_GetInfo(g2, (qhandle_t)args[2]);

		return re->G2API_SetSkin(ghlinfo, args[3], 0);
	}

	case CG_G2_COLLISIONDETECT:
		CL_G2API_CollisionDetect ( (CollisionRecord_t*)VMA(1), (void *)args[2], (const float*)VMA(3), (const float*)VMA(4), args[5], args[6], (float*)VMA(7), (float*)VMA(8), (float*)VMA(9), args[10], args[11], VMF(12) );
		return 0;

	/*case CG_G2_COLLISIONDETECTCACHE:
		CL_G2API_CollisionDetectCache ( (CollisionRecord_t*)VMA(1), (void *)args[2], (const float*)VMA(3), (const float*)VMA(4), args[5], args[6], (float*)VMA(7), (float*)VMA(8), (float*)VMA(9), args[10], args[11], VMF(12) );
		return 0;*/

	case CG_G2_ANGLEOVERRIDE:
		return CL_G2API_SetBoneAngles( (void *)args[1], args[2], (const char *)VMA(3), (float *)VMA(4), args[5], args[6], args[7], args[8], (qhandle_t *)VMA(9), args[10], args[11] );

	case CG_G2_CLEANMODELS:
#if id386
		CL_G2API_CleanGhoul2Models( (void **)VMA(1) );
#else
		if( VM_IsNative( cgvm ) )
			re->G2API_CleanGhoul2Models((CGhoul2Info_v **)VMA(1));
		else
			re->G2API_VM_CleanGhoul2Models((qhandle_t *)VMA(1));
#endif
		return 0;

	case CG_G2_PLAYANIM:
		return CL_G2API_SetBoneAnim( (void *)args[1], args[2], (const char *)VMA(3), args[4], args[5], args[6], VMF(7), args[8], VMF(9), args[10] );

	case CG_G2_GETGLANAME:
		return (intptr_t)re->G2API_GetGLAName(*(GhoulHandleVMCheck(args[1])), args[2]);
		//CL_G2API_GetGLAName( (void *)args[1], args[2], (char *)VMA(3) );
		return 0;

	case CG_G2_COPYGHOUL2INSTANCE:
		return (int)re->G2API_CopyGhoul2Instance(GhoulHandleVMCheck(args[1]), GhoulHandleVMCheck(args[2]), args[3]);

	case CG_G2_COPYSPECIFICGHOUL2MODEL:
		re->G2API_CopySpecificG2Model(*(GhoulHandleVMCheck(args[1])), args[2], *(GhoulHandleVMCheck(args[3])), args[4]);
		return 0;

	case CG_G2_DUPLICATEGHOUL2INSTANCE:
#if id386
		re->G2API_DuplicateGhoul2Instance(GhoulHandle(args[1]), (CGhoul2Info_v **)VMA(2));
#else
		if( VM_IsNative( cgvm ) )
			re->G2API_DuplicateGhoul2Instance((CGhoul2Info_v *)args[1], (CGhoul2Info_v **)VMA(2));
		else
			re->G2API_VM_DuplicateGhoul2Instance(GhoulHandle(args[1]), (qhandle_t *)VMA(2));
#endif
		return 0;

	case CG_G2_REMOVEGHOUL2MODEL:
#if id386
		return (int)re->G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);
#else
		if( VM_IsNative( cgvm ) )
			return (int)re->G2API_RemoveGhoul2Model((CGhoul2Info_v **)VMA(1), args[2]);
		else
			return (int)re->G2API_VM_RemoveGhoul2Model((qhandle_t *)VMA(1), args[2]);
#endif

	case CG_G2_ADDSKINGORE:
		CL_G2API_AddSkinGore( (void *)args[1], (SSkinGoreData *)VMA(2));
		return 0;

	case CG_G2_CLEARSKINGORE:
		CL_G2API_ClearSkinGore ( (void *)args[1] );
		return 0;

	case CG_G2_ADDBOLT:
		return CL_G2API_AddBolt( (void *)args[1], args[2], (const char *)VMA(3));

	case CG_G2_SETBOLTON:
		CL_G2API_SetBoltInfo( (void *)args[1], args[2], args[3] );
		return 0;

	case CG_G2_SETROOTSURFACE:
		return CL_G2API_SetRootSurface( (void *)args[1], args[2], (const char *)VMA(3));

	case CG_G2_SETSURFACEONOFF:
		return CL_G2API_SetSurfaceOnOff( (void *)args[1], args[2], (const char *)VMA(3), args[4]);

	case CG_G2_SETNEWORIGIN:
		return CL_G2API_SetNewOrigin( (void *)args[1], args[2], args[3]);

	case CG_SP_GETSTRINGTEXTSTRING:
//	case CG_SP_GETSTRINGTEXT:
		assert(VMA(1));
		assert(VMA(2));

//		if (args[0] == CG_SP_GETSTRINGTEXT)
//		{
//			text = SP_GetStringText( args[1] );
//		}
//		else
		Com_sprintf( (char *) VMA(2), args[3], "??%s", VMA(1) );
		return qfalse;

	case CG_SET_SHARED_BUFFER:
		RegisterSharedMemory( (char *)VMA(1) );
		return 0;

	case CG_CM_REGISTER_TERRAIN:
		return 0;

	case CG_RMG_INIT:
		return 0;

	case CG_RE_INIT_RENDERER_TERRAIN:
		return 0;

	case GP_PARSE:
		if( VM_IsNative( cgvm ) )
			return (intptr_t)GP_Parse((char **) VMA(1), (bool) args[2], (bool) args[3]);
		else
			return GP_VM_Parse( (char **) VMA(1), (bool) args[2], (bool) args[3] );
		//return (intptr_t)GP_Parse((char **) VMA(1), (bool) args[2], (bool) args[3]);
	case GP_PARSE_FILE:
		{
			char * data;
			FS_ReadFile((char *) VMA(1), (void **) &data);
			if( VM_IsNative( cgvm ) )
				return (intptr_t)GP_Parse(&data, (bool) args[2], (bool) args[3]);
			else
				return GP_VM_Parse( &data, (bool) args[2], (bool) args[3] );
			//return (intptr_t)GP_Parse(&data, (bool) args[2], (bool) args[3]);
		}
	case GP_CLEAN:
		if( VM_IsNative( cgvm ) )
			GP_Clean((TGenericParser2) VMA(1));
		else
			GP_VM_Clean( (qhandle_t *)VMA(1) );
		//GP_Clean((TGenericParser2) VMA(1));
		return 0;
	case GP_DELETE:
		if( VM_IsNative( cgvm ) )
			GP_Delete((TGenericParser2 *) VMA(1));
		else
			GP_VM_Delete( (qhandle_t *)VMA(1) );
		//GP_Delete((TGenericParser2 *) VMA(1));
		return 0;
	case GP_GET_BASE_PARSE_GROUP:
		return (intptr_t)GP_GetBaseParseGroup((TGenericParser2) VMA(1));


	case GPG_GET_NAME:
		return (intptr_t)GPG_GetName((TGPGroup) VMA(1), (char *) VMA(2));
	case GPG_GET_NEXT:
		return (intptr_t)GPG_GetNext((TGPGroup) VMA(1));
	case UI_GPG_GET_INORDER_NEXT:
		return (intptr_t)GPG_GetInOrderNext((TGPGroup) VMA(1));
	case GPG_GET_INORDER_PREVIOUS:
		return (intptr_t)GPG_GetInOrderPrevious((TGPGroup) VMA(1));
	case GPG_GET_PAIRS:
		return (intptr_t)GPG_GetPairs((TGPGroup) VMA(1));
	case GPG_GET_INORDER_PAIRS:
		return (intptr_t)GPG_GetInOrderPairs((TGPGroup) VMA(1));
	case GPG_GET_SUBGROUPS:
		return (intptr_t)GPG_GetSubGroups((TGPGroup) VMA(1));
	case GPG_GET_INORDER_SUBGROUPS:
		return (intptr_t)GPG_GetInOrderSubGroups((TGPGroup) VMA(1));
	case GPG_FIND_SUBGROUP:
		return (intptr_t)GPG_FindSubGroup((TGPGroup) VMA(1), (char *) VMA(2));
	case GPG_FIND_PAIR:
		return (intptr_t)GPG_FindPair((TGPGroup) VMA(1), (const char *) VMA(2));
	case GPG_FIND_PAIRVALUE:
		return (intptr_t)GPG_FindPairValue((TGPGroup) VMA(1), (const char *) VMA(2), (const char *) VMA(3), (char *) VMA(4));


	case GPV_GET_NAME:
		return (intptr_t)GPV_GetName((TGPValue) VMA(1), (char *) VMA(2));
	case GPV_GET_NEXT:
		return (intptr_t)GPV_GetNext((TGPValue) VMA(1));
	case GPV_GET_INORDER_NEXT:
		return (intptr_t)GPV_GetInOrderNext((TGPValue) VMA(1));
	case GPV_GET_INORDER_PREVIOUS:
		return (intptr_t)GPV_GetInOrderPrevious((TGPValue) VMA(1));
	case GPV_IS_LIST:
		return (intptr_t)GPV_IsList((TGPValue) VMA(1));
	case GPV_GET_TOP_VALUE:
		{
			const char * topValue = GPV_GetTopValue((TGPValue) VMA(1));
			if (topValue)
			{
				strcpy((char *) VMA(2), topValue);
			}
			return 0;
		}
	case GPV_GET_LIST:
		return (intptr_t)GPV_GetList((TGPValue) VMA(1));


	case CG_VM_LOCALALLOC:
		return (intptr_t)VM_Local_Alloc(args[1]);
	case CG_VM_LOCALALLOCUNALIGNED:
		return (intptr_t)VM_Local_AllocUnaligned(args[1]);
	case CG_VM_LOCALTEMPALLOC:
		return (intptr_t)VM_Local_TempAlloc(args[1]);
	case CG_VM_LOCALTEMPFREE:
		VM_Local_TempFree(args[1]);
		return 0;
	case CG_VM_LOCALSTRINGALLOC:
		return (intptr_t)VM_Local_StringAlloc((char *) VMA(1));


	case CG_R_DRAWTEXT:
		re->Font_DrawString(args[1], args[2], args[3], VMF(4), (vec_t *)VMA(5), (const char *)VMA(6), args[7], args[8], 0, 0);
		return 0;
	case CG_R_DRAWTEXTWITHCURSOR:
		re->Font_DrawString(args[1], args[2], args[3], VMF(4), (vec_t *)VMA(5), (const char *)VMA(6), args[7], args[8], args[9], args[10]);
		return 0;
	case CG_R_GETTEXTWIDTH:
		return re->Font_StrLenPixels((const char *)VMA(1), args[2], VMF(3));
	case CG_R_GETTEXTHEIGHT:
		return re->Font_HeightPixels(args[2], VMF(3));


	case CG_G2_REGISTERSKIN:
		return re->RegisterSkin((const char *)VMA(1), args[2], (char *)VMA(3) );
	case CG_G2_GETANIMFILENAMEINDEX:
		{
			void *testnull = (void *)args[1];
			if(!testnull)
				return 0;
			CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(ghoul2, (qhandle_t)args[2]);
			char * srcFilename;
			qboolean retval = re->G2API_GetAnimFileName(ghlinfo, &srcFilename);
			strncpy((char *) VMA(3), srcFilename, MAX_QPATH);
			return (int) retval;
		}

	case CG_MAT_RESET:
		Mat_Reset();
		return 0;
	case CG_MAT_CACHE:
		Mat_Init();
		return 0;
	case CG_MAT_GET_SOUND:
		return Mat_GetSound((char*) VMA(1), args[2]);
	case CG_MAT_GET_DECAL:
		return Mat_GetDecal((char*) VMA(1), args[2]);
	case CG_MAT_GET_DECAL_SCALE:
		return Mat_GetDecalScale((char*) VMA(1), args[2]);
	case CG_MAT_GET_EFFECT:
		return Mat_GetEffect((char*) VMA(1), args[2]);
	case CG_MAT_GET_DEBRIS:
		return Mat_GetDebris((char*) VMA(1), args[2]);
	case CG_MAT_GET_DEBRIS_SCALE:
		return Mat_GetDebrisScale((char*) VMA(1), args[2]);

	case CG_G2_ATTACHG2MODEL:
		{
			CGhoul2Info_v *g2From = ((CGhoul2Info_v *)args[1]);
			CGhoul2Info_v *g2To = ((CGhoul2Info_v *)args[3]);
			
			return re->G2API_AttachG2Model(*g2From, args[2], *g2To, args[4], args[5]);
		}
	case CG_G2_DETACHG2MODEL:
		{
			CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(ghoul2, (qhandle_t)args[2]);
			return re->G2API_DetachG2Model(ghlinfo);
		}


	case CG_RESETAUTORUN:
		//SOF2 TODO
		return 0;

	case CG_AS_UPDATEAMBIENTSET:
		S_UpdateAmbientSet((const char *)VMA(1), (float *)VMA(2));
		return 0;

	case CG_G2_GETBOLTINDEX:
		{
			void *testnull = (void *)args[1];
			if(!testnull)
				return 0;
			//CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info_v &ghoul2 = *(GhoulHandleVMCheck(args[1]));
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(ghoul2, (qhandle_t)args[2]);
			//SOF2 TODO
			return re->G2API_GetBoltIndex(ghlinfo, args[2]);
		}

	case CG_UI_SETACTIVEMENU:
		UIVM_SetActiveMenu( (uiMenuCommand_t)args[1] );
		return 0;

	case CG_UI_CLOSEALL:
		UIVM_CloseAll();
		return 0;

	case CG_G2_SETGHOUL2MODELFLAGSBYINDEX:
		{
			//CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info_v &ghoul2 = *(GhoulHandleVMCheck(args[1]));
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(ghoul2, (qhandle_t)args[2]);
			re->G2API_SetGhoul2ModelFlags(ghlinfo, args[3]);
			return 0;
		}

	case CG_G2_GETGHOUL2MODELFLAGSBYINDEX:
		{
			CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)args[1]);
			CGhoul2Info *ghlinfo = re->G2API_GetInfo(ghoul2, (qhandle_t)args[2]);
			return re->G2API_GetGhoul2ModelFlags(ghlinfo);
		}

	case CG_S_STOPALLSOUNDS:
		S_StopAllSounds();
		return 0;

	case CG_AS_ADDLOCALSET:
		return S_AddLocalSet( (const char *)VMA(1), (float *)VMA(2), (float *)VMA(3), args[4], args[5] );
		return 0;

	case CG_G2_GETNUMMODELS:
		//SOF2 TODO
		return -1;

	case CG_G2_FINDBOLTINDEX:
		//SOF2 TODO
		return -1;
	default:
		assert(0); // bk010102
		Com_Error( ERR_DROP, "Bad cgame system trap: %ld", (long int) args[0] );
	}
	return 0;
}

void CL_BindCGame( void ) {
	// load the dll or bytecode
	vmInterpret_t interpret = (vmInterpret_t)Cvar_VariableValue("vm_cgame");
	if(cl_connectedToPureServer)
	{
		// if sv_pure is set we only allow qvms to be loaded
		if(interpret != VMI_COMPILED && interpret != VMI_BYTECODE)
			interpret = VMI_COMPILED;
	}

	interpret = VMI_NATIVE;

	cgvm = VM_Create( VM_CGAME, CL_CgameSystemCalls, interpret );
	if ( !cgvm ) {
		cls.cgameStarted = qfalse;
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}
}

void CL_UnbindCGame( void ) {
	CGVM_Shutdown();
	VM_Free( cgvm );
	cgvm = NULL;
}
