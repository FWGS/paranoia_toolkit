
/*******************************************************
*	CBaseToggleWeapon and CBaseSpreadWeapon class declaration.
*
*	look paranoia_wpn.cpp for implementation
*
*	written by BUzer
*******************************************************/


enum spread_equalize
{
	E_LINEAR = 0,	// изменение разброса линейно во времени
	E_QUAD,		// по параболе (вначале узкий, потом резко расширяется)
	E_CUBE,		// кубическая парабола
	E_SQRT,		// наоборот (быстро расширяется и плавно переходит в максимальный)
};

typedef struct spreadparams_s {
	char szWeaponName[32];

	Vector			pri_minspread;
	Vector			pri_addspread;
	spread_equalize	pri_equalize;
	float			pri_expand;
	Vector			pri_minpunch;
	Vector			pri_maxpunch;

	Vector			sec_minspread;
	Vector			sec_addspread;
	spread_equalize	sec_equalize;
	float			sec_expand;
	Vector			sec_minpunch;
	Vector			sec_maxpunch;

	float	pri_speed;
	float	sec_speed;

	int		pri_jump;
	int		sec_jump;

	float		returntime;
} spreadparams_t;


class CBaseSpreadWeapon : public CBasePlayerWeapon
{
public:
	// weapons should call this from their Spawn() func
	void InitSpread (float time);

	// weapons should call this from functions, who need m_pMySpread pointer
	void SetTablePointer( void );

	// weapons should call this from their attack functions
	float ExpandSpread( float expandPower );

	virtual void PlayerJump();

	spreadparams_t	*m_pMySpread; // pointer to global spread table for this gun

	void DefPrimPunch();
	void DefSecPunch();

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	CalcSpread( void );
	virtual Vector	GetSpreadVec( void );

	float m_flSpreadTime; // time to return from full spread
	float m_flLastShotTime;
	float m_flLastSpreadPower; // [0..1] range value
};


Vector	AdvanceSpread( Vector &baseSpread, Vector &addSpread, float spread );
void	EqualizeSpread( float *spread, spread_equalize type );



enum weapon_mode
{
	MODE_A = 0,
	MODE_B,
};

class CBaseToggleWeapon : public CBaseSpreadWeapon
{
public:
	// weapons should redefine this funcs
	virtual void Attack1( void ) {};
	virtual void Attack2( void ) {};
	virtual void Reload1( void ) {};
	virtual void Reload2( void ) {};
	virtual void Idle1( void ) {}; //	weapons only should send animation and
	virtual void Idle2( void ) {}; //		set next time
	virtual int ChangeModeTo1( void ) {return 0;};
	virtual int ChangeModeTo2( void ) {return 0;};
	virtual Vector GetSpreadVec1( void );
	virtual Vector GetSpreadVec2( void );

	// weapons should call this from Deploy() func
	void	InitToggling( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void PrimaryAttack( void );				// do "+ATTACK"
	virtual void SecondaryAttack( void );			// do "+ATTACK2"
	virtual void Reload( void );					// do "+RELOAD"
	virtual void WeaponIdle( void );
	virtual void Holster( int skiplocal );
	virtual Vector GetSpreadVec( void );
	virtual int GetMode( void );

	int		m_iWeaponMode;
};