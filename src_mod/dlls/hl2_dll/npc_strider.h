//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef NPC_STRIDER_H
#define NPC_STRIDER_H

#include "ai_blended_movement.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_utils.h"
#include "smoke_trail.h"
#include "physics_bone_follower.h"
#if defined( _WIN32 )
#pragma once
#endif

class CNPC_Strider;
class CNPC_Bullseye;
class CStriderMinigun;

//-----------------------------------------------------------------------------
//
// Support for moving Strider air nodes to the correct Z for the Strider
// regardless of Hammer placement
//
//-----------------------------------------------------------------------------

class CAI_Network;
class CAI_Node;
struct StriderMinigunViewcone_t;
struct AI_EnemyInfo_t;

void AdjustStriderNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );

//-----------------------------------------------------------------------------
//
// Strider Minigun
//
//-----------------------------------------------------------------------------

class IMinigunHost
{
public:
	virtual void ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread = vec3_origin ) = 0;
	virtual void UpdateMinigunControls( float &yaw, float &pitch ) = 0;
	virtual void GetViewCone( StriderMinigunViewcone_t &cone ) = 0;
	virtual void NewTarget() = 0;
	virtual void OnMinigunStartShooting( CBaseEntity *pTarget ) = 0;
	virtual void OnMinigunStopShooting( CBaseEntity *pTarget ) = 0;
	virtual CAI_BaseNPC *GetEntity() = 0;
};

//-----------------------------------------------------------------------------
//
// npc_strider
//
//-----------------------------------------------------------------------------

const int NUM_STRIDER_IK_TARGETS = 6;

//---------------------------------------------------------

