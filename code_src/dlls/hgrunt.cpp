/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// hgrunt
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"scripted.h" //LRC


extern short g_sModelIndexLaser;// buz

// buz: test draw line
void BuzTestDrawLine( Vector p1, Vector p2, int r, int g, int b )
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( p1.x );
		WRITE_COORD( p1.y );
		WRITE_COORD( p1.z );

		WRITE_COORD( p2.x );
		WRITE_COORD( p2.y );
		WRITE_COORD( p2.z );
		WRITE_SHORT( g_sModelIndexLaser );
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 1 ); // life
		WRITE_BYTE( 40 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( r );   // r, g, b
		WRITE_BYTE( g );   // r, g, b
		WRITE_BYTE( b );   // r, g, b
		WRITE_BYTE( 128 );	// brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
}


int g_fGruntQuestion;	// true if an idle grunt asked a question. Cleared when someone answers.
int g_fTerrorQuestion;	// buz: same for terrorists

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	GRUNT_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL						0.35		// volume of grunt sounds
#define GRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH				20
#define HGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS				2 // how many grunt heads are there? 
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	HGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define HGRUNT_9MMAR				( 1 << 0) // AK for terrorists
#define HGRUNT_HANDGRENADE			( 1 << 1)
#define HGRUNT_GRENADELAUNCHER		( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3) // RPK for terrorists

#define HEAD_GROUP					1 // used by terrors
//#define HEAD_GRUNT					0
//#define HEAD_COMMANDER				1
//#define HEAD_SHOTGUN				2
//#define HEAD_M203					3

//buz:
#define DIVERSANT_HEADSTUFF_GROUP		2
#define DIVERSANT_HEADSTUFF_PNV			0
#define DIVERSANT_HEADSTUFF_KASKA		1
#define DIVERSANT_HEADSTUFF_KASKA_PNV	2
#define DIVERSANT_HEADSTUFF_NO			3

#define DIVERSANT_AMMUNITION_GROUP		3
#define DIVERSANT_AMMUNITION_BACKPACK	0
#define DIVERSANT_AMMUNITION_NO			1

#define DIVERSANT_GUN_GROUP		4
#define DIVERSANT_GUN_MP5		0
#define DIVERSANT_GUN_SHOTGUN	1
#define DIVERSANT_GUN_NONE		2

//#define GUN_GROUP					2
//#define GUN_MP5						0
//#define GUN_SHOTGUN					1
//#define GUN_NONE					2


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HGRUNT_AE_RELOAD		( 2 )
#define		HGRUNT_AE_KICK			( 3 )
#define		HGRUNT_AE_BURST1		( 4 )
#define		HGRUNT_AE_BURST2		( 5 ) 
#define		HGRUNT_AE_BURST3		( 6 ) 
#define		HGRUNT_AE_GREN_TOSS		( 7 )
#define		HGRUNT_AE_GREN_LAUNCH	( 8 )
#define		HGRUNT_AE_GREN_DROP		( 9 )
#define		HGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HGRUNT_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL,
	SCHED_GRUNT_DUCK_COVER_WAIT, // buz
//	SCHED_GRUNT_RUN_SHOOTING, // buz
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
	TASK_GRUNT_MOVE_SHOOTING, // buz
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CHGrunt : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	virtual void Shoot ( void );
	void Shotgun ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	virtual void SpeakSentence( void );
	virtual void SetEyePosition ( void ); // buz

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	virtual int ObjectCaps( void ) { return CSquadMonster::ObjectCaps() | (m_iDeadAmmo?FCAP_IMPULSE_USE:0); }
	int m_iDeadAmmo;
	void EXPORT DeadUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Killed( entvars_t *pevAttacker, int iGib );

	// buz: overriden for soldiers - eye position is differs when monster is crouching
	virtual Vector EyePosition( )
	{
		if (m_Activity == ACT_TWITCH)
			return pev->origin + Vector(0, 0, 36);
		else
			return pev->origin + pev->view_ofs;
	}

	void ResetSequenceInfo ( );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;

	int		m_iLastFireCheckResult; // buz. 1-only crouch, 2-only standing, 0-any

	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS( monster_human_grunt, CHGrunt );
LINK_ENTITY_TO_CLASS( monster_diversant, CHGrunt );

