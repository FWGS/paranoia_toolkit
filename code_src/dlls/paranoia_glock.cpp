
/*******************************************************
*	weapon_aps class
*
*	(Автоматический пистолет Стечкина)
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

enum glock_e
{
	GLOCK_IDLE_A = 0,
	GLOCK_RELOAD_A,
	GLOCK_DRAW,
	GLOCK_SHOOT1_A,
	GLOCK_SHOOT2_A,
	GLOCK_SHOOTLAST_A,
	GLOCK_IDLE_B,
	GLOCK_SHOOT_B,
	GLOCK_SHOOTLAST_B,
	GLOCK_CHANGETO_B,
	GLOCK_CHANGETO_A,	
	GLOCK_RELOAD_B,
};

class CGlock : public CBaseToggleWeapon
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		iItemSlot( void ) { return 2; }
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
	unsigned short m_usGlock;
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock );

void CGlock::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun");
	Precache( );
	SET_MODEL(ENT(pev), "models/w_glock.mdl");
	m_iId = WEAPON_GLOCK;

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();
}

void CGlock::Precache( void )
{
	PRECACHE_MODEL("models/v_glock.mdl");
	PRECACHE_MODEL("models/w_glock.mdl");
	PRECACHE_MODEL("models/p_glock.mdl");

	m_iShell = PRECACHE_MODEL ("models/glock_shell.mdl");

	PRECACHE_SOUND ("weapons/glock_fire.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usGlock = PRECACHE_EVENT( 1, "events/glock1.sc" );

	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "barret";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CGlock::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGlock::Deploy()
{
	InitToggling();
	return DefaultDeploy( "models/v_glock.mdl", "models/p_glock.mdl", GLOCK_DRAW, "onehanded" );
}


void CGlock::Attack1()
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
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	float spread = ExpandSpread( m_pMySpread->pri_expand );
	EqualizeSpread( &spread, m_pMySpread->pri_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->pri_minspread, m_pMySpread->pri_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_9MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int iAnim = m_iClip ? GLOCK_SHOOT1_A : GLOCK_SHOOTLAST_A;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, iAnim, (int)(spread * 255), 0, 0 );

	DefPrimPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGlock::Attack2()
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
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	float spread = ExpandSpread( m_pMySpread->sec_expand );
	EqualizeSpread( &spread, m_pMySpread->sec_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->sec_minspread, m_pMySpread->sec_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_9MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int iAnim = m_iClip ? GLOCK_SHOOT_B : GLOCK_SHOOTLAST_B;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, iAnim, (int)(spread * 255), 0, 0 );

	DefSecPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGlock::Reload1()
{
	if ( m_pPlayer->ammo_barret <= 0 )
		return;

	DefaultReload( GLOCK_MAX_CLIP, GLOCK_RELOAD_A, 2.3 );
}

void CGlock::Reload2()
{
	if ( m_pPlayer->ammo_barret <= 0 )
		return;

	DefaultReload( GLOCK_MAX_CLIP, GLOCK_RELOAD_B, 2.3 );
}

void CGlock::Idle1( void )
{
	SendWeaponAnim( GLOCK_IDLE_A );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

void CGlock::Idle2( void )
{
	SendWeaponAnim( GLOCK_IDLE_B );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

int CGlock::ChangeModeTo1()
{
	SendWeaponAnim( GLOCK_CHANGETO_A );
	WeaponDelay(GLOCK_CHANGETO_A);
	return 1;
}

int CGlock::ChangeModeTo2()
{
	SendWeaponAnim( GLOCK_CHANGETO_B );
	WeaponDelay(GLOCK_CHANGETO_B);
	return 1;
}







/**************************** Ammoboxes and ammoclips *********************/


class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_glockammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_glockammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "barret", _9MM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_glock, CGlockAmmo );


class CGlockBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_glockammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_glockammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_CHAINBOX_GIVE, "barret", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_glockbox, CGlockBox );












