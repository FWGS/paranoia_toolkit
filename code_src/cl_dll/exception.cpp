//
// exception filer - written by BUzer for HL: Paranoia modification
//
//		2006

#ifdef _WIN32
#include "hud.h"
#include "cl_util.h"
#include "windows.h"
#include "log.h"
#include "stdio.h"


// Unused now - replaced by CrashRpt library, who makes crash dumps

#include "crashrpt.h" // buz

typedef LPVOID (*CRASHPRT_INSTALL_FUNC) (LPGETLOGFILE pfn, LPCTSTR lpTo, LPCTSTR lpSubject);   
typedef void (*CRASHRPT_ADDFILE_FUNC) (LPVOID lpState, LPCTSTR lpFile, LPCTSTR lpDesc);


HINSTANCE	m_hCrashPrt = NULL;
extern int  log_enabled;

void LoadCrashRpt()
{
	char savedCurrentDirectory[512];
	char path[512];

	if (gEngfuncs.CheckParm ("-noexhandl", NULL ))
		return;

	DWORD result = GetCurrentDirectory( 512, savedCurrentDirectory );
	if (!result || result > 512)
	{
		Log("GetCurrentDirectory failed (%d)!\n", result);
		return;
	}

	sprintf( path, "%s/cl_dlls/", gEngfuncs.pfnGetGameDirectory());
	// replace forward slashes with backslashes
	for( int i=0; i < 512; i++ )
		if( path[i] == '/' ) path[i] = '\\';

	SetCurrentDirectory(path);

/*	sprintf( path, "%s/cl_dlls/CrashRpt.dll", gEngfuncs.pfnGetGameDirectory());
	// replace forward slashes with backslashes
	for( int i=0; i < 512; i++ )
		if( path[i] == '/' ) path[i] = '\\';*/
	
	m_hCrashPrt = LoadLibrary( "CrashRpt.dll" /*path*/ );
	SetCurrentDirectory(savedCurrentDirectory);
	if( !m_hCrashPrt )
	{
		Log("Could not load CrashRpt.dll!\n");
		return;
	}

	// fill in the function pointers
	CRASHPRT_INSTALL_FUNC CrashRpt_Install = (CRASHPRT_INSTALL_FUNC)GetProcAddress(m_hCrashPrt, "Install");
	CRASHRPT_ADDFILE_FUNC CrashRpt_AddFile = (CRASHRPT_ADDFILE_FUNC)GetProcAddress(m_hCrashPrt, "AddFile");

	if (!CrashRpt_Install || !CrashRpt_AddFile)
	{
		Log("Could not load functions from CrashRpt.dll!\n");
		return;
	}

	LPVOID lpvState = CrashRpt_Install(NULL, NULL, NULL);
	if (log_enabled)
	{
		sprintf( path, "%s/paranoia_log.txt", gEngfuncs.pfnGetGameDirectory());
		// replace forward slashes with backslashes
		for( int i=0; i < 512; i++ )
			if( path[i] == '/' ) path[i] = '\\';

		CrashRpt_AddFile(lpvState, path, "Game log file");
	}
	Log("CrashRpt installed\n");
}


void UnloadCrashRpt()
{
	if (m_hCrashPrt)
	{
		Log("Free CrashRpt.dll library\n");
		FreeLibrary( m_hCrashPrt );
		m_hCrashPrt = NULL;
	}
}



typedef struct _MODULEINFO {  LPVOID lpBaseOfDll;  DWORD SizeOfImage;  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

typedef BOOL (APIENTRY *ENUMPROCESSMODULESFUNC)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded);
typedef BOOL (APIENTRY *GETMODULEINFORMATIONPROC)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);



typedef struct exception_codes_s {
	DWORD		exCode;
	const char*	exName;
	const char*	exDescription;
} exception_codes;

exception_codes except_info[] = {
	{EXCEPTION_ACCESS_VIOLATION,		"ACCESS VIOLATION",
	"The thread tried to read from or write to a virtual address for which it does not have the appropriate access."},

	{EXCEPTION_ARRAY_BOUNDS_EXCEEDED,	"ARRAY BOUNDS EXCEEDED",
	"The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking."},

	{EXCEPTION_BREAKPOINT,				"BREAKPOINT",
	"A breakpoint was encountered."},

	{EXCEPTION_DATATYPE_MISALIGNMENT,	"DATATYPE MISALIGNMENT",
	"The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on."},

	{EXCEPTION_FLT_DENORMAL_OPERAND,	"FLT DENORMAL OPERAND",
	"One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value. "},

	{EXCEPTION_FLT_DIVIDE_BY_ZERO,		"FLT DIVIDE BY ZERO",
	"The thread tried to divide a floating-point value by a floating-point divisor of zero. "},

	{EXCEPTION_FLT_INEXACT_RESULT,		"FLT INEXACT RESULT",
	"The result of a floating-point operation cannot be represented exactly as a decimal fraction. "},

	{EXCEPTION_FLT_INVALID_OPERATION,	"FLT INVALID OPERATION",
	"This exception represents any floating-point exception not included in this list. "},

	{EXCEPTION_FLT_OVERFLOW,			"FLT OVERFLOW",
	"The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type. "},

	{EXCEPTION_FLT_STACK_CHECK,			"FLT STACK CHECK",
	"The stack overflowed or underflowed as the result of a floating-point operation. "},
	
	{EXCEPTION_FLT_UNDERFLOW,			"FLT UNDERFLOW",
	"The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type. "},
	
	{EXCEPTION_ILLEGAL_INSTRUCTION,		"ILLEGAL INSTRUCTION",
	"The thread tried to execute an invalid instruction. "},
	
	{EXCEPTION_IN_PAGE_ERROR,			"IN PAGE ERROR",
	"The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network. "},
	
	{EXCEPTION_INT_DIVIDE_BY_ZERO,		"INT DIVIDE BY ZERO",
	"The thread tried to divide an integer value by an integer divisor of zero. "},
	
	{EXCEPTION_INT_OVERFLOW,			"INT OVERFLOW",
	"The result of an integer operation caused a carry out of the most significant bit of the result. "},
	
	{EXCEPTION_INVALID_DISPOSITION,		"INVALID DISPOSITION",
	"An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception. "},

	{EXCEPTION_NONCONTINUABLE_EXCEPTION,"NONCONTINUABLE EXCEPTION",
	"The thread tried to continue execution after a noncontinuable exception occurred. "},

	{EXCEPTION_PRIV_INSTRUCTION,		"PRIV INSTRUCTION",
	"The thread tried to execute an instruction whose operation is not allowed in the current machine mode. "},

	{EXCEPTION_SINGLE_STEP,				"SINGLE STEP",
	"A trace trap or other single-instruction mechanism signaled that one instruction has been executed. "},

	{EXCEPTION_STACK_OVERFLOW,			"STACK OVERFLOW",
	"The thread used up its stack. "}
};


