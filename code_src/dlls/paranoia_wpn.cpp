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

/*******************************************************
*	CBaseSpreadWeapon class implementation.
*
*	contains functions to calculate spread.
*	written by BUzer for Half-Life:Paranoia modification
*******************************************************/


spreadparams_t	gSpreadTable[16];
int				gSCounter;

spreadparams_t	gDefaultSpread;



Vector GetConeVectorForAngle(int num)
{
	switch(num)
	{
	case 0:
		return Vector(0, 0, 0);
	case 1:
		return VECTOR_CONE_1DEGREES;
	case 2:
		return VECTOR_CONE_2DEGREES;
	case 3:
		return VECTOR_CONE_3DEGREES;
	case 4:
		return VECTOR_CONE_4DEGREES;
	case 5:
		return VECTOR_CONE_5DEGREES;
	case 6:
		return VECTOR_CONE_6DEGREES;
	case 7:
		return VECTOR_CONE_7DEGREES;
	case 8:
		return VECTOR_CONE_8DEGREES;
	case 9:
		return VECTOR_CONE_9DEGREES;
	case 10:
		return VECTOR_CONE_10DEGREES;
	case 15:
		return VECTOR_CONE_15DEGREES;
	case 20:
		return VECTOR_CONE_20DEGREES;
	default:
		ALERT(at_console, " *\n");
		ALERT(at_console, " * GetConeVectorForAngle error: %d - unsupported angle value\n", num);
		ALERT(at_console, " *\n");
	}
	return Vector(0, 0, 0);
}


void DebugListSpreadTable( void )
{
	ALERT(at_console, "|== Listing spread table ==\n");
	ALERT(at_console, "| %d weapons total\n", gSCounter);
	ALERT(at_console, "|\n");
	
	for (int i=0; i < gSCounter; i++)
	{
		ALERT(at_console, "| %s\n", gSpreadTable[i].szWeaponName);
		ALERT(at_console, "|    prim min cone: %f\n", gSpreadTable[i].pri_minspread.x);
		ALERT(at_console, "|    prim add cone: %f\n", gSpreadTable[i].pri_addspread.x);
		ALERT(at_console, "|    prim equalize: %d\n", gSpreadTable[i].pri_equalize);
		ALERT(at_console, "|    prim expand: %f\n", gSpreadTable[i].pri_expand);
		ALERT(at_console, "|    prim punch (%f, %f) (%f, %f) (%f, %f)\n",
			gSpreadTable[i].pri_minpunch[0], gSpreadTable[i].pri_maxpunch[0],
			gSpreadTable[i].pri_minpunch[1], gSpreadTable[i].pri_maxpunch[1],
			gSpreadTable[i].pri_minpunch[2], gSpreadTable[i].pri_maxpunch[2]);
		ALERT(at_console, "|    sec min cone: %f\n", gSpreadTable[i].sec_minspread.x);
		ALERT(at_console, "|    sec add cone: %f\n", gSpreadTable[i].sec_addspread.x);
		ALERT(at_console, "|    sec equalize: %d\n", gSpreadTable[i].sec_equalize);
		ALERT(at_console, "|    sec expand: %f\n", gSpreadTable[i].sec_expand);
		ALERT(at_console, "|    sec punch (%f, %f) (%f, %f) (%f, %f)\n",
			gSpreadTable[i].sec_minpunch[0], gSpreadTable[i].sec_maxpunch[0],
			gSpreadTable[i].sec_minpunch[1], gSpreadTable[i].sec_maxpunch[1],
			gSpreadTable[i].sec_minpunch[2], gSpreadTable[i].sec_maxpunch[2]);
		ALERT(at_console, "|    return time: %f\n", gSpreadTable[i].returntime);
		ALERT(at_console, "|\n");
	}

	ALERT(at_console, "|== spread table end ==\n");
}



extern char com_token[ 1500 ];
char *COM_Parse (char *data);

