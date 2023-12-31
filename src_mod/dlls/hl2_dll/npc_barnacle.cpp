//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		barnacle - stationary ceiling mounted 'fishing' monster	
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "physics_prop_ragdoll.h"
#include "npc_barnacle.h"
#include "npcevent.h"
#include "gib.h"
#include "ai_default.h"
#include "activitylist.h"
#include "hl2_player.h"
#include "vstdlib/random.h"
#include "physics_saverestore.h"
#include "vcollide_parse.h"
#include "vphysics/constraints.h"
#include "studio.h"
#include "bone_setup.h"
#include "iservervehicle.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar	sv_gravity;
ConVar	sk_barnacle_health( "sk_barnacle_health","0");

static ConVar npc_barnacle_swallow( "npc_barnacle_swallow", "0", 0, "Use prototype swallow code." );

const char *CNPC_Barnacle::m_szGibNames[NUM_BARNACLE_GIBS] =
{
	"models/gibs/hgibs.mdl",
	"models/gibs/hgibs_scapula.mdl",
	"models/gibs/hgibs_rib.mdl",
	"models/gibs/hgibs_spine.mdl"
};

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
int ACT_BARNACLE_SLURP;			// Pulling the tongue up with prey on the end
int ACT_BARNACLE_BITE_HUMAN;	// Biting the head of a humanoid
int ACT_BARNACLE_BITE_PLAYER;	// Biting the head of the player
int ACT_BARNACLE_CHEW_HUMAN;	// Slowly swallowing the humanoid
int ACT_BARNACLE_BARF_HUMAN;	// Spitting out human legs & gibs
int ACT_BARNACLE_TONGUE_WRAP;	// Wrapping the tongue around a target
int ACT_BARNACLE_TASTE_SPIT;	// Yuck! Me no like that!
int ACT_BARNACLE_BITE_SMALL_THINGS;	// Eats small things
int ACT_BARNACLE_CHEW_SMALL_THINGS;	// Chews small things


//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionBarnacleVictimDangle	= 0;
int	g_interactionBarnacleVictimReleased	= 0;
int	g_interactionBarnacleVictimGrab		= 0;

LINK_ENTITY_TO_CLASS( npc_barnacle, CNPC_Barnacle );

// Tongue Spring constants
#define BARNACLE_TONGUE_SPRING_CONSTANT_HANGING			10000
#define BARNACLE_TONGUE_SPRING_CONSTANT_LIFTING			10000
#define BARNACLE_TONGUE_SPRING_CONSTANT_LOWERING		7000
#define BARNACLE_TONGUE_SPRING_DAMPING					20
#define BARNACLE_TONGUE_TIP_MASS						100
#define BARNACLE_TONGUE_MAX_LIFT_MASS					70

#define BARNACLE_BITE_DAMAGE_TO_PLAYER					15
#define BARNACLE_DEAD_TONGUE_ALTITUDE					164
#define BARNACLE_MIN_DEAD_TONGUE_CLEARANCE				78


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BARNACLE_AE_PUKEGIB	2
#define	BARNACLE_AE_BITE	3
#define	BARNACLE_AE_SPIT	4

