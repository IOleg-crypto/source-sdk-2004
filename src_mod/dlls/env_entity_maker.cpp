//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "entityoutput.h"
#include "TemplateEntities.h"
#include "point_template.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_ENTMAKER_AUTOSPAWN				0x0001
#define SF_ENTMAKER_WAITFORDESTRUCTION		0x0002
#define SF_ENTMAKER_IGNOREFACING			0x0004
#define SF_ENTMAKER_CHECK_FOR_SPACE			0x0008
#define SF_ENTMAKER_CHECK_PLAYER_LOOKING	0x0010


//-----------------------------------------------------------------------------
// Purpose: An entity that mapmakers can use to ensure there's a required entity never runs out.
//			i.e. physics cannisters that need to be used.
//-----------------------------------------------------------------------------
class CEnvEntityMaker : public CPointEntity
{
	DECLARE_CLASS( CEnvEntityMaker, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Activate( void );

	void		 SpawnEntity( void );
	void		 CheckSpawnThink( void );
	void		 InputForceSpawn( inputdata_t &inputdata );

private:

	CPointTemplate *FindTemplate();

	bool HasRoomToSpawn();
	bool IsPlayerLooking();

	Vector			m_vecEntityMins;
	Vector			m_vecEntityMaxs;
	EHANDLE			m_hCurrentInstance;
	EHANDLE			m_hCurrentBlocker;	// Last entity that blocked us spawning something
	Vector			m_vecBlockerOrigin;

	string_t		m_iszTemplate;

	COutputEvent	m_pOutputOnSpawned;
};

BEGIN_DATADESC( CEnvEntityMaker )
	// DEFINE_FIELD( m_vecEntityMins, FIELD_VECTOR ),
	// DEFINE_FIELD( m_vecEntityMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( m_hCurrentInstance, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hCurrentBlocker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecBlockerOrigin, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_iszTemplate, FIELD_STRING, "EntityTemplate" ),

	// Outputs
	DEFINE_OUTPUT( m_pOutputOnSpawned, "OnEntitySpawned" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceSpawn", InputForceSpawn ),

	// Functions
	DEFINE_THINKFUNC( CheckSpawnThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_entity_maker, CEnvEntityMaker );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::Spawn( void )
{
	m_vecEntityMins = vec3_origin;
	m_vecEntityMaxs = vec3_origin;
	m_hCurrentInstance = NULL;
	m_hCurrentBlocker = NULL;
	m_vecBlockerOrigin = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::Activate( void )
{
	BaseClass::Activate();

	// check for valid template
	if ( m_iszTemplate == NULL_STRING )
	{
		Warning( "env_entity_maker %s has no template entity!\n", GetEntityName() );
		UTIL_Remove( this );
		return;
	}

	// Spawn an instance
	if ( m_spawnflags & SF_ENTMAKER_AUTOSPAWN )
	{
		SpawnEntity();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointTemplate *CEnvEntityMaker::FindTemplate()
{
	// Find our point_template
	CPointTemplate *pTemplate = dynamic_cast<CPointTemplate *>(gEntList.FindEntityByName( NULL, STRING(m_iszTemplate), NULL ));
	if ( !pTemplate )
	{
		Warning( "env_entity_maker %s failed to find template %s.\n", GetEntityName(), STRING(m_iszTemplate) );
	}

	return pTemplate;
}


//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntity( void )
{
	CPointTemplate *pTemplate = FindTemplate();
	if (!pTemplate)
		return;

	// Spawn our template
	CUtlVector<CBaseEntity*> hNewEntities;
	if ( !pTemplate->CreateInstance( GetAbsOrigin(), GetAbsAngles(), &hNewEntities ) )
		return;
	
	//Adrian: oops we couldn't spawn the entity (or entities) for some reason!
	if ( hNewEntities.Count() == 0 )
		 return;
	
	m_hCurrentInstance = hNewEntities[0];

	// Assume it'll block us
	m_hCurrentBlocker = m_hCurrentInstance;
	m_vecBlockerOrigin = m_hCurrentBlocker->GetAbsOrigin();

	// Store off the mins & maxs the first time we spawn
	if ( m_vecEntityMins == vec3_origin )
	{
		m_hCurrentInstance->CollisionProp()->WorldSpaceAABB( &m_vecEntityMins, &m_vecEntityMaxs );
		m_vecEntityMins -= m_hCurrentInstance->GetAbsOrigin();
		m_vecEntityMaxs -= m_hCurrentInstance->GetAbsOrigin();
	}

	// Fire our output
	m_pOutputOnSpawned.FireOutput( this, this );

	// Start thinking
	if ( m_spawnflags & SF_ENTMAKER_AUTOSPAWN )
	{
		SetThink( &CEnvEntityMaker::CheckSpawnThink );
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not the template entities can fit if spawned.
// Input  : pBlocker - Returns blocker unless NULL.
//-----------------------------------------------------------------------------
bool CEnvEntityMaker::HasRoomToSpawn()
{
	// Do we have a blocker from last time?
	if ( m_hCurrentBlocker )
	{
		// If it hasn't moved, abort immediately
		if ( m_vecBlockerOrigin == m_hCurrentBlocker->GetAbsOrigin() )
		{
			return false;
		}
	}

	// Check to see if there's enough room to spawn
	trace_t tr;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), m_vecEntityMins, m_vecEntityMaxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.m_pEnt || tr.startsolid )
	{
		// Store off our blocker to check later
		m_hCurrentBlocker = tr.m_pEnt;
		if ( m_hCurrentBlocker )
		{
			m_vecBlockerOrigin = m_hCurrentBlocker->GetAbsOrigin();
		}

		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the player is looking towards us.
//-----------------------------------------------------------------------------
bool CEnvEntityMaker::IsPlayerLooking()
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			// Only spawn if the player's looking away from me
			Vector vLookDir = pPlayer->EyeDirection3D();
			Vector vTargetDir = GetAbsOrigin() - pPlayer->EyePosition();
			VectorNormalize( vTargetDir );

			float fDotPr = DotProduct( vLookDir,vTargetDir );
			if ( fDotPr > 0 )
				return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Check to see if we should spawn another instance
//-----------------------------------------------------------------------------
void CEnvEntityMaker::CheckSpawnThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.5f );

	// Do we have an instance?
	if ( m_hCurrentInstance )
	{
		// If Wait-For-Destruction is set, abort immediately
		if ( m_spawnflags & SF_ENTMAKER_WAITFORDESTRUCTION )
			return;
	}

	// Check to see if there's enough room to spawn
	if ( !HasRoomToSpawn() )
		return;

	// We're clear, now check to see if the player's looking
	if ( !( HasSpawnFlags( SF_ENTMAKER_IGNOREFACING ) ) && IsPlayerLooking() )
		return;

	// Clear, no player watching, so spawn!
	SpawnEntity();
}


//-----------------------------------------------------------------------------
// Purpose: Spawns the entities, checking for space if flagged to do so.
//-----------------------------------------------------------------------------
void CEnvEntityMaker::InputForceSpawn( inputdata_t &inputdata )
{
	CPointTemplate *pTemplate = FindTemplate();
	if (!pTemplate)
		return;

	if ( HasSpawnFlags( SF_ENTMAKER_CHECK_FOR_SPACE ) && !HasRoomToSpawn() )
		return;

	if ( HasSpawnFlags( SF_ENTMAKER_CHECK_PLAYER_LOOKING ) && IsPlayerLooking() )
		return;

	SpawnEntity();
}