void LoadSpreadTable( char *filename )
{
	gSCounter = 0;
	
	int length;
	char *pFile;
	char *aFile = pFile = (char*)LOAD_FILE_FOR_ME( filename, &length );

	if (!pFile || !length)
	{
		ALERT(at_console, " *\n");
		ALERT(at_console, " * LoadSpreadTable failed to load file %s\n", filename);
		ALERT(at_console, " *\n");
		return;
	}

	// parse weapons
	while (1)
	{		
		int pri_minspreadindex = 0, pri_maxspreadindex = 0;
		int sec_minspreadindex = 0, sec_maxspreadindex = 0;
		
		// get weapon name
		pFile = COM_Parse( pFile );
		if ( strlen( com_token ) <= 0 )
			break; // end of file

		strcpy(gSpreadTable[gSCounter].szWeaponName, com_token);

		 // default settings
		gSpreadTable[gSCounter].pri_equalize = E_LINEAR;
		gSpreadTable[gSCounter].sec_equalize = E_LINEAR;
		gSpreadTable[gSCounter].pri_expand = 0.25;
		gSpreadTable[gSCounter].sec_expand = 0.2;
		gSpreadTable[gSCounter].returntime = 1.5;

		gSpreadTable[gSCounter].pri_speed = 0;
		gSpreadTable[gSCounter].sec_speed = 0;

		gSpreadTable[gSCounter].pri_jump = -1;
		gSpreadTable[gSCounter].sec_jump = -1;

		gSpreadTable[gSCounter].pri_minpunch[0] = -0.5;
		gSpreadTable[gSCounter].pri_maxpunch[0] = 1.5;
		gSpreadTable[gSCounter].pri_minpunch[1] = -0.5;
		gSpreadTable[gSCounter].pri_maxpunch[1] = 0.5;
		gSpreadTable[gSCounter].pri_minpunch[2] = 0;
		gSpreadTable[gSCounter].pri_maxpunch[2] = 0;
	
		gSpreadTable[gSCounter].sec_minpunch[0] = -0.3;
		gSpreadTable[gSCounter].sec_maxpunch[0] = 1;
		gSpreadTable[gSCounter].sec_minpunch[1] = -0.3;
		gSpreadTable[gSCounter].sec_maxpunch[1] = 0.3;
		gSpreadTable[gSCounter].sec_minpunch[2] = 0;
		gSpreadTable[gSCounter].sec_maxpunch[2] = 0;

		// '{' expected
		pFile = COM_Parse( pFile );
		if (com_token[0] != '{')
		{
			ALERT(at_console, " *\n");
			ALERT(at_console, " * LoadSpreadTable parsing error: expecting { after weapon name\n");
			ALERT(at_console, " *\n");
			break;
		}

		// parse sections
		while(1)
		{
			// 'primary', 'secondary', or 'returntime'
			pFile = COM_Parse( pFile );
			if (com_token[0] == '}')
				break; // no more sections

			// primary settings section
			if (!strcmp("primary", com_token))
			{
				// '{' expected
				pFile = COM_Parse( pFile );
				if (com_token[0] != '{')
				{
					ALERT(at_console, " *\n");
					ALERT(at_console, " * LoadSpreadTable parsing error: expecting { after section name\n");
					ALERT(at_console, " *\n");
					break;
				}			
				// parse spread parameters
				while (1)
				{
					pFile = COM_Parse( pFile );
					if (com_token[0] == '}')
						break; // section finished

					if (!strcmp("minspread", com_token))
					{
						pFile = COM_Parse( pFile ); // read min spread value
						pri_minspreadindex = atoi(com_token);
					}
					else if (!strcmp("maxspread", com_token))
					{
						pFile = COM_Parse( pFile ); // read max spread value
						pri_maxspreadindex = atoi(com_token);
					}
					else if (!strcmp("equalize", com_token))
					{
						// 'E_LINEAR' or 'E_QUAD' or 'E_CUBE' or 'E_SQRT'
						pFile = COM_Parse( pFile );
						if (!strcmp("E_LINEAR", com_token))
							gSpreadTable[gSCounter].pri_equalize = E_LINEAR;
						else if (!strcmp("E_QUAD", com_token))
							gSpreadTable[gSCounter].pri_equalize = E_QUAD;
						else if (!strcmp("E_CUBE", com_token))
							gSpreadTable[gSCounter].pri_equalize = E_CUBE;
						else if (!strcmp("E_SQRT", com_token))
							gSpreadTable[gSCounter].pri_equalize = E_SQRT;
						else
						{
							ALERT(at_console, " *\n");
							ALERT(at_console, " * LoadSpreadTable parsing error: %s - unknown equalize type\n", com_token);
							ALERT(at_console, " *\n");
						}
					}
					else if (!strcmp("expand", com_token))
					{
						pFile = COM_Parse( pFile ); // read expand value
						gSpreadTable[gSCounter].pri_expand = atof(com_token);
					}
					else if (!strcmp("speed", com_token))
					{
						pFile = COM_Parse( pFile ); // read speed modifier
						gSpreadTable[gSCounter].pri_speed = atof(com_token);
					}
					else if (!strcmp("jump", com_token))
					{
						pFile = COM_Parse( pFile ); // read jump force
						gSpreadTable[gSCounter].pri_jump = atoi(com_token);
					}
					else if (!strcmp("minpunch", com_token))
					{
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].pri_minpunch[0] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].pri_minpunch[1] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].pri_minpunch[2] = atof(com_token);
					}
					else if (!strcmp("maxpunch", com_token))
					{
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].pri_maxpunch[0] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].pri_maxpunch[1] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].pri_maxpunch[2] = atof(com_token);
					}
					else
					{
						ALERT(at_console, " *\n");
						ALERT(at_console, " * LoadSpreadTable parsing error: %s - unknown variable name\n", com_token);
						ALERT(at_console, " *\n");
					}
				}
			}
			// secondary settings section
			else if (!strcmp("secondary", com_token))
			{
				// '{' expected
				pFile = COM_Parse( pFile );
				if (com_token[0] != '{')
				{
					ALERT(at_console, " *\n");
					ALERT(at_console, " * LoadSpreadTable parsing error: expecting { after section name\n");
					ALERT(at_console, " *\n");
					break;
				}			
				// parse spread parameters
				while (1)
				{
					pFile = COM_Parse( pFile );
					if (com_token[0] == '}')
						break; // section finished

					if (!strcmp("minspread", com_token))
					{
						pFile = COM_Parse( pFile ); // read min spread value
						sec_minspreadindex = atoi(com_token);
					}
					else if (!strcmp("maxspread", com_token))
					{
						pFile = COM_Parse( pFile ); // read max spread value
						sec_maxspreadindex = atoi(com_token);
					}
					else if (!strcmp("equalize", com_token))
					{
						// 'E_LINEAR' or 'E_QUAD' or 'E_CUBE' or 'E_SQRT'
						pFile = COM_Parse( pFile );
						if (!strcmp("E_LINEAR", com_token))
							gSpreadTable[gSCounter].sec_equalize = E_LINEAR;
						else if (!strcmp("E_QUAD", com_token))
							gSpreadTable[gSCounter].sec_equalize = E_QUAD;
						else if (!strcmp("E_CUBE", com_token))
							gSpreadTable[gSCounter].sec_equalize = E_CUBE;
						else if (!strcmp("E_SQRT", com_token))
							gSpreadTable[gSCounter].sec_equalize = E_SQRT;
						else
						{
							ALERT(at_console, " *\n");
							ALERT(at_console, " * LoadSpreadTable parsing error: %s - unknown equalize type\n", com_token);
							ALERT(at_console, " *\n");
						}
					}
					else if (!strcmp("expand", com_token))
					{
						pFile = COM_Parse( pFile ); // read expand value
						gSpreadTable[gSCounter].sec_expand = atof(com_token);
					}
					else if (!strcmp("speed", com_token))
					{
						pFile = COM_Parse( pFile ); // read speed modifier
						gSpreadTable[gSCounter].sec_speed = atof(com_token);
					}
					else if (!strcmp("jump", com_token))
					{
						pFile = COM_Parse( pFile ); // read jump force
						gSpreadTable[gSCounter].sec_jump = atoi(com_token);
					}
					else if (!strcmp("minpunch", com_token))
					{
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].sec_minpunch[0] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].sec_minpunch[1] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].sec_minpunch[2] = atof(com_token);
					}
					else if (!strcmp("maxpunch", com_token))
					{
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].sec_maxpunch[0] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].sec_maxpunch[1] = atof(com_token);
						pFile = COM_Parse( pFile );
						gSpreadTable[gSCounter].sec_maxpunch[2] = atof(com_token);
					}
					else
					{
						ALERT(at_console, " *\n");
						ALERT(at_console, " * LoadSpreadTable parsing error: %s - unknown variable name\n", com_token);
						ALERT(at_console, " *\n");
					}
				}
			}
			// returntime variable
			else if (!strcmp("returntime", com_token))
			{
				pFile = COM_Parse( pFile ); // read max spread value
				gSpreadTable[gSCounter].returntime = atof(com_token);
			}
			else
			{
				ALERT(at_console, " *\n");
				ALERT(at_console, " * LoadSpreadTable parsing error: %s - unknown section name\n", com_token);
				ALERT(at_console, " *\n");
			}
		}

		// weapon parsing finished
		// check spread cone angles for warnings
		if (pri_minspreadindex > pri_maxspreadindex)
		{
			ALERT(at_console, " *\n");
			ALERT(at_console, " * LoadSpreadTable warning: max prispread lower than min in %\n", gSpreadTable[gSCounter].szWeaponName);
			ALERT(at_console, " *\n");
		}
		if (sec_minspreadindex > sec_maxspreadindex)
		{
			ALERT(at_console, " *\n");
			ALERT(at_console, " * LoadSpreadTable warning: max secspread lower than min in %\n", gSpreadTable[gSCounter].szWeaponName);
			ALERT(at_console, " *\n");
		}

		// convert max cone angles to offsets
		pri_maxspreadindex = pri_maxspreadindex - pri_minspreadindex;
		sec_maxspreadindex = sec_maxspreadindex - sec_minspreadindex;

		// get appropriate vectors for cone angles
		gSpreadTable[gSCounter].pri_minspread = GetConeVectorForAngle( pri_minspreadindex );
		gSpreadTable[gSCounter].pri_addspread = GetConeVectorForAngle( pri_maxspreadindex );
		gSpreadTable[gSCounter].sec_minspread = GetConeVectorForAngle( sec_minspreadindex );
		gSpreadTable[gSCounter].sec_addspread = GetConeVectorForAngle( sec_maxspreadindex );
		
		gSCounter++;
	}
	
	ALERT(at_console, "%d spread weapons info loaded from %s\n", gSCounter, filename);

	FREE_FILE( aFile );

	// initialize default spread
	sprintf (gDefaultSpread.szWeaponName, "default\n");
	gDefaultSpread.pri_minspread = VECTOR_CONE_6DEGREES;
	gDefaultSpread.pri_addspread = VECTOR_CONE_6DEGREES;
	gDefaultSpread.pri_equalize = E_LINEAR;
	gDefaultSpread.pri_expand = 0.25;
	gDefaultSpread.sec_minspread = VECTOR_CONE_2DEGREES;
	gDefaultSpread.sec_addspread = VECTOR_CONE_4DEGREES;
	gDefaultSpread.sec_equalize = E_LINEAR;
	gDefaultSpread.sec_expand = 0.2;
	gDefaultSpread.returntime = 1.5;
	gDefaultSpread.pri_speed = 0;
	gDefaultSpread.sec_speed = 0;
}




