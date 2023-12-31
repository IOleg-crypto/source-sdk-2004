//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Hint node utilities and functions.
//
// $NoKeywords: $
//=============================================================================//

#ifndef	AI_HINT_H
#define	AI_HINT_H
#pragma once

#include "ai_initutils.h"

//Flags for FindHintNode
#define bits_HINT_NODE_NONE						0x00000000
#define bits_HINT_NODE_VISIBLE					0x00000001
#define	bits_HINT_NODE_NEAREST					0x00000002		// Choose the node nearest me
#define bits_HINT_NODE_RANDOM					0x00000004		// Find a random hintnode meeting other criteria
#define bits_HINT_NODE_CLEAR					0x00000008		// Only choose nodes that have clear room for my bounding box (requires NPC)
#define bits_HINT_NODE_USE_GROUP				0x00000010		// Use the NPC's hintgroup when searching for a node (requires NPC)
#define	bits_HINT_NODE_VISIBLE_TO_PLAYER		0x00000020
#define	bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER	0x00000040
#define bits_HINT_NODE_REPORT_FAILURES			0x00000080
#define bits_HINT_NODE_IN_VIEWCONE				0x00000100
#define bits_HINT_NODE_IN_AIMCONE				0x00000200
#define bits_HINT_NPC_IN_NODE_FOV				0x00000400		// Is the searcher inside the hint node's FOV?

//-----------------------------------------------------------------------------
//
// hints - these MUST coincide with the HINTS listed under
// info_node in the FGD file!
//
//-----------------------------------------------------------------------------
enum Hint_e
{
	HINT_ANY = -1,
	HINT_NONE = 0,
	HINT_NOT_USED_WORLD_DOOR,
	HINT_WORLD_WINDOW,
	HINT_NOT_USED_WORLD_BUTTON,
	HINT_NOT_USED_WORLD_MACHINERY,
	HINT_NOT_USED_WORLD_LEDGE,
	HINT_NOT_USED_WORLD_LIGHT_SOURCE,
	HINT_NOT_USED_WORLD_HEAT_SOURCE,
	HINT_NOT_USED_WORLD_BLINKING_LIGHT,
	HINT_NOT_USED_WORLD_BRIGHT_COLORS,
	HINT_NOT_USED_WORLD_HUMAN_BLOOD,
	HINT_NOT_USED_WORLD_ALIEN_BLOOD,

	HINT_WORLD_WORK_POSITION,
	HINT_WORLD_VISUALLY_INTERESTING,
	HINT_WORLD_VISUALLY_INTERESTING_DONT_AIM,
	HINT_WORLD_INHIBIT_COMBINE_MINES,

	HINT_TACTICAL_COVER_MED	= 100,
	HINT_TACTICAL_COVER_LOW,
	HINT_TACTICAL_SPAWN,
	HINT_TACTICAL_PINCH,				// Exit / entrance to an arena
	HINT_NOT_USED_TACTICAL_GUARD,
	HINT_TACTICAL_ENEMY_DISADVANTAGED,	//Disadvantageous position for the enemy
	HINT_HEALTH_KIT,

	HINT_NOT_USED_URBAN_STREETCORNER = 200,
	HINT_NOT_USED_URBAN_STREETLAMP,
	HINT_NOT_USED_URBAN_DARK_SPOT,
	HINT_NOT_USED_URBAN_POSTER,
	HINT_URBAN_SHELTER,

	HINT_NOT_USED_ASSASSIN_SECLUDED = 300,
	HINT_NOT_USED_ASSASSIN_RAFTERS,
	HINT_NOT_USED_ASSASSIN_GROUND,
	HINT_NOT_USED_ASSASSIN_MONKEYBARS,

	HINT_ANTLION_BURROW_POINT = 400,
	HINT_ANTLION_THUMPER_FLEE_POINT,

	HINT_HEADCRAB_BURROW_POINT = 450,

	HINT_ROLLER_PATROL_POINT = 500,
	HINT_ROLLER_CLEANUP_POINT,

	HINT_NOT_USED_PSTORM_ROCK_SPAWN = 600,

	HINT_CROW_FLYTO_POINT = 700,

	 // TF2 Hints
	HINT_BUG_PATROL_POINT = 800,

