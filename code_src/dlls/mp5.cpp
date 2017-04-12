/***

	Written by BUzer for half-life: Paranoia modification
	based on valve's mp5 code

***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#include "paranoia_wpn.h"
/*
enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};*/
enum mp5_e
{
	MP5_IDLE1,
	MP5_IDLE2,
	MP5_DRAW1,
	MP5_DRAW2,
	MP5_FIRE,
	MP5_GRENADE,
	MP5_LASTGRENADE,
	MP5_LOADGRENADE,	// when player picks up his first grenades
	MP5_RELOAD,
};

// buz: class declaration moved here
class CMP5 : public CBaseToggleWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	// Wargon: Фикс невозможности использовать MP5, если у него остались только подствольные гранаты.
	BOOL IsUseable( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int SecondaryAmmoIndex( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle();
	void Idle1( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

//	float m_flNextAnimTime;
	int m_iShell;
	float m_flLoadGrenadeTime;
	int m_iGrenadeLoaded;

private:
	unsigned short m_usMP5;
//	unsigned short m_usMP52;
};


LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 );
LINK_ENTITY_TO_CLASS( weapon_9mmAR, CMP5 );

TYPEDESCRIPTION	CMP5::m_SaveData[] = 
{
	DEFINE_FIELD( CMP5, m_flLoadGrenadeTime, FIELD_TIME ),
	DEFINE_FIELD( CMP5, m_iGrenadeLoaded, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CMP5, CBaseToggleWeapon );

//=========================================================
//=========================================================
int CMP5::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CMP5::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;
	m_iGrenadeLoaded = 1;

	FallInit();// get ready to fall down.
}


void CMP5::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/mp5_shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/hks1.wav");// H to the K
	PRECACHE_SOUND ("weapons/hks2.wav");// H to the K
	PRECACHE_SOUND ("weapons/hks3.wav");// H to the K

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMP5 = PRECACHE_EVENT( 1, "events/mp5.sc" );
//	m_usMP52 = PRECACHE_EVENT( 1, "events/mp52.sc" );

	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CMP5::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

// Wargon: Фикс невозможности использовать MP5, если у него остались только подствольные гранаты.
BOOL CMP5::IsUseable( void )
{
	if (m_iClip <= 0 && (m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 && iMaxAmmo1() != -1) && (m_pPlayer->m_rgAmmo[ SecondaryAmmoIndex() ] <= 0 && iMaxAmmo2() != -1))
	{
		return FALSE;
	}
	return TRUE;
}

int CMP5::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

float GetSequenceTime( void *pmodel, int seqnum ); // buz

BOOL CMP5::Deploy( )
{
	InitToggling();

	int iAnim = RANDOM_LONG(0, 1) ? MP5_DRAW1 : MP5_DRAW2;
	m_flLoadGrenadeTime = UTIL_WeaponTimeBase() + GetSequenceTime(GET_MODEL_PTR(ENT(pev)), iAnim ) + 0.1;
	return DefaultDeploy( "models/v_9mmAR.mdl", "models/p_9mmAR.mdl", iAnim, "mp5" );
}


void CMP5::PrimaryAttack()
{
	// don't fire underwater
	if ((m_iClip <= 0) || (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD))
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iClip--;
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	float spread = ExpandSpread( m_pMySpread->pri_expand );
	EqualizeSpread( &spread, m_pMySpread->pri_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->pri_minspread, m_pMySpread->pri_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMP5, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, MP5_FIRE, (int)(spread * 255), 0, FALSE );

	DefPrimPunch();

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_flLoadGrenadeTime = UTIL_WeaponTimeBase() + GetSequenceTime(GET_MODEL_PTR(ENT(pev)), MP5_FIRE ) + 0.1;
}



void CMP5::SecondaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0 || !m_iGrenadeLoaded)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;
			
	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16, 
							gpGlobals->v_forward * 800 );

	// buz: play special animation when last grenade fired
	int iAnim = MP5_GRENADE;
	if (!m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
	{
		iAnim = MP5_LASTGRENADE;
		m_iGrenadeLoaded = 0;
		m_flLoadGrenadeTime = UTIL_WeaponTimeBase() + GetSequenceTime(GET_MODEL_PTR(ENT(pev)), iAnim ) + 0.1;
	}

//	PLAYBACK_EVENT( 0, m_pPlayer->edict(), m_usMP52 );
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMP5, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, iAnim, 128, 0, TRUE );

	DefSecPunch();
	
//	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
//	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
//	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;// idle pretty soon after shooting.
	WeaponDelay(iAnim);
}

void CMP5::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	if (DefaultReload( MP5_MAX_CLIP, MP5_RELOAD, 1.5 ))
		m_flLoadGrenadeTime = UTIL_WeaponTimeBase() + GetSequenceTime(GET_MODEL_PTR(ENT(pev)), MP5_RELOAD ) + 0.1;
}

void CMP5::WeaponIdle()
{
	CBaseToggleWeapon::WeaponIdle(); // launch anims, set speed, jump etc
	
	if (m_flLoadGrenadeTime < UTIL_WeaponTimeBase() && !m_iGrenadeLoaded && m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
	{
		SendWeaponAnim(MP5_LOADGRENADE);
		WeaponDelay(MP5_LOADGRENADE);
		m_iGrenadeLoaded = 1;
	}
}

void CMP5::Idle1( void )
{
	SendWeaponAnim( RANDOM_LONG(0, 1) ? MP5_IDLE1 : MP5_IDLE2 );
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
//		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		SET_MODEL(ENT(pev), "models/w_mp5ammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
	//	PRECACHE_MODEL ("models/w_9mmARclip.mdl");
		PRECACHE_MODEL ("models/w_mp5ammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_MP5CLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5, CMP5AmmoClip );
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5AmmoClip );



class CMP5Chainammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
	//	SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		SET_MODEL(ENT(pev), "models/w_mp5ammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
	//	PRECACHE_MODEL ("models/w_chainammo.mdl");
		PRECACHE_MODEL ("models/w_mp5ammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_CHAINBOX_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_9mmbox, CMP5Chainammo );
LINK_ENTITY_TO_CLASS( ammo_mp5box, CMP5Chainammo );


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade );
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade );


















