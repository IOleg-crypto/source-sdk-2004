//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_ANTLION_H
#define NPC_ANTLION_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_blended_movement.h"
#include "soundent.h"
#include "ai_behavior_follow.h"

class CAntlionTemplateMaker;

#define	ANTLION_FOLLOW_DISTANCE	350
#define	ANTLION_FOLLOW_DISTANCE_SQR	(ANTLION_FOLLOW_DISTANCE*ANTLION_FOLLOW_DISTANCE)
#define ANTLION_SKIN_COUNT 4

class CNPC_Antlion;

// Antlion follow behavior
class CAI_AntlionFollowBehavior : public CAI_FollowBehavior
{
	typedef CAI_FollowBehavior BaseClass;

public:

	CAI_AntlionFollowBehavior()
	 :	BaseClass( AIF_ANTLION )
	{
	}

	bool FarFromFollowTarget( void )
	{ 
		return ( GetFollowTarget() && (GetAbsOrigin() - GetFollowTarget()->GetAbsOrigin()).LengthSqr() > ANTLION_FOLLOW_DISTANCE_SQR ); 
	}

	bool ShouldFollow( void )
	{
		if ( GetFollowTarget() == NULL )
			return false;
		
		if ( GetEnemy() != NULL )
			return false;

		return true;
	}
};

//
// Antlion class
//

enum AntlionMoveState_e
{
	ANTLION_MOVE_FREE,
	ANTLION_MOVE_FOLLOW,
	ANTLION_MOVE_FIGHT_TO_GOAL,
};

#define	SF_ANTLION_BURROW_ON_ELUDED		( 1 << 16 )
#define	SF_ANTLION_USE_GROUNDCHECKS		( 1 << 17 )

typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BlendedNPC> > CAI_BaseAntlionBase;

class CNPC_Antlion : public CAI_BaseAntlionBase
{
public:

	DECLARE_CLASS( CNPC_Antlion, CAI_BaseAntlionBase  );

	CNPC_Antlion( void );

	float		GetIdealAccel( void ) const;
	float		MaxYawSpeed( void );
	bool		FInViewCone( CBaseEntity *pEntity );
	bool		FInViewCone( const Vector &vecSpot );
				
	void		Activate( void );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		IdleSound( void );
	void		PainSound( void );
	void		Precache( void );
	void		Spawn( void );
	void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void		BuildScheduleTestBits( void );
	void		GatherConditions( void );
	void		PrescheduleThink( void );
	void		BurrowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
				
	bool		IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
	bool		HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sender = NULL );
	bool		QuerySeeEntity( CBaseEntity *pEntity );
	bool		ShouldPlayIdleSound( void );
	bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	bool		IsValidEnemy(CBaseEntity *pEnemy);
	bool		QueryHearSound( CSound *pSound );
	bool		IsLightDamage( const CTakeDamageInfo &info );
	bool		CreateBehaviors( void );
	bool		ShouldHearBugbait( void ) { return ( m_bIgnoreBugbait == false ); }
	int			SelectSchedule( void );

	void		Touch( CBaseEntity *pOther );

	int			MeleeAttack1Conditions( float flDot, float flDist );
	int			MeleeAttack2Conditions( float flDot, float flDist );
	int			GetSoundInterests( void ) { return (BaseClass::GetSoundInterests())|(SOUND_DANGER|SOUND_PHYSICS_DANGER|SOUND_THUMPER|SOUND_BUGBAIT); }

	Class_T		Classify( void ) { return CLASS_ANTLION; }
	
	void		Event_Killed( const CTakeDamageInfo &info );
	bool		FValidateHintType ( CAI_Hint *pHint );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	
	bool		IsAllied( void );
	bool		ShouldGib( const CTakeDamageInfo &info );
	bool		CorpseGib( const CTakeDamageInfo &info );

	float		GetMaxJumpSpeed() const { return 1024.0f; }

	void		SetFightTarget( CBaseEntity *pTarget );
	void		InputFightToPosition( inputdata_t &inputdata );
	void		InputStopFightToPosition( inputdata_t &inputdata );

	void		SetFollowTarget( CBaseEntity *pTarget );
	int			TranslateSchedule( int scheduleType );

	virtual		Activity NPC_TranslateActivity( Activity baseAct );

	bool		ShouldResumeFollow( void );
	bool		ShouldAbandonFollow( void );

	void		SetMoveState( AntlionMoveState_e state );
	int			ChooseMoveSchedule( void );

	DECLARE_DATADESC();

	bool		m_bStartBurrowed;
	float		m_flNextJumpPushTime;

	void		SetParentSpawnerName( const char *szName ) { m_strParentSpawner = MAKE_STRING( szName ); }
	const char *GetParentSpawnerName( void ) { return STRING( m_strParentSpawner ); }

	virtual void StopLoopingSounds( void );
	bool    AllowedToBePushed( void );