	// HL2 Hints
	HINT_FOLLOW_WAIT_POINT	= 900,
	HINT_JUMP_OVERRIDE		= 901,
	HINT_PLAYER_SQUAD_TRANSITON_POINT = 902,
	HINT_NPC_EXIT_POINT = 903,
	HINT_STRIDER_NODE = 904,

	// HL1 port hints
	HINT_HL1_WORLD_MACHINERY = 1000,
	HINT_HL1_WORLD_BLINKING_LIGHT,
	HINT_HL1_WORLD_HUMAN_BLOOD,
	HINT_HL1_WORLD_ALIEN_BLOOD,

	// CS port hints
	HINT_CSTRIKE_HOSTAGE_ESCAPE = 1100,
};

//-----------------------------------------------------------------------------
// CHintCriteria
//-----------------------------------------------------------------------------

class CHintCriteria
{
public:

	CHintCriteria();
	~CHintCriteria();

	bool		HasFlag( int bitmask )	const	{ return ( m_iFlags & bitmask ) != 0;	}
	void		SetFlag( int bitmask );
	void		ClearFlag( int bitmask );

	void		SetGroup( string_t group );
	string_t	GetGroup( void )	const	{ return m_strGroup;	}

	int			GetFirstHintType( void ) const	{ return m_iFirstHintType; }
	int			GetLastHintType( void ) const	{ return m_iLastHintType; }

	bool		HasIncludeZones( void )	const	{ return ( m_zoneInclude.Count() != 0 ); }
	bool		HasExcludeZones( void )	const	{ return ( m_zoneExclude.Count() != 0 ); }
	
	void		AddIncludePosition( const Vector &position, float radius );
	void		AddExcludePosition( const Vector &position, float radius );
	void		SetHintType( int hintType );
	void		SetHintTypeRange( int firstType, int lastType );

	bool		InIncludedZone( const Vector &testPosition ) const;
	bool		InExcludedZone( const Vector &testPosition ) const;

private:

	struct	hintZone_t
	{
		Vector		position;
		float		radiussqr;
	};

	typedef CUtlVector < hintZone_t >	zoneList_t;

	void		AddZone( zoneList_t &list, const Vector &position, float radius );
	bool		InZone( const zoneList_t &zone, const Vector &testPosition ) const;

	int			m_iFlags;
	int			m_iFirstHintType;
	int			m_iLastHintType;
	string_t	m_strGroup;
	
	zoneList_t	m_zoneInclude;
	zoneList_t	m_zoneExclude;
};

class CAI_Node;

//-----------------------------------------------------------------------------
// CAI_HintManager
//-----------------------------------------------------------------------------

DECLARE_POINTER_HANDLE(AIHintIter_t);

class CAI_HintManager
{
	friend class CAI_Hint;
public:
	// Hint node creation
	static CAI_Hint		*CreateHint( HintNodeData *pNodeData, const char *pMapData = NULL );
	static void			DrawHintOverlays(float flDrawDuration);

	static void			AddHint( CAI_Hint *pTestHint );
	static void			RemoveHint( CAI_Hint *pTestHint );

	// Interface for searching the hint node list
	static CAI_Hint		*FindHint( CAI_BaseNPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria );
	static CAI_Hint		*FindHint( CAI_BaseNPC *pNPC, const CHintCriteria &hintCriteria );
	static CAI_Hint		*FindHint( const Vector &position, const CHintCriteria &hintCriteria );
	static CAI_Hint		*FindHint( CAI_BaseNPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom = NULL );

	// Purpose: Finds a random suitable hint within the requested radious of the npc
	static CAI_Hint		*FindHintRandom( CAI_BaseNPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria );
	static int			FindAllHints( CAI_BaseNPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria, CUtlVector<CAI_Hint *> *pResult );
	static int			FindAllHints( const Vector &position, const CHintCriteria &hintCriteria, CUtlVector<CAI_Hint *> *pResult )	{ return FindAllHints( NULL, position, hintCriteria, pResult ); }
	static int			FindAllHints( CAI_BaseNPC *pNPC, const CHintCriteria &hintCriteria, CUtlVector<CAI_Hint *> *pResult )		{ return FindAllHints( pNPC, pNPC->GetAbsOrigin(), hintCriteria, pResult ); }
	static int			GetFlags( const char *token );

