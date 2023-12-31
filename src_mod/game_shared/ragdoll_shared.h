//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef RAGDOLL_SHARED_H
#define RAGDOLL_SHARED_H
#ifdef _WIN32
#pragma once
#endif

class IPhysicsObject;
class IPhysicsConstraint;
class IPhysicsConstraintGroup;
class IPhysicsCollision;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
struct matrix3x4_t;

struct vcollide_t;
struct studiohdr_t;
class CBoneAccessor;

#include "vector.h"
#include "bone_accessor.h"

// UNDONE: Remove and make dynamic?
#define RAGDOLL_MAX_ELEMENTS	24
#define RAGDOLL_INDEX_BITS		5			// NOTE 1<<RAGDOLL_INDEX_BITS >= RAGDOLL_MAX_ELEMENTS

struct ragdollelement_t
{
	Vector				originParentSpace;
	IPhysicsObject		*pObject;		// all valid elements have an object
	IPhysicsConstraint	*pConstraint;	// all valid elements have a constraint (except the root)
	int					parentIndex;
};

struct ragdollanimatedfriction_t
{
	float					flFrictionTimeIn;
	float					flFrictionTimeOut;
	float					flFrictionTimeHold;
	int						iMinAnimatedFriction;
	int						iMaxAnimatedFriction;
};

struct ragdoll_t
{
	int					listCount;
	IPhysicsConstraintGroup *pGroup;
	// store these in separate arrays for save/load
	ragdollelement_t 	list[RAGDOLL_MAX_ELEMENTS];
	int					boneIndex[RAGDOLL_MAX_ELEMENTS];
	ragdollanimatedfriction_t animfriction;
};

struct ragdollparams_t
{
	void		*pGameData;
	vcollide_t	*pCollide;
	studiohdr_t *pStudioHdr;
	int			modelIndex;
	Vector		forcePosition;
	Vector		forceVector;
	int			forceBoneIndex;
	CBoneAccessor pPrevBones;
	CBoneAccessor pCurrentBones;
	float		boneDt;		// time delta between prev/cur samples
	float		jointFrictionScale;
};

//-----------------------------------------------------------------------------
// This hooks the main game systems callbacks to allow the AI system to manage memory
//-----------------------------------------------------------------------------
class CRagdollLRURetirement : public CAutoGameSystem
{
public:
	// Methods of IGameSystem
	virtual void Update( float frametime );
	virtual void FrameUpdatePostEntityThink( void );

	// Move it to the top of the LRU
	void MoveToTopOfLRU( CBaseAnimating *pRagdoll );
	void SetMaxRagdollCount( int iMaxCount ){ m_iMaxRagdolls = iMaxCount; }

	virtual void LevelInitPreEntity( void );

private:
	typedef CHandle<CBaseAnimating> CRagdollHandle;
	CUtlLinkedList< CRagdollHandle > m_LRU; 

	int m_iMaxRagdolls;
};

extern CRagdollLRURetirement s_RagdollLRU;


bool RagdollCreate( ragdoll_t &ragdoll, const ragdollparams_t &params, IPhysicsEnvironment *pPhysEnv );

void RagdollActivate( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex, bool bForceWake = true );
void RagdollSetupCollisions( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex );
void RagdollDestroy( ragdoll_t &ragdoll );

// Gets the bone matrix for a ragdoll object
// NOTE: This is different than the object's position because it is
// forced to be rigidly attached in parent space
bool RagdollGetBoneMatrix( const ragdoll_t &ragdoll, CBoneAccessor &pBoneToWorld, int objectIndex );

// Parse the ragdoll and obtain the mapping from each physics element index to a bone index
// returns num phys elements
int RagdollExtractBoneIndices( int *boneIndexOut, studiohdr_t *pStudioHdr, vcollide_t *pCollide );

// computes an exact bbox of the ragdoll's physics objects
void RagdollComputeExactBbox( const ragdoll_t &ragdoll, const Vector &origin, Vector &outMins, Vector &outMaxs );
bool RagdollIsAsleep( const ragdoll_t &ragdoll );
void RagdollSetupAnimatedFriction( IPhysicsEnvironment *pPhysEnv, ragdoll_t *ragdoll, int iModelIndex );

void RagdollApplyAnimationAsVelocity( 
	ragdoll_t &ragdoll, 
	const CBoneAccessor &pPrevBones, 
	const CBoneAccessor &pCurrentBones, 
	float dt );

void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, const CBoneAccessor &pBoneToWorld );

void RagdollSolveSeparation( ragdoll_t &ragdoll, CBaseEntity *pEntity );

#endif // RAGDOLL_SHARED_H
