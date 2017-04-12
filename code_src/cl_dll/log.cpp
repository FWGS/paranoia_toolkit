
//#include <windows.h>
#include "hud.h"
#include "cl_util.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "exception.h"

int  log_enabled = 0;
static char szLogFile[256];


void Log (const char * fmt, ...)
{
	if (!log_enabled)
		return;

	va_list va_alist;
	char logbuf[256] = "";
    FILE * fp;
   
	va_start (va_alist, fmt);
	_vsnprintf (logbuf+strlen(logbuf), sizeof(logbuf) - strlen(logbuf), fmt, va_alist);
	va_end (va_alist);

	if ( (fp = fopen (szLogFile, "at")) != NULL )
	{
		fprintf ( fp, "%s", logbuf );
		fclose (fp);
	}
}


void ConLog (const char * fmt, ...)
{
	va_list va_alist;
	char logbuf[256] = "";
    FILE * fp;
   
	va_start (va_alist, fmt);
	_vsnprintf (logbuf+strlen(logbuf), sizeof(logbuf) - strlen(logbuf), fmt, va_alist);
	va_end (va_alist);

	CONPRINT("%s", logbuf);

	if (!log_enabled)
		return;

	if ( (fp = fopen (szLogFile, "at")) != NULL )
	{
		fprintf ( fp, "%s", logbuf );
		fclose (fp);
	}
}


void StartLog()
{
	Log("===============================================\n");
	Log("*   Paranoia client library loaded            *\n");
	Log("===============================================\n\n");
}


void SetupLogging ()
{
	if ( gEngfuncs.CheckParm ("-logkeep", NULL ) ) // dont delete existing log
	{
		log_enabled = TRUE;
		sprintf(szLogFile, "%s/paranoia_log.txt", gEngfuncs.pfnGetGameDirectory());
		StartLog();
	}
	else if ( gEngfuncs.CheckParm ("-log", NULL ) ) // start new log at each client.dll run
	{
		FILE * fp;
		log_enabled = TRUE;
		sprintf(szLogFile, "%s/paranoia_log.txt", gEngfuncs.pfnGetGameDirectory());
		
		if ( (fp = fopen (szLogFile, "wt")) != NULL )
			fclose (fp);

		StartLog();
	}
	else
	{
		log_enabled = FALSE;
	}


//	if (log_enabled)
//		InstallExceptionHandler(); // enable crashes monitoring
}
