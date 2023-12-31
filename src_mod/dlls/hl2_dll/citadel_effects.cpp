//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "citadel_effects_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ============================================================================
//
//  Energy core - charges up and then releases energy from its position
//
// ============================================================================

class CCitadelEnergyCore : public CBaseEntity
{
	DECLARE_CLASS( CCitadelEnergyCore, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	void	InputStartCharge( inputdata_t &inputdata );
	void	InputStartDischarge( inputdata_t &inputdata );
	void	InputStop( inputdata_t &inputdata );

	virtual void Precache();
	void	Spawn( void );

private:
	CNetworkVar( float, m_flScale );
	CNetworkVar( int, m_nState );
	CNetworkVar( float, m_flDuration );
	CNetworkVar( float, m_flStartTime );
};

LINK_ENTITY_TO_CLASS( env_citadel_energy_core, CCitadelEnergyCore );

BEGIN_DATADESC( CCitadelEnergyCore )
	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),
	DEFINE_FIELD( m_nState, FIELD_INTEGER ),
	DEFINE_FIELD( m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartTime, FIELD_TIME ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "StartCharge", InputStartCharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartDischarge", InputStartDischarge ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Stop", InputStop ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CCitadelEnergyCore, DT_CitadelEnergyCore )
	SendPropFloat( SENDINFO(m_flScale), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_nState), 8, SPROP_UNSIGNED),
	SendPropFloat( SENDINFO(m_flDuration), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flStartTime), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_spawnflags), 0, SPROP_UNSIGNED),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Precache: 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::Precache()
{
	BaseClass::Precache();
	PrecacheMaterial( "effects/combinemuzzle2_dark" ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::Spawn( void )
{
	Precache();

	// See if we start active
	if ( HasSpawnFlags( SF_ENERGYCORE_START_ON ) )
	{
		m_nState = ENERGYCORE_STATE_DISCHARGING;
		m_flStartTime = gpGlobals->curtime;
	}

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::InputStartCharge( inputdata_t &inputdata )
{
	m_nState = ENERGYCORE_STATE_CHARGING;
	m_flDuration = inputdata.value.Float();
	m_flStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::InputStartDischarge( inputdata_t &inputdata )
{
	m_nState = ENERGYCORE_STATE_DISCHARGING;
	m_flStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CCitadelEnergyCore::InputStop( inputdata_t &inputdata )
{
	m_nState = ENERGYCORE_STATE_OFF;
	m_flDuration = inputdata.value.Float();
	m_flStartTime = gpGlobals->curtime;
}
