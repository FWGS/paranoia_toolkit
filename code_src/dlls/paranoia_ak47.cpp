
/*******************************************************
*	weapon_ak47 class
*
*	written by BUzer for Half-Life:Paranoia modification
*******************************************************/

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

enum ak47_e
{
	AK47_IDLE_A = 0,
	AK47_RELOAD_A,
	AK47_DRAW,
	AK47_SHOOT_A,
	AK47_IDLE_B,
	AK47_CHANGETO_B,
	AK47_CHANGETO_A,
	AK47_SHOOT_B,
	AK47_RELOAD_B,
};

class CAK47 : public CBaseToggleWeapon
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		iItemSlot( void ) { return 3; }
	int		GetItemInfo(ItemInfo *p);
	int		AddToPlayer( CBasePlayer *pPlayer );
	BOOL	Deploy( void );

	void Attack1( void );
	void Attack2( void );
	void Reload1( void );
	void Reload2( void );
	void Idle1( void );
	void Idle2( void );
	int ChangeModeTo1( void );
	int ChangeModeTo2( void );
//	Vector GetSpreadVec1( void );
//	Vector GetSpreadVec2( void );

	int m_iShell;

private:
	unsigned short m_usAK47;
};

LINK_ENTITY_TO_CLASS( weapon_ak74, CAK47 );


void CAK47::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_ak74");
	Precache( );
	SET_MODEL(ENT(pev), "models/w_ak74.mdl");
	m_iId = WEAPON_AK47;

	m_iDefaultAmmo = AK47_DEFAULT_GIVE;

	FallInit();
}

void CAK47::Precache( void )
{
	PRECACHE_MODEL("models/v_ak74.mdl");
	PRECACHE_MODEL("models/w_ak74.mdl");
	PRECACHE_MODEL("models/p_ak74.mdl");

	m_iShell = PRECACHE_MODEL ("models/ak74_shell.mdl");

	PRECACHE_SOUND ("weapons/ak74_fire1.wav");
	PRECACHE_SOUND ("weapons/ak74_fire2.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usAK47 = PRECACHE_EVENT( 1, "events/ak74.sc" );

	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ak";
	p->iMaxAmmo1 = AK47_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AK47_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

int CAK47::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAK47::Deploy()
{
	InitToggling();
	return DefaultDeploy( "models/v_ak74.mdl", "models/p_ak74.mdl", AK47_DRAW, "mp5" );
}


void CAK47::Attack1()
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
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_AK47, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usAK47, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, AK47_SHOOT_A, (int)(spread * 255), 0, 0 );

	DefPrimPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CAK47::Attack2()
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

	float spread = ExpandSpread( m_pMySpread->sec_expand );
	EqualizeSpread( &spread, m_pMySpread->sec_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->sec_minspread, m_pMySpread->sec_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_AK47, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usAK47, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, AK47_SHOOT_B, (int)(spread * 255), 0, 0 );

	DefSecPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CAK47::Reload1()
{
	if ( m_pPlayer->ammo_ak <= 0 )
		return;

	DefaultReload( AK47_MAX_CLIP, AK47_RELOAD_A, 2.3 );
}

void CAK47::Reload2()
{
	if ( m_pPlayer->ammo_ak <= 0 )
		return;

	DefaultReload( AK47_MAX_CLIP, AK47_RELOAD_B, 2.3 );
}

void CAK47::Idle1( void )
{
	SendWeaponAnim( AK47_IDLE_A );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

void CAK47::Idle2( void )
{
	SendWeaponAnim( AK47_IDLE_B );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

int CAK47::ChangeModeTo1()
{
	SendWeaponAnim( AK47_CHANGETO_A );
	WeaponDelay(AK47_CHANGETO_A);
	return 1;
}

int CAK47::ChangeModeTo2()
{
	SendWeaponAnim( AK47_CHANGETO_B );
	WeaponDelay(AK47_CHANGETO_B);
	return 1;
}







/**************************** Ammoboxes and ammoclips *********************/

class CAK74AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ak74ammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ak74ammo.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 30, "ak", AK_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_ak74, CAK74AmmoClip );

class CAK74AmmoBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ak74ammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ak74ammobox.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 90, "ak", AK_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_ak74box, CAK74AmmoBox );