TYPEDESCRIPTION	CHGrunt::m_SaveData[] = 
{
	DEFINE_FIELD( CHGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHGrunt, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CHGrunt, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CHGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_iSentence, FIELD_INTEGER ),
	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	DEFINE_FIELD( CHGrunt, m_iDeadAmmo, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHGrunt, CSquadMonster );

const char *CHGrunt::pGruntSentences[] = 
{
	"DV_GREN", // grenade scared grunt
	"DV_ALERT", // sees player
	"DV_MONSTER", // sees monster
	"DV_COVER", // running to cover
	"DV_THROW", // about to throw grenade
	"DV_CHARGE",  // running out to get the enemy
	"DV_TAUNT", // say rude things
};

enum
{
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN = 0,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT,
} HGRUNT_SENTENCE_TYPES;



// buz: ResetSequenceInfo overriden for grunts -
// m_flGroundSpeed should be set accrording to pev->gaitsequence
void CHGrunt :: ResetSequenceInfo ( )
{
	CBaseAnimating::ResetSequenceInfo();

	if (pev->gaitsequence)
	{
		float dummy;
		int savedsequence = pev->sequence;
		pev->sequence = pev->gaitsequence;

		void *pmodel = GET_MODEL_PTR( ENT(pev) );
		GetSequenceInfo( pmodel, pev, &dummy, &m_flGroundSpeed );
		pev->sequence = savedsequence;
	}
}

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CHGrunt :: SpeakSentence( void )
{
	if ( m_iSentence == HGRUNT_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pGruntSentences[ m_iSentence ], HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CHGrunt::IRelationship ( CBaseEntity *pTarget )
{
	//LRC- only hate alien grunts if my behaviour hasn't been overridden
	if (!m_iClass && FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CHGrunt :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( DIVERSANT_GUN_GROUP ) != DIVERSANT_GUN_NONE && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{// throw a gun if the grunt has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
		{
			pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
		}
		else
		{
			pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
		}
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	
		if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
		{
			pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CHGrunt :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CHGrunt :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void CHGrunt :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = HGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHGrunt :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CHGrunt :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CHGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CHGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients who are close enough, but don't shoot at them.
			return FALSE;
		}

		BOOL savedStanding = m_fStanding;
		m_fStanding = FALSE; // buz: check chrouched fire first
		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
		if ( tr.flFraction == 1.0 && !pev->gaitsequence) // buz: cant fire crouched when moving
		{
			// buz: we can fire crouched, now check for standing
			m_fStanding = TRUE;
			vecSrc = GetGunPosition();
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
			m_fStanding = savedStanding;
			if ( tr.flFraction == 1.0 )
				m_iLastFireCheckResult = 0; // shoot as you wish
			else			
				m_iLastFireCheckResult = 1; // only chrouched

			return TRUE;
		}
		else
		{
			// buz: cant fire crouching, maybe me or enemy in some kind of cover (or running). Check standing.
			m_fStanding = TRUE;
			vecSrc = GetGunPosition();
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
			m_fStanding = savedStanding;
			if ( tr.flFraction == 1.0 )
			{
				m_iLastFireCheckResult = 2; // buz: standing is our only one choice
				return TRUE;
			}
			else
			{
				m_iLastFireCheckResult = 0;
				return FALSE; // cant fire
			}
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CHGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER)))
	{
		return FALSE;
	}
	
	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && (m_hEnemy->pev->waterlevel == 0 || m_hEnemy->pev->watertype==CONTENT_FOG) && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		
	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CHGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
//	ALERT(at_console, "hgrunt hitgr: %d\n", ptr->iHitgroup);
	// check for helmet shot
	if ((ptr->iHitgroup == 8) || (ptr->iHitgroup == 1))
	{
		// make sure we're wearing one
/*		if (GetBodygroup( 1 ) == HEAD_GRUNT && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}*/ // buz
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CHGrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHGrunt :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CHGrunt :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fGruntQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fGruntQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "DV_CHECK", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fGruntQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "DV_QUEST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fGruntQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "DV_IDLE", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fGruntQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "DV_CLEAR", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "DV_ANSWER", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fGruntQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHGrunt :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHGrunt :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CHGrunt :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CHGrunt :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
	//	return pev->origin + Vector( 0, 0, 48 );
		return pev->origin + Vector( 0, 0, 40 ); // buz: prevent firing crouched, when hiding behind the barrels, crates etc.
	}
}

//=========================================================
// Shoot
//=========================================================
void CHGrunt :: Shoot ( void )
{
//	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
//	{
//		return;
//	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		Vector vecBrassPos, vecBrassDir;
		GetAttachment(3, vecBrassPos, vecBrassDir);
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_7DEGREES, 2048, BULLET_MONSTER_MP5 ); // shoot +-5 degrees

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	//	ALERT(at_console, "grunt ammo has %d\n", m_cAmmoLoaded);
	}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CHGrunt :: Shotgun ( void )
{
	if (m_hEnemy == NULL && m_pCine == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector vecBrassPos, vecBrassDir;
	GetAttachment(3, vecBrassPos, vecBrassDir);
	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecBrassPos, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL); 
	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
			{
			if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( DIVERSANT_GUN_GROUP, DIVERSANT_GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
			{
				 DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
			}
			else
			{
				 DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
			}
			if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
			{
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
			}

			}
			break;

		case HGRUNT_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "diversant/gr_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			//LRC - a bit of a hack. Ideally the grunts would work out in advance whether it's ok to throw.
			if (m_pCine)
			{
				Vector vecToss = g_vecZero;
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
				{
					vecToss = VecCheckToss( pev, GetGunPosition(), m_hTargetEnt->pev->origin, 0.5 );
				}
				if (vecToss == g_vecZero)
				{
					vecToss = (gpGlobals->v_forward*0.5+gpGlobals->v_up*0.5).Normalize()*gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootTimed( pev, GetGunPosition(), vecToss, 3.5 );
			}
			else
				CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case HGRUNT_AE_GREN_LAUNCH:
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			//LRC: firing due to a script?
			if (m_pCine)
			{
				Vector vecToss;
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
					vecToss = VecCheckThrow( pev, GetGunPosition(), m_hTargetEnt->pev->origin, gSkillData.hgruntGrenadeSpeed, 0.5 );
				else
				{
					// just shoot diagonally up+forwards
					UTIL_MakeVectors(pev->angles);
					vecToss = (gpGlobals->v_forward*0.5 + gpGlobals->v_up*0.5).Normalize() * gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootContact( pev, GetGunPosition(), vecToss );
			}
			else
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			if (g_iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		case HGRUNT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case HGRUNT_AE_BURST1:
		{
		//	ALERT(at_console, "-------- grunt burst 1\n");
			if ( FBitSet( pev->weapons, HGRUNT_9MMAR ))
			{
				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if (m_cAmmoLoaded > 0)
				{
					if ( RANDOM_LONG(0,1) )
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "diversant/gr_mgun1.wav", 1, ATTN_NORM );
					}
					else
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "diversant/gr_mgun2.wav", 1, ATTN_NORM );
					}
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );
				}

				Shoot();
			}
			else
			{
				Shotgun( );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
			}
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case HGRUNT_AE_BURST2:
		case HGRUNT_AE_BURST3:
			Shoot();
			break;

		case HGRUNT_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
			}
		}
		break;

		case HGRUNT_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "DV_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHGrunt :: Spawn()
{
	Precache( );

	// buz: rename to monster_human_grunt if set as monster_diversant
	pev->classname = MAKE_STRING("monster_human_grunt"); 

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/diversant.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
	}

	// buz: pev->body is head stuff number
	int headstuff = pev->body;
	pev->body = 0;
	SetBodygroup( DIVERSANT_HEADSTUFF_GROUP, headstuff );

	// hack for studiomodelrenderer to draw goggles
	if (headstuff == DIVERSANT_HEADSTUFF_PNV)
		pev->renderfx = 50;

	// buz: pev->effects is ammunition number
	int backpack = pev->effects;
	pev->effects = 0;
	SetBodygroup( DIVERSANT_AMMUNITION_GROUP, backpack );

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
	{
		SetBodygroup( DIVERSANT_GUN_GROUP, DIVERSANT_GUN_SHOTGUN );
		m_cClipSize = 8;
	}
	else
	{
		SetBodygroup( DIVERSANT_GUN_GROUP, DIVERSANT_GUN_MP5 );
		m_cClipSize = GRUNT_CLIP_SIZE;
	}

	m_cAmmoLoaded = m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	m_iDeadAmmo = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGrunt :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/diversant.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	PRECACHE_MODEL("models/mask.mdl"); // buz test

	PRECACHE_SOUND( "diversant/gr_mgun1.wav" );
	PRECACHE_SOUND( "diversant/gr_mgun2.wav" );
	
	PRECACHE_SOUND( "diversant/gr_die1.wav" );
	PRECACHE_SOUND( "diversant/gr_die2.wav" );
	PRECACHE_SOUND( "diversant/gr_die3.wav" );

	PRECACHE_SOUND( "diversant/gr_pain1.wav" );
	PRECACHE_SOUND( "diversant/gr_pain2.wav" );
	PRECACHE_SOUND( "diversant/gr_pain3.wav" );
	PRECACHE_SOUND( "diversant/gr_pain4.wav" );
	PRECACHE_SOUND( "diversant/gr_pain5.wav" );

	PRECACHE_SOUND( "diversant/gr_reload1.wav" );

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND( "weapons/sbarrel1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	PRECACHE_SOUND("items/9mmclip1.wav");

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");
}	

//=========================================================
// start task
//=========================================================
void CHGrunt :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_GRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_GRUNT_MOVE_SHOOTING: // buz
		if (FRouteClear())
			TaskComplete();

		Forget( bits_MEMORY_INCOVER );
		pev->gaitsequence = LookupSequence( "run" ); // test
		if (HasConditions( bits_COND_CAN_RANGE_ATTACK1 ))
			m_IdealActivity = ACT_USE; // special shoot anim (blended)
		else
			m_IdealActivity = ACT_GUARD; // special wait anim (blended)
		
		break;		

	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_GRUNT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHGrunt :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_GRUNT_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}

	case TASK_GRUNT_MOVE_SHOOTING: // buz
		{
			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw ( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
				if (MovementIsComplete() || HasConditions(bits_COND_ENEMY_DEAD))
				{
					TaskComplete();
					RouteClear();		// Stop moving
					m_Activity = ACT_RESET;
					break;
				}

				if (HasConditions( bits_COND_CAN_RANGE_ATTACK1 ))
					m_IdealActivity = ACT_USE; // special shoot anim (blended)
				else
					m_IdealActivity = ACT_GUARD; // special wait anim (blended)
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CHGrunt :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CHGrunt :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "diversant/gr_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slGruntFail[] =
{
	{
		tlGruntFail,
		ARRAYSIZE ( tlGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slGruntCombatFail[] =
{
	{
		tlGruntCombatFail,
		ARRAYSIZE ( tlGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slGruntVictoryDance[] =
{
	{ 
		tlGruntVictoryDance,
		ARRAYSIZE ( tlGruntVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlGruntEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_GRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
//	{ TASK_GRUNT_MOVE_SHOOTING,	(float)0						}, // buz: TEST TEST TEST
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slGruntEstablishLineOfFire[] =
{
	{ 
		tlGruntEstablishLineOfFire,
		ARRAYSIZE ( tlGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slGruntFoundEnemy[] =
{
	{ 
		tlGruntFoundEnemy,
		ARRAYSIZE ( tlGruntFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slGruntCombatFace[] =
{
	{ 
		tlGruntCombatFace1,
		ARRAYSIZE ( tlGruntCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slGruntSignalSuppress[] =
{
	{ 
		tlGruntSignalSuppress,
		ARRAYSIZE ( tlGruntSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slGruntSuppress[] =
{
	{ 
		tlGruntSuppress,
		ARRAYSIZE ( tlGruntSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slGruntWaitInCover[] =
{
	{ 
		tlGruntWaitInCover,
		ARRAYSIZE ( tlGruntWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_GRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_GRUNT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slGruntTakeCover[] =
{
	{ 
		tlGruntTakeCover1,
		ARRAYSIZE ( tlGruntTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slGruntGrenadeCover[] =
{
	{ 
		tlGruntGrenadeCover1,
		ARRAYSIZE ( tlGruntGrenadeCover1 ), 
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slGruntTossGrenadeCover[] =
{
	{ 
		tlGruntTossGrenadeCover1,
		ARRAYSIZE ( tlGruntTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slGruntTakeCoverFromBestSound[] =
{
	{ 
		tlGruntTakeCoverFromBestSound,
		ARRAYSIZE ( tlGruntTakeCoverFromBestSound ), 
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlGruntHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slGruntHideReload[] = 
{
	{
		tlGruntHideReload,
		ARRAYSIZE ( tlGruntHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlGruntSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slGruntSweep[] =
{
	{ 
		tlGruntSweep,
		ARRAYSIZE ( tlGruntSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
//	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH }, buz
	{ TASK_FACE_ENEMY,			(float)0		}, // buz
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slGruntRangeAttack1A[] =
{
	{ 
		tlGruntRangeAttack1A,
		ARRAYSIZE ( tlGruntRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
//	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  }, buz
	{ TASK_FACE_ENEMY,			(float)0		}, // buz
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slGruntRangeAttack1B[] =
{
	{ 
		tlGruntRangeAttack1B,
		ARRAYSIZE ( tlGruntRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_LIGHT_DAMAGE		| // buz: 1B is interruptable by light damage
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_GRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slGruntRangeAttack2[] =
{
	{ 
		tlGruntRangeAttack2,
		ARRAYSIZE ( tlGruntRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlGruntRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slGruntRepel[] =
{
	{ 
		tlGruntRepel,
		ARRAYSIZE ( tlGruntRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlGruntRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slGruntRepelAttack[] =
{
	{ 
		tlGruntRepelAttack,
		ARRAYSIZE ( tlGruntRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slGruntRepelLand[] =
{
	{ 
		tlGruntRepelLand,
		ARRAYSIZE ( tlGruntRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};

//=========================================================
// buz: duck and wait couple seconds behind the barrel, crate, etc..
//=========================================================
Task_t	tlGruntDuckAndCoverWait[] =
{
	{ TASK_STOP_MOVING,				(float)0			},	
	{ TASK_GRUNT_SPEAK_SENTENCE,	(float)0			},
	{ TASK_SET_ACTIVITY,			(float)ACT_TWITCH	},
	{ TASK_WAIT,					(float)3			}, // randomize a bit?
};

Schedule_t	slGruntDuckAndCoverWait[] =
{
	{ 
		tlGruntDuckAndCoverWait,
		ARRAYSIZE ( tlGruntDuckAndCoverWait ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
	//	bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_CROUCH_NOT_SAFE	| // buz: terminate, if crouching is not safe more
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"DuckAndCover!"
	},
};
/*
// ====================================
// SCHED_GRUNT_RUN_SHOOTING - buz
// ====================================
Task_t	tlGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slGruntRepelLand[] =
{
	{ 
		tlGruntRepelLand,
		ARRAYSIZE ( tlGruntRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};*/

DEFINE_CUSTOM_SCHEDULES( CHGrunt )
{
	slGruntFail,
	slGruntCombatFail,
	slGruntVictoryDance,
	slGruntEstablishLineOfFire,
	slGruntFoundEnemy,
	slGruntCombatFace,
	slGruntSignalSuppress,
	slGruntSuppress,
	slGruntWaitInCover,
	slGruntTakeCover,
	slGruntGrenadeCover,
	slGruntTossGrenadeCover,
	slGruntTakeCoverFromBestSound,
	slGruntHideReload,
	slGruntSweep,
	slGruntRangeAttack1A,
	slGruntRangeAttack1B,
	slGruntRangeAttack2,
	slGruntRepel,
	slGruntRepelAttack,
	slGruntRepelLand,
	slGruntDuckAndCoverWait, // buz
};

IMPLEMENT_CUSTOM_SCHEDULES( CHGrunt, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CHGrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, HGRUNT_9MMAR))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		}
		else
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_shotgun" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_shotgun" );
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & HGRUNT_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		// LRC: added a test to stop a marine without a launcher from firing.
		else if ( pev->weapons & HGRUNT_GRENADELAUNCHER )
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}
		else
		{
			// ALERT( at_debug, "No grenades available. "); // flow into the error message we get at the end...
		}
		break;
	case ACT_RUN:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_run_primary");
				if (iSequence != -1)
					break;
			}
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_walk_primary");
				if (iSequence != -1)
					break;
			}
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_debug, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CHGrunt :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_GRUNT_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_GRUNT_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_GRUNT_REPEL );
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "DV_GREN", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			// buz: this was commented. Why?
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE: // buz: ��������������, ���� ����� ��� � ������� ��������
	case MONSTERSTATE_ALERT:

		if (m_cAmmoLoaded < m_cClipSize / 2)
		{
			return GetScheduleOfType ( SCHED_RELOAD );
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						// ALERT(at_aiconsole,"leader spotted player!\n");
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if (m_hEnemy != NULL)
							{
								if (m_hEnemy->IsPlayer())
									SENTENCEG_PlayRndSz( ENT(pev), "DV_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
								else if ((m_hEnemy->Classify() == CLASS_ALIEN_MILITARY) || 
										(m_hEnemy->Classify() == CLASS_ALIEN_MONSTER) || 
										(m_hEnemy->Classify() == CLASS_ALIEN_PREY) || 
										(m_hEnemy->Classify() == CLASS_ALIEN_PREDATOR))
									SENTENCEG_PlayRndSz( ENT(pev), "DV_MONST", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
							}

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) // buz: reload here, if safe
					return GetScheduleOfType ( SCHED_RELOAD );
				else
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
			/*	// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}*/
				
				int iPercent;

				// buz: 90% to duck and cover, if can
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) && m_hEnemy != NULL )
				{
					iPercent = RANDOM_LONG(0,99);
					if (iPercent <= 90)
						return GetScheduleOfType( SCHED_GRUNT_DUCK_COVER_WAIT ); // wait some time in cover
				}

				// buz: now 50% to try normal way of taking cover
				iPercent = RANDOM_LONG(0,99);
				if ( iPercent <= 50 && m_hEnemy != NULL )
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch

			else if ( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_GRUNT_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "DV_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "DV_TAUNT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CHGrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "DV_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slGruntTossGrenadeCover;
				}
				else
				{
					return &slGruntTakeCover[ 0 ];
				}
			}
			else
			{
				if ( OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) && RANDOM_LONG(0,1) )
				{
					return &slGruntGrenadeCover[ 0 ];
				}
				else
				{
					return &slGruntTakeCover[ 0 ];
				}
			}
		}
	case SCHED_GRUNT_DUCK_COVER_WAIT: // buz
		{
			return &slGruntDuckAndCoverWait[ 0 ]; 
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slGruntTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			// buz: use CheckRangedAttack1's recommendations
			switch (m_iLastFireCheckResult)
			{
			case 0: // fire any
			default:
				if (RANDOM_LONG(0,5) == 0)
					m_fStanding = RANDOM_LONG(0,1);
				break;
			case 1: // only crouched
				m_fStanding = FALSE;
				break;
			case 2: // only standing
				m_fStanding = TRUE;
				break;
			}

/*			if (m_fStanding)
				return &slGruntRangeAttack1B[ 0 ];
			else
				return &slGruntRangeAttack1A[ 0 ];*/
			// buz: 1B is interruptable - use it if grunt can fast take cover when hit
			if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) 
				return &slGruntRangeAttack1B[ 0 ];
			else
				return &slGruntRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slGruntRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slGruntCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slGruntWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slGruntSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slGruntHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slGruntFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slGruntFail[ 0 ];
				}
			}

			return &slGruntVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			// buz: use CheckRangedAttack1's recommendations
			switch (m_iLastFireCheckResult)
			{
			case 0: // fire any
			default:
				if (RANDOM_LONG(0,5) == 0)
					m_fStanding = RANDOM_LONG(0,1);
				break;
			case 1: // only crouched
				m_fStanding = FALSE;
				break;
			case 2: // only standing
				m_fStanding = TRUE;
				break;
			}

			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slGruntSignalSuppress[ 0 ];
			}
			else
			{
				return &slGruntSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slGruntCombatFail[ 0 ];
			}

			return &slGruntFail[ 0 ];
		}
	case SCHED_GRUNT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slGruntRepel[ 0 ];
		}
	case SCHED_GRUNT_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slGruntRepelAttack[ 0 ];
		}
	case SCHED_GRUNT_REPEL_LAND:
		{
			return &slGruntRepelLand[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

class CHGruntRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( monster_grunt_repel, CHGruntRepel );

void CHGruntRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse(&CHGruntRepel:: RepelUse );
}

void CHGruntRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_human_grunt" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHGruntRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity *pEntity = Create( "monster_human_grunt", pev->origin, pev->angles );
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer( );
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink(&CBeam:: SUB_Remove );
	pBeam->SetNextThink( -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5 );

	UTIL_Remove( this );
}



//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadHGrunt : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadHGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadHGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hgrunt_dead, CDeadHGrunt );

//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadHGrunt :: Spawn( void )
{
	int oldBody;

	PRECACHE_MODEL("models/diversant.mdl");
	PRECACHE_MODEL("models/mask.mdl");
	SET_MODEL(ENT(pev), "models/diversant.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_debug, "Dead hgrunt with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	oldBody = pev->body;
	pev->body = 0;

	SetBodygroup( DIVERSANT_HEADSTUFF_GROUP, DIVERSANT_HEADSTUFF_NO );
	SetBodygroup( DIVERSANT_GUN_GROUP, DIVERSANT_GUN_NONE );

	MonsterInitDead();
}

// buz: overriden for grunts to fix model's bugs...
void CHGrunt :: SetEyePosition ( void )
{
	Vector  vecEyePosition;
	void	*pmodel = GET_MODEL_PTR( ENT(pev) );

	GetEyePosition( pmodel, vecEyePosition );

	pev->view_ofs = vecEyePosition;

	if ( pev->view_ofs == g_vecZero )
	{
		pev->view_ofs = Vector(0, 0 ,73);
		// ALERT ( at_aiconsole, "using default view ofs for %s\n", STRING ( pev->classname ) );
	}
}

// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
void CHGrunt :: DeadUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator->IsPlayer())
		return;
	if (FBitSet(pev->weapons, HGRUNT_9MMAR) && pActivator->GiveAmmo(m_iDeadAmmo, "9mm", _9MM_MAX_CARRY) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		m_iDeadAmmo = 0;
		SetUse(NULL);
	}
	else if (FBitSet(pev->weapons, HGRUNT_SHOTGUN) && pActivator->GiveAmmo(m_iDeadAmmo, "buckshot", BUCKSHOT_MAX_CARRY) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		m_iDeadAmmo = 0;
		SetUse(NULL);
	}
}

// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
void CHGrunt :: Killed( entvars_t *pevAttacker, int iGib )
{
	if (gSkillData.maxDeadEnemyAmmo >= 1 && !ShouldGibMonster(iGib))
	{
		m_iDeadAmmo = RANDOM_LONG(1, gSkillData.maxDeadEnemyAmmo);
		SetUse(&CHGrunt::DeadUse);
	}
	CSquadMonster::Killed(pevAttacker, iGib);
}


// ====================== DIVERSANTS WITH GLOCK ====================

#define DIVERSANT2_GUN_GROUP	3
#define DIVERSANT2_GUN_GLOCK	0
#define DIVERSANT2_GUN_NO		1

class CHGruntGlock : public CHGrunt
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void Shoot ( void );
	void SetActivity ( Activity NewActivity );
	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	void EXPORT DeadUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Killed( entvars_t *pevAttacker, int iGib );
};

LINK_ENTITY_TO_CLASS( monster_diversant_pistol, CHGruntGlock );

void CHGruntGlock :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/diversant_pistol.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	// clear all bits except grenades
	if (FBitSet(pev->weapons, HGRUNT_HANDGRENADE))
		pev->weapons = HGRUNT_HANDGRENADE;
	else
		pev->weapons = 0;

	// buz: pev->body is head stuff number
	int headstuff = pev->body;
	pev->body = 0;
	SetBodygroup( DIVERSANT_HEADSTUFF_GROUP, headstuff );

	// hack for studiomodelrenderer to draw goggles
	if (headstuff == DIVERSANT_HEADSTUFF_PNV)
		pev->renderfx = 50;

	m_cClipSize = 14;
	m_cAmmoLoaded = m_cClipSize;	

	CTalkMonster::g_talkWaitTime = 0;

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	m_iDeadAmmo = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGruntGlock :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/diversant_pistol.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	PRECACHE_MODEL("models/mask.mdl"); // buz test

	PRECACHE_SOUND( "weapons/glock_fire.wav" );
	
	PRECACHE_SOUND( "diversant/gr_die1.wav" );
	PRECACHE_SOUND( "diversant/gr_die2.wav" );
	PRECACHE_SOUND( "diversant/gr_die3.wav" );

	PRECACHE_SOUND( "diversant/gr_pain1.wav" );
	PRECACHE_SOUND( "diversant/gr_pain2.wav" );
	PRECACHE_SOUND( "diversant/gr_pain3.wav" );
	PRECACHE_SOUND( "diversant/gr_pain4.wav" );
	PRECACHE_SOUND( "diversant/gr_pain5.wav" );

	PRECACHE_SOUND( "diversant/gr_reload_glock.wav" );

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND( "weapons/sbarrel1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGruntGlock :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
			{
			if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( DIVERSANT2_GUN_GROUP, DIVERSANT2_GUN_NO );

			// now spawn a gun.
			DropItem( "weapon_glock", vecGunPos, vecGunAngles );
		
			if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
			{
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
			}

			}
			break;

		case HGRUNT_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "diversant/gr_reload_glock.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_AE_BURST1:
		{
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (m_cAmmoLoaded > 0)
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/glock_fire.wav", 1, ATTN_NORM );
				Shoot();
			}
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );
			
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		default:
			CHGrunt::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// Shoot
//=========================================================
void CHGruntGlock :: Shoot ( void )
{
//	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
//	{
//		return;
//	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector vecBrassPos, vecBrassDir;
	GetAttachment(3, vecBrassPos, vecBrassDir);
	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_MONSTER_GLOCK );

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}


void CHGruntGlock :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		//iSequence = LookupActivity ( NewActivity );
		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "standing" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouching" );
		}

		break;
	case ACT_RANGE_ATTACK2:
		// get toss anim
		iSequence = LookupSequence( "throwgrenade" );
	
		break;
	case ACT_RUN:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_run_primary");
				if (iSequence != -1)
					break;
			}
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_walk_primary");
				if (iSequence != -1)
					break;
			}
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_debug, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
void CHGruntGlock :: DeadUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator->IsPlayer())
		return;
	if (pActivator->GiveAmmo(m_iDeadAmmo, "barret", _9MM_MAX_CARRY) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		m_iDeadAmmo = 0;
		SetUse(NULL);
	}
}

// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
void CHGruntGlock :: Killed( entvars_t *pevAttacker, int iGib )
{
	if (gSkillData.maxDeadEnemyAmmo >= 1 && !ShouldGibMonster(iGib))
	{
		m_iDeadAmmo = RANDOM_LONG(1, gSkillData.maxDeadEnemyAmmo);
		SetUse(&CHGruntGlock::DeadUse);
	}
	CSquadMonster::Killed(pevAttacker, iGib);
}


// ====================== TERRORISTS =============================

#define TERR_GUN_GROUP					2
#define TERR_GUN_AK						0
#define TERR_GUN_RPK					1
#define TERR_GUN_NONE					2

#define TERR_HEAD_GASMASK	3


class CTerror : public CHGrunt
{
public:
	void Spawn( void );
	void Precache( void );
//	void SetYawSpeed ( void );
	int  Classify ( void ) {return m_iClass?m_iClass:CLASS_TERROR;}
//	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
//	BOOL FCanCheckAttacks ( void );
//	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
//	BOOL CheckRangeAttack2 ( float flDot, float flDist );
//	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
//	void StartTask ( Task_t *pTask );
//	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
//	Vector GetGunPosition( void );
	void Shoot ( void );
//	void Shotgun ( void );
//	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );

//	void ResetSequenceInfo ( );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
//	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

//	int IRelationship ( CBaseEntity *pTarget );

//	BOOL FOkToSpeak( void );
//	void JustSpoke( void );

//	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pTerrorSentences[];

	int		m_iNoGasDamage;

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	void EXPORT DeadUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Killed( entvars_t *pevAttacker, int iGib );
};

LINK_ENTITY_TO_CLASS( monster_human_terror, CTerror );

TYPEDESCRIPTION	CTerror::m_SaveData[] = 
{
	DEFINE_FIELD( CTerror, m_iNoGasDamage, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTerror, CHGrunt );

const char *CTerror::pTerrorSentences[] = 
{
	"TE_GREN", // grenade scared grunt
	"TE_ALERT", // sees player
	"TE_MONSTER", // sees monster
	"TE_COVER", // running to cover
	"TE_THROW", // about to throw grenade
	"TE_CHARGE",  // running out to get the enemy
	"TE_TAUNT", // say rude things
};


//=========================================================
// Spawn
//=========================================================
void CTerror :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/terror.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.terrorHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	int head = pev->body; // body is head number
	pev->body = 0;
	SetBodygroup( HEAD_GROUP, head );

	if (head == TERR_HEAD_GASMASK) m_iNoGasDamage = 1;
	else m_iNoGasDamage = 0;

	if (pev->weapons == 0)
	{
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE; // ak + grens
	}

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )) // rpk
	{
		SetBodygroup( TERR_GUN_GROUP, TERR_GUN_RPK );
		m_cClipSize	= 100; // hehe
	}
	else
	{
		m_cClipSize		= GRUNT_CLIP_SIZE;
	}
	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	m_iDeadAmmo = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTerror :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/terror.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )) // rpk
	{
		PRECACHE_SOUND( "weapons/rpk_fire1.wav" );
		PRECACHE_SOUND( "weapons/rpk_fire2.wav" );
		PRECACHE_SOUND( "weapons/rpk_fire3.wav" );
		PRECACHE_SOUND( "terror/ter_reload_rpk.wav" );
		m_iBrassShell = PRECACHE_MODEL ("models/rpk_shell.mdl");
	}
	else
	{
		PRECACHE_SOUND( "weapons/ak74_fire1.wav" );
		PRECACHE_SOUND( "weapons/ak74_fire2.wav" );
		PRECACHE_SOUND( "terror/ter_reload_ak.wav" );
		m_iBrassShell = PRECACHE_MODEL ("models/ak74_shell.mdl");
	}
	
	PRECACHE_SOUND( "terror/ter_die1.wav" );
	PRECACHE_SOUND( "terror/ter_die2.wav" );
	PRECACHE_SOUND( "terror/ter_die3.wav" );

	PRECACHE_SOUND( "terror/ter_pain1.wav" );
	PRECACHE_SOUND( "terror/ter_pain2.wav" );
	PRECACHE_SOUND( "terror/ter_pain3.wav" );
	PRECACHE_SOUND( "terror/ter_pain4.wav" );
	PRECACHE_SOUND( "terror/ter_pain5.wav" );	

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;
}

// buz - overriden because terrorist with RPK cannot fire crouched
BOOL CTerror :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients who are close enough, but don't shoot at them.
			// ALERT(at_aiconsole, "== too close\n");
			return FALSE;
		}

		BOOL savedStanding = m_fStanding;
		m_fStanding = FALSE; // buz: check chrouched fire first
		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
		if ( tr.flFraction == 1.0 && !pev->gaitsequence && !(FBitSet( pev->weapons, HGRUNT_SHOTGUN )))
		{
			// buz: we can fire crouched, now check for standing
			m_fStanding = TRUE;
			vecSrc = GetGunPosition();
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
			m_fStanding = savedStanding;
			if ( tr.flFraction == 1.0 )
			{
				// ALERT(at_aiconsole, "== shoot any\n");
				m_iLastFireCheckResult = 0; // shoot as you wish
			}
			else
			{
				// ALERT(at_aiconsole, "== shoot crouched\n");
				m_iLastFireCheckResult = 1; // only chrouched
			}

			return TRUE;
		}
		else
		{
			// buz: cant fire crouching, maybe me or enemy in some kind of cover (or running). Check standing.
			m_fStanding = TRUE;
			vecSrc = GetGunPosition();
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);
			
		//	BuzTestDrawLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), 255, 255, 0 );

			m_fStanding = savedStanding;
			if ( tr.flFraction == 1.0 )
			{
				// ALERT(at_aiconsole, "== shoot standing\n");
				m_iLastFireCheckResult = 2; // buz: standing is our only one choice
				return TRUE;
			}
			else
			{
				// ALERT(at_aiconsole, "== cant fire\n");
				m_iLastFireCheckResult = 0;
				return FALSE; // cant fire				
			}
		}
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTerror :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
			{
			if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( TERR_GUN_GROUP, TERR_GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
				 DropItem( "weapon_rpk", vecGunPos, vecGunAngles );
			else
				 DropItem( "weapon_ak74", vecGunPos, vecGunAngles );

			if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );

			}
			break;

		case HGRUNT_AE_RELOAD:
			if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "terror/ter_reload_rpk.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "terror/ter_reload_ak.wav", 1, ATTN_NORM );

			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_AE_BURST1:
			// Shoot() will play sound		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

		case HGRUNT_AE_BURST2:
		case HGRUNT_AE_BURST3:
			Shoot();
			break;

		case HGRUNT_AE_CAUGHT_ENEMY:
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "TE_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		default:
			CHGrunt::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// SetActivity 
//=========================================================
void CTerror :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, HGRUNT_9MMAR))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		}
		else
		{
			// buz: no crouched shoot animation for RPK
			iSequence = LookupSequence( "machinegun_fire" );
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & HGRUNT_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		// LRC: added a test to stop a marine without a launcher from firing.
		else if ( pev->weapons & HGRUNT_GRENADELAUNCHER )
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}
		else
		{
			// ALERT( at_debug, "No grenades available. "); // flow into the error message we get at the end...
		}
		break;
	case ACT_RUN:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_run_primary");
				if (iSequence != -1)
					break;
			}
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			// buz: get combat movement animation in combat state
		//	if (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT)
			if (m_iUseAlertAnims)
			{
				iSequence = LookupSequence("combat_walk_primary");
				if (iSequence != -1)
					break;
			}
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_debug, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}


//=========================================================
// PainSound
//=========================================================
void CTerror :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CTerror :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "terror/ter_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// Shoot
//=========================================================
void CTerror :: Shoot ( void )
{
//	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
//	{
//		return;
//	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		Vector vecBrassPos, vecBrassDir;
		GetAttachment(3, vecBrassPos, vecBrassDir);
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecBrassPos, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

		if (FBitSet( pev->weapons, HGRUNT_9MMAR))
		{
			switch(RANDOM_LONG(0,1))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/ak74_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/ak74_fire2.wav", 1, ATTN_NORM ); break;
			}
			FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_7DEGREES, 2048, BULLET_TERROR_AK );
		}
		else
		{
			switch(RANDOM_LONG(0,2))
			{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/rpk_fire1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/rpk_fire2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/rpk_fire3.wav", 1, ATTN_NORM ); break;
			}
			FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_TERROR_RPK );
		}

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	//	ALERT(at_console, "grunt ammo has %d\n", m_cAmmoLoaded);
	}
	else
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}


void CTerror :: SpeakSentence( void )
{
	if ( m_iSentence == HGRUNT_SENT_NONE )
		return; 

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pTerrorSentences[ m_iSentence ], HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}



//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CTerror :: GetSchedule( void )
{
	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
		return CHGrunt::GetSchedule();

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "TE_GREN", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				// ALERT(at_aiconsole, "COVER FROM SOUND\n");
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );				
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE: // buz: ��������������, ���� ����� ��� � ������� ��������
	case MONSTERSTATE_ALERT:

		if (m_cAmmoLoaded < m_cClipSize / 2)
		{
			return GetScheduleOfType ( SCHED_RELOAD );
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						// ALERT(at_aiconsole, "COVER FROM ENEMY (SQUAD)\n");
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						// ALERT(at_aiconsole,"leader spotted player!\n");

						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if (m_hEnemy != NULL)
							{
								if (m_hEnemy->IsPlayer())
									SENTENCEG_PlayRndSz( ENT(pev), "TE_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
								else if ((m_hEnemy->Classify() == CLASS_ALIEN_MILITARY) || 
										(m_hEnemy->Classify() == CLASS_ALIEN_MONSTER) || 
										(m_hEnemy->Classify() == CLASS_ALIEN_PREY) || 
										(m_hEnemy->Classify() == CLASS_ALIEN_PREDATOR))
									SENTENCEG_PlayRndSz( ENT(pev), "TE_MONST", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
							}

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) // buz: reload here, if safe
					return GetScheduleOfType ( SCHED_RELOAD );
				else
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				int iPercent;

				// buz: 90% to duck and cover, if can
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) && m_hEnemy != NULL )
				{
//					ALERT(at_console, "TERROR HIDES\n");
					iPercent = RANDOM_LONG(0,99);
					if (iPercent <= 90)
						return GetScheduleOfType( SCHED_GRUNT_DUCK_COVER_WAIT ); // wait some time in cover
				}

				// buz: now 50% to try normal way of taking cover
				iPercent = RANDOM_LONG(0,99);
				if ( iPercent <= 50 && m_hEnemy != NULL )
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_COVER;
						//JustSpoke();
					}
//					ALERT(at_console, "TERROR TAKES COVER\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
//					ALERT(at_console, "TERROR FLINCHES\n");
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch

			else if ( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_GRUNT_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					// ALERT(at_aiconsole, "COVER FROM ENEMY (HIDE)\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "TE_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "TE_TAUNT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CTerror :: GetScheduleOfType ( int Type ) 
{
	if (Type == SCHED_TAKE_COVER_FROM_ENEMY)
	{
		if ( InSquad() )
		{
			if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "TE_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return slGruntTossGrenadeCover;
			}
			else
			{
				return &slGruntTakeCover[ 0 ];
			}
		}
		else
		{
			if ( OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) && RANDOM_LONG(0,1) )
			{
				return &slGruntGrenadeCover[ 0 ];
			}
			else
			{
				return &slGruntTakeCover[ 0 ];
			}
		}
	}
	else
		return CHGrunt::GetScheduleOfType(Type);
}

void CTerror :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// terrorists doesnt have helmets
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CTerror :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// buz: refuse gas damage while wearing gasmask
	if (m_iNoGasDamage && ( bitsDamageType & DMG_NERVEGAS ))
		return 0;

	Forget( bits_MEMORY_INCOVER );
	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CTerror :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(TERR_GUN_GROUP) != TERR_GUN_NONE && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{// throw a gun if the grunt has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
		{
			pGun = DropItem( "weapon_rpk", vecGunPos, vecGunAngles );
		}
		else
		{
			pGun = DropItem( "weapon_ak74", vecGunPos, vecGunAngles );
		}
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	
		if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
		{
			pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseMonster :: GibMonster();
}


void CTerror :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fTerrorQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fTerrorQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "TE_CHECK", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTerrorQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "TE_QUEST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTerrorQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "TE_IDLE", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fTerrorQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "TE_CLEAR", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "TE_ANSWER", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fTerrorQuestion = 0;
		}
		JustSpoke();
	}
}

// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
void CTerror :: DeadUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator->IsPlayer())
		return;
	if (FBitSet(pev->weapons, HGRUNT_9MMAR) && pActivator->GiveAmmo(m_iDeadAmmo, "ak", AK_MAX_CARRY) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		m_iDeadAmmo = 0;
		SetUse(NULL);
	}
	else if (FBitSet(pev->weapons, HGRUNT_SHOTGUN) && pActivator->GiveAmmo(m_iDeadAmmo, "rpk", RPK_MAX_CARRY) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		m_iDeadAmmo = 0;
		SetUse(NULL);
	}
}

// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
void CTerror :: Killed( entvars_t *pevAttacker, int iGib )
{
	if (gSkillData.maxDeadEnemyAmmo >= 1 && !ShouldGibMonster(iGib))
	{
		m_iDeadAmmo = RANDOM_LONG(1, gSkillData.maxDeadEnemyAmmo);
		SetUse(&CTerror::DeadUse);
	}
	CSquadMonster::Killed(pevAttacker, iGib);
}


// ======================== CLONES ==============================

class CClone : public CTerror
{
public:
	void Spawn( void );
	void Precache( void );

	int  Classify ( void ) {return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;}

	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );

	void SpeakSentence( void );

//	int	Save( CSave &save ); 
//	int Restore( CRestore &restore );
	
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );

//	static TYPEDESCRIPTION m_SaveData[];

	static const char *pCloneSentences[];
};

LINK_ENTITY_TO_CLASS( monster_soldier_clone, CClone );

/*TYPEDESCRIPTION	CClone::m_SaveData[] = 
{
	DEFINE_FIELD( CClone, m_iNoGasDamage, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CClone, CTerror );*/

const char *CClone::pCloneSentences[] = 
{
	"CL_GREN", // grenade scared grunt
	"CL_ALERT", // sees player
	"CL_MONSTER", // sees monster
	"CL_COVER", // running to cover
	"CL_THROW", // about to throw grenade
	"CL_CHARGE",  // running out to get the enemy
	"CL_TAUNT", // say rude things
};


//=========================================================
// Spawn
//=========================================================
void CClone :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/soldier_clon.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.cloneHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	int head = pev->body; // body is head number
	pev->body = 0;
	SetBodygroup( HEAD_GROUP, head );

	if (pev->weapons == 0)
	{
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE; // ak + grens
	}

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )) // rpk
	{
		SetBodygroup( TERR_GUN_GROUP, TERR_GUN_RPK );
		m_cClipSize	= 100; // hehe
	}
	else
	{
		m_cClipSize		= GRUNT_CLIP_SIZE;
	}
	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;
	m_iNoGasDamage = 0;

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	m_iDeadAmmo = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CClone :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/soldier_clon.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )) // rpk
	{
		PRECACHE_SOUND( "weapons/rpk_fire1.wav" );
		PRECACHE_SOUND( "weapons/rpk_fire2.wav" );
		PRECACHE_SOUND( "weapons/rpk_fire3.wav" );
		PRECACHE_SOUND( "clone/cl_reload_rpk.wav" );
		m_iBrassShell = PRECACHE_MODEL ("models/rpk_shell.mdl");
	}
	else
	{
		PRECACHE_SOUND( "weapons/ak74_fire1.wav" );
		PRECACHE_SOUND( "weapons/ak74_fire2.wav" );
		PRECACHE_SOUND( "clone/cl_reload_ak.wav" );
		m_iBrassShell = PRECACHE_MODEL ("models/ak74_shell.mdl");
	}
	
	PRECACHE_SOUND( "clone/cl_die1.wav" );
	PRECACHE_SOUND( "clone/cl_die2.wav" );
	PRECACHE_SOUND( "clone/cl_die3.wav" );

	PRECACHE_SOUND( "clone/cl_pain1.wav" );
	PRECACHE_SOUND( "clone/cl_pain2.wav" );
	PRECACHE_SOUND( "clone/cl_pain3.wav" );
	PRECACHE_SOUND( "clone/cl_pain4.wav" );
	PRECACHE_SOUND( "clone/cl_pain5.wav" );	

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CClone :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
			{
			if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( TERR_GUN_GROUP, TERR_GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
				 DropItem( "weapon_rpk", vecGunPos, vecGunAngles );
			else
				 DropItem( "weapon_ak74", vecGunPos, vecGunAngles );

			if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );

			}
			break;

		case HGRUNT_AE_RELOAD:
			if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "clone/cl_reload_rpk.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "clone/cl_reload_ak.wav", 1, ATTN_NORM );

			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HGRUNT_AE_BURST1:
			// Shoot() will play sound		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

		case HGRUNT_AE_BURST2:
		case HGRUNT_AE_BURST3:
			Shoot();
			break;

		case HGRUNT_AE_CAUGHT_ENEMY:
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "CL_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		default:
			CTerror::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// PainSound
//=========================================================
void CClone :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CClone :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "clone/cl_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}


//=========================================================
// IdleSound 
//=========================================================
void CClone :: IdleSound( void )
{
	// no idle sounds for clones!

/*	if (FOkToSpeak() && (g_fTerrorQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fTerrorQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "CL_CHECK", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTerrorQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "CL_QUEST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTerrorQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "CL_IDLE", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fTerrorQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "CL_CLEAR", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "CL_ANSWER", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fTerrorQuestion = 0;
		}
		JustSpoke();
	}*/
}

void CClone :: SpeakSentence( void )
{
	if ( m_iSentence == HGRUNT_SENT_NONE )
		return; 

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pCloneSentences[ m_iSentence ], HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}



//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CClone :: GetSchedule( void )
{
	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
		return CTerror::GetSchedule();

	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "CL_GREN", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				// ALERT(at_aiconsole, "COVER FROM SOUND\n");
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );				
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE: // buz: ��������������, ���� ����� ��� � ������� ��������
	case MONSTERSTATE_ALERT:

		if (m_cAmmoLoaded < m_cClipSize / 2)
		{
			return GetScheduleOfType ( SCHED_RELOAD );
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						// ALERT(at_aiconsole, "COVER FROM ENEMY (SQUAD)\n");
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						// ALERT(at_aiconsole,"leader spotted player!\n");

						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if (m_hEnemy != NULL)
							{
							//	if (m_hEnemy->IsPlayer())
									SENTENCEG_PlayRndSz( ENT(pev), "CL_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
							}

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) ) // buz: reload here, if safe
					return GetScheduleOfType ( SCHED_RELOAD );
				else
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				int iPercent;

				// buz: 90% to duck and cover, if can
				if ((m_afCapability & bits_CAP_CROUCH_COVER) && !HasConditions(bits_COND_CROUCH_NOT_SAFE) && m_hEnemy != NULL )
				{
//					ALERT(at_console, "TERROR HIDES\n");
					iPercent = RANDOM_LONG(0,99);
					if (iPercent <= 90)
						return GetScheduleOfType( SCHED_GRUNT_DUCK_COVER_WAIT ); // wait some time in cover
				}

				// buz: now 50% to try normal way of taking cover
				iPercent = RANDOM_LONG(0,99);
				if ( iPercent <= 50 && m_hEnemy != NULL )
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_COVER;
						//JustSpoke();
					}
//					ALERT(at_console, "TERROR TAKES COVER\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
//					ALERT(at_console, "TERROR FLINCHES\n");
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch

			else if ( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_GRUNT_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					// ALERT(at_aiconsole, "COVER FROM ENEMY (HIDE)\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "CL_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "CL_TAUNT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CClone :: GetScheduleOfType ( int Type ) 
{
	if (Type == SCHED_TAKE_COVER_FROM_ENEMY)
	{
		if ( InSquad() )
		{
			if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "CL_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return slGruntTossGrenadeCover;
			}
			else
			{
				return &slGruntTakeCover[ 0 ];
			}
		}
		else
		{
			if ( OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) && RANDOM_LONG(0,1) )
			{
				return &slGruntGrenadeCover[ 0 ];
			}
			else
			{
				return &slGruntTakeCover[ 0 ];
			}
		}
	}
	else
		return CHGrunt::GetScheduleOfType(Type);
}






class CCloneHeavy : public CClone
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( monster_soldier_clone_heavy, CCloneHeavy );

//=========================================================
// Spawn
//=========================================================
void CCloneHeavy :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/soldier_clon_heavy.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.cloneHealthHeavy;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	int head = pev->body; // body is head number
	pev->body = 0;
	SetBodygroup( HEAD_GROUP, head );

	if (pev->weapons == 0)
	{
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE; // ak + grens
	}

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )) // rpk
	{
		SetBodygroup( TERR_GUN_GROUP, TERR_GUN_RPK );
		m_cClipSize	= 100; // hehe
	}
	else
	{
		m_cClipSize		= GRUNT_CLIP_SIZE;
	}
	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;
	m_iNoGasDamage = 0;

	// Wargon: ����������� ��������� ������� ���� �� ������� ������. (1.1)
	m_iDeadAmmo = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCloneHeavy :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/soldier_clon_heavy.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN )) // rpk
	{
		PRECACHE_SOUND( "weapons/rpk_fire1.wav" );
		PRECACHE_SOUND( "weapons/rpk_fire2.wav" );
		PRECACHE_SOUND( "weapons/rpk_fire3.wav" );
		PRECACHE_SOUND( "clone/cl_reload_rpk.wav" );
		m_iBrassShell = PRECACHE_MODEL ("models/rpk_shell.mdl");
	}
	else
	{
		PRECACHE_SOUND( "weapons/ak74_fire1.wav" );
		PRECACHE_SOUND( "weapons/ak74_fire2.wav" );
		PRECACHE_SOUND( "clone/cl_reload_ak.wav" );
		m_iBrassShell = PRECACHE_MODEL ("models/ak74_shell.mdl");
	}
	
	PRECACHE_SOUND( "clone/cl_die1.wav" );
	PRECACHE_SOUND( "clone/cl_die2.wav" );
	PRECACHE_SOUND( "clone/cl_die3.wav" );

	PRECACHE_SOUND( "clone/cl_pain1.wav" );
	PRECACHE_SOUND( "clone/cl_pain2.wav" );
	PRECACHE_SOUND( "clone/cl_pain3.wav" );
	PRECACHE_SOUND( "clone/cl_pain4.wav" );
	PRECACHE_SOUND( "clone/cl_pain5.wav" );	

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;
}
