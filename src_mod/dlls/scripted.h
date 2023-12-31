//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SCRIPTED_H
#define SCRIPTED_H
#ifdef _WIN32
#pragma once
#endif

#ifndef SCRIPTEVENT_H
#include "scriptevent.h"
#endif

#include "ai_basenpc.h"


//
// The number of unique outputs that a script can fire from animation events.
// These are fired via SCRIPT_EVENT_FIREEVENT in CAI_BaseNPC::HandleAnimEvent.
//
#define MAX_SCRIPT_EVENTS				8


#define SF_SCRIPT_WAITTILLSEEN			1
#define SF_SCRIPT_EXITAGITATED			2
#define SF_SCRIPT_REPEATABLE			4		// Whether the script can be played more than once.
#define SF_SCRIPT_LEAVECORPSE			8
#define SF_SCRIPT_START_ON_SPAWN		16
#define SF_SCRIPT_NOINTERRUPT			32
#define SF_SCRIPT_OVERRIDESTATE			64
#define SF_SCRIPT_NOSCRIPTMOVEMENT		128
#define SF_SCRIPT_LOOP_IN_POST_IDLE		256		// Loop in the post idle animation after playing the action animation.
#define SF_SCRIPT_HIGH_PRIORITY			512		// If set, we don't allow other scripts to steal our spot in the queue.
#define SF_SCRIPT_SEARCH_CYCLICALLY		1024	// Start search from last entity found.
#define SF_SCRIPT_NO_COMPLAINTS			2048	// doesn't bitch if it can't find anything


enum script_moveto_t
{
	CINE_MOVETO_WAIT = 0,
	CINE_MOVETO_WALK = 1,
	CINE_MOVETO_RUN = 2,
	CINE_MOVETO_CUSTOM = 3,
	CINE_MOVETO_TELEPORT = 4,
	CINE_MOVETO_WAIT_FACING = 5,
};


//
// Interrupt levels for grabbing NPCs to act out scripted events. These indicate
// how important it is to get a specific NPC, and can affect how they respond.
//
enum SS_INTERRUPT
{
	SS_INTERRUPT_BY_CLASS = 0,		// Indicates that we are asking for this NPC by class
	SS_INTERRUPT_BY_NAME,			// Indicates that we are asking for this NPC by name
};


// when a NPC finishes an AI scripted sequence, we can choose
// a schedule to place them in. These defines are the aliases to
// resolve worldcraft input to real schedules (sjb)
#define SCRIPT_FINISHSCHED_DEFAULT	0
#define SCRIPT_FINISHSCHED_AMBUSH	1

class CAI_ScriptedSequence : public CBaseEntity
{
	DECLARE_CLASS( CAI_ScriptedSequence, CBaseEntity );
public:
	void Spawn( void );
	virtual void Blocked( CBaseEntity *pOther );
	virtual void Touch( CBaseEntity *pOther );
	virtual int	 ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual void Activate( void );
	virtual void UpdateOnRemove( void );
	void StartThink();
	void ScriptThink( void );
	void StopThink();

	DECLARE_DATADESC();

	void Pain( void );
	void Die( void );
	void DelayStart( bool bDelay );
	bool FindEntity( void );
	void StartScript( void );
	void FireScriptEvent( int nEvent );
	void OnBeginSequence( void );

	void SetTarget( CBaseEntity *pTarget ) { m_hTargetEnt = pTarget; };
	CBaseEntity *GetTarget( void ) { return m_hTargetEnt; };

	// Input handlers
	void InputBeginSequence( inputdata_t &inputdata );
	void InputCancelSequence( inputdata_t &inputdata );
	void InputMoveToPosition( inputdata_t &inputdata );

	bool IsTimeToStart( void );
	bool IsWaitingForBegin( void );
	void ReleaseEntity( CAI_BaseNPC *pEntity );
	void CancelScript( void );
	bool StartSequence( CAI_BaseNPC *pTarget, string_t iszSeq, bool completeOnEmpty );
	void SynchronizeSequence( CAI_BaseNPC *pNPC );
	bool FCanOverrideState ( void );
	void SequenceDone( CAI_BaseNPC *pNPC );
	void PostIdleDone( CAI_BaseNPC *pNPC );
	void FixScriptNPCSchedule( CAI_BaseNPC *pNPC, int iSavedCineFlags );
	void FixFlyFlag( CAI_BaseNPC *pNPC, int iSavedCineFlags );
	bool	CanInterrupt( void );
	void	AllowInterrupt( bool fAllow );
	void	RemoveIgnoredConditions( void );
	bool    PlayedSequence( void ) { return m_sequenceStarted; }
	bool	ScriptHasNoMovement( void ) { return HasSpawnFlags(SF_SCRIPT_NOSCRIPTMOVEMENT); }
	bool CanEnqueueAfter( void );

private:
	friend class CAI_BaseNPC;	// should probably try to eliminate this relationship

	string_t m_iszPreIdle;		// String index for idle animation to play before playing the action anim
	string_t m_iszPlay;			// String index for scripted action animation
	string_t m_iszPostIdle;		// String index for idle animation to play before playing the action anim
	string_t m_iszCustomMove;	// String index for custom movement animation
	string_t m_iszNextScript;	// Name of the script to run immediately after this one.
	string_t m_iszEntity;		// Entity that is wanted for this script

	int m_fMoveTo;

	float m_flRadius;			// Range to search for an NPC to possess.
	float m_flRepeat;			// Repeat rate

	int m_iDelay;					// A counter indicating how many scripts are NOT ready to start.
	float m_startTime;				// Time when script actually started, used for synchronization
	bool m_bWaitForBeginSequence;	// Set to true when we are told to MoveToPosition. Holds the actor in the pre-action idle until BeginSequence is called.

	int m_saved_effects;
	int m_savedFlags;

	bool m_interruptable;
	bool m_sequenceStarted;

	EHANDLE	m_hTargetEnt;

	EHANDLE m_hNextCine;		// The script to hand the NPC off to when we finish with them.
	
	bool	m_bThinking;
	bool 	m_bInitiatedSelfDelete;

	CAI_BaseNPC *FindScriptEntity( void );
	EHANDLE m_hLastFoundEntity;

	COutputEvent m_OnBeginSequence;
	COutputEvent m_OnEndSequence;
	COutputEvent m_OnScriptEvent[MAX_SCRIPT_EVENTS];

	static void ScriptEntityCancel( CBaseEntity *pentCine );

	static const char *GetSpawnPreIdleSequenceForScript( CBaseEntity *pTargetEntity );
};


#endif // SCRIPTED_H
