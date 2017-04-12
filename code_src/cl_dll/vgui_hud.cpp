// ====================================
// Paranoia vgui hud
// written by BUzer.
// ====================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_hud.h"
#include "vgui_paranoiatext.h" // Wargon
#include "..\game_shared\vgui_loadtga.h"
#include<VGUI_TextImage.h>

#include "ammohistory.h"

#define WEAPON_PAINKILLER	24 // should match server's index


#define HEALTH_RIGHT_OFFSET	(XRES(10))
#define HEALTH_DOWN_OFFSET	(YRES(10))
#define HEALTH_MIN_SPACE	(YRES(3)) // minimum space between bars
#define HEALTH_FLASH_TIME	0.3

#define HEALTH_FADE_TIME	5
#define HEALTH_ZERO_ALPHA	150
#define HEALTH_ALPHA		70

// Wargon: ������ ���.
#define USAGE_FADE_TIME 1
#define USAGE_ALPHA 70

const char* weaponNames[NUM_WEAPON_ICONS] = 
{
	"weapon_aps",
	"weapon_9mmhandgun", // glock
	"weapon_aks",
	"weapon_ak74",
	"weapon_9mmAR",
	"weapon_9mmAR_sec", // ar grenades
	"weapon_shotgun",
	"weapon_val",
	"weapon_groza",
	"weapon_rpk",
	"weapon_handgrenade",
	"weapon_rpg", // Wargon: ������ ����������� ��� RPG.
	"machinegun",
};

// Wargon: ������ ���.
int CanUseStatus;
int __MsgFunc_CanUse( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	CanUseStatus = READ_BYTE();
	return 1;
}

void CanUseInit( void )
{
	HOOK_MESSAGE( CanUse );
}

BitmapTGA* LoadResolutionImage (const char *imgname)
{
	BitmapTGA *pBitmap;
	static int resArray[] =
	{
		320, 400, 512, 640, 800,
		1024, 1152, 1280, 1600
	};

	// try to load image directly
	pBitmap = vgui_LoadTGA(imgname);
	if (!pBitmap)
	{
		//resolution based image. Should contain %d substring
		int resArrayIndex = 0;
		int i = 0;
		while ((resArray[i] <= ScreenWidth) && (i < 9))
		{
			resArrayIndex = i;
			i++;
		}

		while(pBitmap == NULL && resArrayIndex >= 0)
		{
			char imgName[64];
			sprintf(imgName, imgname, resArray[resArrayIndex]);
			pBitmap = vgui_LoadTGA(imgName);
			resArrayIndex--;
		}
	}

	return pBitmap;
}


int ShouldDrawHUD()
{
	if (!gHUD.m_pCvarDraw->value)
		return FALSE;

	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly() )
		return FALSE;

	if (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)))
		return TRUE;

	return FALSE;
}


// simple class that owns pointer to a bitmap and draws it
class ImageHolder : public Panel
{
public:
	ImageHolder(const char *imgname, Panel *parent) : Panel(0, 0, 10, 10)
	{
		m_pBitmap = LoadResolutionImage(imgname);
		setParent(parent);
		setPaintBackgroundEnabled(false);
		setVisible(true);
		if (m_pBitmap) m_pBitmap->setPos(0, 0);
	}

	~ImageHolder() {delete m_pBitmap;}
	BitmapTGA *GetBitmap() {return m_pBitmap;}

protected:
	virtual void paint()
	{
		if (ShouldDrawHUD())
		{
			if (m_pBitmap)
				m_pBitmap->doPaint(this);
		}
	}

	BitmapTGA *m_pBitmap;
};


class ShadowLabel : public Label
{
public:
	ShadowLabel(const char* text,int x,int y) : Label(text, x, y) {}

protected:
	virtual void paint()
	{
		int mr, mg, mb, ma;
		int ix, iy;
		getFgColor(mr, mg, mb, ma);
		_textImage->getPos(ix, iy);
		_textImage->setPos(ix+1, iy+1);
		_textImage->setColor( Color(0, 0, 0, ma) );
		_textImage->doPaint(this);
		_textImage->setPos(ix, iy);
		_textImage->setColor( Color(mr, mg, mb, ma) );
		_textImage->doPaint(this);
	}
};


void Hud2Init()
{
//	gEngfuncs.pfnHookUserMsg("RadioIcon", MsgShowRadioIcon);
}

