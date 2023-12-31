//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_PLAYERALLY_H
#define AI_PLAYERALLY_H

#include "utlmap.h"
#include "simtimer.h"
#include "AI_Criteria.h"
#include "ai_baseactor.h"
#include "ai_speechfilter.h"
#ifndef _WIN32
#undef min
#endif
#include "stdstring.h"
#ifndef _WIN32
#undef MINMAX_H
#include "minmax.h"
#endif

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

#define TLK_ANSWER 			"TLK_ANSWER"
#define TLK_QUESTION 		"TLK_QUESTION"
#define TLK_IDLE 			"TLK_IDLE"
#define TLK_STARE 			"TLK_STARE"
#define TLK_USE				"TLK_USE"
#define TLK_STARTFOLLOW 	"TLK_STARTFOLLOW"
#define TLK_STOPFOLLOW		"TLK_STOPFOLLOW"
#define TLK_JOINPLAYER		"TLK_JOINPLAYER"
#define TLK_STOP 			"TLK_STOP"
#define TLK_NOSHOOT			"TLK_NOSHOOT"
#define TLK_HELLO 			"TLK_HELLO"
#define TLK_PHELLO 			"TLK_PHELLO"
#define TLK_PIDLE 			"TLK_PIDLE"
#define TLK_PQUESTION 		"TLK_PQUESTION"
#define TLK_PLHURT1 		"TLK_PLHURT1"
#define TLK_PLHURT2 		"TLK_PLHURT2"
#define TLK_PLHURT3 		"TLK_PLHURT3"
#define TLK_PLPUSH 			"TLK_PLPUSH"
#define TLK_PLRELOAD		"TLK_PLRELOAD"
#define TLK_SMELL 			"TLK_SMELL"
#define TLK_SHOT			"TLK_SHOT"
#define TLK_WOUND 			"TLK_WOUND"
#define TLK_MORTAL 			"TLK_MORTAL"
#define TLK_DANGER			"TLK_DANGER"
#define TLK_SEE_COMBINE		"TLK_SEE_COMBINE"
#define TLK_ENEMY_DEAD		"TLK_ENEMY_DEAD"
#define TLK_SELECTED		"TLK_SELECTED"	// selected by player in command mode.
#define TLK_COMMANDED		"TLK_COMMANDED" // received orders from player in command mode
#define TLK_COMMAND_FAILED	"TLK_COMMAND_FAILED" 
#define TLK_BETRAYED		"TLK_BETRAYED"	// player killed an ally in front of me.
#define TLK_ALLY_KILLED		"TLK_ALLY_KILLED" // witnessed an ally die some other way.
#define TLK_ATTACKING		"TLK_ATTACKING" // about to fire my weapon at a target
#define TLK_HEAL			"TLK_HEAL" // healing someone
#define TLK_GIVEAMMO		"TLK_GIVEAMMO" // giving ammo to someone
#define TLK_DEATH			"TLK_DEATH"	// Death rattle
#define TLK_HELP_ME			"TLK_HELP_ME" // call out to the player for help
#define TLK_PLYR_PHYSATK	"TLK_PLYR_PHYSATK"	// Player's attacked me with a thrown physics object
#define TLK_ANSWER_VORT		"TLK_ANSWER_VORT"
#define TLK_ANSWER_CIT		"TLK_ANSWER_CIT"
#define TLK_QUESTION_VORT	"TLK_QUESTION_VORT"
#define TLK_QUESTION_CIT	"TLK_QUESTION_CIT"
#define TLK_NEWWEAPON		"TLK_NEWWEAPON"
#define TLK_PLDEAD			"TLK_PLDEAD"
#define TLK_HIDEANDRELOAD	"TLK_HIDEANDRELOAD"
#define TLK_STARTCOMBAT		"TLK_STARTCOMBAT"
#define TLK_WATCHOUT		"TLK_WATCHOUT"

// resume is "as I was saying..." or "anyhow..."
#define TLK_RESUME 			"TLK_RESUME"

// tourguide stuff below
#define TLK_TGSTAYPUT 	"TLK_TGSTAYPUT"
#define TLK_TGFIND 		"TLK_TGFIND"
#define TLK_TGSEEK 		"TLK_TGSEEK"
#define TLK_TGLOSTYOU 	"TLK_TGLOSTYOU"
#define TLK_TGCATCHUP 	"TLK_TGCATCHUP"
#define TLK_TGENDTOUR 	"TLK_TGENDTOUR"

//-----------------------------------------------------------------------------

#define TALKRANGE_MIN 500.0				// don't talk to anyone farther away than this

//-----------------------------------------------------------------------------

#define TALKER_STARE_DIST	128				// anyone closer than this and looking at me is probably staring at me.

