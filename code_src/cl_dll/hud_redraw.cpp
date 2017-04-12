/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud_redraw.cpp
//
#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_hud.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

// buz: for IsHardware check
#include "r_studioint.h"
#include "com_model.h"
extern engine_studio_api_t IEngineStudio;

#include "vgui_TeamFortressViewport.h"
#include "vgui_paranoiatext.h"// buz
#include "mp3.h" // buz
#include "gl_renderer.h" // buz

#define MAX_LOGO_FRAMES 56

// buz: spread value
vec3_t g_vSpread;
int g_iGunMode;

int grgLogoFrame[MAX_LOGO_FRAMES] = 
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31 
};

void DrawBlurTest(float frametime); // buz
extern int g_iVisibleMouse;

//float HUD_GetFOV( void ); buz

extern cvar_t *sensitivity;

// Think
void CHud::Think(void)
{
	// buz: calc dead time
	if (gHUD.m_Health.m_iHealth <= 0 && !m_flDeadTime)
	{
		m_flDeadTime = gEngfuncs.GetClientTime();
	}

	// Wargon: Исправление бага эффекта grayscale, остававшегося после загрузки из мертвого состояния.
	if (gHUD.m_Health.m_iHealth > 0)
	{
		m_flDeadTime = 0;
	}

	float targetFOV;
	HUDLIST *pList = m_pHudList;
	static float lasttime = 0;

	while (pList)
	{
		if (pList->p->m_iFlags & HUD_ACTIVE)
			pList->p->Think();
		pList = pList->pNext;
	}

	if		(g_iGunMode == 3)	targetFOV = 30;
	else if	(g_iGunMode == 2)	targetFOV = 60;
	else						targetFOV = 90;

	static float lastFixedFov = 0;

	if (m_flFOV < 0)
	{
		m_flFOV = targetFOV;
		lasttime = gEngfuncs.GetClientTime();
		lastFixedFov = m_flFOV;
	}
	else
	{
		float curtime = gEngfuncs.GetClientTime();
		float mod = targetFOV - m_flFOV;
		if (mod < 0) mod *= -1;
		if (mod < 30) mod = 30;
		if (g_iGunMode == 3 || lastFixedFov == 30) mod *= 2; // хаками халфа полнится (c)
		mod /= 30;

		if (m_flFOV < targetFOV) {
			m_flFOV += (curtime - lasttime) * m_pZoomSpeed->value * mod;
			if (m_flFOV > targetFOV)
			{
				m_flFOV = targetFOV;
				lastFixedFov = m_flFOV;
			}
		}
		else if (m_flFOV > targetFOV) {
			m_flFOV -= (curtime - lasttime) * m_pZoomSpeed->value * mod;
			if (m_flFOV < targetFOV)
			{
				m_flFOV = targetFOV;
				lastFixedFov = m_flFOV;
			}
		}
		lasttime = curtime;
	}

	m_iFOV = m_flFOV;

	// Set a new sensitivity
	if ( m_iFOV == 90)//default_fov->value )// buz: turn off
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * (m_flFOV / (float)90) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}
}

//LRC - fog fading values
extern float g_fFadeDuration;
extern float g_fStartDist;
extern float g_fEndDist;
//extern int g_iFinalStartDist;
extern int g_iFinalEndDist;

void OrthoGasMask(void); // buz
void OrthoScope(void); // buz
void OrthoVGUI(void); // buz
void ApplyPostEffects(); // buz
extern vec3_t g_CrosshairAngle; // buz
extern int g_HeadShieldState; // buz


// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud :: Redraw( float flTime, int intermission )
{
	ApplyPostEffects(); // buz

	gMP3.Frame();

	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static m_flShotTime = 0;

	DrawBlurTest(m_flTimeDelta);

	//LRC - handle fog fading effects. (is this the right place for it?)
	if (g_fFadeDuration)
	{
		// Nicer might be to use some kind of logarithmic fade-in?
		double fFraction = m_flTimeDelta/g_fFadeDuration;
//		g_fStartDist -= (FOG_LIMIT - g_iFinalStartDist)*fFraction;
		g_fEndDist -= (FOG_LIMIT - g_iFinalEndDist)*fFraction;

//		CONPRINT("FogFading: %f - %f, frac %f, time %f, final %d\n", g_fStartDist, g_fEndDist, fFraction, flTime, g_iFinalEndDist);

		// cap it
//		if (g_fStartDist > FOG_LIMIT)				g_fStartDist = FOG_LIMIT;
		if (g_fEndDist   > FOG_LIMIT)				g_fEndDist = FOG_LIMIT;
//		if (g_fStartDist < g_iFinalStartDist)	g_fStartDist = g_iFinalStartDist;
		if (g_fEndDist   < g_iFinalEndDist)		g_fEndDist   = g_iFinalEndDist;
	}
	
	// Clock was reset, reset delta
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;

	// buz: draw gasmask
	if (m_iGasMask)
	{
		if (IEngineStudio.IsHardware())
			OrthoGasMask();
		else
			DrawHudString(XRES(10), YRES(350), XRES(600), "Using Gas Mask", 180, 180, 180);
	}

	if (g_iGunMode == 3) // buz: special sniper scope
	{
		if (IEngineStudio.IsHardware())
			OrthoScope();
	}

	if (!IEngineStudio.IsHardware() && g_HeadShieldState != 1)
		DrawHudString(XRES(10), YRES(350), XRES(600), "Using Head Shield", 180, 180, 180);	

	OrthoVGUI(); // buz: panels background

	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if ( m_iIntermission && !intermission )
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
//			gViewPort->UpdateSpectatorPanel();
		}
		else if ( !m_iIntermission && intermission )
		{
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideVGUIMenu();
			gViewPort->ShowScoreBoard();
//			gViewPort->UpdateSpectatorPanel();

			// Take a screenshot if the client's got the cvar set
			if ( CVAR_GET_FLOAT( "hud_takesshots" ) != 0 )
				m_flShotTime = flTime + 1.0;	// Take a screenshot in a second
		}
	}

	if (m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;
	
	if ( m_pCvarDraw->value )
	{
		HUDLIST *pList = m_pHudList;

		while (pList)
		{
			if ( !intermission )
			{
				if ( (pList->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL) )
					pList->p->Draw(flTime);
			}
			else
			{  // it's an intermission,  so only draw hud elements that are set to draw during intermissions
				if ( pList->p->m_iFlags & HUD_INTERMISSION )
					pList->p->Draw( flTime );
			}

			pList = pList->pNext;
		}
	}

	// are we in demo mode? do we need to draw the logo in the top corner?
	if (m_iLogo)
	{
		int x, y, i;

		if (m_hsprLogo == 0)
			m_hsprLogo = LoadSprite("sprites/%d_logo.spr");

		SPR_Set(m_hsprLogo, 250, 250, 250 );
		
		x = SPR_Width(m_hsprLogo, 0);
		x = ScreenWidth - x;
		y = SPR_Height(m_hsprLogo, 0)/2;

		// Draw the logo at 20 fps
		int iFrame = (int)(flTime * 20) % MAX_LOGO_FRAMES;
		i = grgLogoFrame[iFrame] - 1;

		SPR_DrawAdditive(i, x, y, NULL);
	}

	// buz: draw crosshair
	if ((g_vSpread[0] && g_iGunMode != 3 && m_SpecTank_on == 0) && gHUD.m_pCvarDraw->value) // Wargon: Прицел рисуется только если hud_draw = 1.
	{
		if (gViewPort && gViewPort->m_pParanoiaText && gViewPort->m_pParanoiaText->isVisible())
			return 1;

		int barsize = XRES(g_iGunMode == 1 ? 9 : 6); 
		int hW = ScreenWidth / 2;
		int hH = ScreenHeight / 2;
		float mod = (1/(tan(M_PI/180*(m_iFOV/2))));
		int dir = ((g_vSpread[0] * hW) / 500) * mod;
	//	gEngfuncs.Con_Printf("mod is %f, %d\n", mod, m_iFOV);

		if (g_CrosshairAngle[0] != 0 || g_CrosshairAngle[1] != 0)
		{
			// adjust for autoaim
			hW -= g_CrosshairAngle[1] * (ScreenWidth / m_iFOV);
			hH -= g_CrosshairAngle[0] * (ScreenWidth / m_iFOV);
		}		

		// g_vSpread[2] - is redish [0..500]
		// gEngfuncs.Con_Printf("received spread: %f\n", g_vSpread[2]);
		int c = 255 - (g_vSpread[2] * 0.5);

		FillRGBA(hW - dir - barsize, hH, barsize, 1, 255, c, c, 200);
		FillRGBA(hW + dir, hH, barsize, 1, 255, c, c, 200);
		FillRGBA(hW, hH - dir - barsize, 1, barsize, 255, c, c, 200);
		FillRGBA(hW, hH + dir, 1, barsize, 255, c, c, 200);	
		
	//	FillRGBA(hW - dir, hH - dir, dir*2, dir*2, 20, 150, 20, 100);
	}

	RendererDrawHud();

	/*
	if ( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );
		
		if (m_hsprCursor == 0)
		{
			char sz[256];
			sprintf( sz, "sprites/cursor.spr" );
			m_hsprCursor = SPR_Load( sz );
		}

		SPR_Set(m_hsprCursor, 250, 250, 250 );
		
		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/

	return 1;
}

void ScaleColors( int &r, int &g, int &b, int a )
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

int CHud :: DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b )
{
	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		int next = xpos + gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
		xpos = next;		
	}

	return xpos;
}

int CHud :: DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );

}

// draws a string from right to left (right-aligned)
int CHud :: DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b )
{
	// find the end of the string
	for ( char *szIt = szString; *szIt != 0; szIt++ )
	{ // we should count the length?		
	}

	// iterate throug the string in reverse
	for ( szIt--;  szIt != (szString-1);  szIt-- )	
	{
		int next = xpos - gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next < iMinX )
			return xpos;
		xpos = next;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
	}

	return xpos;
}

int CHud :: DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b)
{
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;

	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			 k = iNumber/100;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100)/10;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_0 + k));
		x += iWidth;
	} 
	else if (iFlags & DHN_DRAWZERO) 
	{
		SPR_Set(GetSprite(m_HUD_number_0), r, g, b );

		// SPR_Draw 100's
		if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		
		SPR_DrawAdditive( 0,  x, y, &GetSpriteRect(m_HUD_number_0));
		x += iWidth;
	}

	return x;
}


int CHud::GetNumWidth( int iNumber, int iFlags )
{
	if (iFlags & (DHN_3DIGITS))
		return 3;

	if (iFlags & (DHN_2DIGITS))
		return 2;

	if (iNumber <= 0)
	{
		if (iFlags & (DHN_DRAWZERO))
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;

}	

// buz
void DrawBlurTest(float frametime)
{
	static int blurstate = 0;
	if (cv_blurtest->value && frametime)
	{
		if (!gl.IsGLAllowed())
		{
			gEngfuncs.Con_Printf("GL effects are not allowed\n");
			gEngfuncs.Cvar_SetValue( "gl_blurtest", 0 );
			return;
		}

		GLint val_r, val_g, val_b;
		gl.glGetIntegerv(GL_ACCUM_RED_BITS, &val_r);
		gl.glGetIntegerv(GL_ACCUM_GREEN_BITS, &val_g);
		gl.glGetIntegerv(GL_ACCUM_BLUE_BITS, &val_b);
		if (!val_r || !val_g || !val_b)
		{
			gEngfuncs.Con_Printf("Accumulation buffer is not present\n");
			gEngfuncs.Cvar_SetValue( "gl_blurtest", 0 );
			return;
		}

		if (!blurstate) // load entire screen first time
		{
			gl.glAccum(GL_LOAD, 1.0);
		}
		else
		{
			float blur = cv_blurtest->value * frametime;
			if (blur < 0) blur = 0;
			if (blur > 1) blur = 1;

			gl.glReadBuffer(GL_BACK);
			gl.glAccum(GL_MULT, 1 - blur); // scale contents of accumulation buffer
			gl.glAccum(GL_ACCUM, blur); // add screen contents
			gl.glAccum(GL_RETURN, 1); // read result back
		}
		blurstate = 1;
	}
	else
		blurstate = 0;
}