TYPEDESCRIPTION	CBaseSpreadWeapon::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseSpreadWeapon, m_flSpreadTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseSpreadWeapon, m_flLastShotTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseSpreadWeapon, m_flLastSpreadPower, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CBaseSpreadWeapon, CBasePlayerWeapon );


void CBaseSpreadWeapon::SetTablePointer( void )
{
	const char *name = STRING(pev->classname);
	for (int i=0; i < gSCounter; i++)
	{
		if (!strcmp(name, gSpreadTable[i].szWeaponName))
		{
			m_pMySpread = &gSpreadTable[i];
			break;
		}
	}

	if (!m_pMySpread)
	{
		ALERT(at_console, " *\n");
		ALERT(at_console, " * SetTablePointer error: cant find record for %s, using default\n", name);
		ALERT(at_console, " *\n");
		m_pMySpread = &gDefaultSpread;
	}
}	
	

void CBaseSpreadWeapon::InitSpread (float time)
{
	m_flSpreadTime = time;
	m_flLastSpreadPower = 0;
	m_flLastShotTime = 0;
}


float CBaseSpreadWeapon::ExpandSpread( float expandPower )
{
	// buz: in ducking more accuracy
	if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		expandPower *= 0.7;

	float curspread = CalcSpread();
	m_flLastShotTime = gpGlobals->time;
	m_flLastSpreadPower = curspread + expandPower;
	if (m_flLastSpreadPower > 1)
		m_flLastSpreadPower = 1;
	
	return curspread;
}

