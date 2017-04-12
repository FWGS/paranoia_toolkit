/***
*
* Written by BUzer
* for Half-Life: Paranoia
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

#include "paranoia_wpn.h"

enum shotgun_e {
	SHOTGUN_IDLE_A = 0,
	SHOTGUN_SHOOT1_A,
	SHOTGUN_SHOOT2_A,
	SHOTGUN_INSERT,
	SHOTGUN_ENDRELOAD_A,
	SHOTGUN_STARTRELOAD_A,
	SHOTGUN_DRAW,
	SHOTGUN_CHANGETO_A,
	SHOTGUN_CHANGETO_B,
	SHOTGUN_IDLE_B,
	SHOTGUN_SHOOT1_B,
	SHOTGUN_SHOOT2_B,
	SHOTGUN_ENDRELOAD_B,
	SHOTGUN_STARTRELOAD_B
};

class CShotgun : public CBaseToggleWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );

	void Attack1( void );
	void Attack2( void );
	void Reload1( void );
	void Reload2( void );
	void Idle1( void );
	void Idle2( void );
	int ChangeModeTo1( void );
	int ChangeModeTo2( void );

	int m_iInReload; // 1 - reload start, 2 - continue
	int m_iShell;

private:
	unsigned short m_usSingleFire;
};

LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun );

TYPEDESCRIPTION	CShotgun::m_SaveData[] = 
{
	DEFINE_FIELD( CShotgun, m_iInReload, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CShotgun, CBaseToggleWeapon );

void CShotgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_spas12.mdl");

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CShotgun::Precache( void )
{
	PRECACHE_MODEL("models/v_spas12.mdl");
	PRECACHE_MODEL("models/w_spas12.mdl");
	PRECACHE_MODEL("models/p_spas12.mdl");

	m_iShell = PRECACHE_MODEL ("models/spas12_shell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");    

	PRECACHE_SOUND ("weapons/spas12-1.wav");//shotgun
	PRECACHE_SOUND ("weapons/spas12-2.wav");//shotgun

	PRECACHE_SOUND ("weapons/spas12_insertshell.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/spas12_pump.wav");	// shotgun reload

	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND ("weapons/scock1.wav");	// cock gun

	m_usSingleFire = PRECACHE_EVENT( 1, "events/shotgun1.sc" );
	m_flTimeWeaponIdle = 0; // fix to resend idle animation
}

int CShotgun::AddToPlayer( CBasePlayer *pPlayer )
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

int CShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}

BOOL CShotgun::Deploy( )
{
	m_iInReload = 0;
	InitToggling();
	return DefaultDeploy( "models/v_spas12.mdl", "models/p_spas12.mdl", SHOTGUN_DRAW, "shotgun" );
}

void CShotgun::Attack1()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	// hack
	if (!m_iClip)
	{
		if (m_iInReload && (UTIL_WeaponTimeBase() > m_flTimeWeaponIdle))
			Idle1();
		else
			Reload();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_iClip--;
	m_iInReload = 0;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	float spread = ExpandSpread( m_pMySpread->pri_expand );
	EqualizeSpread( &spread, m_pMySpread->pri_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->pri_minspread, m_pMySpread->pri_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecSpread.x, vecSpread.y, SHOTGUN_SHOOT1_A, (int)(spread * 255), 0, 0 );

	DefPrimPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.4;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.4;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CShotgun::Attack2()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	// hack
	if (!m_iClip)
	{
		if (m_iInReload && (UTIL_WeaponTimeBase() > m_flTimeWeaponIdle))
			Idle1();
		else
			Reload();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_iClip--;
	m_iInReload = 0;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	float spread = ExpandSpread( m_pMySpread->sec_expand );
	EqualizeSpread( &spread, m_pMySpread->sec_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->sec_minspread, m_pMySpread->sec_addspread, spread);
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecSpread.x, vecSpread.y, SHOTGUN_SHOOT1_B, (int)(spread * 255), 0, 0 );

	DefSecPunch();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.4;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.4;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


float GetSequenceTime( void *pmodel, int seqnum );

void CShotgun::Reload1( void )
{
	// hack
	if (!m_iClip && m_iInReload && (UTIL_WeaponTimeBase() > m_flTimeWeaponIdle))
	{
		Idle1();
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if (m_iInReload)
		return;

	m_iInReload = 1;
	SendWeaponAnim( SHOTGUN_STARTRELOAD_A );
	float flDelay = GetSequenceTime( GET_MODEL_PTR(ENT(pev)), SHOTGUN_STARTRELOAD_A );	
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + flDelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDelay;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flDelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flDelay;
}

void CShotgun::Reload2( void )
{
	// hack
	if (!m_iClip && m_iInReload && (UTIL_WeaponTimeBase() > m_flTimeWeaponIdle))
	{
		Idle2();
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if (m_iInReload)
		return;

	m_iInReload = 1;
	SendWeaponAnim( SHOTGUN_STARTRELOAD_B );
	float flDelay = GetSequenceTime( GET_MODEL_PTR(ENT(pev)), SHOTGUN_STARTRELOAD_B );	
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + flDelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDelay;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flDelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flDelay;
}


void CShotgun::Idle1( void )
{
	float flDelay;
	
	if (m_iInReload == 1) // reload just started, dont increase clip
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		{
			// stop reload
			m_iInReload = 0;
			SendWeaponAnim( SHOTGUN_ENDRELOAD_A );
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_pump.wav", 0.8, ATTN_NORM);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
			return;
		}

		m_iInReload = 2;
		flDelay = GetSequenceTime( GET_MODEL_PTR(ENT(pev)), SHOTGUN_INSERT );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDelay;
		SendWeaponAnim( SHOTGUN_INSERT );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_insertshell.wav", 0.8, ATTN_NORM);
		return;
	}
	else if (m_iInReload == 2)
	{
		// continue reloading
		m_iClip++;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		{
			// stop reload
			m_iInReload = 0;
			SendWeaponAnim( SHOTGUN_ENDRELOAD_A );
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_pump.wav", 0.8, ATTN_NORM);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
			return;
		}
		flDelay = GetSequenceTime( GET_MODEL_PTR(ENT(pev)), SHOTGUN_INSERT );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDelay;
		SendWeaponAnim( SHOTGUN_INSERT );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_insertshell.wav", 0.8, ATTN_NORM);
		return;
	}

	// normal idle
	SendWeaponAnim( SHOTGUN_IDLE_A );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}

void CShotgun::Idle2( void )
{
	float flDelay;
	
	if (m_iInReload == 1) // reload just started, dont increase clip
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		{
			// stop reload
			m_iInReload = 0;
			SendWeaponAnim( SHOTGUN_ENDRELOAD_B );
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_pump.wav", 0.8, ATTN_NORM);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
			return;
		}

		m_iInReload = 2;
		flDelay = GetSequenceTime( GET_MODEL_PTR(ENT(pev)), SHOTGUN_INSERT );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDelay;
		SendWeaponAnim( SHOTGUN_INSERT );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_insertshell.wav", 0.8, ATTN_NORM);
		return;
	}
	else if (m_iInReload == 2)
	{
		// continue reloading
		m_iClip++;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		{
			// stop reload
			m_iInReload = 0;
			SendWeaponAnim( SHOTGUN_ENDRELOAD_B );
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_pump.wav", 0.8, ATTN_NORM);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
			return;
		}
		flDelay = GetSequenceTime( GET_MODEL_PTR(ENT(pev)), SHOTGUN_INSERT );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDelay;
		SendWeaponAnim( SHOTGUN_INSERT );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "spas12_insertshell.wav", 0.8, ATTN_NORM);
		return;
	}

	// normal idle
	SendWeaponAnim( SHOTGUN_IDLE_B );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
}


int CShotgun::ChangeModeTo1()
{
	m_iInReload = 0;
	SendWeaponAnim( SHOTGUN_CHANGETO_A );
	WeaponDelay(SHOTGUN_CHANGETO_A);
	return 1;
}

int CShotgun::ChangeModeTo2()
{
	m_iInReload = 0;
	SendWeaponAnim( SHOTGUN_CHANGETO_B );
	WeaponDelay(SHOTGUN_CHANGETO_B);
	return 1;
}




class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_spas12_ammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_spas12_ammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo );


