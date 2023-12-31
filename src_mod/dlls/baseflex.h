//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEFLEX_H
#define BASEFLEX_H
#ifdef _WIN32
#pragma once
#endif


#include "BaseAnimatingOverlay.h"
#include "utlvector.h"
#include "utlrbtree.h"

struct flexsettinghdr_t;
struct flexsetting_t;

//-----------------------------------------------------------------------------
// Purpose:  A .vfe referenced by a scene during .vcd playback
//-----------------------------------------------------------------------------
class CFlexSceneFile
{
public:
	enum
	{
		MAX_FLEX_FILENAME = 128,
	};

	char			filename[ MAX_FLEX_FILENAME ];
	void			*buffer;
};

class CBaseFlex;
class CChoreoEvent;
class CChoreoScene;
class CChoreoActor;
class CSceneEntity;

//-----------------------------------------------------------------------------
// Purpose: One of a number of currently playing scene events for this actor
//-----------------------------------------------------------------------------
// FIXME: move this, it's only used in in baseflex and baseactor
class CSceneEventInfo
{
public:
	CSceneEventInfo()
		:
	m_pEvent( 0 ),
	m_pScene( 0 ),
	m_pActor( 0 ),
	m_bStarted( false ),
	m_iLayer( -1 ),
	m_iPriority( 0 ),
	m_nSequence( 0 ),
	m_bIsPosture( false ),
	m_flWeight( 0.0f ),
	m_hTarget(),
	m_bIsMoving( false ),
	m_flInitialYaw( 0.0f ),
	m_flTargetYaw( 0.0f ),
	m_flFacingYaw( 0.0f ),
	m_nType( 0 ),
	m_flNext( 0.0f )
	{
	}

	// The event handle of the current scene event
	CChoreoEvent	*m_pEvent;

	// Current Scene
	CChoreoScene	*m_pScene;

	// Current actor
	CChoreoActor	*m_pActor;

	// Set after the first time the event has been configured ( allows
	//  bumping markov index only at start of event playback, not every frame )
	bool			m_bStarted;

public:
	//	EVENT local data...
	// FIXME: Evil, make accessors or figure out better place
	// FIXME: This won't work, scenes don't save and restore...
	int						m_iLayer;
	int						m_iPriority;
	int						m_nSequence;
	bool					m_bIsPosture;
	float					m_flWeight; // used for suppressions of posture while moving

	// movement, faceto targets?
	EHANDLE					m_hTarget;
	bool					m_bIsMoving;
	bool					m_bHasArrived;
	float					m_flInitialYaw;
	float					m_flTargetYaw;
	float					m_flFacingYaw;

	// generic AI events
	int						m_nType;
	float					m_flNext;

	void					InitWeight( CBaseFlex *pActor );
	float					UpdateWeight( CBaseFlex *pActor );
};



//-----------------------------------------------------------------------------
// Purpose: Animated characters who have vertex flex capability (e.g., facial expressions)
//-----------------------------------------------------------------------------
class CBaseFlex : public CBaseAnimatingOverlay
{
	DECLARE_CLASS( CBaseFlex, CBaseAnimatingOverlay );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// Construction
						CBaseFlex( void );
						~CBaseFlex( void );

	virtual void		SetModel( const char *szModelName );

	void Blink( );

	virtual	void		SetViewtarget( const Vector &viewtarget );
	const Vector		&GetViewtarget( void ) const;

	void				SetFlexWeight( char *szName, float value );
	void				SetFlexWeight( int index, float value );
	float				GetFlexWeight( char *szName );
	float				GetFlexWeight( int index );

	// Look up flex controller index by global name
	int					FindFlexController( const char *szName );
	void				EnsureTranslations( const flexsettinghdr_t *pSettinghdr );

	// Keep track of what scenes are being played
	void				StartChoreoScene( CChoreoScene *scene );
	void				RemoveChoreoScene( CChoreoScene *scene );

	// Start the specifics of an scene event
	virtual bool		StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );

	// Manipulation of events for the object
	// Should be called by think function to process all scene events
	// The default implementation resets m_flexWeight array and calls
	//  AddSceneEvents
	virtual void		ProcessSceneEvents( void );

	// Assumes m_flexWeight array has been set up, this adds the actual currently playing
	//  expressions to the flex weights and adds other scene events as needed
	virtual	bool		ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );

	// Remove all playing events
	void				ClearSceneEvents( CChoreoScene *scene, bool canceled );

	// Stop specifics of event
	virtual	bool		ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );

	// Add the event to the queue for this actor
	void				AddSceneEvent( CChoreoScene *scene, CChoreoEvent *event, CBaseEntity *pTarget = NULL );

	// Remove the event from the queue for this actor
	void				RemoveSceneEvent( CChoreoScene *scene, CChoreoEvent *event, bool fastKill );

	// Checks to see if the event should be considered "completed"
	bool				CheckSceneEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	// Checks to see if a event should be considered "completed"
	virtual bool		CheckSceneEventCompletion( CSceneEventInfo *info, float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	// Finds the layer priority of the current scene
	int					GetScenePriority( CChoreoScene *scene );

	// Returns true if the actor is not currently in a scene OR if the actor
	//  is in a scene, but a PERMIT_RESPONSES event is active and the permit time
	//  period has enough time remaining to handle the response in full.
	bool				PermitResponse( float response_length );

	// Set response end time (0 to clear response blocking)
	void				SetPermitResponse( float endtime );

protected:
	// For handling .vfe files
	// Search list, or add if not in list
	const void			*FindSceneFile( const char *filename );

	// Find setting by name
	const flexsetting_t *FindNamedSetting( const flexsettinghdr_t *pSettinghdr, const char *expr );

	// Called at the lowest level to actually apply an expression
	void				AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr, 
							const flexsettinghdr_t *pOverrideHdr, bool newexpression );

	// Called at the lowest level to actually apply a flex animation
	void				AddFlexAnimation( CSceneEventInfo *info );

	bool				HasSceneEvents() const;

	int					FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key );