void GetExceptionStrings( DWORD code, const char* *pName, const char* *pDescription )
{
	int i;
	int count = sizeof(except_info) / sizeof(exception_codes);
	for (i = 0; i < count; i++)
	{
		if (code == except_info[i].exCode)
		{
			*pName = except_info[i].exName;
			*pDescription = except_info[i].exDescription;
			return;
		}
	}

	*pName = "Unknown exception";
	*pDescription = "n/a";
}


static LONG WINAPI _exceptionCB(EXCEPTION_POINTERS *ExceptionInfo)
{
    EXCEPTION_RECORD* pRecord = ExceptionInfo->ExceptionRecord;

	const char *pName, *pDescription;
	GetExceptionStrings( pRecord->ExceptionCode, &pName, &pDescription );

	Log("\n============== !!! Unhandled Exception !!! ===============\n");
	Log("Exception code: %s (%p)\n", pName, pRecord->ExceptionCode);
	Log("Address: %p\n", pRecord->ExceptionAddress);

	if (pRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if (pRecord->ExceptionInformation[0])
		{
			Log("Info: the thread attempted to write to an inaccessible address %p\n",
				pRecord->ExceptionInformation[1]);
		}
		else
		{
			Log("Info: the thread attempted to read the inaccessible data at %p\n",
				pRecord->ExceptionInformation[1]);
		}
	}

	Log("Description: %s\n", pDescription);
	Log("\nModules listing:\n");

	// show modules list
	HMODULE PsapiLib = LoadLibrary("psapi.dll");
	if (PsapiLib)
	{
		ENUMPROCESSMODULESFUNC		EnumProcessModules; 
		GETMODULEINFORMATIONPROC	GetModuleInformation;
		EnumProcessModules = (ENUMPROCESSMODULESFUNC)GetProcAddress (PsapiLib, "EnumProcessModules");
		GetModuleInformation = (GETMODULEINFORMATIONPROC)GetProcAddress (PsapiLib, "GetModuleInformation");
		if (EnumProcessModules && GetModuleInformation)
		{
			DWORD needBytes;
			HMODULE hModules[1024];
			if (EnumProcessModules( GetCurrentProcess(), hModules, sizeof(hModules), &needBytes ))
			{
				if (needBytes <= sizeof(hModules))
				{
					DWORD i;
					DWORD numModules = needBytes / sizeof(HMODULE);

					for(i = 0; i < numModules; i++)
					{
						char modname[MAX_PATH];
						if (GetModuleFileName(hModules[i], modname, MAX_PATH))
						{
							modname[MAX_PATH-1] = 0;
							Log("%s : ", modname);
						}
						else
							Log("<error> : ");

						MODULEINFO modInfo;
						if (GetModuleInformation(GetCurrentProcess(), hModules[i], &modInfo, sizeof(modInfo)))
						{
							Log("Base address: %p, Image size: %p\n", modInfo.lpBaseOfDll, modInfo.SizeOfImage);
						}
						else
							Log("<error>\n");
					}
				}
				else
					Log("Too many modules loaded!\n");
			}
			else
				Log("EnumProcessModules failed!\n");
		}
		else
			Log("Can't import functions from psapi.dll!\n");

		FreeLibrary(PsapiLib);
	}
	else
		Log("Unable to load psapi.dll!\n");

	Log("==========================================================\n\n");

    return EXCEPTION_EXECUTE_HANDLER;
}



typedef LONG (WINAPI *EXCEPTHANDLER)(EXCEPTION_POINTERS *ExceptionInfo);
static EXCEPTHANDLER oldHandler = NULL;
static int handler_installed = 0;

void InstallExceptionHandler()
{
	Log("Installing exception handler...\n");
//	SetErrorMode(SEM_NOGPFAULTERRORBOX);
	oldHandler = SetUnhandledExceptionFilter(_exceptionCB);
	if (oldHandler)
	{
	//	MessageBox(NULL, "Some filter is already installed!", "Warning!", MB_OK);
		Log("Saved previous filter at %p\n", oldHandler);
	}
	Log("Handler installed\n");
	handler_installed = 1;
}


void UnInstallExceptionHandler()
{
	if (handler_installed)
	{
		Log("Removing exception handler...");
		SetUnhandledExceptionFilter(oldHandler);
		Log("ok\n");
	}
}
#endif // WIN32