int AE_BARNACLE_PUKEGIB;
int AE_BARNACLE_BITE;
int AE_BARNACLE_SPIT;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------	
CNPC_Barnacle::CNPC_Barnacle(void)
{
	m_flRestUnitsAboveGround = 16.0f;
	m_flNextBloodTime = -1.0f;
	m_nBloodColor = BLOOD_COLOR_RED;
	m_bPlayerWasStanding = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_Barnacle::~CNPC_Barnacle( void )
{
	// Destroy the ragdoll->tongue tip constraint
  	if ( m_pConstraint )
  	{
  		physenv->DestroyConstraint( m_pConstraint );
  		m_pConstraint = NULL;
  	}
}

BEGIN_DATADESC( CNPC_Barnacle )

	DEFINE_FIELD( m_flAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( m_cGibs, FIELD_INTEGER ),// barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD( m_bLiftingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSwallowingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDigestFinish, FIELD_TIME ),
	DEFINE_FIELD( m_bPlayedPullSound, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPlayerWasStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flVictimHeight, FIELD_FLOAT ),
	DEFINE_FIELD( m_iGrabbedBoneIndex, FIELD_INTEGER ),

	DEFINE_FIELD( m_vecRoot, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecTip, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_hTongueRoot, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTongueTip, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hRagdoll, FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( m_pRagdollBones, FIELD_MATRIX3X4_WORLDSPACE ),
	DEFINE_PHYSPTR( m_pConstraint ),
	DEFINE_KEYFIELD( m_flRestUnitsAboveGround, FIELD_FLOAT, "RestDist" ),
	DEFINE_FIELD( m_nSpitAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( m_hLastSpitEnemy, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nShakeCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextBloodTime, FIELD_TIME ),
	DEFINE_FIELD( m_nBloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( m_vecBloodPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flBarnaclePullSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLocalTimer, FIELD_TIME ),
	DEFINE_FIELD( m_vLastEnemyPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flLastPull, FIELD_FLOAT ),
	DEFINE_EMBEDDED( m_StuckTimer ),

	// Function pointers
	DEFINE_THINKFUNC( BarnacleThink ),
	DEFINE_THINKFUNC( WaitTillDead ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CNPC_Barnacle, DT_Barnacle )
	SendPropFloat(  SENDINFO( m_flAltitude ), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO( m_vecRoot ), 0, SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecTip ), 0, SPROP_COORD ),
END_SEND_TABLE()


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Barnacle::Classify ( void )
{
	return	CLASS_BARNACLE;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CNPC_Barnacle::ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// Extend our bounding box downwards the length of the tongue
	CollisionProp()->WorldSpaceAABB( pVecWorldMins, pVecWorldMaxs );

	// We really care about the tongue tip. The altitude is not really relevant.
	VectorMin( *pVecWorldMins, m_vecTip, *pVecWorldMins );
	VectorMax( *pVecWorldMaxs, m_vecTip, *pVecWorldMaxs );

//	pVecWorldMins->z -= m_flAltitude;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Barnacle::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event== AE_BARNACLE_PUKEGIB )
	{
		CGib::SpawnSpecificGibs( this, 1, 50, 1, "models/gibs/hgibs_rib.mdl");
		return;
	}
	if ( pEvent->event == AE_BARNACLE_BITE )
	{
		BitePrey();
		return;
	}
	if ( pEvent->event == AE_BARNACLE_SPIT )
	{
		SpitPrey();
		return;
	}
	
	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// Spawn
//=========================================================
void CNPC_Barnacle::Spawn()
{
	Precache( );

	SetModel( "models/barnacle.mdl" );
	UTIL_SetSize( this, Vector(-16, -16, -40), Vector(16, 16, 0) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	CollisionProp()->SetSurroundingBoundsType( USE_GAME_CODE );
	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( BLOOD_COLOR_GREEN );
	m_iHealth			= sk_barnacle_health.GetFloat();
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	m_cGibs				= 0;
	m_bLiftingPrey		= false;
	m_bSwallowingPrey	= false;
	m_flDigestFinish	= 0;
	m_takedamage		= DAMAGE_YES;
	m_pConstraint		= NULL;
	m_nShakeCount = 0;
	InitBoneControllers();
	InitTonguePosition();

	// set eye position
	SetDefaultEyeOffset();

	SetActivity( ACT_IDLE );

	SetThink ( &CNPC_Barnacle::BarnacleThink );
	SetNextThink( gpGlobals->curtime + 0.5f );

	m_flBarnaclePullSpeed = BARNACLE_PULL_SPEED;

	//Do not have a shadow
	AddEffects( EF_NOSHADOW );
}


//-----------------------------------------------------------------------------
// Sets the tongue's height
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SetAltitude( float flAltitude )
{
	m_flAltitude = flAltitude;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::Activate( void )
{
	BaseClass::Activate();

	// Create our tongue tips
	if ( !m_hTongueRoot )
	{
		m_hTongueRoot = CBarnacleTongueTip::CreateTongueRoot( m_vecRoot, QAngle(90,0,0) );
		m_hTongueTip = CBarnacleTongueTip::CreateTongueTip( this, m_hTongueRoot, m_vecTip, QAngle(0,0,0) );
		m_nSpitAttachment = LookupAttachment( "StrikeHeadAttach" );
		Assert( m_hTongueRoot && m_hTongueTip );
	}
	else if ( GetEnemy() && IsEnemyAPlayer() && !m_pConstraint )
	{
		IPhysicsObject *pPlayerPhys = GetEnemy()->VPhysicsGetObject();
		IPhysicsObject *pTonguePhys = m_hTongueTip->VPhysicsGetObject();

		constraint_fixedparams_t fixed;
		fixed.Defaults();
		fixed.InitWithCurrentObjectState( pTonguePhys, pPlayerPhys );
		fixed.constraint.Defaults();
		m_pConstraint = physenv->CreateFixedConstraint( pTonguePhys, pPlayerPhys, NULL, fixed );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Barnacle::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;
	if ( info.GetDamageType() & DMG_CLUB )
	{
		info.SetDamage( m_iHealth );
	}

	if ( GetActivity() == ACT_IDLE )
	{
		SetActivity( ACT_SMALL_FLINCH );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize tongue position when first spawned
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Barnacle::InitTonguePosition( void )
{
	CBaseEntity *pTouchEnt;
	float flLength;

	pTouchEnt = TongueTouchEnt( &flLength );
	SetAltitude( flLength );

	Vector origin;
	QAngle angle;

	GetAttachment( "TongueEnd", origin, angle );

	float flTongueAdj = origin.z - GetAbsOrigin().z;
	m_vecRoot = origin - Vector(0,0,flTongueAdj);
	m_vecTip.Set( m_vecRoot.Get() - Vector(0,0,(float)m_flAltitude) );
	CollisionProp()->MarkSurroundingBoundsDirty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::BarnacleThink ( void )
{
	CBaseEntity *pTouchEnt;
	float flLength;

	SetNextThink( gpGlobals->curtime + 0.1f );

	UpdateTongue();

	// AI Disabled, don't do anything?
	if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
		return;
	
	// Do we have an enemy?
	if ( m_hRagdoll )
	{
		if ( m_bLiftingPrey )
		{	
			LiftPrey();
		}
		else if ( m_bSwallowingPrey )
		{
			// Slowly swallowing the ragdoll
			SwallowPrey();
		}
		// Stay bloated as we digest
		else if ( m_flDigestFinish )
		{
			// Still digesting him>
			if ( m_flDigestFinish > gpGlobals->curtime )
			{
				if ( IsActivityFinished() )
				{
					SetActivity( ACT_IDLE );
				}

				// bite prey every once in a while
				if ( random->RandomInt(0,25) == 0 )
				{
					EmitSound( "NPC_Barnacle.Digest" );
				}
			}
			else
			{
				// Finished digesting
				LostPrey( true ); // Remove all evidence
				m_flDigestFinish = 0;
			}
		}
	}
	else if ( GetEnemy() )
	{
		if ( m_bLiftingPrey )
		{	
			LiftPrey();
		}
	}
	else
	{
		// Were we lifting prey?
		if ( m_bSwallowingPrey || m_bLiftingPrey ) 
		{
			// Something removed our prey.
			LostPrey( false );
		}

		// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		// NOTE: Use the surrounding bounds so that we'll think often event if the tongue
		// tip is in the PVS but the body isn't
		Vector vecSurroundMins, vecSurroundMaxs;
		CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
		if ( !UTIL_FindClientInPVS( vecSurroundMins, vecSurroundMaxs ) )
		{
			SetNextThink( gpGlobals->curtime + random->RandomFloat(1,1.5) );	// Stagger a bit to keep barnacles from thinking on the same frame
		}

		if ( IsActivityFinished() && GetActivity() != ACT_IDLE )
		{
			// this is done so barnacle will fidget.
			SetActivity( ACT_IDLE );
		}

		if ( m_cGibs && random->RandomInt(0,99) == 1 )
		{
			// cough up a gib.
			CGib::SpawnSpecificGibs( this, 1, 50, 1, "models/gibs/hgibs_rib.mdl");
			m_cGibs--;

			EmitSound( "NPC_Barnacle.Digest" );
		}

		pTouchEnt = TongueTouchEnt( &flLength );

		// If there's something under us, lower the tongue down so we can grab it
		if ( m_flAltitude < flLength )
		{
			float dt = gpGlobals->curtime - GetLastThink();
			SetAltitude( m_flAltitude + m_flBarnaclePullSpeed * dt );
		}

		// NOTE: SetAltitude above will change m_flAltitude, hence the second check
		if ( m_flAltitude >= flLength )
		{
			// If we're already low enough, try to grab.
			bool bGrabbedTarget = false;
			if ( ( pTouchEnt != NULL ) && ( pTouchEnt != m_hLastSpitEnemy.Get() ) )
			{
				// tongue is fully extended, and is touching someone.
				CBaseCombatCharacter *pBCC = dynamic_cast<CBaseCombatCharacter *>(pTouchEnt);

				if( CanPickup( pBCC ) )
				{
					Vector vecGrabPos = pTouchEnt->EyePosition();
					if( !pBCC || pBCC->DispatchInteraction( g_interactionBarnacleVictimGrab, &vecGrabPos, this ) )
					{
						EmitSound( "NPC_Barnacle.BreakNeck" );
						AttachTongueToTarget( pTouchEnt, vecGrabPos );
						
						// Set the local timer to 60 seconds, which starts the lifting phase on
						// the upshot of the sine wave which right away makes it more obvious
						// that the player is being lifted.
						m_flLocalTimer = 60.0f;
						m_vLastEnemyPos = pTouchEnt->GetAbsOrigin();
						m_flLastPull = 0;
						m_StuckTimer.Set(3.0);
						bGrabbedTarget = true;
					}
				}
			}

			if ( !bGrabbedTarget )
			{
				// Restore the hanging spring constant 
				m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_HANGING );
				SetAltitude( flLength );
			}
		}
	}

	// NDebugOverlay::Box( GetAbsOrigin() - Vector( 0, 0, m_flAltitude ), Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,255,255, 0, 0.1 );

	StudioFrameAdvance();
	DispatchAnimEvents( this );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Barnacle::CanPickup( CBaseCombatCharacter *pBCC )
{
	// Barnacle can pick this item up because it has already passed the filters
	// in TongueTouchEnt. It just isn't an NPC or player and doesn't need further inspection.
	if( !pBCC )
		return true;

	// Don't pickup turrets
	if( FClassnameIs( pBCC, "npc_turret_floor" ) )
		return false;

	// Don't pick up a dead player or NPC
	if( !pBCC->IsAlive() )
		return false;

	if( pBCC->IsPlayer() )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(pBCC);
	
		Assert( pPlayer != NULL );

		// Don't pick up a player held by another barnacle
		if( pPlayer->HasPhysicsFlag(PFLAG_ONBARNACLE) )
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Allows the ragdoll to settle before biting it
//-----------------------------------------------------------------------------
bool CNPC_Barnacle::WaitForRagdollToSettle( float flBiteZOffset )
{
	Vector vecVictimPos = GetEnemy()->GetAbsOrigin();

	Vector vecCheckPos;
	QAngle vecBoneAngles;
	m_hRagdoll->GetBonePosition( m_iGrabbedBoneIndex, vecCheckPos, vecBoneAngles );

	// Stop sucking while we wait for the ragdoll to settle
	SetActivity( ACT_IDLE );

	Vector vecVelocity;
	AngularImpulse angVel;
	float flDelta = 4.0;

	// Only bite if the target bone is in the right position.
	Vector vecBitePoint = GetAbsOrigin();
	vecBitePoint.z -= flBiteZOffset;

	//NDebugOverlay::Box( vecBitePoint, -Vector(10,10,10), Vector(10,10,10), 0,255,0, 0, 0.1 );
	if ( (vecBitePoint.x - vecCheckPos.x) > flDelta || (vecBitePoint.y - vecCheckPos.y) > flDelta )
	{
		// I can't bite this critter because it's not lined up with me on the X/Y plane. If it is 
		// as close to my mouth as I can get it, I should drop it.
		if( vecBitePoint.z - vecVictimPos.z < 72.0f )
		{
			// A man-sized target has been pulled up to my mouth, but 
			// is not aligned for biting. Drop it.
			SpitPrey();
		}

		return false;
	}

	// Right height?
	if ( (vecBitePoint.z - vecCheckPos.z) > flDelta )
	{
		// Slowly raise / lower the target into the right position
		if ( vecBitePoint.z > vecCheckPos.z )
		{
			// Pull the victim towards the mouth
			SetAltitude( m_flAltitude - 1 );
			vecVictimPos.z += 1;
		}
		else
		{
			// We pulled 'em up too far, so lower them a little
			SetAltitude( m_flAltitude + 1 );
			vecVictimPos.z -= 1;
		}
		UTIL_SetOrigin ( GetEnemy(), vecVictimPos );
		return false;
	}

	// Get the velocity of the bone we've grabbed, and only bite when it's not moving much
	studiohdr_t *pStudioHdr = m_hRagdoll->GetModelPtr();
	mstudiobone_t *pBone = pStudioHdr->pBone( m_iGrabbedBoneIndex );
	int iBoneIndex = pBone->physicsbone;
	ragdoll_t *pRagdoll = m_hRagdoll->GetRagdoll();
	IPhysicsObject *pRagdollPhys = pRagdoll->list[iBoneIndex].pObject;
	pRagdollPhys->GetVelocity( &vecVelocity, &angVel );
	return ( vecVelocity.LengthSqr() < 20 );
}


//-----------------------------------------------------------------------------
// Allows the physics prop to settle before biting it
//-----------------------------------------------------------------------------
bool CNPC_Barnacle::WaitForPhysicsObjectToSettle( float flBiteZOffset )
{
	--m_nShakeCount;
	if ( m_nShakeCount & 0x1 )
	{
		SetAltitude( flBiteZOffset + 15 );
	}
	else
	{
		SetAltitude( flBiteZOffset );
	}

	return ( m_nShakeCount <= 0 );


	/*
	IPhysicsObject *pPhysicsObject = GetEnemy()->VPhysicsGetObject();

	Vector vecVelocity;
	AngularImpulse angVel;
	pPhysicsObject->GetVelocity( &vecVelocity, &angVel );

	return ( vecVelocity.LengthSqr() < 25 );
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Lift the prety stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::PlayLiftingScream( float flBiteZOffset )
{
	if ( !m_bPlayedPullSound && m_flAltitude < (flBiteZOffset + 100) )
	{
		EmitSound( "NPC_Barnacle.Scream" );
		m_bPlayedPullSound = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Lift the prety stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::PullEnemyTorwardsMouth( bool bAdjustEnemyOrigin )
{
	if ( GetEnemy()->IsPlayer() && GetEnemy()->GetMoveType() == MOVETYPE_NOCLIP )
	{
		LostPrey( false );
		return;
	}

	// Pull the victim towards the mouth
	float dt = gpGlobals->curtime - GetLastThink();

	// Assumes constant frame rate :|
	m_flLocalTimer += dt;

	float flPull = fabs(sin( m_flLocalTimer * 5 ));

	flPull *= m_flBarnaclePullSpeed * dt;

	SetAltitude( m_flAltitude - flPull );

	if ( bAdjustEnemyOrigin )
	{
		if ( m_flLastPull > 1.0 )
		{
			if ( (GetEnemy()->GetAbsOrigin() - m_vLastEnemyPos).LengthSqr() < Square( m_flLastPull - 1.0 ) )
			{
				if ( m_StuckTimer.Expired() )
				{
					LostPrey( false );
					return;
				}
			}
			else
			{
				m_StuckTimer.Set(3.0);
			}
		}
		else
			m_StuckTimer.Delay(dt);

		m_vLastEnemyPos = GetEnemy()->GetAbsOrigin();
		m_flLastPull = flPull;

		Vector vecNewPos = m_vLastEnemyPos;
		vecNewPos.z += flPull;

#if 0
		const float MAX_CENTERING_VELOCITY = 2.0;
		float distToMove = MAX_CENTERING_VELOCITY * dt;
		Vector2D vToCenter = GetAbsOrigin().AsVector2D() - GetEnemy()->GetAbsOrigin().AsVector2D();
		float distFromCenter = vToCenter.NormalizeInPlace();

		if ( distFromCenter < distToMove )
		{
			vecNewPos.x = GetAbsOrigin().x;
			vecNewPos.y = GetAbsOrigin().y;
		}
		else
		{
			vToCenter *= distToMove;
			vecNewPos.x += vToCenter.x;
			vecNewPos.y += vToCenter.y;
		}
#endif

		GetEnemy()->SetAbsOrigin( vecNewPos );

		if( GetEnemy()->GetFlags() & FL_ONGROUND )
		{
			// Try to fight OnGround
			GetEnemy()->SetGravity( 0 );
			GetEnemy()->RemoveFlag( FL_ONGROUND );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Barnacle::UpdatePlayerContraint( void )
{
	// Check to see if the player's standing/ducking state has changed.
	CBasePlayer *pPlayer = static_cast<CBasePlayer*>( GetEnemy() );
	bool bStanding = ( ( pPlayer->GetFlags() & FL_DUCKING ) == 0 );
	if ( bStanding == m_bPlayerWasStanding )
		return;

	// Destroy the current constraint.
	physenv->DestroyConstraint( m_pConstraint );
	m_pConstraint = NULL;

	// Create the new constraint for the standing/ducking player physics object.
	IPhysicsObject *pPlayerPhys = pPlayer->VPhysicsGetObject();
	IPhysicsObject *pTonguePhys = m_hTongueTip->VPhysicsGetObject();
	
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( pTonguePhys, pPlayerPhys );
	fixed.constraint.Defaults();
	m_pConstraint = physenv->CreateFixedConstraint( pTonguePhys, pPlayerPhys, NULL, fixed );

	// Save state for the next check.
	m_bPlayerWasStanding = bStanding;
}

//-----------------------------------------------------------------------------
// Purpose: Lift the prey stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LiftPlayer( float flBiteZOffset )
{
	// Add an additional height for the player to avoid view clipping
	flBiteZOffset += 25.0;

	// Play a scream when we're almost within bite range
	PlayLiftingScream( flBiteZOffset );

	// Update player constraint.
	UpdatePlayerContraint();

	// Figure out when the prey has reached our bite range use eye position to avoid
	// clipping into the barnacle body
	if ( GetAbsOrigin().z - GetEnemy()->EyePosition().z < flBiteZOffset)
	{
		m_bLiftingPrey = false;

		// Start the bite animation. The anim event in it will finish the job.
		SetActivity( (Activity)ACT_BARNACLE_BITE_PLAYER );
	}
	else
	{
		PullEnemyTorwardsMouth( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Lift the prey stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LiftNPC( float flBiteZOffset )
{
	// Necessary to make the NPCs not do things like talk
	GetEnemy()->AddEFlags( EFL_IS_BEING_LIFTED_BY_BARNACLE );

	// Play a scream when we're almost within bite range
	PlayLiftingScream( flBiteZOffset );

	// Figure out when the prey has reached our bite range
	if ( GetAbsOrigin().z - m_vecTip.Get().z < flBiteZOffset )
	{
		m_bLiftingPrey = false;

		const Vector &vecSize = GetEnemy()->CollisionProp()->OBBSize();
		if ( vecSize.z < 40 )
		{
			// Start the bite animation. The anim event in it will finish the job.
			SetActivity( (Activity)ACT_BARNACLE_BITE_SMALL_THINGS );
		}
		else
		{
			// Start the bite animation. The anim event in it will finish the job.
			SetActivity( (Activity)ACT_BARNACLE_BITE_HUMAN );
		}
	}
	else
	{
		PullEnemyTorwardsMouth( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Lift the prey stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LiftRagdoll( float flBiteZOffset )
{
	// Necessary to make the NPCs not do things like talk
	GetEnemy()->AddEFlags( EFL_IS_BEING_LIFTED_BY_BARNACLE );

	// Play a scream when we're almost within bite range
	PlayLiftingScream( flBiteZOffset );

	// Figure out when the prey has reached our bite range
	if ( GetAbsOrigin().z - m_vecTip.Get().z < flBiteZOffset )
	{
		// If we've got a ragdoll, wait until the bone is down below the mouth.
		if ( !WaitForRagdollToSettle( flBiteZOffset ) )
			return;

		if( GetEnemy()->Classify() == CLASS_ZOMBIE )
		{
			// lifted the prey high enough to see it's a zombie. Spit it out.
			SpitPrey();
			return;
		}

		m_bLiftingPrey = false;

		const Vector &vecSize = GetEnemy()->CollisionProp()->OBBSize();
		if ( vecSize.z < 40 )
		{
			// Start the bite animation. The anim event in it will finish the job.
			SetActivity( (Activity)ACT_BARNACLE_BITE_SMALL_THINGS );
		}
		else
		{
			// Start the bite animation. The anim event in it will finish the job.
			SetActivity( (Activity)ACT_BARNACLE_BITE_HUMAN );
		}
	}
	else
	{
		// Pull the victim towards the mouth
		PullEnemyTorwardsMouth( false );

		// Apply forces to the attached ragdoll based upon the animations of the enemy, if the enemy is still alive.
		if ( GetEnemy()->IsAlive() )
		{
			CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>( GetEnemy() );

			// Get the current bone matrix
			/*
			Vector pos[MAXSTUDIOBONES];
			Quaternion q[MAXSTUDIOBONES];
			matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
			CalcPoseSingle( pStudioHdr, pos, q, pAnimating->GetSequence(), pAnimating->m_flCycle, pAnimating->GetPoseParameterArray(), BONE_USED_BY_ANYTHING );
			Studio_BuildMatrices( pStudioHdr, vec3_angle, vec3_origin, pos, q, -1, pBoneToWorld, BONE_USED_BY_ANYTHING );


			// Apply the forces to the ragdoll
			RagdollApplyAnimationAsVelocity( *(m_hRagdoll->GetRagdoll()), pBoneToWorld );
			*/

			// Get the current bone matrix
			matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
			pAnimating->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );

			// Apply the forces to the ragdoll
			RagdollApplyAnimationAsVelocity( *(m_hRagdoll->GetRagdoll()), m_pRagdollBones, pBoneToWorld, 0.2 );

			// Store off the current bone matrix for next time
			pAnimating->SetupBones( m_pRagdollBones, BONE_USED_BY_ANYTHING );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Lift the prey stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LiftPhysicsObject( float flBiteZOffset )
{
	CBaseEntity *pVictim = GetEnemy();

	// Bite a little higher up, since the bits point is the tip of the tongue
	flBiteZOffset -= 5.0f;

	//NDebugOverlay::Box( vecCheckPos, -Vector(10,10,10), Vector(10,10,10), 255,255,255, 0, 0.1 );

	// Play a scream when we're almost within bite range
	PlayLiftingScream( flBiteZOffset );

	// Figure out when the prey has reached our bite range
	if ( GetAbsOrigin().z - m_vecTip.Get().z < flBiteZOffset )
	{

		m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_HANGING );

		// Wait until the physics object stops flailing
		if ( !WaitForPhysicsObjectToSettle( flBiteZOffset ) )
			return;

		// Necessary for good +use interactions
		pVictim->RemoveEFlags( EFL_IS_BEING_LIFTED_BY_BARNACLE );

		// If we got a physics prop, wait until the thing has settled down
		m_bLiftingPrey = false;

		// Start the bite animation. The anim event in it will finish the job.
		SetActivity( (Activity)ACT_BARNACLE_TASTE_SPIT );
	}
	else
	{
		// Necessary for good +use interactions
		pVictim->AddEFlags( EFL_IS_BEING_LIFTED_BY_BARNACLE );

		// Pull the victim towards the mouth
		PullEnemyTorwardsMouth( false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Lift the prey stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LiftPrey( void )
{
	CBaseEntity *pVictim = GetEnemy();
	Assert( GetEnemy() );

	// Drop the prey if it's been obscured by something
	trace_t tr;
	AI_TraceLine( WorldSpaceCenter(), pVictim->WorldSpaceCenter(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( !pVictim->IsAlive() || (tr.fraction < 1.0 && tr.m_pEnt != pVictim && tr.m_pEnt != m_hRagdoll) )
	{
		LostPrey( false );
		return;
	}

	// Height from the barnacle's origin to the point at which it bites
	float flBiteZOffset = 60.0;

	if ( IsEnemyAPlayer() )
	{
		LiftPlayer(flBiteZOffset);
	}
	else if ( IsEnemyARagdoll() )
	{
		LiftRagdoll(flBiteZOffset);
	}
	else if ( IsEnemyAnNPC() )
	{
		LiftNPC(flBiteZOffset);
	}
	else
	{
		LiftPhysicsObject(flBiteZOffset);
	}

	if ( m_hRagdoll )
	{
		QAngle newAngles( 0, m_hRagdoll->GetAbsAngles()[YAW], 0 );

		Vector centerDelta = m_hRagdoll->WorldSpaceCenter() - GetEnemy()->WorldSpaceCenter();
		Vector newOrigin = GetEnemy()->GetAbsOrigin() + centerDelta;
		GetEnemy()->SetAbsOrigin( newOrigin );
		GetEnemy()->SetAbsAngles( newAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Grab the specified target with our tongue
//-----------------------------------------------------------------------------
void CNPC_Barnacle::AttachTongueToTarget( CBaseEntity *pTouchEnt, Vector vecGrabPos )
{
	// Reset this value each time we attach prey. If it needs to be reduced, code below will do so.
	m_flBarnaclePullSpeed = BARNACLE_PULL_SPEED;

	if ( RandomFloat(0,1) > 0.5 )
	{
		EmitSound( "NPC_Barnacle.PullPant" );
	}
	else
	{
		EmitSound( "NPC_Barnacle.TongueStretch" );
	}

	SetActivity( (Activity)ACT_BARNACLE_SLURP );

	// Get the player out of the vehicle he's in.
	if ( pTouchEnt->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>(pTouchEnt);
		if ( pPlayer->IsInAVehicle() )
		{
			pPlayer->LeaveVehicle( pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles() );

			// The player could have warped through the tongue while on a high-speed vehicle.
			// Move him back under the barnacle.
			Vector vecDelta;
			VectorSubtract( pPlayer->GetAbsOrigin(), GetAbsOrigin(), vecDelta );
			vecDelta.z = 0.0f;
			float flDist = VectorNormalize( vecDelta );
			if ( flDist > 20 )
			{
				Vector vecNewPos;
				VectorMA( GetAbsOrigin(), 20, vecDelta, vecNewPos );
				vecNewPos.z = pPlayer->GetAbsOrigin().z;
				pPlayer->SetAbsOrigin( vecNewPos );
			}
		}

		m_bPlayerWasStanding = ( ( pPlayer->GetFlags() & FL_DUCKING ) == 0 );
	}

	SetEnemy( pTouchEnt );

	if ( pTouchEnt->IsPlayer() || pTouchEnt->MyNPCPointer() )
	{
		Vector origin = GetAbsOrigin();
		origin.z = pTouchEnt->GetAbsOrigin().z;

		CTraceFilterSkipTwoEntities traceFilter( this, pTouchEnt, COLLISION_GROUP_NONE );
		trace_t placementTrace;
		UTIL_TraceHull( origin, origin, pTouchEnt->WorldAlignMins(), pTouchEnt->WorldAlignMaxs(), MASK_NPCSOLID, &traceFilter, &placementTrace );
		if ( placementTrace.startsolid )
		{
			UTIL_TraceHull( origin + Vector(0, 0, 24), origin, pTouchEnt->WorldAlignMins(), pTouchEnt->WorldAlignMaxs(), MASK_NPCSOLID, &traceFilter, &placementTrace );
			if ( !placementTrace.startsolid )
				pTouchEnt->SetAbsOrigin( placementTrace.endpos );
		}
		else
		{
			pTouchEnt->SetAbsOrigin( origin );
		}
	}

	m_nShakeCount = 6;
	m_bLiftingPrey = true;// indicate that we should be lifting prey.
	SetAltitude( (GetAbsOrigin().z - vecGrabPos.z) );
	m_bPlayedPullSound  = false;

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pTouchEnt);

	if ( IsEnemyAPlayer() || IsEnemyAPhysicsObject() )
	{
		// The player (and phys objects) doesn't ragdoll, so just grab him and pull him up manually
		IPhysicsObject *pPlayerPhys = pTouchEnt->VPhysicsGetObject();
		IPhysicsObject *pTonguePhys = m_hTongueTip->VPhysicsGetObject();

		Vector vecGrabPos;
		if ( pTouchEnt->IsPlayer() )
		{
			vecGrabPos = pTouchEnt->EyePosition();
		}
		else
		{
			VectorSubtract( m_vecTip, pTouchEnt->GetAbsOrigin(), vecGrabPos	);
			VectorNormalize( vecGrabPos );
			vecGrabPos = physcollision->CollideGetExtent( pPlayerPhys->GetCollide(), pTouchEnt->GetAbsOrigin(), pTouchEnt->GetAbsAngles(), vecGrabPos );
		}

		m_hTongueTip->Teleport( &vecGrabPos, NULL, NULL );

		float flDist = (vecGrabPos - GetAbsOrigin() ).Length();
		float flTime = flDist / m_flBarnaclePullSpeed;

		// If this object would be pulled in too quickly, change the pull speed.
		if( flTime < BARNACLE_MIN_PULL_TIME )
		{
			m_flBarnaclePullSpeed = flDist / BARNACLE_MIN_PULL_TIME;
		}

		constraint_fixedparams_t fixed;
		fixed.Defaults();
		fixed.InitWithCurrentObjectState( pTonguePhys, pPlayerPhys );
		fixed.constraint.Defaults();
		m_pConstraint = physenv->CreateFixedConstraint( pTonguePhys, pPlayerPhys, NULL, fixed );

		// Increase the tongue's spring constant while lifting 
		m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LIFTING );
		UpdateTongue();

		return;
	}

	// NPC case...
	pAnimating->InvalidateBoneCache();

	// Make a ragdoll for the guy, and hide him.
	pTouchEnt->AddSolidFlags( FSOLID_NOT_SOLID );

	// Find his head bone
	m_iGrabbedBoneIndex = -1;
	Vector vecNeckOffset = (pTouchEnt->EyePosition() - m_hTongueTip->GetAbsOrigin());
	studiohdr_t *pHdr = pAnimating->GetModelPtr();
	if ( pHdr )
	{
		int set = pAnimating->GetHitboxSet();
		for( int i = 0; i < pHdr->iHitboxCount(set); i++ )
		{
			mstudiobbox_t *pBox = pHdr->pHitbox( i, set );
			if ( !pBox )
				continue;
			
			if ( pBox->group == HITGROUP_HEAD )
			{
				m_iGrabbedBoneIndex = pBox->bone;
				break;
			}
		}
	}
	
	// HACK: Until we have correctly assigned hitgroups on our models, lookup the bones
	//		 for the models that we know are in the barnacle maps.
	//m_iGrabbedBoneIndex = pAnimating->LookupBone( "Bip01 L Foot" );
	if ( m_iGrabbedBoneIndex == -1 )
	{
		// Citizens, Conscripts
		m_iGrabbedBoneIndex = pAnimating->LookupBone( "Bip01 Head" );
	}
	if ( m_iGrabbedBoneIndex == -1 )
	{
		// Metrocops, Combine soldiers
		m_iGrabbedBoneIndex = pAnimating->LookupBone( "ValveBiped.Bip01_Head1" );
	}
	if ( m_iGrabbedBoneIndex == -1 )
	{
		// Vortigaunts
		m_iGrabbedBoneIndex = pAnimating->LookupBone( "ValveBiped.head" );
	}
	if ( m_iGrabbedBoneIndex == -1 )
	{
		// Bullsquids
		m_iGrabbedBoneIndex = pAnimating->LookupBone( "Bullsquid.Head_Bone1" );
	}

	if ( m_iGrabbedBoneIndex == -1 )
	{
		// Just use the first bone
		m_iGrabbedBoneIndex = 0;
	}

	// Move the tip to the bone
	Vector vecBonePos;
	QAngle vecBoneAngles;
	pAnimating->GetBonePosition( m_iGrabbedBoneIndex, vecBonePos, vecBoneAngles );
	m_hTongueTip->Teleport( &vecBonePos, NULL, NULL );

	//NDebugOverlay::Box( vecBonePos, -Vector(5,5,5), Vector(5,5,5), 255,255,255, 0, 10.0 );

	// Create the ragdoll attached to tongue
	IPhysicsObject *pTonguePhysObject = m_hTongueTip->VPhysicsGetObject();
	m_hRagdoll = CreateServerRagdollAttached( pAnimating, vec3_origin, -1, COLLISION_GROUP_NONE, pTonguePhysObject, m_hTongueTip, 0, vecBonePos, m_iGrabbedBoneIndex, vec3_origin );
	m_hRagdoll->DisableAutoFade();
	m_hRagdoll->SetThink( NULL );
	m_hRagdoll->SetDamageEntity( pAnimating );

	// Make it try to blend out of ragdoll on the client on deletion
	// NOTE: This isn't fully implemented, so disable
	//m_hRagdoll->SetUnragdoll( pAnimating );

	// Apply the target's current velocity to each of the ragdoll's bones
	Vector vecVelocity = pAnimating->GetGroundSpeedVelocity() * 0.5;
	ragdoll_t *pRagdoll = m_hRagdoll->GetRagdoll();
	
	// barnacle might let go if ragdoll is separated - so increase the separation checking a bit
	constraint_groupparams_t params;
	pRagdoll->pGroup->GetErrorParams( &params );
	params.minErrorTicks = min( params.minErrorTicks, 5 );
	pRagdoll->pGroup->SetErrorParams( params );

	for ( int i = 0; i < pRagdoll->listCount; i++ )
	{
		pRagdoll->list[i].pObject->AddVelocity( &vecVelocity, NULL );
	}

	if ( npc_barnacle_swallow.GetBool() )
	{
		m_hRagdoll->SetOverlaySequence( ACT_GESTURE_BARNACLE_STRANGLE );
		m_hRagdoll->SetBlendWeight( 1.0f );
	}

	// Now hide the actual enemy
	pTouchEnt->AddEffects( EF_NODRAW );

	// Increase the tongue's spring constant while lifting 
	m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LIFTING );
	UpdateTongue();

	// Store off the current bone matrix so we have it next frame
	pAnimating->SetupBones( m_pRagdollBones, BONE_USED_BY_ANYTHING );
}


//-----------------------------------------------------------------------------
// Spit out the prey; add physics force!
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SpitPrey()
{
	if ( GetEnemy() )
	{
		IPhysicsObject *pObject = GetEnemy()->VPhysicsGetObject();
		if (pObject)
		{
			Vector vecPosition;
			QAngle angles;
			GetAttachment( m_nSpitAttachment, vecPosition, angles );

			Vector force;
			AngleVectors( angles, &force ); 

			force *= pObject->GetMass() * 50.0f;
			pObject->ApplyForceOffset( force, vec3_origin );
		}

		m_hLastSpitEnemy = GetEnemy();
	}

	LostPrey( false );
}


//-----------------------------------------------------------------------------
// Purpose: Prey is in position, bite them and start swallowing them
//-----------------------------------------------------------------------------
void CNPC_Barnacle::BitePrey( void )
{
	Assert( GetEnemy() );

	CBaseCombatCharacter *pVictim = GetEnemyCombatCharacterPointer();
	Assert( pVictim );

	EmitSound( "NPC_Barnacle.FinalBite" );

	m_flVictimHeight = GetEnemy()->WorldAlignSize().z;

	// Kill the victim instantly
	int iDamageType = DMG_SLASH | DMG_ALWAYSGIB;
	int nDamage; 
	if ( !pVictim->IsPlayer() )
	{
		iDamageType |= DMG_ALWAYSGIB;
		nDamage = pVictim->m_iHealth; 
	}
	else
	{
		nDamage = BARNACLE_BITE_DAMAGE_TO_PLAYER; 
	}

	if ( m_hRagdoll )
	{
		// We've got a ragdoll, so prevent this creating another one
		iDamageType |= DMG_REMOVENORAGDOLL;
		m_hRagdoll->SetDamageEntity( NULL );
	}
	// DMG_CRUSH because we don't wan't to impart physics forces
	pVictim->TakeDamage( CTakeDamageInfo( this, this, nDamage, iDamageType | DMG_CRUSH ) );
	m_cGibs = 3;

	// Players are never swallowed, nor is anything we don't have a ragdoll for
	if ( !m_hRagdoll || pVictim->IsPlayer() )
	{
		if ( !pVictim->IsPlayer() || pVictim->GetHealth() <= 0 )
		{
			LostPrey( false );
		}
		return;
	}

	// Stop the ragdoll moving and start to pull the sucker up into our mouth
	m_bSwallowingPrey = true;
	IPhysicsObject *pTonguePhys = m_hTongueTip->VPhysicsGetObject();

	// Make it nonsolid to the world so we can pull it through the roof
	PhysDisableEntityCollisions( m_hRagdoll->VPhysicsGetObject(), g_PhysWorldObject );

	// Stop the tongue's spring getting in the way of swallowing
	m_hTongueTip->m_pSpring->SetSpringConstant( 0 );

	// Switch the tongue tip to shadow and drag it up
	pTonguePhys->SetShadow( 1e4, 1e4, false, false );
	pTonguePhys->UpdateShadow( m_hTongueTip->GetAbsOrigin(), m_hTongueTip->GetAbsAngles(), false, 0 );
	m_hTongueTip->SetMoveType( MOVETYPE_NOCLIP );
	m_hTongueTip->SetAbsVelocity( Vector(0,0,32) );

	SetAltitude( (GetAbsOrigin().z - m_hTongueTip->GetAbsOrigin().z) );

	if ( !npc_barnacle_swallow.GetBool() )
		return;

	// Because the victim is dead, remember the blood color
	m_flNextBloodTime = 0.0f;
	m_nBloodColor = pVictim->BloodColor();
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &m_vecBloodPos );

	// m_hRagdoll->SetOverlaySequence( ACT_DIE_BARNACLE_SWALLOW );
	m_hRagdoll->SetBlendWeight( 0.0f );

	SprayBlood();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SprayBlood()
{
	if ( gpGlobals->curtime < m_flNextBloodTime )
		return;

	m_flNextBloodTime = gpGlobals->curtime + 0.2f;

	Vector bloodDir = RandomVector( -1.0f, 1.0f );
	bloodDir.z = -fabs( bloodDir.z );

	Vector jitterPos = RandomVector( -8, 8 );
	jitterPos.z = 0.0f;

	UTIL_BloodSpray( m_vecBloodPos + jitterPos, Vector( 0,0,-1),
		m_nBloodColor, RandomInt( 4, 8 ), RandomInt(0,2) == 0 ? FX_BLOODSPRAY_ALL : FX_BLOODSPRAY_CLOUD );
}

//-----------------------------------------------------------------------------
// Purpose: Slowly swallow the prey whole. Only used on humanoids.
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SwallowPrey( void )
{
	if ( IsActivityFinished() )
	{
		if (GetActivity() == ACT_BARNACLE_BITE_HUMAN )
		{
			SetActivity( (Activity)ACT_BARNACLE_CHEW_HUMAN );
		}
		else
		{
			SetActivity( (Activity)ACT_BARNACLE_CHEW_SMALL_THINGS );
		}
	}

	// Move the body up slowly
	Vector vecSwallowPos = m_hTongueTip->GetAbsOrigin();
	vecSwallowPos.z -= m_flVictimHeight;
	//NDebugOverlay::Box( vecSwallowPos, -Vector(5,5,5), Vector(5,5,5), 255,255,255, 0, 0.1 );

	// bite prey every once in a while
	if ( random->RandomInt(0,25) == 0 )
	{
		EmitSound( "NPC_Barnacle.Digest" );
	}

	// Fully swallowed it?
	float flDistanceToGo = GetAbsOrigin().z - vecSwallowPos.z;
	if ( flDistanceToGo <= 0 )
	{
		// He's dead jim
		m_bSwallowingPrey = false;
		m_hTongueTip->SetAbsVelocity( vec3_origin );
		m_flDigestFinish = gpGlobals->curtime + 10.0;
	}

	if ( npc_barnacle_swallow.GetBool() )
	{
		SprayBlood();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove the fake ragdoll and bring the actual enemy back in view
//-----------------------------------------------------------------------------
void CNPC_Barnacle::RemoveRagdoll( bool bDestroyRagdoll )
{
	// Destroy the tongue tip constraint
  	if ( m_pConstraint )
  	{
  		physenv->DestroyConstraint( m_pConstraint );
  		m_pConstraint = NULL;
  	}

	// Remove the ragdoll
	if ( m_hRagdoll )
	{
		// Only destroy the ragdoll if told to. We might be just dropping
		// the ragdoll because the target was killed on the way up.
		m_hRagdoll->SetDamageEntity( NULL );
		if ( npc_barnacle_swallow.GetBool() )
		{
			m_hRagdoll->SetThink( NULL );
			m_hRagdoll->SetBlendWeight( 1.0f );
		}
		DetachAttachedRagdoll( m_hRagdoll );
		if ( bDestroyRagdoll )
		{
			UTIL_Remove( m_hRagdoll );
		}
		m_hRagdoll = NULL;

		// Reduce the spring constant while we lower
		m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LOWERING );

		// Unhide the enemy
		if ( GetEnemy() )
		{
			GetEnemy()->RemoveEffects( EF_NODRAW );
			GetEnemy()->RemoveSolidFlags( FSOLID_NOT_SOLID );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For some reason (he was killed, etc) we lost the prey we were dragging towards our mouth.
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LostPrey( bool bRemoveRagdoll )
{
	if ( GetEnemy() )
	{
		//No one survives being snatched by a barnacle anymore, so leave
		// this flag set so that their entity gets removed.
		//GetEnemy()->RemoveEFlags( EFL_IS_BEING_LIFTED_BY_BARNACLE );
		CBaseCombatCharacter *pVictim = GetEnemyCombatCharacterPointer();
		if ( pVictim )
		{
			pVictim->DispatchInteraction( g_interactionBarnacleVictimReleased, NULL, this );
			if ( m_hRagdoll )
			{
				QAngle newAngles( 0, m_hRagdoll->GetAbsAngles()[ YAW ], 0 );

				Vector centerDelta = m_hRagdoll->WorldSpaceCenter() - GetEnemy()->WorldSpaceCenter();
				Vector newOrigin = GetEnemy()->GetAbsOrigin() + centerDelta;
				GetEnemy()->SetAbsOrigin( newOrigin );

				pVictim->SetAbsAngles( newAngles );
			}
			pVictim->SetGroundEntity( NULL );
		}
	}

	RemoveRagdoll( bRemoveRagdoll );
	m_bLiftingPrey = false;
	m_bSwallowingPrey = false;
	SetEnemy( NULL );

	// Remove our tongue's shadow object, in case we just finished swallowing something
	IPhysicsObject *pPhysicsObject = m_hTongueTip->VPhysicsGetObject();
	if ( pPhysicsObject && pPhysicsObject->GetShadowController() )
	{
		Vector vecCenter = WorldSpaceCenter();
		m_hTongueTip->Teleport( &vecCenter, NULL, &vec3_origin );

		// Reduce the spring constant while we lower
		m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LOWERING );

		// Start colliding with the world again
		pPhysicsObject->RemoveShadowController();
		m_hTongueTip->SetMoveType( MOVETYPE_VPHYSICS );
		pPhysicsObject->EnableMotion( true );
		pPhysicsObject->EnableGravity( true );
		pPhysicsObject->RecheckCollisionFilter();
	}
}


//-----------------------------------------------------------------------------
// The tongue's vphysics updated
//-----------------------------------------------------------------------------
void CNPC_Barnacle::OnTongueTipUpdated()
{
	// Update the tip's position
	const Vector &vecNewTip = m_hTongueTip->GetAbsOrigin();
	if ( vecNewTip != m_vecTip )
	{
		m_vecTip = vecNewTip;
		CollisionProp()->MarkSurroundingBoundsDirty();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Update the positions of the tongue points
//-----------------------------------------------------------------------------
void CNPC_Barnacle::UpdateTongue( void )
{
	// Set the spring's length to that of the tongue's extension

	// Compute the rest length of the tongue based on the spring. 
	// This occurs when mg == kx or x = mg/k
	float flRestStretch = (BARNACLE_TONGUE_TIP_MASS * sv_gravity.GetFloat()) / BARNACLE_TONGUE_SPRING_CONSTANT_HANGING;

	// FIXME: HACK!!!! The code above doesn't quite make the tip end up in the right place.
	// but it should. So, we're gonna hack it the rest of the way.
	flRestStretch += 4;

	m_hTongueTip->m_pSpring->SetSpringLength( m_flAltitude - flRestStretch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SpawnDeathGibs( void )
{
	bool bDroppedAny = false;

	// Drop a random number of gibs
	for ( int i=0; i < ARRAYSIZE(m_szGibNames); i++ )
	{
		if ( random->RandomInt( 0, 1 ) )
		{
			CGib::SpawnSpecificGibs( this, 1, 32, 1, m_szGibNames[i] );
			bDroppedAny = true;
		}
	}

	// Make sure we at least drop something
	if ( bDroppedAny == false )
	{
		CGib::SpawnSpecificGibs( this, 1, 32, 1, m_szGibNames[0] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::Event_Killed( const CTakeDamageInfo &info )
{
	m_OnDeath.FireOutput( info.GetAttacker(), this );

	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;
	m_lifeState	= LIFE_DYING;

	// Are we lifting prey?
	if ( GetEnemy() )
	{
		// Cleanup
		LostPrey( false );
	}
	else if ( m_bSwallowingPrey && m_hRagdoll )
	{
		// We're swallowing a body. Make it stick inside us.
		m_hTongueTip->SetAbsVelocity( vec3_origin );

		m_hRagdoll->StopFollowingEntity();
		m_hRagdoll->SetMoveType( MOVETYPE_VPHYSICS );
		m_hRagdoll->SetAbsOrigin( m_hTongueTip->GetAbsOrigin() );
		m_hRagdoll->RemoveSolidFlags( FSOLID_NOT_SOLID );
		m_hRagdoll->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		m_hRagdoll->RecheckCollisionFilter();
		if ( npc_barnacle_swallow.GetBool() )
		{
			m_hRagdoll->SetThink( NULL );
			m_hRagdoll->SetBlendWeight( 1.0f );
		}
	}
	else
	{
		// Destroy the ragdoll->tongue tip constraint
  		if ( m_pConstraint )
  		{
  			physenv->DestroyConstraint( m_pConstraint );
  			m_pConstraint = NULL;
  		}
		LostPrey( true );
	}

	// Puke gibs unless we're told to be cheap
	bool spawnGibs = ( !HasSpawnFlags( SF_BARNACLE_CHEAP_DEATH ) || random->RandomInt( 0, 1 ) );

	if ( spawnGibs )
	{
		SpawnDeathGibs();
	}

	// Puke blood
	UTIL_BloodSpray( GetAbsOrigin(), Vector(0,0,-1), BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_ALL );

	// Put blood on the ground if near enough
	trace_t bloodTrace;
	AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 256 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &bloodTrace);
	
	if ( bloodTrace.fraction < 1.0f )
	{
		UTIL_BloodDecalTrace( &bloodTrace, BLOOD_COLOR_RED );
	}

	EmitSound( "NPC_Barnacle.Die" );

	SetActivity( ACT_DIESIMPLE );

	StudioFrameAdvance();

	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink ( &CNPC_Barnacle::WaitTillDead );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::WaitTillDead ( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance();
	DispatchAnimEvents ( this );

	if ( IsActivityFinished() )
	{
		// death anim finished. 
		StopAnimation();
	}

	float goalAltitude = BARNACLE_DEAD_TONGUE_ALTITUDE;

	trace_t tr;
	AI_TraceLine( m_vecRoot.Get(), m_vecRoot.Get() - Vector( 0, 0, 256 ), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0 )
	{
		float distToFloor = ( m_vecRoot.Get() - tr.endpos ).Length();
		float clearance = distToFloor - goalAltitude;

		if ( clearance < BARNACLE_MIN_DEAD_TONGUE_CLEARANCE )
		{
			if ( distToFloor - BARNACLE_MIN_DEAD_TONGUE_CLEARANCE > distToFloor * .5 )
			{
				goalAltitude = distToFloor - BARNACLE_MIN_DEAD_TONGUE_CLEARANCE;
			}
			else
			{
				goalAltitude = distToFloor * .5;
			}
		}
	}

	// Keep moving the tongue to its dead position
	// FIXME: This stupid algorithm is necessary because 
	// I can't seem to get reproduceable behavior from springs
	bool bTongueInPosition = false;
	float flDist = m_vecRoot.Get().z - m_vecTip.Get().z;
	if ( fabs(flDist - goalAltitude) > 20.0f )	
	{
		float flNewAltitude;
		float dt = gpGlobals->curtime - GetLastThink();
		if ( m_flAltitude >= goalAltitude )
		{
			flNewAltitude = max( goalAltitude, m_flAltitude - m_flBarnaclePullSpeed * dt );
		}
		else
		{
			flNewAltitude = min( goalAltitude, m_flAltitude + m_flBarnaclePullSpeed * dt );
		}
		SetAltitude( flNewAltitude );
	}
	else
	{
		// Wait for settling...
		IPhysicsObject *pTipObject = m_hTongueTip->VPhysicsGetObject();
		
		Vector vecVelocity;
		AngularImpulse angVel;
		pTipObject->GetVelocity( &vecVelocity, &angVel );
		if ( vecVelocity.LengthSqr() < 1.0f )
		{
			// We may need to have a heavier spring constant until we settle
			// to avoid strange looking rest conditions (when the tongue is really bent from
			// picking up a barrel, it looks strange to switch to the hanging constant)
			m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_HANGING );
			if ( fabs(flDist - goalAltitude) > 1.0f )
			{
				float flSign = ( flDist > goalAltitude ) ? -1.0f : 1.0f;
				SetAltitude( m_flAltitude + flSign );
			}
			else if ( vecVelocity.LengthSqr() < 0.01f )
			{
				bTongueInPosition = ( fabs(flDist - goalAltitude) <= 1.0f );
			}
		}
	}

	if ( IsActivityFinished() && bTongueInPosition )
	{
		// Remove our tongue pieces
		UTIL_Remove( m_hTongueTip );
		UTIL_Remove( m_hTongueRoot );
		m_hTongueTip = NULL;
		m_hTongueRoot = NULL;

		SetThink ( NULL );
		m_lifeState	= LIFE_DEAD;
	}
	else
	{
		UpdateTongue();
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Barnacle::Precache()
{
	PrecacheModel("models/barnacle.mdl");

	// Precache all gibs
	for ( int i=0; i < ARRAYSIZE(m_szGibNames); i++ )
	{
		PrecacheModel( m_szGibNames[i] );
	}

	PrecacheScriptSound( "NPC_Barnacle.Digest" );
	PrecacheScriptSound( "NPC_Barnacle.Scream" );
	PrecacheScriptSound( "NPC_Barnacle.PullPant" );
	PrecacheScriptSound( "NPC_Barnacle.TongueStretch" );
	PrecacheScriptSound( "NPC_Barnacle.FinalBite" );
	PrecacheScriptSound( "NPC_Barnacle.Die" );
	PrecacheScriptSound( "NPC_Barnacle.BreakNeck" );

	BaseClass::Precache();
}	

//=========================================================
// TongueTouchEnt - does a trace along the barnacle's tongue
// to see if any entity is touching it. Also stores the length
// of the trace in the int pointer provided.
//=========================================================
// enumerate entities that match a set of edict flags into a static array
class CTongueEntitiesEnum : public IPartitionEnumerator
{
public:
	CTongueEntitiesEnum( CBaseEntity **pList, int listMax );
	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );
	
	int GetCount() { return m_nCount; }
	bool AddToList( CBaseEntity *pEntity );
	
private:
	CBaseEntity		**m_pList;
	int				m_nListMax;
	int				m_nCount;
};

CTongueEntitiesEnum::CTongueEntitiesEnum( CBaseEntity **pList, int listMax )
{
	m_pList = pList;
	m_nListMax = listMax;
	m_nCount = 0;
}

bool CTongueEntitiesEnum::AddToList( CBaseEntity *pEntity )
{
	m_pList[m_nCount] = pEntity;
	++m_nCount;
	return ( m_nCount < m_nListMax );
}

IterationRetval_t CTongueEntitiesEnum::EnumElement( IHandleEntity *pHandleEntity )
{
	CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
	if ( pEntity )
	{
		if ( !AddToList( pEntity ) )
			return ITERATION_STOP;
	}
	return ITERATION_CONTINUE;
}


//-----------------------------------------------------------------------------
// Barnacle must trace against only brushes and its last enemy
//-----------------------------------------------------------------------------
class CBarnacleTongueFilter : public CTraceFilterSimple
{
	DECLARE_CLASS( CBarnacleTongueFilter, CTraceFilterSimple );

public:
	CBarnacleTongueFilter( CBaseEntity *pLastEnemy, const IHandleEntity *passedict, int collisionGroup ) : 
		CTraceFilterSimple( passedict, collisionGroup )
	{
		m_pLastEnemy = pLastEnemy;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( pServerEntity == m_pLastEnemy )
			return true;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

private:
	CBaseEntity *m_pLastEnemy;
};


#define BARNACLE_CHECK_SPACING	8
CBaseEntity *CNPC_Barnacle::TongueTouchEnt ( float *pflLength )
{
	trace_t		tr;
	float		length;

	// trace once to hit architecture and see if the tongue needs to change position.
	CBarnacleTongueFilter tongueFilter( m_hLastSpitEnemy, this, COLLISION_GROUP_NONE );
	AI_TraceLine ( GetAbsOrigin(), GetAbsOrigin() - Vector ( 0 , 0 , 2048 ), 
		MASK_SOLID_BRUSHONLY, &tongueFilter, &tr );
	
	length = fabs( GetAbsOrigin().z - tr.endpos.z );
	// Pull it up a tad
	length = max(8, length - m_flRestUnitsAboveGround);
	if ( pflLength )
	{
		*pflLength = length;
	}

	Vector delta = Vector( BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0 );
	Vector mins = GetAbsOrigin() - delta;
	Vector maxs = GetAbsOrigin() + delta;
	maxs.z = GetAbsOrigin().z;
	mins.z -= length;

	CBaseEntity *pList[10];
	CTongueEntitiesEnum tongueEnum( pList, 10 );
	partition->EnumerateElementsInBox( PARTITION_ENGINE_SOLID_EDICTS, mins, maxs, false, &tongueEnum );
	int nCount = tongueEnum.GetCount();
	if ( !nCount )
		return NULL;

	for ( int i = 0; i < nCount; i++ )
	{
		CBaseEntity *pTest = pList[i];

		// Can't lift something that's in the process of being lifted...
		// Necessary for good +use interactions
		if ( pTest->IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
			continue;

		// Vehicles can drive so fast that players can warp through the barnacle tongue.
		// Therefore, we have to do a check to ensure that doesn't happen.
		if ( pTest->GetServerVehicle() )
		{
			CBaseEntity *pDriver = pTest->GetServerVehicle()->GetPassenger();
			if ( pDriver )
			{
				Vector vecPrevDriverPos;
				pTest->GetVelocity( &vecPrevDriverPos );
				VectorMA( pDriver->GetAbsOrigin(), -0.1f, vecPrevDriverPos, vecPrevDriverPos );

				Ray_t sweptDriver;
				sweptDriver.Init( vecPrevDriverPos, pDriver->GetAbsOrigin(), pDriver->WorldAlignMins(), pDriver->WorldAlignMaxs() );
				if ( IsBoxIntersectingRay( mins, maxs, sweptDriver ) )
				{
					pTest = pDriver;
				}
			}
		}

		// Deal with physics objects
		if ( pTest->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			IPhysicsObject *pObject = pTest->VPhysicsGetObject();
			if ( pObject && pObject->GetMass() <= BARNACLE_TONGUE_MAX_LIFT_MASS )
			{
				// If this is an item, make sure it's near the tongue before lifting it.
				// Weapons and other items have very large bounding boxes.
				if( pTest->GetSolidFlags() & FSOLID_TRIGGER )
				{
					if( UTIL_DistApprox2D( WorldSpaceCenter(), pTest->WorldSpaceCenter() ) > 16 )
					{
						continue;
					}
				}

				return pTest;
			}
		}

		// NPCs + players
		CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( pTest );
		if ( !pVictim )
			continue;

		// only clients and monsters
		if ( pTest != this && 
			 IRelationType( pTest ) == D_HT && 
			 pVictim->m_lifeState != LIFE_DEAD &&
			 pVictim->m_lifeState != LIFE_DYING &&
			 !( pVictim->GetFlags() & FL_NOTARGET )	)	
		{
			return pTest;
		}
	}

	return NULL;
}

//===============================================================================================================================
// BARNACLE TONGUE TIP
//===============================================================================================================================
// Crane tip
LINK_ENTITY_TO_CLASS( npc_barnacle_tongue_tip, CBarnacleTongueTip );

BEGIN_DATADESC( CBarnacleTongueTip )

	DEFINE_FIELD( m_hBarnacle, FIELD_EHANDLE ),
	DEFINE_PHYSPTR( m_pSpring ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: To by usable by vphysics, this needs to have a phys model.
//-----------------------------------------------------------------------------
void CBarnacleTongueTip::Spawn( void )
{
	Precache();
	SetModel( "models/props_junk/rock001a.mdl" );
	AddEffects( EF_NODRAW );

	// We don't want this to be solid, because we don't want it to collide with the barnacle.
	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_NOT_SOLID );
	BaseClass::Spawn();

	m_pSpring = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBarnacleTongueTip::Precache( void )
{
	PrecacheModel( "models/props_junk/rock001a.mdl" );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBarnacleTongueTip::UpdateOnRemove( )
{
	if ( m_pSpring )
	{
		physenv->DestroySpring( m_pSpring );
		m_pSpring = NULL;
	}
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// If the tip changes, we gotta update the barnacle's notion of his tongue 
//-----------------------------------------------------------------------------
void CBarnacleTongueTip::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	if ( m_hBarnacle.Get() )
	{
		m_hBarnacle->OnTongueTipUpdated();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Activate/create the spring
//-----------------------------------------------------------------------------
bool CBarnacleTongueTip::CreateSpring( CBaseAnimating *pTongueRoot )
{
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	IPhysicsObject *pRootPhysObject = pTongueRoot->VPhysicsGetObject();
	Assert( pRootPhysObject );
	Assert( pPhysObject );

	// Root has huge mass, tip has little
	pRootPhysObject->SetMass( VPHYSICS_MAX_MASS );
	pPhysObject->SetMass( BARNACLE_TONGUE_TIP_MASS );
	float damping = 3;
	pPhysObject->SetDamping( &damping, &damping );

	springparams_t spring;
	spring.constant = BARNACLE_TONGUE_SPRING_CONSTANT_HANGING;
	spring.damping = BARNACLE_TONGUE_SPRING_DAMPING;
	spring.naturalLength = (GetAbsOrigin() - pTongueRoot->GetAbsOrigin()).Length();
	spring.relativeDamping = 10;
	spring.startPosition = GetAbsOrigin();
	spring.endPosition = pTongueRoot->GetAbsOrigin();
	spring.useLocalPositions = false;
	m_pSpring = physenv->CreateSpring( pPhysObject, pRootPhysObject, &spring );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Create a barnacle tongue tip at the bottom of the tongue
//-----------------------------------------------------------------------------
CBarnacleTongueTip *CBarnacleTongueTip::CreateTongueTip( CNPC_Barnacle *pBarnacle, CBaseAnimating *pTongueRoot, const Vector &vecOrigin, const QAngle &vecAngles )
{
	CBarnacleTongueTip *pTip = (CBarnacleTongueTip *)CBaseEntity::Create( "npc_barnacle_tongue_tip", vecOrigin, vecAngles );
	if ( !pTip )
		return NULL;

	pTip->VPhysicsInitNormal( pTip->GetSolid(), pTip->GetSolidFlags(), false );
	if ( !pTip->CreateSpring( pTongueRoot ) )
		return NULL;

	// Set the backpointer to the barnacle
	pTip->m_hBarnacle = pBarnacle;

	// Don't collide with the world
	IPhysicsObject *pTipPhys = pTip->VPhysicsGetObject();

	// turn off all floating / fluid simulation
	pTipPhys->SetCallbackFlags( pTipPhys->GetCallbackFlags() & (~CALLBACK_DO_FLUID_SIMULATION) );
	
	return pTip;
}

//-----------------------------------------------------------------------------
// Purpose: Create a barnacle tongue tip at the root (i.e. inside the barnacle)
//-----------------------------------------------------------------------------
CBarnacleTongueTip *CBarnacleTongueTip::CreateTongueRoot( const Vector &vecOrigin, const QAngle &vecAngles )
{
	CBarnacleTongueTip *pTip = (CBarnacleTongueTip *)CBaseEntity::Create( "npc_barnacle_tongue_tip", vecOrigin, vecAngles );
	if ( !pTip )
		return NULL;

	pTip->AddSolidFlags( FSOLID_NOT_SOLID );

	// Disable movement on the root, we'll move this thing manually.
	pTip->VPhysicsInitShadow( false, false );
	pTip->SetMoveType( MOVETYPE_NONE );
	return pTip;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_barnacle, CNPC_Barnacle )

	// Register our interactions
	DECLARE_INTERACTION( g_interactionBarnacleVictimDangle )
	DECLARE_INTERACTION( g_interactionBarnacleVictimReleased )
	DECLARE_INTERACTION( g_interactionBarnacleVictimGrab )

	// Conditions
		
	// Tasks

	// Activities
	DECLARE_ACTIVITY( ACT_BARNACLE_SLURP )			// Pulling the tongue up with prey on the end
	DECLARE_ACTIVITY( ACT_BARNACLE_BITE_HUMAN )		// Biting the head of a humanoid
	DECLARE_ACTIVITY( ACT_BARNACLE_BITE_PLAYER )	// Biting the head of a humanoid
	DECLARE_ACTIVITY( ACT_BARNACLE_CHEW_HUMAN )		// Slowly swallowing the humanoid
	DECLARE_ACTIVITY( ACT_BARNACLE_BARF_HUMAN )		// Spitting out human legs & gibs
	DECLARE_ACTIVITY( ACT_BARNACLE_TONGUE_WRAP )	// Wrapping the tongue around a target
	DECLARE_ACTIVITY( ACT_BARNACLE_TASTE_SPIT )		// Yuck! Me no like that!
	DECLARE_ACTIVITY( ACT_BARNACLE_BITE_SMALL_THINGS )	// Biting small things, like a headcrab
	DECLARE_ACTIVITY( ACT_BARNACLE_CHEW_SMALL_THINGS )	// Chewing small things, like a headcrab

	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_BARNACLE_PUKEGIB )
	DECLARE_ANIMEVENT( AE_BARNACLE_BITE )
	DECLARE_ANIMEVENT( AE_BARNACLE_SPIT )
	// Schedules

AI_END_CUSTOM_NPC()