class CNPC_Strider : public CAI_BlendingHost<CAI_BaseNPC>,
					 public IMinigunHost
{
	DECLARE_CLASS( CNPC_Strider, CAI_BaseNPC );
	DECLARE_SERVERCLASS();

public:
	CNPC_Strider();
	~CNPC_Strider();

	//---------------------------------

	void			Precache();
	void			Spawn();
	bool			CreateVPhysics();
	void			PostNPCInit();
	void			Activate();
	void			UpdateOnRemove();
	void			InitBoneControllers();
	void			OnRestore();

	Class_T			Classify();

	int				DrawDebugTextOverlays();

	void			UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }

	//---------------------------------

	virtual Vector	GetNodeViewOffset()					{ return BaseClass::GetDefaultEyeOffset();		}

	Vector			EyePosition();
	const Vector &	GetViewOffset();
	Vector			EyePositionCrouched() { return GetAbsOrigin() - Vector( 0, 0, 330 ); }

	//---------------------------------
	// CBaseAnimating
	void			CalculateIKLocks( float currentTime );
	float			GetIdealAccel() const { return GetIdealSpeed(); }

	//---------------------------------
	// Behavior
	//---------------------------------
	void			NPCThink();
	void			PrescheduleThink();
	void			GatherConditions();
	void			CheckFlinches() {} // Strider handles on own
	void			GatherHeightConditions( const Vector &vTestPos, CBaseEntity *pEntity );
	void			OnStateChange( NPC_STATE oldState, NPC_STATE newState );
	void			BuildScheduleTestBits();
	int				SelectSchedule();
	int				TranslateSchedule( int scheduleType );
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	void			HandleAnimEvent( animevent_t *pEvent );

	Disposition_t	IRelationType( CBaseEntity *pTarget );
	void			AddEntityRelationship( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority );

	bool			ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity );
	bool			ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity );

	//---------------------------------
	// Inputs
	//---------------------------------
	void			InputSetMinigunTime( inputdata_t &inputdata );
	void			InputSetMinigunTarget( inputdata_t &inputdata );
	void			InputSetCannonTarget( inputdata_t &inputdata );
	void			InputFlickRagdoll( inputdata_t &inputdata );
	void			InputCrouch( inputdata_t &inputdata );
	void			InputStand( inputdata_t &inputdata );
	void			InputSetHeight( inputdata_t &inputdata );
	void			InputSetTargetPath( inputdata_t &inputdata );
	void 			InputClearTargetPath( inputdata_t &inputdata );
	void			InputDisableCrouchWalk( inputdata_t &inputdata );
	void			InputEnableCrouchWalk( inputdata_t &inputdata );

	//---------------------------------
	// Combat
	//---------------------------------
	bool			HasPass()	{ return m_PlayerFreePass.HasPass(); }

	bool			FVisible( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );
	Vector			BodyTarget( const Vector &posSrc, bool bNoisy );

	bool			IsValidEnemy( CBaseEntity *pTarget );
	bool			UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer = NULL );
	float			StriderEnemyDistance( CBaseEntity *pEnemy );

	bool			FCanCheckAttacks();
	int				RangeAttack2Conditions( float flDot, float flDist );
	int				MeleeAttack1Conditions( float flDot, float flDist );
	int				MeleeAttack2Conditions( float flDot, float flDist );
	bool			WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	bool			IsValidShootPosition ( const Vector &vecCoverLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	bool			TestShootPosition(const Vector &vecShootPos, const Vector &targetPos );

	Vector			Weapon_ShootPosition();

	void			MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void			DoImpactEffect( trace_t &tr, int nDamageType );
	void			DoMuzzleFlash( void );

	bool			CanShootThrough( const trace_t &tr, const Vector &vecTarget );

	void			CreateFocus();
	CNPC_Bullseye *	GetFocus();

	bool			GetWeaponLosZ( const Vector &vOrigin, float minZ, float maxZ, float increment, CBaseEntity *pTarget, float *pResult );

	//---------------------------------
	//	Sounds & speech
	//---------------------------------
	void			AlertSound();
	void			PainSound();
	void			DeathSound();

	//---------------------------------
	// Damage handling
	//---------------------------------
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int				TakeDamageFromCombineBall( const CTakeDamageInfo &info );
	void			Event_Killed( const CTakeDamageInfo &info );
	bool			BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	void			StartSmoking();
	void			StopSmoking( float flDelay = 0.1 );
	bool			IsSmoking() { return m_hSmoke != NULL; }
	void			Explode();
	
	//---------------------------------
	// Posture
	//---------------------------------
	float			GetMaxHeightModel() const	{ return 500.0; }
	float			GetMaxHeight() const		{ return 490.0; }
	float			GetMinHeight() const		{ return 200.0; }
	float			GetHeightRange() const		{ return GetMaxHeight() - GetMinHeight(); }
	void			SetHeight( float h );
	float			GetHeight()					{ return GetPoseParameter( gm_BodyHeightPoseParam ); }
	void 			SetIdealHeight( float h );
	void 			SetAbsIdealHeight( float z );
	float			GetIdealHeight()			{ return m_idealHeight; }
	Vector			GetAdjustedOrigin()			{ Vector result = GetAbsOrigin(); result.z -= GetMaxHeightModel() - GetHeight(); return result; }

	bool			IsInCrouchedPosture() { return GetIdealHeight() < GetMaxHeight() * .5; }
	bool			IsInStandingPosture() { return !IsInCrouchedPosture(); }
	bool			IsCrouching();
	bool			IsStanding();
	void			SetupGlobalModelData();
	
	//---------------------------------
	// Navigation & Movement
	//---------------------------------
	class CNavigator : public CAI_ComponentWithOuter<CNPC_Strider, CAI_Navigator>
	{
		typedef CAI_ComponentWithOuter<CNPC_Strider, CAI_Navigator> BaseClass;
	public:
		CNavigator( CNPC_Strider *pOuter )
		 :	BaseClass( pOuter )
		{
		}

		void MoveCalcBaseGoal( AILocalMoveGoal_t *pMoveGoal );
		bool MoveUpdateWaypoint( AIMoveResult_t *pResult );
		bool DoFindPathToPos();
		bool ShouldOptimizeInitialPathSegment( AI_Waypoint_t *pFirstWaypoint );
		bool GetStoppingPath( CAI_WaypointList *pClippedWaypoints );
	};

	class CPathfinder : public CAI_Pathfinder
	{
		typedef CAI_Pathfinder BaseClass;
	public:
		CPathfinder( CNPC_Strider *pOuter ) : BaseClass( pOuter ) {}
		virtual bool CanUseLocalNavigation() { return false; }
	};

	friend class CNavigator;
	friend void AdjustStriderNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );

	bool			OverrideMove( float flInterval );
	void			MaintainTurnActivity( void );
	bool			IsUnusableNode(int iNodeID, CAI_Hint *pHint); // Override for special NPC behavior
	void 			TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition );
	bool			HasPendingTargetPath();
	void			SetTargetPath();
	float			GetDefaultNavGoalTolerance();
	void			OnMovementComplete();

	float			MaxYawSpeed();

	CAI_Navigator *	CreateNavigator()	{ return new CNavigator( this );	}
	CAI_Pathfinder *CreatePathfinder()	{ return new CPathfinder( this );	}

	//---------------------------------
	// Minigun
	//---------------------------------
	void			ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread = vec3_origin );
	void			UpdateMinigunControls( float &yaw, float &pitch );
	void			GetViewCone( StriderMinigunViewcone_t &cone );
	void			NewTarget() { m_flTargetAcquiredTime = gpGlobals->curtime; }
	void			OnMinigunStartShooting( CBaseEntity *pTarget ) {};
	void			OnMinigunStopShooting( CBaseEntity *pTarget );

	CAI_BaseNPC *	GetEntity() { return this; }

	//---------------------------------
	// Cannon
	//---------------------------------
	Vector			CannonPosition();
	CBaseEntity *	GetCannonTarget();
	bool			HasCannonTarget() const;
	bool			IsCannonTarget( CBaseEntity *pTarget ) const;
	bool			AimCannonAt( CBaseEntity *pEntity, float flInterval );
	void			FireCannon();
	void			CannonHitThink();

	//---------------------------------
	// Collision handling
	//---------------------------------

	void			VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
	bool			TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	// Conservative collision volumes
	static Vector gm_cullBoxStandMins;
	static Vector gm_cullBoxStandMaxs;
	static Vector gm_cullBoxCrouchMins;
	static Vector gm_cullBoxCrouchMaxs;
	static float gm_strideLength;

