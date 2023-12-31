//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERVEHICLE_H
#define ISERVERVEHICLE_H

#ifdef _WIN32
#pragma once
#endif

#include "IVehicle.h"
#include "vphysics/vehicles.h"

class CBaseEntity;
class CBasePlayer;
class CNPC_VehicleDriver;

// This is used by the player to access vehicles. It's an interface so the
// vehicles are not restricted in what they can derive from.
class IServerVehicle : public IVehicle
{
public:
	// Get the entity associated with the vehicle.
	virtual CBaseEntity*	GetVehicleEnt() = 0;

	// Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
	virtual void			SetPassenger( int nRole, CBasePlayer *pPassenger ) = 0;
	
	// Is the player visible while in the vehicle? (this is a constant the vehicle)
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_DRIVER ) = 0;

	// Can a given passenger take damage?
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_DRIVER ) = 0;

	// Is the vehicle upright?
	virtual bool			IsVehicleUpright( void ) = 0;

	// Get a position in *world space* inside the vehicle for the player to start at
	virtual void			GetPassengerStartPoint( int nRole, Vector *pPoint, QAngle *pAngles ) = 0;

	virtual void			HandlePassengerEntry( CBasePlayer *pPlayer, bool bAllowEntryOutsideZone = false ) = 0;
	virtual bool			HandlePassengerExit( CBasePlayer *pPlayer ) = 0;

	// Get a point in *world space* to leave the vehicle from (may be in solid)
	virtual bool			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles ) = 0;
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint ) = 0;
	virtual int				GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked ) = 0;
	virtual void			HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim ) = 0;

	virtual Class_T			ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification ) = 0;
	virtual float			DamageModifier ( CTakeDamageInfo &info ) = 0;

	// Get me the parameters for this vehicle
	virtual const vehicleparams_t	*GetVehicleParams( void ) = 0;

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) = 0;
	virtual void			NPC_SetDriver( CNPC_VehicleDriver *pDriver ) = 0;
  	virtual void			NPC_DriveVehicle( void ) = 0;
	virtual void			NPC_ThrottleCenter( void ) = 0;
	virtual void			NPC_ThrottleReverse( void ) = 0;
	virtual void			NPC_ThrottleForward( void ) = 0;
	virtual void			NPC_Brake( void ) = 0;
	virtual void			NPC_TurnLeft( float flDegrees ) = 0;
	virtual void			NPC_TurnRight( float flDegrees ) = 0;
	virtual void			NPC_TurnCenter( void ) = 0;
	virtual void			NPC_PrimaryFire( void ) = 0;
	virtual void			NPC_SecondaryFire( void ) = 0;
	virtual bool			NPC_HasPrimaryWeapon( void ) = 0;
	virtual bool			NPC_HasSecondaryWeapon( void ) = 0;
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) = 0;
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) = 0;

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange ) = 0;	
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange ) = 0;	
	virtual float			Weapon_PrimaryCanFireAt( void ) = 0;	// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void ) = 0;	// Return the time at which this vehicle's secondary weapon can fire again
};

// This is an interface to derive from if your class contains an IServerVehicle 
// handler (i.e. something derived CBaseServerVehicle.
class IDrivableVehicle
{
public:
	virtual CBaseEntity		*GetDriver( void ) = 0;

	// Process movement
	virtual void			ItemPostFrame( CBasePlayer *pPlayer ) = 0;
	virtual void			SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) = 0;
	virtual void			ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) = 0;
	virtual void			FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) = 0;

	// Entering / Exiting
	virtual bool			CanEnterVehicle( CBaseEntity *pEntity ) = 0;
	virtual bool			CanExitVehicle( CBaseEntity *pEntity ) = 0;
	virtual void			SetVehicleEntryAnim( bool bOn ) = 0;
	virtual void			SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) = 0;
	virtual void			EnterVehicle( CBasePlayer *pPlayer ) = 0;

	virtual void			PreExitVehicle( CBasePlayer *pPlayer, int nRole ) = 0;
	virtual void			ExitVehicle( int iRole ) = 0;
	virtual bool			AllowBlockedExit( CBasePlayer *pPlayer, int nRole ) = 0;
	virtual bool			AllowMidairExit( CBasePlayer *pPlayer, int nRole ) = 0;
};

#endif // IVEHICLE_H
