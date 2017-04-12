
/*******************************************************
*	weapon_rpk class
*
*	(Ручной Пулемет Калашникова)
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
/*
enum rpk_e
{
	RPK_IDLE = 0,
	RPK_DRAW,
	RPK_SHOOT,
	RPK_SHOOT2,
	RPK_RELOAD,
};*/
/*
enum rpk_e
{
	RPK_IDLE = 0,
	RPK_SHOOT,
	RPK_SHOOT2,
	RPK_RELOAD,
	RPK_DRAW,
};
*/
enum rpk_e
{
	RPK_IDLE_B = 0,
	RPK_CHANGETO_B,
	RPK_CHANGETO_A,	
	RPK_IDLE_A,
	RPK_SHOOT_A,
	RPK_SHOOT_B,
	RPK_RELOAD_A,
	RPK_RELOAD_B,
	RPK_DRAW,
};

class CRPK : public CBaseToggleWeapon
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		iItemSlot( void ) { return 4; }
	int		GetItemInfo(ItemInfo *p);
	int		AddToPlayer( CBasePlayer *pPlayer );
	BOOL	Deploy( void );

	void Attack1( void );
	void Attack2( void );
	void Reload1( void );
	void Reload2( void );
	void Idle1( void );
	void Idle2( void );
	int ChangeModeTo1();
	int ChangeModeTo2();

	int m_iShell;

private:
	unsigned short m_usRPK;
};

LINK_ENTITY_TO_CLASS( weapon_rpk, CRPK );


void CRPK::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_rpk");
	Precache( );
	SET_MODEL(ENT(pev), "models/w_rpk.mdl");
	m_iId = WEAPON_RPK;

	m_iDefaultAmmo = RPK_DEFAULT_GIVE;

	FallInit();
}

void CRPK::Precache( void )
{
	PRECACHE_MODEL("models/v_rpk.mdl");
	PRECACHE_MODEL("models/w_rpk.mdl");
	PRECACHE_MODEL("models/p_rpk.mdl");

	m_iShell = PRECACHE_MODEL ("models/rpk_shell.mdl");

	PRECACHE_SOUND ("weapons/rpk_fire1.wav");
	PRECACHE_SOUND ("weapons/rpk_fire2.wav");
	PRECACHE_SOUND ("weapons/rpk_fire3.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usRPK = PRECACHE_EVENT( 1, "events/rpk.sc" );

	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CRPK::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rpk";
	p->iMaxAmmo1 = RPK_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPK_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_RPK;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

int CRPK::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CRPK::Deploy()
{
//	if (DefaultDeploy( "models/v_rpk.mdl", "models/p_rpk.mdl", RPK_DRAW, "mp5" ))
//	{
//		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
//		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
//		return TRUE;
//	}
//	return FALSE;

	InitToggling();
	return DefaultDeploy( "models/v_rpk.mdl", "models/p_rpk.mdl", RPK_DRAW, "mp5" );
}


void CRPK::Attack1()
{
	// don't fire underwater
	if ((m_iClip <= 0) || (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD))
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iClip--;
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	float spread = ExpandSpread( m_pMySpread->pri_expand );
	EqualizeSpread( &spread, m_pMySpread->pri_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->pri_minspread, m_pMySpread->pri_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_RPK, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usRPK, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, RPK_SHOOT_A, (int)(spread * 255), 0, 0 );

	DefPrimPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CRPK::Attack2()
{
	// don't fire underwater
	if ((m_iClip <= 0) || (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD))
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iClip--;
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	float spread = ExpandSpread( m_pMySpread->sec_expand );
	EqualizeSpread( &spread, m_pMySpread->sec_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->sec_minspread, m_pMySpread->sec_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_RPK, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usRPK, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, RPK_SHOOT_B, (int)(spread * 255), 0, 0 );

	DefSecPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CRPK::Reload1()
{
	if ( m_pPlayer->ammo_rpk <= 0 )
		return;

	DefaultReload( RPK_MAX_CLIP, RPK_RELOAD_A, 6 );
}

void CRPK::Reload2()
{
	if ( m_pPlayer->ammo_rpk <= 0 )
		return;

	DefaultReload( RPK_MAX_CLIP, RPK_RELOAD_B, 6 );
}

void CRPK::Idle1( void )
{
	SendWeaponAnim( RPK_IDLE_A );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

void CRPK::Idle2( void )
{
	SendWeaponAnim( RPK_IDLE_B );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

int CRPK::ChangeModeTo1()
{
	SendWeaponAnim( RPK_CHANGETO_A );
	WeaponDelay(RPK_CHANGETO_A);
	return 1;
}

int CRPK::ChangeModeTo2()
{
	SendWeaponAnim( RPK_CHANGETO_B );
	WeaponDelay(RPK_CHANGETO_B);
	return 1;
}





/**************************** Ammoboxes and ammoclips *********************/

class CRpkAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rpkammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpkammo.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 100, "rpk", RPK_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_rpk, CRpkAmmoClip );


class CRpkAmmoBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rpkammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpkammobox.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 200, "rpk", RPK_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_rpkbox, CRpkAmmoBox );