public:

	//---------------------------------
	// Misc
	//---------------------------------
	bool			CarriedByDropship();
	void			CarriedThink();

	//---------------------------------
	// Foot handling
	//---------------------------------
	Vector			LeftFootHit( float eventtime );
	Vector			RightFootHit( float eventtime );
	Vector			BackFootHit( float eventtime );
	void			StompHit( int followerBoneIndex );

	void			FootFX( const Vector &origin );
	Vector			CalculateStompHitPosition( CBaseEntity *pEnemy );

private:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum 
	{
		SCHED_STRIDER_RANGE_ATTACK1 = BaseClass::NEXT_SCHEDULE,
		SCHED_STRIDER_RANGE_ATTACK2, // Immolator
		SCHED_STRIDER_CROUCH,
		SCHED_STRIDER_STAND,
		SCHED_STRIDER_DODGE,
		SCHED_STRIDER_STOMPL,
		SCHED_STRIDER_STOMPR,
		SCHED_STRIDER_FLICKL,
		SCHED_STRIDER_FLICKR,
		SCHED_STRIDER_HUNT,
		SCHED_STRIDER_DIE,
		SCHED_STRIDER_ATTACK_CANNON_TARGET,
		SCHED_STRIDER_CHASE_ENEMY,
		SCHED_STRIDER_COMBAT_FACE,
		SCHED_STRIDER_ESTABLISH_LINE_OF_FIRE_CANNON,
		SCHED_STRIDER_FALL_TO_GROUND,

		TASK_STRIDER_AIM = BaseClass::NEXT_TASK,
		TASK_STRIDER_DODGE,
		TASK_STRIDER_STOMP,
		TASK_STRIDER_BREAKDOWN,
		TASK_STRIDER_START_MOVING,
		TASK_STRIDER_REFRESH_HUNT_PATH,
		TASK_STRIDER_GET_PATH_TO_CANNON_TARGET,
		TASK_STRIDER_FACE_CANNON_TARGET,
		TASK_STRIDER_SET_HEIGHT,
		TASK_STRIDER_GET_PATH_TO_CANNON_LOS,
		TASK_STRIDER_SET_CANNON_HEIGHT,
		TASK_STRIDER_FIRE_CANNON,
		TASK_STRIDER_FALL_TO_GROUND,

		COND_STRIDER_DO_FLICK = BaseClass::NEXT_CONDITION,
		COND_TRACK_PATH_GO,
		COND_STRIDER_SHOULD_CROUCH,
		COND_STRIDER_SHOULD_STAND,
		COND_STRIDER_MINIGUN_SHOOTING,
		COND_STRIDER_MINIGUN_NOT_SHOOTING,
		COND_STRIDER_HAS_CANNON_TARGET,
		COND_STRIDER_ENEMY_UPDATED,
		COND_STRIDER_HAS_LOS_Z,
	};

	CStriderMinigun *m_pMinigun;
	int				m_miniGunAmmo;
	float			m_nextShootTime;
	float			m_nextStompTime;
	float			m_ragdollTime;
	float			m_miniGunShootDuration;
	float			m_aimYaw;
	float			m_aimPitch;
	Vector			m_blastHit;
	Vector			m_blastNormal;
	CNetworkVector( m_vecHitPos );
	CNetworkArray( Vector, m_vecIKTarget, NUM_STRIDER_IK_TARGETS );

	CRandSimTimer	m_PostureAnimationTimer;

	EHANDLE			m_hRagdoll;

	EHANDLE			m_hCannonTarget;
	CSimpleSimTimer	m_AttemptCannonLOSTimer;

	CSimpleSimTimer	m_LowZCorrectionTimer;

	// Contained Bone Follower manager
	CBoneFollowerManager m_BoneFollowerManager;

	int				m_BodyTargetBone;

	bool			m_bDisableBoneFollowers;

	int				m_iVisibleEnemies;
	float			m_flTargetAcquiredTime;
	bool			m_bCrouchLocked; // Designer made the strider crouch. Don't let the AI stand him up.
	bool			m_bNoCrouchWalk;
	
	float			m_idealHeight;
	float			m_HeightVelocity;

	// FIXME: move to a base class to handle turning for blended movement derived characters
	float			m_prevYaw;
	float			m_doTurn;
	float			m_doLeft;
	float			m_doRight;
	float			m_flNextTurnAct;

	string_t		m_strTrackName;

	EHANDLE			m_hFocus;

	CHandle<SmokeTrail> m_hSmoke;

	CSimpleSimTimer	m_EnemyUpdatedTimer;

	CAI_FreePass m_PlayerFreePass;
	

	static float	gm_zCannonDist;
	static float	gm_zMinigunDist;
	static Vector	gm_vLocalRelativePositionCannon;
	static Vector	gm_vLocalRelativePositionMinigun;

	static int		gm_YawControl;
	static int		gm_PitchControl;
	static int		gm_CannonAttachment;
	static int		gm_BodyHeightPoseParam;

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
//---------------------------------------------------------