private:

	inline CBaseEntity *EntityToWatch( void );
	void				UpdateHead( void );

	bool	FindChasePosition( const Vector &targetPos, Vector &result );
	bool	GetGroundPosition( const Vector &testPos, Vector &result );
	bool	GetPathToSoundFleePoint( int soundType );

	void	Burrow( void );
	void	Unburrow( void );
	
	void	InputUnburrow( inputdata_t &inputdata );
	void	InputBurrow( inputdata_t &inputdata );
	void	InputBurrowAway( inputdata_t &inputdata );
	void	InputDisableJump( inputdata_t &inputdata );
	void	InputEnableJump( inputdata_t &inputdata );
	void	InputIgnoreBugbait( inputdata_t &inputdata );
	void	InputHearBugbait( inputdata_t &inputdata );

	bool	FindBurrow( const Vector &origin, float distance, int type, bool excludeNear = true );
	void	ClearBurrowPoint( const Vector &origin );
	void	CreateDust( bool placeDecal = true );

	bool	ValidBurrowPoint( const Vector &point );
	bool	CheckLanding( void );
	bool	Alone( void );
	bool	CheckAlertRadius( void );
	bool	ShouldJump( void );

	void	MeleeAttack( float distance, float damage, QAngle& viewPunch, Vector& shove );
	void	SetWings( bool state );
	void	StartJump( void );
	void	LockJumpNode( void );

	bool	IsUnusableNode(int iNodeID, CAI_Hint *pHint);

	bool	OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	
	void	ManageFleeCapabilities( bool bEnable );
	
	int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	bool	IsFirmlyOnGround( void );

	float	m_flIdleDelay;
	float	m_flBurrowTime;
	float	m_flJumpTime;
	float	m_flAlertRadius;
	float	m_flPounceTime;

	int		m_iContext;			//for FValidateHintType context

	CAI_AntlionFollowBehavior	m_FollowBehavior;

	AntlionMoveState_e	m_MoveState;

	COutputEvent	m_OnReachFightGoal;	//Reached our scripted destination to fight to

	Vector		m_vecSavedJump;
	Vector		m_vecLastJumpAttempt;

	float		m_flIgnoreSoundTime;		// Sound time to ignore if earlier than
	float		m_flNextAcknowledgeTime;	// Next time an antlion can make an acknowledgement noise
	float		m_flSuppressFollowTime;		// Amount of time to suppress our follow time
	float		m_flObeyFollowTime;			// A range of time the antlions must be obedient

	Vector		m_vecHeardSound;
	bool		m_bHasHeardSound;
	bool		m_bAgitatedSound;	//Playing agitated sound?
	bool		m_bWingsOpen;		//Are the wings open?
	bool		m_bIgnoreBugbait;	//If the antlion should ignore bugbait sounds
	string_t	m_strParentSpawner;	//Name of our spawner

	EHANDLE		m_hFollowTarget;
	EHANDLE		m_hFightGoalTarget;
	float		m_flEludeDistance;	//Distance until the antlion will consider himself "eluded" if so flagged
	bool		m_bLeapAttack;
	bool		m_bDisableJump;
	float		m_flTimeDrown;
	bool		m_bLoopingStarted;

	bool		m_bForcedStuckJump;

	DEFINE_CUSTOM_AI;

	//==================================================
	// AntlionConditions
	//==================================================

	enum
	{
		COND_ANTLION_FLIPPED = LAST_SHARED_CONDITION,
		COND_ANTLION_ON_NPC,
		COND_ANTLION_CAN_JUMP,
		COND_ANTLION_FOLLOW_TARGET_TOO_FAR,
		COND_ANTLION_RECEIVED_ORDERS,
		COND_ANTLION_IN_WATER,
	};

	//==================================================
	// AntlionSchedules
	//==================================================

	enum
	{
		SCHED_ANTLION_CHASE_ENEMY_BURROW = LAST_SHARED_SCHEDULE,
		SCHED_ANTLION_JUMP,
		SCHED_ANTLION_RUN_TO_BURROW_IN,
		SCHED_ANTLION_BURROW_IN,
		SCHED_ANTLION_BURROW_WAIT,
		SCHED_ANTLION_BURROW_OUT,
		SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER,
		SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW,
		SCHED_ANTLION_WAIT_UNBORROW,
		SCHED_ANTLION_FLEE_THUMPER,
		SCHED_ANTLION_CHASE_BUGBAIT,
		SCHED_ANTLION_FLIP,
		SCHED_ANTLION_DISMOUNT_NPC,
		SCHED_ANTLION_RUN_TO_FIGHT_GOAL,
		SCHED_ANTLION_RUN_TO_FOLLOW_GOAL,
		SCHED_ANTLION_BUGBAIT_IDLE_STAND,
		SCHED_ANTLION_BURROW_AWAY,
		SCHED_ANTLION_FLEE_PHYSICS_DANGER,
		SCHED_ANTLION_POUNCE,
		SCHED_ANTLION_POUNCE_MOVING,
		SCHED_ANTLION_DROWN,
	};

	//==================================================
	// AntlionTasks
	//==================================================

	enum
	{
		TASK_ANTLION_SET_CHARGE_GOAL = LAST_SHARED_TASK,
		TASK_ANTLION_FIND_BURROW_IN_POINT,
		TASK_ANTLION_FIND_BURROW_OUT_POINT,
		TASK_ANTLION_BURROW,
		TASK_ANTLION_UNBURROW,
		TASK_ANTLION_VANISH,
		TASK_ANTLION_BURROW_WAIT,
		TASK_ANTLION_CHECK_FOR_UNBORROW,
		TASK_ANTLION_JUMP,
		TASK_ANTLION_WAIT_FOR_TRIGGER,
		TASK_ANTLION_GET_THUMPER_ESCAPE_PATH,
		TASK_ANTLION_GET_PATH_TO_BUGBAIT,
		TASK_ANTLION_FACE_BUGBAIT,
		TASK_ANTLION_DISMOUNT_NPC,
		TASK_ANTLION_REACH_FIGHT_GOAL,
		TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH,
		TASK_ANTLION_FACE_JUMP,
		TASK_ANTLION_DROWN,
	};
};


//-----------------------------------------------------------------------------
// Purpose: Shield
//-----------------------------------------------------------------------------
class CAntlionRepellant : public CPointEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CAntlionRepellant, CPointEntity );
	~CAntlionRepellant();

public:
	void Spawn( void );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	float GetRadius( void );
	void SetRadius( float flRadius ) { m_flRepelRadius = flRadius; }

	static bool IsPositionRepellantFree( Vector vDesiredPos );

	void OnRestore( void );

private:

	float m_flRepelRadius;
	bool  m_bEnabled;
};

#endif // NPC_ANTLION_H