#define TALKER_DEFER_IDLE_SPEAK_MIN		10
#define TALKER_DEFER_IDLE_SPEAK_MAX		20

//-----------------------------------------------------------------------------

class CAI_PlayerAlly;

//-----------------------------------------------------------------------------
//
// CLASS: CAI_AllySpeechManager
//
//-----------------------------------------------------------------------------

enum ConceptCategory_t
{
	SPEECH_IDLE,
	SPEECH_IMPORTANT,
	SPEECH_PRIORITY,

	SPEECH_NUM_CATEGORIES
};

struct ConceptCategoryInfo_t
{
	float	minGlobalDelay;
	float	maxGlobalDelay;
	float	minPersonalDelay;
	float	maxPersonalDelay;
};

enum AIConceptFlags_t
{
	AICF_DEFAULT 			= 0,
	AICF_SPEAK_ONCE			= 0x01,
	AICF_PROPAGATE_SPOKEN	= 0x02,
	AICF_TARGET_PLAYER		= 0x04,
};

struct ConceptInfo_t
{
	AIConcept_t			concept;
	ConceptCategory_t   category;
	float				minGlobalCategoryDelay;
	float				maxGlobalCategoryDelay;
	float				minPersonalCategoryDelay;
	float				maxPersonalCategoryDelay;
	float				minConceptDelay;
	float				maxConceptDelay;
	int 				flags;
};

//-------------------------------------

class CAI_AllySpeechManager : public CLogicalEntity
{
	DECLARE_CLASS( CAI_AllySpeechManager, CLogicalEntity );
public:
	CAI_AllySpeechManager();
	~CAI_AllySpeechManager();
	
	void Spawn();

	ConceptCategoryInfo_t *GetConceptCategoryInfo( ConceptCategory_t category );
	ConceptInfo_t *GetConceptInfo( AIConcept_t concept );
	void OnSpokeConcept( CAI_PlayerAlly *pPlayerAlly, AIConcept_t concept );

	void SetCategoryDelay( ConceptCategory_t category, float minDelay, float maxDelay = 0.0 );
	bool CategoryDelayExpired( ConceptCategory_t category );
	bool ConceptDelayExpired( AIConcept_t concept );

private:

	CSimpleSimTimer	m_ConceptCategoryTimers[SPEECH_NUM_CATEGORIES];

	CUtlMap<string_t, CSimpleSimTimer, char> m_ConceptTimers;

	friend CAI_AllySpeechManager *GetAllySpeechManager();
	static CAI_AllySpeechManager *gm_pSpeechManager;

	DECLARE_DATADESC();
};

//-------------------------------------

CAI_AllySpeechManager *GetAllySpeechManager();

//-----------------------------------------------------------------------------
//
// CLASS: CAI_PlayerAlly
//
//-----------------------------------------------------------------------------

class CAI_AllySpeechManager;

enum AISpeechTargetSearchFlags_t
{
	AIST_PLAYERS 				= (1<<0),
	AIST_NPCS					= (1<<1),
	AIST_IGNORE_RELATIONSHIP	= (1<<2),
	AIST_ANY_QUALIFIED			= (1<<3),
	AIST_FACING_TARGET			= (1<<4),
};

struct AISpeechSelection_t
{
	AISpeechSelection_t()
	 :	pResponse(NULL)
	{
	}
	
	void Set( AIConcept_t newConcept, AI_Response *pNewResponse, CBaseEntity *pTarget = NULL )
	{
		pResponse = pNewResponse;
		concept = newConcept;
		hSpeechTarget = pTarget;
	}
	
	std::string 		concept;
	AI_Response *		pResponse;
	EHANDLE			hSpeechTarget;				
};

//-------------------------------------

class CAI_PlayerAlly : public CAI_BaseActor
{
	DECLARE_CLASS( CAI_PlayerAlly, CAI_BaseActor );
public:
	//---------------------------------

	int			ObjectCaps( void ) { return UsableNPCObjectCaps(BaseClass::ObjectCaps()); }
	void		TalkInit( void );				

	//---------------------------------
	// Behavior
	//---------------------------------
	void		GatherConditions( void );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	void		OnStateChange( NPC_STATE OldState, NPC_STATE NewState );
	void		PrescheduleThink( void );
	int			SelectSchedule( void );
	virtual int	SelectNonCombatSpeechSchedule();
	int			TranslateSchedule( int scheduleType );
	void		OnStartSchedule( int scheduleType );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		TaskFail( AI_TaskFailureCode_t );
	void		TaskFail( const char *pszGeneralFailText )	{ BaseClass::TaskFail( pszGeneralFailText ); }
	void		ClearTransientConditions();
	void		Touch(	CBaseEntity *pOther );

	//---------------------------------
	// Combat
	//---------------------------------
	void		OnKilledNPC( CBaseCombatCharacter *pKilled );

