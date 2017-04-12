
/*******************************************************
*	weapon_val class
*
*	(Автомат специальный ВАЛ)
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

enum val_e
{
	VAL_IDLE = 0,
	VAL_DRAW,
	VAL_SHOOT,
	VAL_RELOAD,
};

class CVAL : public CBaseToggleWeapon
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		iItemSlot( void ) { return 4; }
	int		GetItemInfo(ItemInfo *p);
	int		AddToPlayer( CBasePlayer *pPlayer );
	BOOL	Deploy( void );
	void	Holster( int skiplocal );

	void Attack1( void );
	void Attack2( void );
	void Reload1( void );
	void Reload2( void );
	void Idle1( void );
	void Idle2( void );
	int ChangeModeTo1( void );
	int ChangeModeTo2( void );
//	Vector GetSpreadVec1( void );
	Vector GetSpreadVec2( void );
	int	GetMode( void );

	int m_iShell;

private:
	unsigned short m_usVAL;
};

LINK_ENTITY_TO_CLASS( weapon_val, CVAL );


void CVAL::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_val");
	Precache( );
	SET_MODEL(ENT(pev), "models/w_val.mdl");
	m_iId = WEAPON_VAL;

	m_iDefaultAmmo = ASVAL_DEFAULT_GIVE;

	FallInit();
}

void CVAL::Precache( void )
{
	PRECACHE_MODEL("models/v_val.mdl");
	PRECACHE_MODEL("models/w_val.mdl");
	PRECACHE_MODEL("models/p_val.mdl");

	m_iShell = PRECACHE_MODEL ("models/val_shell.mdl");

	PRECACHE_SOUND ("weapons/val_fire1.wav");
	PRECACHE_SOUND ("weapons/val_fire2.wav");
	PRECACHE_SOUND ("weapons/val_fire3.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usVAL = PRECACHE_EVENT( 1, "events/val.sc" );

	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CVAL::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "asval";
	p->iMaxAmmo1 = ASVAL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = ASVAL_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_VAL;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

int CVAL::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CVAL::Deploy()
{
	InitToggling();
	return DefaultDeploy( "models/v_val.mdl", "models/p_val.mdl", VAL_DRAW, "mp5" );
}

Vector CVAL::GetSpreadVec2( void )
{
	return VECTOR_CONE_1DEGREES;
}

void CVAL::Attack1()
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
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_ASVAL, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usVAL, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, VAL_SHOOT, (int)(spread * 255), 0, 0 );

	DefPrimPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CVAL::Attack2()
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

//	float spread = ExpandSpread( 0.2 );
//	Vector vecSpread = AdvanceSpread( VECTOR_CONE_2DEGREES, VECTOR_CONE_4DEGREES, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_ASVAL, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usVAL, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, VAL_SHOOT, 0, 0, 0 );

	DefSecPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.17;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CVAL::Reload1()
{
	if ( m_pPlayer->ammo_asval <= 0 )
		return;

	DefaultReload( ASVAL_MAX_CLIP, VAL_RELOAD, 2.3 );
}

void CVAL::Reload2()
{
	if ( m_pPlayer->ammo_asval <= 0 )
		return;

	if (DefaultReload( ASVAL_MAX_CLIP, VAL_RELOAD, 2.3 ))
	{
		CBaseToggleWeapon::SecondaryAttack(); // remove scope
		WeaponDelay(VAL_RELOAD); // bugfix: set delay again
	}
}

void CVAL::Idle1( void )
{
	SendWeaponAnim( VAL_IDLE );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

void CVAL::Idle2( void )
{
	SendWeaponAnim( VAL_IDLE );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

int CVAL::ChangeModeTo1()
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
	WeaponDelayTime(0.3);
//	ALERT(at_console, "val to 0\n");
	return 1;
}

int CVAL::ChangeModeTo2()
{
	if (m_pPlayer->m_iGasMaskOn)
	{
		UTIL_ShowMessage("#GAS_AND_SCOPE", m_pPlayer );
		WeaponDelayTime(0.5);
		return 0;
	//	m_pPlayer->ToggleGasMask();
	}
	if (m_pPlayer->m_iHeadShieldOn)
	{
		UTIL_ShowMessage("#SCOPE_AND_SHIELD", m_pPlayer );
		WeaponDelayTime(0.5);
		return 0;
	//	m_pPlayer->ToggleHeadShield();
	}

//	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 30;
	WeaponDelayTime(0.3);
//	ALERT(at_console, "val to 30\n");
	return 1;
}

void CVAL::Holster( int skiplocal )
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // reset fov
	CBaseToggleWeapon::Holster(skiplocal);
}

int CVAL::GetMode( void )
{
	switch (m_iWeaponMode)
	{
	default:
	case MODE_A:
		return 1;

	case MODE_B:
		return 3; // hack to draw gun-specific scope on client
	}
}





/**************************** Ammoboxes and ammoclips *********************/

class CValAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_valammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_valammo.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 20, "asval", ASVAL_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_val, CValAmmoClip );


class CValAmmoBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_valammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_valammobox.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 80, "asval", ASVAL_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_valbox, CValAmmoBox );