private:
	// Starting various expression types 

	bool RequestStartSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	bool RequestStartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );

	bool HandleStartSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor );
	bool HandleStartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor );
	bool StartFacingSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	bool StartMoveToSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );

	// Processing various expression types
	bool ProcessFlexAnimationSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessFlexSettingSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessSequenceSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessFacingSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessMoveToSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool ProcessLookAtSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );

	// Set playing the scene sequence
public:
	bool EnterSceneSequence( bool bRestart = false );
private:
	bool ExitSceneSequence( void );

private:
	// FIXME: ( MAXSTUDIOFLEXES == 64 ) seems high, this should be reset to what we're really going to use, but we can't use a per-model dynamic number
	//  since save/restore and data declaration can't take a variable number of fields.
	CNetworkArray( float, m_flexWeight, 64 );	

	// Vector from actor to eye target
	CNetworkVector( m_viewtarget );

	// Blink state
	CNetworkVar( int, m_blinktoggle );

	// Array of active SceneEvents, in order oldest to newest
	CUtlVector < CSceneEventInfo >		m_SceneEvents;

	// Mapping for each loaded scene file used by this actor
	struct FS_LocalToGlobal_t
	{
		explicit FS_LocalToGlobal_t() :
			m_Key( 0 ),
			m_nCount( 0 ),
			m_Mapping( 0 )
		{
		}

		explicit FS_LocalToGlobal_t( const flexsettinghdr_t *key ) :
			m_Key( key ),
			m_nCount( 0 ),
			m_Mapping( 0 )
		{
		}		

		void SetCount( int count )
		{
			Assert( !m_Mapping );
			Assert( count > 0 );
			m_nCount = count;
			m_Mapping = new int[ m_nCount ];
			Q_memset( m_Mapping, 0, m_nCount * sizeof( int ) );
		}

		FS_LocalToGlobal_t( const FS_LocalToGlobal_t& src )
		{
			m_Key = src.m_Key;
			delete m_Mapping;
			m_Mapping = new int[ src.m_nCount ];
			Q_memcpy( m_Mapping, src.m_Mapping, src.m_nCount * sizeof( int ) );

			m_nCount = src.m_nCount;
		}

		~FS_LocalToGlobal_t()
		{
			delete m_Mapping;
			m_nCount = 0;
			m_Mapping = 0;
		}

		const flexsettinghdr_t	*m_Key;
		int						m_nCount;
		int						*m_Mapping;	
	};

	static bool FlexSettingLessFunc( const FS_LocalToGlobal_t& lhs, const FS_LocalToGlobal_t& rhs );

	CUtlRBTree< FS_LocalToGlobal_t, unsigned short > m_LocalToGlobal;

	// The NPC is in a scene, but another .vcd (such as a short wave to say in response to the player doing something )
	//  can be layered on top of this actor (assuming duration matches, etc.
	float				m_flAllowResponsesEndTime;

	// List of actively playing scenes
	CUtlVector < CChoreoScene * >		m_ActiveChoreoScenes;
	bool				m_bUpdateLayerPriorities;

public:
	bool				IsSuppressedFlexAnimation( CSceneEventInfo *info );

private:
	// last time a foreground flex animation was played
	float				m_flLastFlexAnimationTime;
};


//-----------------------------------------------------------------------------
// For toggling blinking
//-----------------------------------------------------------------------------
inline void CBaseFlex::Blink()
{
	m_blinktoggle = !m_blinktoggle;
}

//-----------------------------------------------------------------------------
// Do we have active expressions?
//-----------------------------------------------------------------------------
inline bool CBaseFlex::HasSceneEvents() const
{
	return m_SceneEvents.Count() != 0;
}


//-----------------------------------------------------------------------------
// Other inlines
//-----------------------------------------------------------------------------
inline const Vector &CBaseFlex::GetViewtarget( ) const
{
	return m_viewtarget.Get();	// bah
}

inline void CBaseFlex::SetFlexWeight( char *szName, float value )
{
	SetFlexWeight( FindFlexController( szName ), value );
}

inline float CBaseFlex::GetFlexWeight( char *szName )
{
	return GetFlexWeight( FindFlexController( szName ) );
}


EXTERN_SEND_TABLE(DT_BaseFlex);


#endif // BASEFLEX_H
