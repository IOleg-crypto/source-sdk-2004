//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VEHICLE_BASE_H
#define VEHICLE_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include "vphysics/vehicles.h"
#include "iservervehicle.h"
#include "fourwheelvehiclephysics.h"
#include "props.h"
#include "vehicle_sounds.h"
#include "phys_controller.h"
#include "entityblocker.h"
#include "vehicle_baseserver.h"

class CNPC_VehicleDriver;
class CFourWheelVehiclePhysics;
class CPropVehicleDriveable;
class CSoundPatch;

//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle
//-----------------------------------------------------------------------------
class CFourWheelServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;
// IServerVehicle
public:
	virtual ~CFourWheelServerVehicle( void )
	{
	}

	virtual bool			IsVehicleUpright( void );
	virtual bool			IsVehicleBodyInWater( void );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	const vehicleparams_t	*GetVehicleParams( void );

	// NPC Driving
	void					NPC_SetDriver( CNPC_VehicleDriver *pDriver );
	void					NPC_DriveVehicle( void );

	CPropVehicleDriveable	*GetFourWheelVehicle( void );

public:
	virtual void	SetVehicle( CBaseEntity *pVehicle );

private:
	CFourWheelVehiclePhysics	*GetFourWheelVehiclePhysics( void );
};

//-----------------------------------------------------------------------------
// Purpose: Base class for four wheel physics vehicles
//-----------------------------------------------------------------------------
class CPropVehicle : public CBaseProp, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS( CPropVehicle, CBaseProp );
public:
	CPropVehicle();
	virtual ~CPropVehicle();

	void SetVehicleType( unsigned int nVehicleType )			{ m_nVehicleType = nVehicleType; }
	unsigned int GetVehicleType( void )							{ return m_nVehicleType; }

	// CBaseEntity
	void			Spawn( void );
	virtual int		Restore( IRestore &restore );
	void			VPhysicsUpdate( IPhysicsObject *pPhysics );
	void			DrawDebugGeometryOverlays();
	int				DrawDebugTextOverlays();
	void			Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	virtual void	Think( void );
	CFourWheelVehiclePhysics *GetPhysics( void ) { return &m_VehiclePhysics; }
	CBasePlayer		*HasPhysicsAttacker( float dt );
	void			OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );

	Vector GetSmoothedVelocity( void );	//Save and update our smoothed velocity for prediction

	virtual void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles ) {}

	// Inputs
	void InputThrottle( inputdata_t &inputdata );
	void InputSteering( inputdata_t &inputdata );
	void InputAction( inputdata_t &inputdata );
	void InputHandBrakeOn( inputdata_t &inputdata );
	void InputHandBrakeOff( inputdata_t &inputdata );
	DECLARE_DATADESC();


protected:
	// engine sounds
	void SoundInit();
	void SoundShutdown();
	void SoundUpdate( const vehicle_operatingparams_t &params, const vehicleparams_t &vehicle );
	void CalcWheelData( vehicleparams_t &vehicle );
	void ResetControls();

protected:
	CFourWheelVehiclePhysics		m_VehiclePhysics;
	unsigned int					m_nVehicleType;
	string_t						m_vehicleScript;

private:
	Vector							m_vecSmoothedVelocity;

	CHandle<CBasePlayer>			m_hPhysicsAttacker;
	float							m_flLastPhysicsInfluenceTime;
};

//-----------------------------------------------------------------------------
// Purpose: Drivable four wheel physics vehicles
//-----------------------------------------------------------------------------
class CPropVehicleDriveable : public CPropVehicle, public IDrivableVehicle
{
	DECLARE_CLASS( CPropVehicleDriveable, CPropVehicle );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
public:
	CPropVehicleDriveable( void );
	~CPropVehicleDriveable( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual int		Restore( IRestore &restore );
	virtual void	OnRestore();
	virtual void	CreateServerVehicle( void );
	virtual int		ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };
	virtual void	GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;
	virtual void	VehicleAngleVectors( const QAngle &angles, Vector *pForward, Vector *pRight, Vector *pUp );
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	Think( void );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	// Vehicle handling
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual int		VPhysicsGetObjectList( IPhysicsObject **pList, int listMax );

	// Inputs
	void	InputLock( inputdata_t &inputdata );
	void	InputUnlock( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	// Locals
	void	ResetUseKey( CBasePlayer *pPlayer );

	// Driving
	void	DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd );	// Player driving entrypoint
	virtual void DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased ); // Driving Button handling

	virtual bool IsOverturned( void );
	virtual bool IsVehicleBodyInWater( void ) { return false; }
		
	// Engine handling
	void	StartEngine( void );
	void	StopEngine( void );
	bool	IsEngineOn( void );

// IDrivableVehicle
public:
	virtual CBaseEntity *GetDriver( void );
	virtual void		ItemPostFrame( CBasePlayer *pPlayer ) { return; }
	virtual void		SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool		CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool		CanExitVehicle( CBaseEntity *pEntity );
	virtual void		SetVehicleEntryAnim( bool bOn ) { m_bEnterAnimOn = bOn; }
	virtual void		SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = bOn; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	virtual void		EnterVehicle( CBasePlayer *pPlayer );

	virtual bool		AllowBlockedExit( CBasePlayer *pPlayer, int iRole ) { return true; }
	virtual bool		AllowMidairExit( CBasePlayer *pPlayer, int iRole ) { return false; }
	virtual void		PreExitVehicle( CBasePlayer *pPlayer, int iRole ) {}
	virtual void		ExitVehicle( int iRole );

	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle *GetServerVehicle() { return m_pServerVehicle; }

protected:

	virtual bool	ShouldThink() { return ( GetDriver() != NULL ); }

	inline bool HasGun();
	void DestroyServerVehicle();

	// Contained IServerVehicle
	CFourWheelServerVehicle	*m_pServerVehicle;

	COutputEvent		m_playerOn;
	COutputEvent		m_playerOff;

	COutputEvent		m_pressedAttack;
	COutputEvent		m_pressedAttack2;

	COutputFloat		m_attackaxis;
	COutputFloat		m_attack2axis;

	CNetworkHandle( CBasePlayer, m_hPlayer );
public:
	CNetworkVar( int, m_nSpeed );
	CNetworkVar( int, m_nRPM );
	CNetworkVar( float, m_flThrottle );
	CNetworkVar( int, m_nBoostTimeLeft );
	CNetworkVar( int, m_nHasBoost );

	CNetworkVector( m_vecEyeExitEndpoint );
	CNetworkVector( m_vecGunCrosshair );
	CNetworkVar( bool, m_bUnableToFire );
	CNetworkVar( bool, m_bHasGun );

	CNetworkVar( bool, m_nScannerDisabledWeapons );
	CNetworkVar( bool, m_nScannerDisabledVehicle );

	// NPC Driver
	CHandle<CNPC_VehicleDriver>	 m_hNPCDriver;
	EHANDLE						 m_hKeepUpright;

protected:
	// Entering / Exiting
	bool		m_bEngineLocked;	// Mapmaker override on whether the vehicle's allowed to be turned on/off
	bool		m_bLocked;
	float		m_flMinimumSpeedToEnterExit;
	CNetworkVar( bool, m_bEnterAnimOn );
	CNetworkVar( bool, m_bExitAnimOn );
	
	// Used to turn the keepupright off after a short time
	float		m_flTurnOffKeepUpright;
	float		m_flNoImpactDamageTime;
};


inline bool CPropVehicleDriveable::HasGun()
{
	return m_bHasGun;
}


#endif // VEHICLE_BASE_H