void CHud2::Initialize()
{
	health = -1;
	armor = -1;
	m_fMedkitUpdateTime = 0;
	m_fUsageUpdateTime = 0; // Wargon: ������ ���.
}

CHud2::CHud2() : Panel(0, 0, XRES(640), YRES(480))
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Default Text" );
	Font *pFont = pSchemes->getFont( hTextScheme );

	hTextScheme = pSchemes->getSchemeHandle( "impact" );
	m_pFontDigits = pSchemes->getFont( hTextScheme );

	//
	// create medkits icon
	//
	int mediconypos = YRES(340);
	m_pMedkitsIcon = new CImageLabel("painkiller", XRES(10), mediconypos);
	if (!m_pMedkitsIcon->m_pTGA)
	{
		delete m_pMedkitsIcon;
		m_pMedkitsIcon = NULL;
		m_pMedkitsCount = NULL;
		gEngfuncs.Con_Printf("Painkiller icon cannot be loaded\n");
	}
	else
	{
		m_pMedkitsIcon->setParent(this);
		m_pMedkitsIcon->setPaintBackgroundEnabled(false);
		m_pMedkitsIcon->setVisible(false);

		int bw, bt;
		m_pMedkitsIcon->m_pTGA->getSize(bw, bt);
		m_pMedkitsIcon->setSize(bw, bt);
		m_pMedkitsCount = new ShadowLabel("", XRES(15)+bw, (mediconypos+bt/2)-(m_pFontDigits->getTall()/2));
		m_pMedkitsCount->setSize(XRES(200), YRES(200)); // ?
		m_pMedkitsCount->setContentAlignment(Label::a_northwest); // ?
		m_pMedkitsCount->setTextAlignment(Label::a_northwest); // ?
		m_pMedkitsCount->setParent(this);
		m_pMedkitsCount->setVisible(false);
		m_pMedkitsCount->setFont(m_pFontDigits /*pFont*/);
		m_pMedkitsCount->setFgColor(255, 255, 255, 0);
		m_pMedkitsCount->setPaintBackgroundEnabled(false);
		m_pMedkitsOldNum = 0;
		m_fMedkitUpdateTime = 0;
	}

	//
	// load health bars
	//
	m_pBitmapHealthFull = new ImageHolder("gfx/vgui/%d_health_full.tga", this);
	m_pBitmapHealthEmpty = new ImageHolder("gfx/vgui/%d_health_empty.tga", this);
	m_pBitmapHealthFlash = new ImageHolder("gfx/vgui/%d_health_flash.tga", this);
	if (m_pBitmapHealthFull->GetBitmap())
		m_pBitmapHealthFull->GetBitmap()->getSize(m_iHealthBarWidth, m_iHealthBarHeight);
	else
		m_iHealthBarWidth = m_iHealthBarHeight = 0;

	m_pBitmapArmorFull = new ImageHolder("gfx/vgui/%d_armor_full.tga", this);
	m_pBitmapArmorEmpty = new ImageHolder("gfx/vgui/%d_armor_empty.tga", this);
	m_pBitmapArmorFlash = new ImageHolder("gfx/vgui/%d_armor_flash.tga", this);
	if (m_pBitmapArmorFull->GetBitmap())
		m_pBitmapArmorFull->GetBitmap()->getSize(m_iArmorBarWidth, m_iArmorBarHeight);
	else
		m_iArmorBarWidth = m_iArmorBarHeight = 0;
	
	m_pHealthIcon = new ImageHolder("gfx/vgui/%d_health_icon.tga", this);
	m_pArmorIcon = new ImageHolder("gfx/vgui/%d_armor_icon.tga", this);
	int healthiconwidth = 0, healthiconheight = 0;
	int armoriconwidth = 0, armoriconheight = 0;
	if (m_pHealthIcon->GetBitmap())
	{
		m_pHealthIcon->GetBitmap()->getSize(healthiconwidth, healthiconheight);
		m_pHealthIcon->setSize(healthiconwidth, healthiconheight);
	}

	if (m_pArmorIcon->GetBitmap())
	{
		m_pArmorIcon->GetBitmap()->getSize(armoriconwidth, armoriconheight);
		m_pArmorIcon->setSize(armoriconwidth, armoriconheight);
	}

	int line = ScreenHeight - HEALTH_DOWN_OFFSET;
	if (m_pArmorIcon->GetBitmap())
	{
		if (armoriconheight > m_iArmorBarHeight) line -= armoriconheight/2;
		else line -= m_iArmorBarHeight/2;

		m_pArmorIcon->setPos(HEALTH_RIGHT_OFFSET, line - armoriconheight/2);
		m_iArmorBarXpos = HEALTH_RIGHT_OFFSET + armoriconwidth + YRES(5);
		m_iArmorBarYpos = line - m_iArmorBarHeight/2;

		if (armoriconheight > m_iArmorBarHeight) line = ScreenHeight - HEALTH_DOWN_OFFSET - armoriconheight - HEALTH_MIN_SPACE;
		else line = ScreenHeight - HEALTH_DOWN_OFFSET - m_iArmorBarHeight - HEALTH_MIN_SPACE;
	}
	else
	{
		line -= m_iArmorBarHeight;
		m_iArmorBarXpos = HEALTH_RIGHT_OFFSET + YRES(5);
		m_iArmorBarYpos = line;
		line -= HEALTH_MIN_SPACE;
	}

	if (m_pHealthIcon->GetBitmap())
	{
		if (healthiconheight > m_iHealthBarHeight) line -= healthiconheight/2;
		else line -= m_iHealthBarHeight/2;

		m_pHealthIcon->setPos(HEALTH_RIGHT_OFFSET, line - healthiconheight/2);
		m_iHealthBarXpos = HEALTH_RIGHT_OFFSET + healthiconwidth + YRES(5);
		m_iHealthBarYpos = line - m_iHealthBarHeight/2;
	}
	else
	{
		line -= m_iHealthBarHeight;
		m_iHealthBarXpos = HEALTH_RIGHT_OFFSET + YRES(5);
		m_iHealthBarYpos = line;
	}

	//
	// load ammo icons
	//
	for (int i = 0; i < NUM_WEAPON_ICONS; i++)
	{
		char path[256] = "gfx/vgui/ammo/%d_";
		strcat(path, weaponNames[i]);
		strcat(path, ".tga");
	//	CONPRINT("Loading ammo icon [%s]\n", path);
		m_pWeaponIconsArray[i] = LoadResolutionImage(path);
		if (!m_pWeaponIconsArray[i])
			CONPRINT("Failed to load ammo icon [%s]\n", path);
	}

	//
	// Wargon: ������ ���.
	//
	m_pUsageIcon = new CImageLabel("usage", ScreenWidth / 2 - 12, ScreenHeight / 2 + 100);
	if (!m_pUsageIcon->m_pTGA)
	{
		delete m_pUsageIcon;
		m_pUsageIcon = NULL;
		gEngfuncs.Con_Printf("Usage icon can't be loaded\n");
	}
	else
	{
		int bw, bt;
		m_pUsageIcon->setParent(this);
		m_pUsageIcon->setPaintBackgroundEnabled(false);
		m_pUsageIcon->setVisible(false);
		m_pUsageIcon->m_pTGA->getSize(bw, bt);
		m_pUsageIcon->setSize(bw, bt);
		m_fUsageUpdateTime = 0;
	}
}

