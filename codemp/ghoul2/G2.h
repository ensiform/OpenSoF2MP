#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif
#if !defined(G2_H_INC)
#define G2_H_INC


#define BONE_ANGLES_PREMULT			0x0001
#define BONE_ANGLES_POSTMULT		0x0002
#define BONE_ANGLES_REPLACE			0x0004
#define	BONE_ANGLES_REPLACE_TO_ANIM	0x0400
#define	BONE_ANGLES_RAGDOLL			0x0800

#define BONE_ANGLES_TOTAL			(BONE_ANGLES_RAGDOLL | BONE_ANGLES_PREMULT | BONE_ANGLES_POSTMULT | BONE_ANGLES_REPLACE | BONE_ANGLES_REPLACE_TO_ANIM )
#define BONE_ANIM_OVERRIDE			0x0008
#define BONE_ANIM_OVERRIDE_LOOP		0x0010
#define BONE_ANIM_OVERRIDE_DEFAULT	( 0x0020 + BONE_ANIM_OVERRIDE )
#define BONE_ANIM_OVERRIDE_FREEZE	( 0x0040 + BONE_ANIM_OVERRIDE )
#define BONE_ANIM_BLEND				0x0080
#define BONE_ANIM_BLEND_FROM_PARENT	0x0100
#define BONE_ANIM_BLEND_TO_PARENT	0x0200
#define BONE_ANIM_TOTAL				( BONE_ANIM_OVERRIDE | BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE_DEFAULT | BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND	| BONE_ANIM_BLEND_TO_PARENT | BONE_ANIM_BLEND_FROM_PARENT )


// defines to setup the
#define		ENTITY_WIDTH 12
#define		MODEL_WIDTH	10
#define		BOLT_WIDTH	10
 
#define		MODEL_AND	((1<<MODEL_WIDTH)-1)
#define		BOLT_AND	((1<<BOLT_WIDTH)-1)
#define		ENTITY_AND	((1<<ENTITY_WIDTH)-1)

#define		BOLT_SHIFT	0
#define		MODEL_SHIFT	(BOLT_SHIFT + BOLT_WIDTH)
#define		ENTITY_SHIFT (MODEL_SHIFT + MODEL_WIDTH)


#endif // G2_H_INC

