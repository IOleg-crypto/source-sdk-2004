//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VEHICLE_BASESERVER_H
#define VEHICLE_BASESERVER_H
#ifdef _WIN32
#pragma once
#endif

class CSoundPatch;

struct vbs_sound_update_t
{
	float	flFrameTime;
	float	flCurrentSpeedFraction;
	float	flWorldSpaceSpeed;
	bool	bThrottleDown;
	bool	bReverse;
	bool	bTurbo;
	bool	bVehicleInWater;
	bool	bExitVehicle;

	void Defaults()
	{
		flFrameTime = gpGlobals->frametime;
		flCurrentSpeedFraction = 0;
		flWorldSpaceSpeed = 0;
		bThrottleDown = false;
		bReverse = false;
		bTurbo = false;
		bVehicleInWater = false;
		bExitVehicle = false;
	}
};


//-----------------------------------------------------------------------------
// Purpose: Base class for drivable vehicle handling. Contain it in your 
//			drivable vehicle.
//-----------------------------------------------------------------------------
class CBaseServerVehicle : public IServerVehicle
{
public:
	DECLARE_DATADESC();

	CBaseServerVehicle( void );
	~CBaseServerVehicle( void );

	virtual void			Precache( void );

// IVehicle
public:
	virtual CBasePlayer*	GetPassenger( int nRole = VEHICLE_DRIVER );
	virtual int				GetPassengerRole( CBasePlayer *pPassenger );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	virtual bool			IsPassengerUsingStandardWeapons( int nRole = VEHICLE_DRIVER ) { return false; }
	virtual void			SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void			ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void			FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
	virtual void			ItemPostFrame( CBasePlayer *pPlayer );

// IServerVehicle
public:
	virtual CBaseEntity		*GetVehicleEnt( void ) { return m_pVehicle; }
	virtual void			SetPassenger( int nRole, CBasePlayer *pPassenger );
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_DRIVER ) { return false; }
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_DRIVER ) { return true; }
	virtual bool			IsVehicleUpright( void ) { return true; }
	virtual void			HandlePassengerEntry( CBasePlayer *pPlayer, bool bAllowEntryOutsideZone = false );
	virtual bool			HandlePassengerExit( CBasePlayer *pPlayer );
	virtual void			GetPassengerStartPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual bool			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual Class_T			ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification ) { return defaultClassification; }
	virtual float			DamageModifier ( CTakeDamageInfo &info ) { return 1.0; }
	virtual const vehicleparams_t	*GetVehicleParams( void ) { return NULL; }
	virtual bool			IsVehicleBodyInWater( void ) { return false; }

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) { return true; }
	virtual void			NPC_SetDriver( CNPC_VehicleDriver *pDriver ) { return; }
	virtual void			NPC_DriveVehicle( void ) { return; }
	virtual void			NPC_ThrottleCenter( void );
	virtual void			NPC_ThrottleReverse( void );
	virtual void			NPC_ThrottleForward( void );
	virtual void			NPC_Brake( void );
	virtual void			NPC_TurnLeft( float flDegrees );
	virtual void			NPC_TurnRight( float flDegrees );
	virtual void			NPC_TurnCenter( void );
	virtual void			NPC_PrimaryFire( void );
	virtual void			NPC_SecondaryFire( void );
	virtual bool			NPC_HasPrimaryWeapon( void ) { return false; }
	virtual bool			NPC_HasSecondaryWeapon( void ) { return false; }
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) { return; }
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) { return; }

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange );
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange );	
	virtual float			Weapon_PrimaryCanFireAt( void );		// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void );		// Return the time at which this vehicle's secondary weapon can fire again

public:
	// Player Driving
	virtual CBaseEntity		*GetDriver( void );

	// Vehicle entering/exiting
	virtual void			ParseEntryExitAnims( void );
	void					ParseExitAnim( KeyValues *pkvExitList, bool bEscapeExit );
	virtual bool			CheckExitPoint( float yaw, int distance, Vector *pEndPoint );
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint );
	virtual int				GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked );
	virtual void			HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim );

	virtual void			SetVehicle( CBaseEntity *pVehicle );
	IDrivableVehicle 		*GetDrivableVehicle( void );

	// Sound handling
	bool					Initialize( const char *pScriptName );
	virtual void			SoundStart();
	virtual void			SoundStartDisabled();
	virtual void			SoundShutdown( float flFadeTime = 0.0 );
	virtual void			SoundUpdate( vbs_sound_update_t &params );
	virtual void			PlaySound( vehiclesound iSound );
	virtual void			StopSound( vehiclesound iSound );
	virtual void 			RecalculateSoundGear( vbs_sound_update_t &params );
	void					SetVehicleVolume( float flVolume ) { m_flVehicleVolume = clamp( flVolume, 0.0, 1.0 ); }

public:
	CBaseEntity			*m_pVehicle;
	IDrivableVehicle 	*m_pDrivableVehicle;

	// NPC Driving
	int								m_nNPCButtons;
	int								m_nPrevNPCButtons;
	float							m_flTurnDegrees;

	// Entry / Exit anims
	struct entryanim_t
	{
		int		iHitboxGroup;
		char	szAnimName[128];
	};
	CUtlVector< entryanim_t >		m_EntryAnimations;

	struct exitanim_t
	{
		int		iAttachment;
		bool	bUpright;
		bool	bEscapeExit;
		char	szAnimName[128];
	};
	CUtlVector< exitanim_t >		m_ExitAnimations;
	bool							m_bParsedAnimations;
	int								m_iCurrentExitAnim;
	Vector							m_vecCurrentExitEndPoint;
	Vector							m_savedViewOffset;
	CHandle<CEntityBlocker>			m_hExitBlocker;				// Entity to prevent other entities blocking the player's exit point during the exit animation

	char							m_chPreviousTextureType;

// sound state
	vehiclesounds_t					m_vehicleSounds;
private:
	float							m_flVehicleVolume;
	int								m_iSoundGear;			// The sound "gear" that we're currently in
	float							m_flSpeedPercentage;

	CSoundPatch						*m_pStateSound;
	CSoundPatch						*m_pStateSoundFade;
	sound_states					m_soundState;
	float							m_soundStateStartTime;
	float							m_lastSpeed;
	
	void	SoundState_OnNewState( sound_states lastState );
	void	SoundState_Update( vbs_sound_update_t &params );
	sound_states SoundState_ChooseState( vbs_sound_update_t &params );
	void	PlaySound( const char *pSound );
	void	StopLoopingSound( float fadeTime = 0.25f );
	void	PlayLoopingSound( const char *pSoundName );
	bool	PlayCrashSound( float speed );
	bool	CheckCrash( vbs_sound_update_t &params );
	const char *StateSoundName( sound_states state );
	void	InitSoundParams( vbs_sound_update_t &params );
};

#endif // VEHICLE_BASESERVER_H