	static CAI_Hint		*GetFirstHint( AIHintIter_t *pIter )					{ *pIter = (AIHintIter_t)gm_pAllHints; return gm_pAllHints; }
	static CAI_Hint		*GetNextHint( AIHintIter_t *pIter );

	static void			SetLastFoundHint( CAI_Hint *pHint ) { gm_pLastFoundHint = pHint; }

	static void DumpHints();

private:
	static CAI_Hint		*gm_pLastFoundHint;			// Last used hint 
	static CAI_Hint		*gm_pAllHints;				// A linked list of all hints
};

//-----------------------------------------------------------------------------
// CAI_Hint
//-----------------------------------------------------------------------------

class CAI_Hint : public CServerOnlyEntity
{
	DECLARE_CLASS( CAI_Hint, CServerOnlyEntity );
public:
	CAI_Hint( void );
	~CAI_Hint( void );

	// Interface for specific nodes
	bool				Lock( CBaseEntity *pNPC );			// Makes unavailable for hints
	void				Unlock( float delay = 0.0 );		// Makes available for hints after delay
	bool				IsLocked(void);						// Whether this node is available for use.
	bool				IsLockedBy( CBaseEntity *pNPC );	// Whether this node is available for use.
	void				GetPosition(CBaseCombatCharacter *pBCC, Vector *vPosition);
	void				GetPosition( Hull_t hull, Vector *vPosition );
	Vector				GetDirection( void );
	float				Yaw( void );
	CAI_Node			*GetNode( void );
	string_t			GetGroup( void ) const			{ return m_NodeData.strGroup;	}
	CBaseEntity			*User( void ) const				{ return m_hHintOwner; };
	Hint_e				HintType( void ) const			{ return (Hint_e)m_NodeData.nHintType;  };
	void				SetHintType( int hintType )		{ m_NodeData.nHintType = hintType; };
	string_t			HintActivityName( void ) const	{ return m_NodeData.iszActivityName; }
	int					GetTargetNode( void ) const		{ return m_nTargetNodeID; }
	bool				IsDisabled( void ) const		{ return (m_NodeData.iDisabled != 0); }
	void				SetDisabled( bool bDisabled	)	{ m_NodeData.iDisabled = bDisabled; }
	void				DisableForSeconds( float flSeconds );
	void				EnableThink();
	void				FixupTargetNode();
	void				NPCStartedUsing( CAI_BaseNPC *pNPC );
	void				NPCStoppedUsing( CAI_BaseNPC *pNPC );

	HintIgnoreFacing_t	GetIgnoreFacing() const			{ return m_NodeData.fIgnoreFacing; }

	NPC_STATE			GetMinState() const				{ return m_NodeData.minState; }
	NPC_STATE			GetMaxState() const				{ return m_NodeData.maxState; }

	int					GetNodeId()	{ return m_NodeData.nNodeID; }
	int					GetWCId()	{ return m_NodeData.nWCNodeID; }

	bool				HintMatchesCriteria( CAI_BaseNPC *pNPC, const CHintCriteria &hintCriteria, const Vector &position, float *flNearestDistance, bool bIgnoreLock = false );

private:
	void				Spawn( void );
	int					DrawDebugTextOverlays(void);
	virtual int			ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual void		OnRestore();
	bool				IsViewable( void );
	bool				IsInNodeFOV( CBaseEntity *pOther );

	// Input handlers
	void				InputEnableHint( inputdata_t &inputdata );
	void				InputDisableHint( inputdata_t &inputdata );

private:
	HintNodeData		m_NodeData;
	int					m_nTargetNodeID;
	EHANDLE				m_hHintOwner;			// Is hint locked (being used by NPC / NPC en-route to use it)
	float				m_flNextUseTime;		// When can I be used again?
	COutputEHANDLE		m_OnNPCStartedUsing;	// Triggered when an NPC has actively begun to use the node.
	COutputEHANDLE		m_OnNPCStoppedUsing;	// Triggered when an NPC has finished using this node.
	float				m_nodeFOV;
	Vector				m_vecForward;

	// The next hint in list of all hints
	friend class CAI_HintManager;
	CAI_Hint			*m_pNextHint;				

	DECLARE_DATADESC();
};


#endif	//AI_HINT_H
