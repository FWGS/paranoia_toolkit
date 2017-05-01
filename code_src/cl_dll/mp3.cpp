//mp3 support added by Killar


#include "hud.h"
#include "cl_util.h"
#include "mp3.h"
#include "glmanager.h"

int CMP3::Initialize()
{
	m_flFadeoutStart = 0;
	m_flFadeoutDuration = 0;
	m_iIsPlaying = 0;
	// Wargon: fmod_volume регулирует громкость звуков, воспроизводимых через FMOD.
	m_pVolume = CVAR_CREATE( "fmod_volume", "1.0", FCVAR_ARCHIVE );


#ifdef MUSIC_FMOD
	char fmodlib[256];

	sprintf( fmodlib, "%s/cl_dlls/fmod.dll", gEngfuncs.pfnGetGameDirectory());
	// replace forward slashes with backslashes
	for( int i=0; i < 256; i++ )
		if( fmodlib[i] == '/' ) fmodlib[i] = '\\';
	
	m_hFMod = LoadLibrary( fmodlib );

	if( m_hFMod != NULL )
	{
		// fill in the function pointers
		(FARPROC&)VER = 	GetProcAddress(m_hFMod, "_FSOUND_GetVersion@0");
		(FARPROC&)SCL = 	GetProcAddress(m_hFMod, "_FSOUND_Stream_Close@4");
		(FARPROC&)SOP = 	GetProcAddress(m_hFMod, "_FSOUND_SetOutput@4");
		(FARPROC&)SBS = 	GetProcAddress(m_hFMod, "_FSOUND_SetBufferSize@4");
		(FARPROC&)SDRV = 	GetProcAddress(m_hFMod, "_FSOUND_SetDriver@4");
		(FARPROC&)INIT = 	GetProcAddress(m_hFMod, "_FSOUND_Init@12");
		(FARPROC&)SOF = 	GetProcAddress(m_hFMod, "_FSOUND_Stream_OpenFile@12");
		//(FARPROC&)LNGTH = GetProcAddress(m_hFMod, "_FSOUND_Stream_GetLength@4");
		(FARPROC&)SO = 		GetProcAddress(m_hFMod, "_FSOUND_Stream_Open@16");//AJH Use new version of fmod
		(FARPROC&)SPLAY = 	GetProcAddress(m_hFMod, "_FSOUND_Stream_Play@8");
		(FARPROC&)CLOSE = 	GetProcAddress(m_hFMod, "_FSOUND_Close@0");

		(FARPROC&)SETVOL =	GetProcAddress(m_hFMod, "_FSOUND_SetVolume@8");
		
		if( !(SETVOL && SCL && SOP && SBS && SDRV && INIT && (SOF||SO) && SPLAY && CLOSE) )
		{
			FreeLibrary( m_hFMod );
			m_hFMod = NULL;
			gEngfuncs.Con_Printf("Fatal Error: FMOD functions couldn't be loaded!\n");
			return 0;
		}
	}

	else
	{
		gEngfuncs.Con_Printf("Fatal Error: FMOD library couldn't be loaded!\n");
		return 0;
	}
	gEngfuncs.Con_Printf("FMOD v.%f library loaded.\n", VER());
#endif

	return 1;
}

int CMP3::Shutdown()
{
#ifdef MUSIC_FMOD
	if( m_hFMod )
	{
		CLOSE();

		FreeLibrary( m_hFMod );
		m_hFMod = NULL;
		m_iIsPlaying = 0;
		return 1;
	}
	else
		return 0;
#else
	return 1;
#endif
}

int CMP3::StopMP3( float fade )
{
#ifdef MUSIC_FMOD
	if (!m_hFMod) return 1;
#endif
	if (fade == 0)
	{
#ifdef MUSIC_FMOD
		SCL( m_Stream );
#else
		gEngfuncs.pfnPrimeMusicStream(NULL, NULL);
#endif
		m_iIsPlaying = 0;
		m_flFadeoutStart = 0;
		m_flFadeoutDuration = 0;
//		gEngfuncs.Con_Printf("mp3stop\n");
		return 1;
	}

	m_flFadeoutStart = gEngfuncs.GetClientTime();
	m_flFadeoutDuration = fade;

	return 1;
}