enum StriderMinigunPeg_t
{
	MINIGUN_PEGGED_DONT_CARE = 0,
	MINIGUN_PEGGED_UP,
	MINIGUN_PEGGED_DOWN,
	MINIGUN_PEGGED_LEFT,
	MINIGUN_PEGGED_RIGHT,
};

//---------------------------------------------------------

struct StriderMinigunViewcone_t
{
	Vector origin;
	Vector axis;
	float cosAngle;
	float length;
};

//---------------------------------------------------------

struct StriderMinigunAnimController_t
{
	float	current;
	float	target;
	float	rate;

	void Update( float dt, bool approach = true )
	{
		if( approach )
		{
			current = Approach( target, current, rate * dt );
		}
		else
		{
			current = target;
		}
	}

	void Random( float minTarget, float maxTarget, float minRate, float maxRate )
	{
		target = random->RandomFloat( minTarget, maxTarget );
		rate = random->RandomFloat( minRate, maxRate );
	}
};

//---------------------------------------------------------

class CStriderMinigun
{
public:
	DECLARE_DATADESC();

	void		Init();
	void		SetTarget( IMinigunHost *pHost, CBaseEntity *pTarget, bool bOverrideEnemy = false );
	CBaseEntity *GetTarget()		{ return m_hTarget.Get(); }
	void		Think( IMinigunHost *pHost, float dt );
	void		SetState( int newState );
	bool		ShouldFindTarget( IMinigunHost *pHost );
	void 		AimAtPoint( IMinigunHost *pHost, const Vector &vecPoint, bool bSnap = false );
	void 		AimAtTarget( IMinigunHost *pHost, CBaseEntity *pTarget, bool bSnap = false );
	void 		ShootAtTarget( IMinigunHost *pHost, CBaseEntity *pTarget, float shootTime );
	void 		StartShooting( IMinigunHost *pHost, CBaseEntity *pTarget, float duration );
	void 		ExtendShooting( float timeExtend );
	void 		SetShootDuration( float duration );
	void 		StopShootingForSeconds( IMinigunHost *pHost, CBaseEntity *pTarget, float duration );
	bool 		IsPegged( int dir = MINIGUN_PEGGED_DONT_CARE );
	bool 		CanStartShooting( IMinigunHost *pHost, CBaseEntity *pTargetEnt );
	float 		GetBurstTimeRemaining() { return m_burstTime - gpGlobals->curtime; }