	//---------------------------------
	// Damage handling
	//---------------------------------
	void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void		Event_Killed( const CTakeDamageInfo &info );

	//---------------------------------

	void		PainSound(void);

	//---------------------------------
	// Speech & Acting
	//---------------------------------
	CBaseEntity	*EyeLookTarget( void );		// Override to look at talk target
	CBaseEntity	*FindNamedEntity( const char *pszName );

	CBaseEntity *FindSpeechTarget( int flags );
	virtual bool IsValidSpeechTarget( int flags, CBaseEntity *pEntity );
	
	CBaseEntity *GetSpeechTarget()								{ return m_hTalkTarget.Get(); }
	void		SetSpeechTarget( CBaseEntity *pSpeechTarget ) 	{ m_hTalkTarget = pSpeechTarget; }
	
	void		SetSpeechFilter( CAI_SpeechFilter *pFilter )	{ m_hSpeechFilter = pFilter; }
	CAI_SpeechFilter *GetSpeechFilter( void )					{ return m_hSpeechFilter; }

	//---------------------------------
	
	virtual bool SelectIdleSpeech( AISpeechSelection_t *pSelection );

	virtual bool SelectInterjection();
	virtual bool SelectPlayerUseSpeech();

	//---------------------------------

	bool 		SelectSpeechResponse( AIConcept_t concept, const char *pszModifiers, CBaseEntity *pTarget, AISpeechSelection_t *pSelection );
	void		SetPendingSpeech( AIConcept_t concept, AI_Response *pResponse );
	void 		ClearPendingSpeech();
	bool		HasPendingSpeech()	{ return !m_PendingConcept.empty(); }

	//---------------------------------
	
	bool		CanPlaySentence( bool fDisregardState );
	int			PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );

	//---------------------------------
	
	void		DeferAllIdleSpeech( float flDelay = -1, CAI_BaseNPC *pIgnore = NULL );

	//---------------------------------
	
	bool		IsOkToSpeak( ConceptCategory_t category, bool fRespondingToPlayer = false );
	
	//---------------------------------
	
	bool		IsOkToSpeak( void );
	bool		IsOkToCombatSpeak( void );
	bool		IsOkToSpeakInResponseToPlayer( void );
	
	bool		ShouldSpeakRandom( AIConcept_t concept, int iChance );
	bool		IsAllowedToSpeak( AIConcept_t concept, bool bRespondingToPlayer = false );
	bool		SpeakIfAllowed( AIConcept_t concept, const char *modifiers = NULL, bool bRespondingToPlayer = false );
	void		ModifyOrAppendCriteria( AI_CriteriaSet& set );

	//---------------------------------
	
	float		GetTimePlayerStaring()		{ return ( m_flTimePlayerStartStare != 0 ) ? gpGlobals->curtime - m_flTimePlayerStartStare : 0; }

	//---------------------------------

	void		OnSpokeConcept( AIConcept_t concept );
	void		OnStartSpeaking();

	// Inputs
	virtual void InputIdleRespond( inputdata_t &inputdata ) {};

protected:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		SCHED_TALKER_SPEAK_PENDING_IDLE = BaseClass::NEXT_SCHEDULE,
		SCHED_TALKER_SPEAK_PENDING_ALERT,
		SCHED_TALKER_SPEAK_PENDING_COMBAT,
		NEXT_SCHEDULE,
		
		TASK_TALKER_SPEAK_PENDING = BaseClass::NEXT_TASK,
		NEXT_TASK,
		
		COND_TALKER_CLIENTUNSEEN = BaseClass::NEXT_CONDITION,
		COND_TALKER_PLAYER_DEAD,
		COND_TALKER_PLAYER_STARING,
		NEXT_CONDITION
	};

private:
	void SetCategoryDelay( ConceptCategory_t category, float minDelay, float maxDelay = 0.0 )	{ m_ConceptCategoryTimers[category].Set( minDelay, maxDelay ); }
	bool CategoryDelayExpired( ConceptCategory_t category )										{ return m_ConceptCategoryTimers[category].Expired(); }

	friend class CAI_AllySpeechManager;

	//---------------------------------
	
	AI_Response		m_PendingResponse;
	std::string		m_PendingConcept;
	float			m_TimePendingSet;

	//---------------------------------
	
	EHANDLE			m_hTalkTarget;	// who to look at while talking
	float			m_flNextRegenTime;
	float			m_flTimePlayerStartStare;

	//---------------------------------

	CSimpleSimTimer	m_ConceptCategoryTimers[3];
	
	//---------------------------------
	
	CHandle<CAI_SpeechFilter>	m_hSpeechFilter;

	DECLARE_DATADESC();
protected:
	DEFINE_CUSTOM_AI;
};

//-----------------------------------------------------------------------------

#endif // AI_PLAYERALLY_H