// returns spread expand power [0..1] based on current time
float CBaseSpreadWeapon::CalcSpread( void )
{
	// hack maybe.. it's anyway called after the table was loaded, and before the pointer will be needed..
	if (!m_pMySpread)
	{		
		SetTablePointer();
		InitSpread( m_pMySpread->returntime );
	}
	
	float decay = (gpGlobals->time - m_flLastShotTime) / (m_flSpreadTime * m_flLastSpreadPower);
	if (decay > 1)
		return 0;

	return (1 - decay) * m_flLastSpreadPower;
}


Vector CBaseSpreadWeapon::GetSpreadVec( void )
{
	float spread = CalcSpread();
	EqualizeSpread( &spread, m_pMySpread->pri_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->pri_minspread, m_pMySpread->pri_addspread, spread);
	vecSpread[2] = spread;
	return vecSpread;
}


void CBaseSpreadWeapon::PlayerJump( void )
{
	ExpandSpread( 0.7 );
}


// helpful functions
Vector AdvanceSpread( Vector &baseSpread, Vector &addSpread, float spread )
{
	return baseSpread + (addSpread * spread);
}

void EqualizeSpread( float *spread, spread_equalize type )
{
	float val = *spread;

	switch (type)
	{
	case E_LINEAR:
	default:
		break;

	case E_QUAD:
		val = val*val;
		break;
	
	case E_CUBE:
		val = val*val*val;
		break;

	case E_SQRT:
		val = sqrt(val);
		break;
	}

	*spread = val;
}