	void 		RecordShotOnTarget()			{ m_iOnTargetShots++; }
	void 		ClearOnTarget()					{ m_iOnTargetShots = 0; }
	bool		IsOnTarget( int numShots = 0 )	{ return ( numShots == 0 ) ? (m_iOnTargetShots > 0) : (m_iOnTargetShots >= numShots); }

	void		Enable( IMinigunHost *pHost, bool enable );
	float		GetAimError();

	enum minigunstates_t
	{
		MINIGUN_OFF = 0,
		MINIGUN_SHOOTING = 1,
	};

	int			GetState()		{ return m_minigunState; }
	bool		IsShooting()	{ return GetState() == MINIGUN_SHOOTING; }

private:
	bool		m_enable;
	int			m_minigunState;
	float		m_nextBulletTime;	// Minigun is shooting, when can I fire my next bullet?
	float		m_burstTime;		// If firing, how long till done? If not, how long till I can?
	float		m_nextTwitchTime;
	int			m_randomState;
	EHANDLE		m_hTarget;
	StriderMinigunAnimController_t m_yaw;
	StriderMinigunAnimController_t m_pitch;
	bool		m_bWarnedAI;
	float		m_shootDuration;
	Vector		m_vecAnchor;		// A burst starts here and goes to the target's orgin.
	bool		m_bOverrideEnemy;	// The minigun wants something other than the Strider's enemy as a target right now.
	Vector		m_vecLastTargetPos;	// Last place minigun saw the target.
	int			m_iOnTargetShots;
};


class CSparkTrail : public CPointEntity
{
	DECLARE_CLASS( CSparkTrail, CPointEntity );
	void Spawn( void );
	void SparkThink( void );

	virtual void Precache();

	DECLARE_DATADESC();
};

#endif // NPC_STRIDER_H

//=============================================================================
