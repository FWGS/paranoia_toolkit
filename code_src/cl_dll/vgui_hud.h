// ======================================
// Paranoia vgui hud header file
// written by BUzer.
// ======================================

#ifndef _VGUIHUD_H
#define _VGUIHUD_H
using namespace vgui;

#define NUM_WEAPON_ICONS 13 // Wargon: Добавлена одна иконка боеприпасов для RPG.

class ImageHolder;

class CHud2 : public Panel
{
public:
    CHud2();
	~CHud2();
	void Initialize();
	
	void UpdateHealth (int newHealth);
	void UpdateArmor (int newArmor);

//	void Think();
	virtual void solve();

protected:
	virtual void paintBackground(); // per-frame calculations and checks
	virtual void paint();

protected:
	// Wargon: Иконка юза.
	CImageLabel *m_pUsageIcon;
	float m_fUsageUpdateTime;

	// painkiller icon
	CImageLabel	*m_pMedkitsIcon;
	Label		*m_pMedkitsCount;
	int		m_pMedkitsOldNum;
	float	m_fMedkitUpdateTime;

	// health and armor bars
	ImageHolder *m_pBitmapHealthFull;
	ImageHolder *m_pBitmapHealthEmpty;
	ImageHolder *m_pBitmapHealthFlash;

	ImageHolder *m_pBitmapArmorFull;
	ImageHolder *m_pBitmapArmorEmpty;
	ImageHolder *m_pBitmapArmorFlash;

	ImageHolder *m_pHealthIcon;
	ImageHolder *m_pArmorIcon;

	int m_iHealthBarWidth, m_iHealthBarHeight;
	int m_iHealthBarXpos, m_iHealthBarYpos;
	int m_iArmorBarWidth, m_iArmorBarHeight;
	int m_iArmorBarXpos, m_iArmorBarYpos;

	BitmapTGA* m_pWeaponIconsArray[NUM_WEAPON_ICONS];
	BitmapTGA* FindAmmoImageForWeapon(const char *wpn);

	Font		*m_pFontDigits;

	int health, armor;
	int oldhealth, oldarmor;
	float m_fHealthUpdateTime, m_fArmorUpdateTime;
};

void Hud2Init();

#endif // _VGUIHUD_H