/*******************************************************
*	CBaseToggleWeapon class implementation.
*
*	Weapon mode is changed on SecondaryAttack, and player slows down.
*	Most of Paranoia weapons will derive from this.
*
*	written by BUzer for Half-Life:Paranoia modification
*******************************************************/

TYPEDESCRIPTION	CBaseToggleWeapon::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseToggleWeapon, m_iWeaponMode, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBaseToggleWeapon, CBaseSpreadWeapon );


void CBaseToggleWeapon::InitToggling(void)
{
//	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
//	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	m_iWeaponMode = MODE_A;

	if (!m_pMySpread)
	{		
		SetTablePointer();
		InitSpread( m_pMySpread->returntime );
	}

	// buz: Paranoia's speed adjustment
	if (m_pMySpread->pri_speed)
		m_pPlayer->pev->maxspeed = m_pMySpread->pri_speed;
	else
		m_pPlayer->pev->maxspeed = gSkillData.plrPrimaryMaxSpeed;

	// buz: set jump force
	if (m_pMySpread->pri_jump != -1)
		m_pPlayer->SetJumpHeight(m_pMySpread->pri_jump);
	else
		m_pPlayer->SetJumpHeight(100);
}


void CBaseToggleWeapon::Holster(int skiplocal)
{
	// buz: Paranoia's speed adjustment
	// return player speed to normal
	m_pPlayer->pev->maxspeed = gSkillData.plrPrimaryMaxSpeed;

	// buz: set jump force
	m_pPlayer->SetJumpHeight(100);

	m_iWeaponMode = MODE_A;

	CBasePlayerWeapon::Holster();
}


void CBaseToggleWeapon::PrimaryAttack(void)
{
	switch (m_iWeaponMode)
	{
	case MODE_A:
		Attack1();
		break;
	case MODE_B:
		Attack2();
		break;
	}

	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
}


void CBaseToggleWeapon::Reload(void)
{
	switch (m_iWeaponMode)
	{
	case MODE_A:
		Reload1();
		break;
	case MODE_B:
		Reload2();
		break;
	}
}