CHud2::~CHud2()
{
/*	delete m_pBitmapHealthFull;
	delete m_pBitmapHealthEmpty;
	delete m_pBitmapHealthFlash;    they are childrens now
	delete m_pBitmapArmorFull;
	delete m_pBitmapArmorEmpty;
	delete m_pBitmapArmorFlash;*/

	for (int i = 0; i < NUM_WEAPON_ICONS; i++)
	{
		delete m_pWeaponIconsArray[i];
	}
}

void CHud2::paintBackground()
{
//	Panel::paintBackground();
}


// buz: I dunno exactly, what solve() function does. I just wanna to find
// some function, who being called each frame, to put panels position setting in there.
// I've tried paintBackground, Chud::Redraw, CHud::Think, and some others, but
// health and armor bars sizes are jumping during interpolation and
// panels motion looks jerky (especially in steam version).
// Putting this in solve() seems to fix first problem.
void CHud2::solve()
{
	float curtime = gEngfuncs.GetClientTime();

	// Wargon: ������ ���.
	if (m_pUsageIcon)
	{
		m_fUsageUpdateTime = curtime;
		if (CanUseStatus && gHUD.m_pCvarDraw->value && !(gViewPort && gViewPort->m_pParanoiaText && gViewPort->m_pParanoiaText->isVisible()))
		{
			m_pUsageIcon->setVisible(true);
			if (m_fUsageUpdateTime > curtime)
				m_fUsageUpdateTime = curtime;
			float frac = curtime - m_fUsageUpdateTime;
			int alpha = USAGE_ALPHA;
			if (frac < USAGE_FADE_TIME)
			{
				frac = frac / USAGE_FADE_TIME;
				alpha = (int)(frac * USAGE_ALPHA);
			}
			m_pUsageIcon->m_pTGA->setColor(Color(255, 255, 255, alpha));
		}
		else
			m_pUsageIcon->setVisible(false);
	}

	// update medkits counter
	if (m_pMedkitsIcon)
	{
		WEAPON *pPainkillers = gWR.GetWeapon(WEAPON_PAINKILLER);
		int pkcount = gWR.CountAmmo(pPainkillers->iAmmoType);
		if (pkcount != m_pMedkitsOldNum)
		{
			char temp[16];
			sprintf(temp, "%d", pkcount);
			m_pMedkitsCount->setText(temp);
			m_pMedkitsOldNum = pkcount;
			m_fMedkitUpdateTime = curtime;
		}

		if (pkcount && ShouldDrawHUD())
		{
			m_pMedkitsIcon->setVisible(true);
			m_pMedkitsCount->setVisible(true);

			if (m_fMedkitUpdateTime > curtime) // fix possible map change bugs
				m_fMedkitUpdateTime = curtime;

			float frac = curtime - m_fMedkitUpdateTime;
			int alpha = HEALTH_ALPHA;
			if (frac < HEALTH_FADE_TIME)
			{
				frac = frac / HEALTH_FADE_TIME;
				alpha = (int)(frac * HEALTH_ALPHA);
			}
			m_pMedkitsIcon->m_pTGA->setColor(Color(255, 255, 255, alpha));
			m_pMedkitsCount->setFgColor(255, 255, 255, alpha);
		}
		else
		{
			m_pMedkitsIcon->setVisible(false);
			m_pMedkitsCount->setVisible(false);
		}
	}

	//
	// update health and armor bars
	// (damn, it's so messy...)

	int healthdiv;
	
	// health bar
	if (m_pBitmapHealthEmpty->GetBitmap() && m_pBitmapHealthFull->GetBitmap())
	{
		m_pBitmapHealthFlash->setVisible(false);
		m_pBitmapHealthEmpty->GetBitmap()->setColor(Color(255, 255, 255, 0));
		m_pBitmapHealthFull->GetBitmap()->setColor(Color(255, 255, 255, 0));
		if (m_pHealthIcon->GetBitmap())
			m_pHealthIcon->GetBitmap()->setColor(Color(255, 255, 255, 0));

		if (curtime >= m_fHealthUpdateTime + HEALTH_FLASH_TIME)
		{
			healthdiv = (int)((float)health/100 * m_iHealthBarWidth);
			float frac = curtime - m_fHealthUpdateTime - HEALTH_FLASH_TIME;
			int targetalpha = HEALTH_ALPHA;
			if (health == 0) targetalpha = HEALTH_ZERO_ALPHA;
			int alpha = targetalpha;
			if (frac < HEALTH_FADE_TIME)
			{
				frac = frac / HEALTH_FADE_TIME;
				alpha = (int)(frac * targetalpha);
			}
			m_pBitmapHealthEmpty->GetBitmap()->setColor(Color(255, 255, 255, alpha));
			m_pBitmapHealthFull->GetBitmap()->setColor(Color(255, 255, 255, alpha));
			if (m_pHealthIcon->GetBitmap())
				m_pHealthIcon->GetBitmap()->setColor(Color(255, 255, 255, alpha));
			
		}
		else
		{
			float frac = (curtime - m_fHealthUpdateTime) / HEALTH_FLASH_TIME;
			if ((health < oldhealth) && m_pBitmapHealthFlash->GetBitmap()) // we had take damage, make red flash
			{
				m_pBitmapHealthFlash->setVisible(true);
				m_pBitmapHealthFlash->GetBitmap()->setColor(Color(255, 255, 255, 255*frac));
			}
			frac = 1 - frac;
			frac *= frac;
			healthdiv = health - (int)((float)(health - oldhealth)*frac);
			healthdiv = (int)((float)healthdiv/100 * m_iHealthBarWidth);
		}

		m_pBitmapHealthFull->setBounds(m_iHealthBarXpos, m_iHealthBarYpos, healthdiv, m_iHealthBarHeight);
		m_pBitmapHealthFlash->setBounds(m_iHealthBarXpos, m_iHealthBarYpos, healthdiv, m_iHealthBarHeight);

		m_pBitmapHealthEmpty->setBounds(m_iHealthBarXpos + healthdiv, m_iHealthBarYpos,
			m_iHealthBarWidth - healthdiv, m_iHealthBarHeight);
		m_pBitmapHealthEmpty->GetBitmap()->setPos( -healthdiv, 0 );
	}

	// armor bar
	if (m_pBitmapArmorEmpty->GetBitmap() && m_pBitmapArmorFull->GetBitmap())
	{
		m_pBitmapArmorFlash->setVisible(false);
		m_pBitmapArmorEmpty->GetBitmap()->setColor(Color(255, 255, 255, 0));
		m_pBitmapArmorFull->GetBitmap()->setColor(Color(255, 255, 255, 0));
		if (m_pArmorIcon->GetBitmap())
			m_pArmorIcon->GetBitmap()->setColor(Color(255, 255, 255, 0));
		if (curtime >= m_fArmorUpdateTime + HEALTH_FLASH_TIME)
		{
			healthdiv = (int)((float)armor/100 * m_iArmorBarWidth);
			float frac = curtime - m_fArmorUpdateTime - HEALTH_FLASH_TIME;
			int targetalpha = HEALTH_ALPHA;
			if (armor == 0) targetalpha = HEALTH_ZERO_ALPHA;
			int alpha = targetalpha;
			if (frac < HEALTH_FADE_TIME)
			{
				frac = frac / HEALTH_FADE_TIME;
				alpha = (int)(frac * targetalpha);
			}
			m_pBitmapArmorEmpty->GetBitmap()->setColor(Color(255, 255, 255, alpha));
			m_pBitmapArmorFull->GetBitmap()->setColor(Color(255, 255, 255, alpha));
			if (m_pArmorIcon->GetBitmap())
				m_pArmorIcon->GetBitmap()->setColor(Color(255, 255, 255, alpha));
		}
		else
		{
			float frac = (curtime - m_fArmorUpdateTime) / HEALTH_FLASH_TIME;
			if ((armor < oldarmor) && m_pBitmapArmorFlash->GetBitmap()) // we had take damage, make red flash
			{
				m_pBitmapArmorFlash->setVisible(true);
				m_pBitmapArmorFlash->GetBitmap()->setColor(Color(255, 255, 255, 255*frac));
			}
			frac = 1 - frac;
			frac *= frac;
			healthdiv = armor - (int)((float)(armor - oldarmor)*frac);
			healthdiv = (int)((float)healthdiv/100 * m_iArmorBarWidth);
		}

		m_pBitmapArmorFull->setBounds(m_iArmorBarXpos, m_iArmorBarYpos, healthdiv, m_iArmorBarHeight);
		m_pBitmapArmorFlash->setBounds(m_iArmorBarXpos, m_iArmorBarYpos, healthdiv, m_iArmorBarHeight);

		m_pBitmapArmorEmpty->setBounds(m_iArmorBarXpos + healthdiv, m_iArmorBarYpos,
			m_iArmorBarWidth - healthdiv, m_iArmorBarHeight);
		m_pBitmapArmorEmpty->GetBitmap()->setPos( -healthdiv, 0 );
	}
	Panel::solve();
}