int CMP3::PlayMP3( const char *pszSong )
{
#ifdef MUSIC_FMOD
	if (!m_hFMod) return 0;
#endif
	m_flFadeoutStart = 0;
	m_flFadeoutDuration = 0;

#ifdef MUSIC_FMOD
	// Wargon: fmod_volume регулирует громкость звуков, воспроизводимых через FMOD.
	SETVOL( 0, (m_pVolume->value * 255) );
#elif defined(MUSIC_XASH)
	gRenderfuncs.S_FadeMusicVolume( 100 - m_pVolume->value * 100 );
#endif

	if( m_iIsPlaying )
	{
		if (!strcmp(m_szMP3File, pszSong))
		{
			// buz: we are playing this song now, dont do anything
			return 1;
		}
		
		// sound system is already initialized
#ifdef MUSIC_FMOD
		SCL( m_Stream );
#else
		// don't do anything, PrimeMusicStream will clear out the stream
#endif
	} 
	else
	{
#ifdef MUSIC_FMOD
		SOP( FSOUND_OUTPUT_DSOUND );
		SBS( 200 );
		SDRV( 0 );
		INIT( 44100, 1, 0 ); // we need just one channel, multiple mp3s at a time would be, erm, strange...	
#endif
	}//AJH not for really cool effects, say walking past cars in a street playing different tunes


#ifdef MUSIC_FMOD
//	gEngfuncs.Con_Printf("Using fmod.dll version %f\n",VER());

	char song[256];

	sprintf( song, "%s/%s", gEngfuncs.pfnGetGameDirectory(), pszSong);

	if( SOF )
	{													
		m_Stream = SOF( song, FSOUND_NORMAL | FSOUND_LOOP_NORMAL, 1 );
	}
	else if (SO)
	{
		m_Stream = SO( song, FSOUND_NORMAL | FSOUND_LOOP_NORMAL, 0 ,0);
	}
	if(m_Stream)
	{
		SPLAY( 0, m_Stream );
		m_iIsPlaying = 1;
		strcpy(m_szMP3File, pszSong);
	
	//	int vol = GETVOL(0);
	//	gEngfuncs.Con_Printf("VOLUME IS: %d\n",vol);
		return 1;
	}
	else
	{
		m_iIsPlaying = 0;
		gEngfuncs.Con_Printf("Error: Could not load %s\n",song);
		return 0;
	}
#else
	gEngfuncs.pfnPrimeMusicStream( pszSong, 0 );
#endif
}

// buz - обработка плавного затухания музыки
void CMP3::Frame()
{
	if (m_flFadeoutDuration)
	{
		float delta = (gEngfuncs.GetClientTime() - m_flFadeoutStart) / m_flFadeoutDuration;
		if (delta > 1)
		{
			// fadeout finished, turn off
			StopMP3(0);
			return;
		}
		// Wargon: Затухание начинается с текущей громкости.
#ifdef MUSIC_FMOD
		int vol = (CVAR_GET_FLOAT("fmod_volume") * 255) - (int)(delta * 255);
		SETVOL( 0, vol );
#elif defined(MUSIC_XASH)
		int vol = (m_pVolume->value * 100) - (int)(delta*100);
		gRenderfuncs.S_FadeMusicVolume( 100 - vol / 255 * 100 );
#endif
	}
	// Wargon: Возможность регулировки громкости в реальном времени.
	else
	{
#ifdef MUSIC_FMOD
		SETVOL( 0, (CVAR_GET_FLOAT("fmod_volume") * 255) );
#elif defined(MUSIC_XASH)
		gRenderfuncs.S_FadeMusicVolume( 100 - m_pVolume->value * 100 );
#endif
	}
}