void CBaseToggleWeapon::WeaponIdle(void)
{
	if (!m_pMySpread)
	{		
		SetTablePointer();
		InitSpread( m_pMySpread->returntime );
	}

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch (m_iWeaponMode)
	{
	case MODE_A:
		Idle1();
		if (m_pMySpread->pri_speed)
			m_pPlayer->pev->maxspeed = m_pMySpread->pri_speed;
		else
			m_pPlayer->pev->maxspeed = gSkillData.plrPrimaryMaxSpeed; // resend after restore

		// buz: set jump force
		if (m_pMySpread->pri_jump != -1)
			m_pPlayer->SetJumpHeight(m_pMySpread->pri_jump);
		else
			m_pPlayer->SetJumpHeight(100);

		break;
	case MODE_B:
		Idle2();
		if (m_pMySpread->sec_speed)
			m_pPlayer->pev->maxspeed = m_pMySpread->sec_speed;
		else
			m_pPlayer->pev->maxspeed = gSkillData.plrSecondaryMaxSpeed; // resend after restore
		
		// buz: set jump force
		if (m_pMySpread->sec_jump != -1)
			m_pPlayer->SetJumpHeight(m_pMySpread->sec_jump);
		else
			m_pPlayer->SetJumpHeight(100);

		break;
	}
}


void CBaseToggleWeapon::SecondaryAttack(void)
{
	if (!m_pMySpread)
	{		
		SetTablePointer();
		InitSpread( m_pMySpread->returntime );
	}

	switch (m_iWeaponMode)
	{
	case MODE_A:
		if (ChangeModeTo2())
		{
			m_iWeaponMode = MODE_B;
			if (m_pMySpread->sec_speed)
				m_pPlayer->pev->maxspeed = m_pMySpread->sec_speed;
			else
				m_pPlayer->pev->maxspeed = gSkillData.plrSecondaryMaxSpeed;

			// buz: set jump force
			if (m_pMySpread->sec_jump != -1)
				m_pPlayer->SetJumpHeight(m_pMySpread->sec_jump);
			else
				m_pPlayer->SetJumpHeight(100);
		}
		break;
	case MODE_B:
		if (ChangeModeTo1())
		{
			m_iWeaponMode = MODE_A;
			if (m_pMySpread->pri_speed)
				m_pPlayer->pev->maxspeed = m_pMySpread->pri_speed;
			else
				m_pPlayer->pev->maxspeed = gSkillData.plrPrimaryMaxSpeed;

			// buz: set jump force
			if (m_pMySpread->pri_jump != -1)
				m_pPlayer->SetJumpHeight(m_pMySpread->pri_jump);
			else
				m_pPlayer->SetJumpHeight(100);
		}
		break;
	}

//	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
//	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
//	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}


Vector CBaseToggleWeapon::GetSpreadVec( void )
{
	switch (m_iWeaponMode)
	{
	default:
	case MODE_A:
		return GetSpreadVec1();

	case MODE_B:
		return GetSpreadVec2();
	}
}


Vector CBaseToggleWeapon::GetSpreadVec1( void )
{
	float spread = CalcSpread();
	EqualizeSpread( &spread, m_pMySpread->pri_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->pri_minspread, m_pMySpread->pri_addspread, spread);
	vecSpread[2] = spread;
	return vecSpread;
}

Vector CBaseToggleWeapon::GetSpreadVec2( void )
{
	float spread = CalcSpread();
	EqualizeSpread( &spread, m_pMySpread->sec_equalize );
	Vector vecSpread = AdvanceSpread( m_pMySpread->sec_minspread, m_pMySpread->sec_addspread, spread);
	vecSpread[2] = spread;
	return vecSpread;
}


int CBaseToggleWeapon::GetMode( void )
{
	switch (m_iWeaponMode)
	{
	default:
	case MODE_A:
		return 1;

	case MODE_B:
		return 2;
	}
}


void CBaseSpreadWeapon::DefPrimPunch()
{
	m_pPlayer->ViewPunch(
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, m_pMySpread->pri_minpunch[0], m_pMySpread->pri_maxpunch[0] ),
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, m_pMySpread->pri_minpunch[1], m_pMySpread->pri_maxpunch[1] ),
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, m_pMySpread->pri_minpunch[2], m_pMySpread->pri_maxpunch[2] )
	);
}

void CBaseSpreadWeapon::DefSecPunch()
{
	m_pPlayer->ViewPunch(
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, m_pMySpread->sec_minpunch[0], m_pMySpread->sec_maxpunch[0] ),
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, m_pMySpread->sec_minpunch[1], m_pMySpread->sec_maxpunch[1] ),
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, m_pMySpread->sec_minpunch[2], m_pMySpread->sec_maxpunch[2] )
	);
}