void CHud2::paint()
{
	//
	// draw ammo counters
	//
	if ( (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))) && ShouldDrawHUD() ) // Wargon: ���������� � �������� �������� ������ ���� hud_draw = 1.
	{
		WEAPON *pw = gHUD.m_Ammo.m_pWeapon; // shorthand
		if (gHUD.m_SpecTank_on)
		{
			int x = ScreenWidth - HEALTH_RIGHT_OFFSET;
			int y = ScreenHeight - HEALTH_DOWN_OFFSET;

			BitmapTGA* pImg = FindAmmoImageForWeapon("machinegun");
			if (pImg)
			{
				int iw, ih;
				pImg->getSize(iw, ih);
				x -= iw;
				pImg->setColor(Color(255, 255, 255, 0)); // TEST
				pImg->setPos(x, y - ih);
				pImg->doPaint(this);
				x -= XRES(6); // make some space between icon and text
				y -= 10;
			}

			if (gHUD.m_SpecTank_Ammo != -1)
			{
				// draw ammo count string
				int tw, th;
				char buf[256];					
				drawSetTextFont(m_pFontDigits);
				drawSetTextColor(250, 250, 250, 0); // TEST

				sprintf(buf, "%d", gHUD.m_SpecTank_Ammo);
				m_pFontDigits->getTextSize(buf, tw, th);
				x -= tw; y -= th;
				drawSetTextPos(x, y); drawPrintText(buf, strlen(buf));
			}
		}
		else if (pw)
		{
			int x = ScreenWidth - HEALTH_RIGHT_OFFSET;
			int y = ScreenHeight - HEALTH_DOWN_OFFSET;

			// Do we have secondary ammo?
			if ((pw->iAmmo2Type > 0) && (gWR.CountAmmo(pw->iAmmo2Type) > 0))
			{
				// Draw the secondary ammo Icon
				char buf[256];
				sprintf(buf, "%s_sec", pw->szName);
				BitmapTGA* pImg = FindAmmoImageForWeapon(buf);
				if (pImg)
				{
					int iw, ih;
					pImg->getSize(iw, ih);
					x -= iw;
					pImg->setColor(Color(255, 255, 255, 0)); // TEST
					pImg->setPos(x, y - ih);
					pImg->doPaint(this);
					x -= XRES(6); // make some space between icon and text
					y -= 10;
				}

				// draw ammo count string
				int tw, th;				
				sprintf(buf, "%d", gWR.CountAmmo(pw->iAmmo2Type));

				m_pFontDigits->getTextSize(buf, tw, th);
				drawSetTextFont(m_pFontDigits);
				drawSetTextColor(250, 250, 250, 0); // TEST
				drawSetTextPos(x - tw, y - th);
				drawPrintText(buf, strlen(buf));
				x = ScreenWidth - HEALTH_RIGHT_OFFSET - XRES(100);
			}

			if (pw->iAmmoType > 0)
			{
				y = ScreenHeight - HEALTH_DOWN_OFFSET;

				// Draw the ammo Icon
				BitmapTGA* pImg = FindAmmoImageForWeapon(pw->szName);
				if (pImg)
				{
					int iw, ih;
					pImg->getSize(iw, ih);
					x -= iw;
					pImg->setColor(Color(255, 255, 255, 0)); // TEST
					pImg->setPos(x, y - ih);
					pImg->doPaint(this);
					x -= XRES(6); // make some space between icon and text
					y -= 10;
				}

				// draw ammo count string
				int tw, th;
				char buf[256];					
				drawSetTextFont(m_pFontDigits);
				drawSetTextColor(250, 250, 250, 0); // TEST

				sprintf(buf, "%d", gWR.CountAmmo(pw->iAmmoType));
				m_pFontDigits->getTextSize(buf, tw, th);
				x -= tw; y -= th;
				drawSetTextPos(x, y); drawPrintText(buf, strlen(buf));

				if (pw->iClip >= 0) // has clip?
				{
					x -= YRES(12);
					drawSetTextPos(x, y); drawPrintText("/", 1);

					sprintf(buf, "%d", pw->iClip);
					m_pFontDigits->getTextSize(buf, tw, th);
					x = x - tw - YRES(5);
					drawSetTextPos(x, y); drawPrintText(buf, strlen(buf));
				}
			}
		}
	}
	
	Panel::paint();
}


BitmapTGA* CHud2::FindAmmoImageForWeapon(const char *wpn)
{
	for (int i = 0; i < NUM_WEAPON_ICONS; i++)
	{
		if (!strcmp(wpn, weaponNames[i]))
			return m_pWeaponIconsArray[i];
	}
	return NULL;
}


void CHud2::UpdateHealth( int newhealth )
{
	if (newhealth == health)
		return;

	if (health == -1) // first update, dont do effects
	{
		health = newhealth;
		m_fHealthUpdateTime = -666;
		return;
	}

	oldhealth = health;
	health = newhealth;
	m_fHealthUpdateTime = gEngfuncs.GetClientTime();
}


void CHud2::UpdateArmor( int newarmor )
{
	if (newarmor == armor)
		return;

	if (armor == -1) // first update, dont do effects
	{
		armor = newarmor;
		m_fArmorUpdateTime = -666;
		return;
	}

	oldarmor = armor;
	armor = newarmor;
	m_fArmorUpdateTime = gEngfuncs.GetClientTime();
}
