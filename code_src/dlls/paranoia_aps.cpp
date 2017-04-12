
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

enum aps_e
{
	APS_IDLE_A = 0,
	APS_RELOAD_A,
	APS_DRAW,
	APS_SHOOT1_A,
	APS_SHOOT2_A,
	APS_SHOOTLAST_A,
	APS_IDLE_B,
	APS_SHOOT_B,
	APS_SHOOTLAST_B,
	APS_CHANGETO_B,
	APS_CHANGETO_A,	
	APS_RELOAD_B,
};

/*enum aps_e
{
	APS_IDLE_A = 0,
	APS_IDLE_B,
	APS_SHOOT1_A,
	APS_SHOOT2_A,
	APS_SHOOT_B,
	APS_SHOOTLAST_A,
	APS_SHOOTLAST_B,
	APS_RELOAD_A,
	APS_RELOAD_B,	
	APS_DRAW,	
	APS_CHANGETO_B,
	APS_CHANGETO_A,		
};*/

class CAPS : public CBaseToggleWeapon
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
	unsigned short m_usAPS;
};

LINK_ENTITY_TO_CLASS( weapon_aps, CAPS );


void CAPS::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_aps");
	Precache( );
	SET_MODEL(ENT(pev), "models/w_aps.mdl");
	m_iId = WEAPON_APS;

	m_iDefaultAmmo = APS_DEFAULT_GIVE;

	FallInit();
}

void CAPS::Precache( void )
{
	PRECACHE_MODEL("models/v_aps.mdl");
	PRECACHE_MODEL("models/w_aps.mdl");
	PRECACHE_MODEL("models/p_aps.mdl");

	m_iShell = PRECACHE_MODEL ("models/aps_shell.mdl");

	PRECACHE_SOUND ("weapons/aps_fire.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usAPS = PRECACHE_EVENT( 1, "events/aps.sc" );

	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CAPS::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "aps";
	p->iMaxAmmo1 = APS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = APS_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_APS;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CAPS::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAPS::Deploy()
{
	InitToggling();
	return DefaultDeploy( "models/v_aps.mdl", "models/p_aps.mdl", APS_DRAW, "onehanded" );
}


void CAPS::Attack1()
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
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_APS, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int iAnim = m_iClip ? APS_SHOOT1_A : APS_SHOOTLAST_A;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usAPS, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, iAnim, (int)(spread * 255), 0, 0 );

	DefPrimPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CAPS::Attack2()
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
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_APS, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int iAnim = m_iClip ? APS_SHOOT_B : APS_SHOOTLAST_B;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usAPS, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, iAnim, (int)(spread * 255), 0, 0 );

	DefSecPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CAPS::Reload1()
{
	if ( m_pPlayer->ammo_aps <= 0 )
		return;

	DefaultReload( APS_MAX_CLIP, APS_RELOAD_A, 2.3 );
}

void CAPS::Reload2()
{
	if ( m_pPlayer->ammo_aps <= 0 )
		return;

	DefaultReload( APS_MAX_CLIP, APS_RELOAD_B, 2.3 );
}

void CAPS::Idle1( void )
{
	SendWeaponAnim( APS_IDLE_A );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

void CAPS::Idle2( void )
{
	SendWeaponAnim( APS_IDLE_B );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

int CAPS::ChangeModeTo1()
{
	SendWeaponAnim( APS_CHANGETO_A );
	WeaponDelay(APS_CHANGETO_A);
	return 1;
}

int CAPS::ChangeModeTo2()
{
	SendWeaponAnim( APS_CHANGETO_B );
	WeaponDelay(APS_CHANGETO_B);
	return 1;
}







/**************************** Ammoboxes and ammoclips *********************/

class CApsAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_apsammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_apsammo.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 12, "aps", APS_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_aps, CApsAmmoClip );


class CApsAmmoBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_apsammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_apsammobox.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 120, "aps", APS_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_apsbox, CApsAmmoBox );