#include "SR_Base.h"
#include "Redirections.h"
#include "Logging.h"
#include "StringUtils.h"
#include "Config.h"
#include "WindowsUtils.h"
#include "PlatformDefinitions.h"

#include <ShlObj.h>
#include <stdbool.h>

#define BASE_NAME_SKYRIM_INI_W        L"SKYRIM.INI"
#define BASE_NAME_SKYRIM_PREFS_INI_W  L"SKYRIMPREFS.INI"
#define BASE_NAME_SKYRIM_CUSTOM_INI_W L"SKYRIMCUSTOM.INI"
#define BASE_NAME_PLUGINS_TXT_W       L"PLUGINS.TXT"

#define BASE_NAME_SKYRIM_INI_A         "SKYRIM.INI"
#define BASE_NAME_SKYRIM_PREFS_INI_A   "SKYRIMPREFS.INI"
#define BASE_NAME_SKYRIM_CUSTOM_INI_A  "SKYRIMCUSTOM.INI"
#define BASE_NAME_PLUGINS_TXT_A        "PLUGINS.TXT"

#define PATH_SKYRIM_INI_W            L"MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_W L"\\SKYRIM.INI"
#define PATH_SKYRIM_PREFS_INI_W      L"MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_W L"\\SKYRIMPREFS.INI"
#define PATH_SKYRIM_CUSTOM_INI_W     L"MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_W L"\\SKYRIMCUSTOM.INI"
#define PATH_PLUGINS_TXT_W           L"\\SKYRIM" SR_FOLDER_SUFFIX_W L"\\PLUGINS.TXT"

#define PATH_SKYRIM_INI_A             "MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_A "\\SKYRIM.INI"
#define PATH_SKYRIM_PREFS_INI_A       "MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_A "\\SKYRIMPREFS.INI"
#define PATH_SKYRIM_CUSTOM_INI_A      "MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_A "\\SKYRIMCUSTOM.INI"
#define PATH_PLUGINS_TXT_A            "\\SKYRIM" SR_FOLDER_SUFFIX_A "\\PLUGINS.TXT"

// +==================================================================+
// |                         Redirect support                         |
// +==================================================================+


// This variable contains a copy of every path stored in the user config, converted
// to the windows ANSI codepage.
// This allows functions to pass a pointer to the Windows API without allocating a new
// string at every ANSI call just to convert a Unicode string to ANSI.
static struct
{
	char* Ini;
	char* PrefsIni;
	char* CustomIni;
	char* Plugins;

} UserConfigA;

// Narrow version of the canonicized path Skyrim will use to search for plugins.txt
static char* SkyrimPluginsPathA = NULL;
// Wide version of the canonicized path Skyrim will use to search for plugins.txt
static wchar_t* SkyrimPluginsPathW = NULL;


// Checks if the canonical version of a wide path ends with a specified wide string.
static bool CanonicalEndsWithW(const wchar_t* path, const wchar_t* component)
{
	wchar_t* canonical = SR_CanonicizePathW(path);

	bool result = SR_EndsWithW(canonical, component);

	free(canonical);
	return result;
}

// Checks if the canonical version of a narrow path ends with a specified narrow string.
static bool CanonicalEndsWithA(const char* path, const char* component)
{
	char* canonical = SR_CanonicizePathA(path);

	bool result = SR_EndsWithA(canonical, component);

	free(canonical);
	return result;
}

// Checks if the canonical version of a wide path is equal to a specified wide string
static bool CanonicalEqualsW(const wchar_t* path, const wchar_t* other)
{
	wchar_t* canonical = SR_CanonicizePathW(path);

	bool result = wcscmp(canonical, other) == 0;

	free(canonical);
	return result;
}

// Checks if the canonical version of a narrow path is equal to a specified narrow string
static bool CanonicalEqualsA(const char* path, const char* other)
{
	char* canonical = SR_CanonicizePathA(path);

	bool result = strcmp(canonical, other) == 0;

	free(canonical);
	return result;
}

// Tries to redirect a wide path. If the path can't be redirected, it is returned unchanged.
// The returned string does not need to be freed.
static const wchar_t* TryRedirectW(const wchar_t* input)
{
	const wchar_t* fileName = SR_GetFileNameW(input);

	// Canonicizing a path is expensive
	// Match the file name first to avoid canonicizing a path whenever possible

	if (SR_AreCaseInsensitiveEqualW(fileName, BASE_NAME_SKYRIM_INI_W))
	{
		if (CanonicalEndsWithW(input, PATH_SKYRIM_INI_W))
			return SR_GetUserConfig()->Redirection.Ini;
	}
	else if (SR_AreCaseInsensitiveEqualW(fileName, BASE_NAME_SKYRIM_PREFS_INI_W))
	{
		if (CanonicalEndsWithW(input, PATH_SKYRIM_PREFS_INI_W))
			return SR_GetUserConfig()->Redirection.PrefsIni;
	}
	else if (SR_AreCaseInsensitiveEqualW(fileName, BASE_NAME_SKYRIM_CUSTOM_INI_W))
	{
		if (CanonicalEndsWithW(input, PATH_SKYRIM_CUSTOM_INI_W))//This was PATH_SKYRIM_CUSTOM_INI_A, but I think this is more appropriate.
			return SR_GetUserConfig()->Redirection.CustomIni;
	}
	else if (SR_AreCaseInsensitiveEqualW(fileName, BASE_NAME_PLUGINS_TXT_W))
	{
		if (CanonicalEqualsW(input, SkyrimPluginsPathW))
			return SR_GetUserConfig()->Redirection.Plugins;
	}
	/*else if (SR_AreCaseInsensitiveEqualW(fileName, L"PAPYRUS.0.LOG"))
	{
		if (CanonicalEqualsW(input, L"MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_W L"\\LOGS\\SCRIPT\\PAPYRUS.0.LOG"))
			return L"My Games\\Skyblivion\\Logs\\Script\\Papyrus.0.log";
	}*///See notes in below method. I've never witnessed the above if statement being true.

	return input;
}

// Tries to redirect a narrow path. If the path can't be redirected, it is returned unchanged.
// The returned string does not need to be freed.
static const char* TryRedirectA(const char* input)
{
	const char* fileName = SR_GetFileNameA(input);

	// Canonicizing a path is expensive
	// Match the file name first to avoid canonicizing a path whenever possible

	if (SR_AreCaseInsensitiveEqualA(fileName, BASE_NAME_SKYRIM_INI_A))
	{
		if (CanonicalEndsWithA(input, PATH_SKYRIM_INI_A))
			return UserConfigA.Ini;
	}
	else if (SR_AreCaseInsensitiveEqualA(fileName, BASE_NAME_SKYRIM_PREFS_INI_A))
	{
		if (CanonicalEndsWithA(input, PATH_SKYRIM_PREFS_INI_A))
			return UserConfigA.PrefsIni;
	}
	else if (SR_AreCaseInsensitiveEqualA(fileName, BASE_NAME_SKYRIM_CUSTOM_INI_A))
	{
		if (CanonicalEndsWithA(input, PATH_SKYRIM_CUSTOM_INI_A))
			return UserConfigA.CustomIni;
	}
	else if (SR_AreCaseInsensitiveEqualA(fileName, BASE_NAME_PLUGINS_TXT_A))
	{
		if (CanonicalEqualsA(input, SkyrimPluginsPathA))
			return UserConfigA.Plugins;
	}
	//This works, but I'm not sure it's worth it. I could not seem to redirect Skyblivion.log calls, so I think all I can redirect all the Papyrus logs.
	//It might just be best to let all of our log files go to Skyrim's folder.
	//If I ever use the below, I will have to make it more efficient by caching some values.
	/*else if (SR_AreCaseInsensitiveEqualA(fileName, "PAPYRUS.0.LOG"))
	{
		wchar_t* documents = SR_GetKnownFolder(&FOLDERID_Documents);
		wchar_t* uncanonicizedPath = SR_Concat(2, documents, L"\\MY GAMES\\SKYRIM" SR_FOLDER_SUFFIX_W L"\\LOGS\\SCRIPT\\PAPYRUS.0.LOG");
		wchar_t* canonicizedPath = SR_CanonicizePathW(uncanonicizedPath);
		free(uncanonicizedPath);
		if (CanonicalEqualsA(input, SR_Utf16ToCodepage(canonicizedPath))) {
			wchar_t* newPath = SR_Concat(2, documents, L"\\My Games\\Skyblivion\\Logs\\Script\\Papyrus.0.log");
			SR_Log(SR_LOG_LEVEL_INFO, newPath);
			return SR_Utf16ToCodepage(newPath);
		}
	}*/

	return input;
}
/*
+==================================================================+
|                        Redirect functions                        |
+==================================================================+
| The functions below are used by the game instead of the          |
| actual Windows API.                                              |
+------------------------------------------------------------------+
*/

/*
The following macro is be used as: REDIRECT(WinAPI function name, WinAPI return value, WinAPI arguments)
It creates three definitions:

  1. A typedef for a function pointer of the specified API, called (name)_t

  2. A static variable of the type just typedef'd, called SR_Original_(name)
	 This variable should store the original WinAPI being redirected, and can
	 be used to call the original API from inside the redirect.

  3. A function signature identical to the API being redirected, called
	 SR_Redirect_(name)
*/


#define REDIRECT(name, ret, ...) typedef ret(WINAPI *##name##_t)(__VA_ARGS__); \
	static name##_t SR_Original_##name; \
	ret WINAPI SR_Redirect_##name(__VA_ARGS__)

REDIRECT(CreateFileA, HANDLE, LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

REDIRECT(CreateFileW, HANDLE, LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

REDIRECT(OpenFile, HFILE, LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_OpenFile(lpFileName, lpReOpenBuff, uStyle);
}

REDIRECT(GetPrivateProfileStringA, DWORD, LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_GetPrivateProfileStringA(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
}

REDIRECT(GetPrivateProfileStringW, DWORD, LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_GetPrivateProfileStringW(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
}

REDIRECT(GetPrivateProfileIntA, UINT, LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_GetPrivateProfileIntA(lpAppName, lpKeyName, nDefault, lpFileName);
}

REDIRECT(GetPrivateProfileIntW, UINT, LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_GetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, lpFileName);
}

REDIRECT(GetPrivateProfileSectionA, DWORD, LPCSTR lpAppName, LPSTR  lpReturnedString, DWORD  nSize, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_GetPrivateProfileSectionA(lpAppName, lpReturnedString, nSize, lpFileName);
}

REDIRECT(GetPrivateProfileSectionW, DWORD, LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_GetPrivateProfileSectionW(lpAppName, lpReturnedString, nSize, lpFileName);
}

REDIRECT(GetPrivateProfileStructA, BOOL, LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT   uSizeStruct, LPCSTR szFile)
{
	szFile = TryRedirectA(szFile);
	return SR_Original_GetPrivateProfileStructA(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile);
}

REDIRECT(GetPrivateProfileStructW, BOOL, LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile)
{
	szFile = TryRedirectW(szFile);
	return SR_Original_GetPrivateProfileStructW(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile);
}

REDIRECT(GetPrivateProfileSectionNamesA, DWORD, LPSTR  lpszReturnBuffer, DWORD  nSize, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_GetPrivateProfileSectionNamesA(lpszReturnBuffer, nSize, lpFileName);
}

REDIRECT(GetPrivateProfileSectionNamesW, DWORD, LPWSTR  lpszReturnBuffer, DWORD   nSize, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_GetPrivateProfileSectionNamesW(lpszReturnBuffer, nSize, lpFileName);
}

REDIRECT(WritePrivateProfileSectionA, BOOL, LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_WritePrivateProfileSectionA(lpAppName, lpString, lpFileName);
}

REDIRECT(WritePrivateProfileSectionW, BOOL, LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_WritePrivateProfileSectionW(lpAppName, lpString, lpFileName);
}

REDIRECT(WritePrivateProfileStringA, BOOL, LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_WritePrivateProfileStringA(lpAppName, lpKeyName, lpString, lpFileName);
}

REDIRECT(WritePrivateProfileStringW, BOOL, LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_WritePrivateProfileStringW(lpAppName, lpKeyName, lpString, lpFileName);
}

REDIRECT(WritePrivateProfileStructA, BOOL, LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT   uSizeStruct, LPCSTR szFile)
{
	szFile = TryRedirectA(szFile);
	return SR_Original_WritePrivateProfileStructA(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile);
}

REDIRECT(WritePrivateProfileStructW, BOOL, LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID  lpStruct, UINT    uSizeStruct, LPCWSTR szFile)
{
	szFile = TryRedirectW(szFile);
	return SR_Original_WritePrivateProfileStructW(lpszSection, lpszKey, lpStruct, uSizeStruct, szFile);
}

REDIRECT(GetFileAttributesA, DWORD, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_GetFileAttributesA(lpFileName);
}

REDIRECT(GetFileAttributesW, DWORD, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_GetFileAttributesW(lpFileName);
}

REDIRECT(GetFileAttributesExA, BOOL, LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_GetFileAttributesExA(lpFileName, fInfoLevelId, lpFileInformation);
}

REDIRECT(GetFileAttributesExW, BOOL, LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
}

REDIRECT(SetFileAttributesA, BOOL, LPCSTR lpFileName, DWORD dwFileAttributes)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_SetFileAttributesA(lpFileName, dwFileAttributes);
}

REDIRECT(SetFileAttributesW, BOOL, LPCWSTR lpFileName, DWORD dwFileAttributes)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_SetFileAttributesW(lpFileName, dwFileAttributes);
}

REDIRECT(CopyFileA, BOOL, LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists)
{
	lpExistingFileName = TryRedirectA(lpExistingFileName);
	lpExistingFileName = TryRedirectA(lpNewFileName);
	return SR_Original_CopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);
}

REDIRECT(CopyFileW, BOOL, LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
	lpExistingFileName = TryRedirectW(lpExistingFileName);
	lpExistingFileName = TryRedirectW(lpNewFileName);
	return SR_Original_CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

REDIRECT(CopyFileExA, BOOL, LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags)
{
	lpExistingFileName = TryRedirectA(lpExistingFileName);
	lpExistingFileName = TryRedirectA(lpNewFileName);
	return SR_Original_CopyFileExA(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

REDIRECT(CopyFileExW, BOOL, LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags)
{
	lpExistingFileName = TryRedirectW(lpExistingFileName);
	lpExistingFileName = TryRedirectW(lpNewFileName);
	return SR_Original_CopyFileExW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

REDIRECT(CreateHardLinkA, BOOL, LPCSTR lpFileName, LPCSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	lpFileName = TryRedirectA(lpFileName);
	lpExistingFileName = TryRedirectA(lpExistingFileName);
	return SR_Original_CreateHardLinkA(lpFileName, lpExistingFileName, lpSecurityAttributes);
}

REDIRECT(CreateHardLinkW, BOOL, LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	lpFileName = TryRedirectW(lpFileName);
	lpExistingFileName = TryRedirectW(lpExistingFileName);
	return SR_Original_CreateHardLinkW(lpFileName, lpExistingFileName, lpSecurityAttributes);
}

REDIRECT(CreateSymbolicLinkA, BOOLEAN, LPCSTR lpSymlinkFileName, LPCSTR lpTargetFileName, DWORD dwFlags)
{
	lpSymlinkFileName = TryRedirectA(lpSymlinkFileName);
	lpTargetFileName = TryRedirectA(lpTargetFileName);
	return SR_Original_CreateSymbolicLinkA(lpSymlinkFileName, lpTargetFileName, dwFlags);
}

REDIRECT(CreateSymbolicLinkW, BOOLEAN, LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
	lpSymlinkFileName = TryRedirectW(lpSymlinkFileName);
	lpTargetFileName = TryRedirectW(lpTargetFileName);
	return SR_Original_CreateSymbolicLinkW(lpSymlinkFileName, lpTargetFileName, dwFlags);
}

REDIRECT(DeleteFileA, BOOL, LPCSTR lpFileName)
{
	lpFileName = TryRedirectA(lpFileName);
	return SR_Original_DeleteFileA(lpFileName);
}

REDIRECT(DeleteFileW, BOOL, LPCWSTR lpFileName)
{
	lpFileName = TryRedirectW(lpFileName);
	return SR_Original_DeleteFileW(lpFileName);
}

REDIRECT(MoveFileA, BOOL, LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
	lpExistingFileName = TryRedirectA(lpExistingFileName);
	lpNewFileName = TryRedirectA(lpNewFileName);
	return SR_Original_MoveFileA(lpExistingFileName, lpNewFileName);
}

REDIRECT(MoveFileW, BOOL, LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	lpExistingFileName = TryRedirectW(lpExistingFileName);
	lpNewFileName = TryRedirectW(lpNewFileName);
	return SR_Original_MoveFileW(lpExistingFileName, lpNewFileName);
}

REDIRECT(MoveFileExA, BOOL, LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags)
{
	lpExistingFileName = TryRedirectA(lpExistingFileName);
	lpNewFileName = TryRedirectA(lpNewFileName);
	return SR_Original_MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}

REDIRECT(MoveFileExW, BOOL, LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
	lpExistingFileName = TryRedirectW(lpExistingFileName);
	lpNewFileName = TryRedirectW(lpNewFileName);
	return SR_Original_MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

REDIRECT(MoveFileWithProgressA, BOOL, LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags)
{
	lpExistingFileName = TryRedirectA(lpExistingFileName);
	lpNewFileName = TryRedirectA(lpNewFileName);
	return SR_Original_MoveFileWithProgressA(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, dwFlags);
}

REDIRECT(MoveFileWithProgressW, BOOL, LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags)
{
	lpExistingFileName = TryRedirectW(lpExistingFileName);
	lpNewFileName = TryRedirectW(lpNewFileName);
	return SR_Original_MoveFileWithProgressW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, dwFlags);
}

#undef REDIRECT

// +==================================================================+
// |                      End Redirect functions                      |
// +==================================================================+

static void CreatePaths()
{
	const SR_UserConfig* config = SR_GetUserConfig();

	UserConfigA.Ini = SR_Utf16ToCodepage(config->Redirection.Ini);
	UserConfigA.PrefsIni = SR_Utf16ToCodepage(config->Redirection.PrefsIni);
	UserConfigA.CustomIni = SR_Utf16ToCodepage(config->Redirection.CustomIni);
	UserConfigA.Plugins = SR_Utf16ToCodepage(config->Redirection.Plugins);

	wchar_t* appData = SR_GetKnownFolder(&FOLDERID_LocalAppData);
	wchar_t* uncanonicizedPath = SR_Concat(2, appData, PATH_PLUGINS_TXT_W);
	SkyrimPluginsPathW = SR_CanonicizePathW(uncanonicizedPath);
	free(uncanonicizedPath);

	SkyrimPluginsPathA = SR_Utf16ToCodepage(SkyrimPluginsPathW);

	free(appData);
}

static SR_Redirection* Redirections = NULL;

// Adds a WinAPI function redirection to the list of redirections.
static void AddRedirection(PVOID* original, PVOID redirected, const wchar_t* name)
{
	SR_Redirection* current = calloc(1, sizeof(SR_Redirection));
	current->Next = Redirections;

	current->Original = original;
	current->Redirected = redirected;
	current->Name = name;

	Redirections = current;
}

/*
The following macro is to be used as: ADD_REDIRECT(name);
It does two things:

  1. Set SR_Original_(name) to the address of (name) in Kernel32

  2. Call AddRedirection with SR_Original_(name) as the original pointer and
	 SR_Redirect_(name) as the redirect pointer.

A convenience macro, ADD_REDIRECTAW, is supplied for redirecting both the A (ANSI)
and W (Wide/Unicode) versions of a function.

*/
#define ADD_REDIRECT(name) SR_Original_##name = (name##_t)GetProcAddress(kernel32, #name); AddRedirection(&(PVOID)SR_Original_##name, (PVOID)SR_Redirect_##name, L#name)
#define ADD_REDIRECTAW(name) ADD_REDIRECT(name##A); ADD_REDIRECT(name##W)

static void CreateRedirections()
{
	SR_FreeRedirections();
	CreatePaths();

	HMODULE kernel32 = GetModuleHandleW(L"kernel32");

	ADD_REDIRECTAW(CreateFile);
	ADD_REDIRECTAW(DeleteFile);
	ADD_REDIRECTAW(CopyFile);
	ADD_REDIRECTAW(CopyFileEx);
	ADD_REDIRECTAW(MoveFile);
	ADD_REDIRECTAW(MoveFileEx);
	ADD_REDIRECTAW(MoveFileWithProgress);
	ADD_REDIRECTAW(CreateHardLink);
	ADD_REDIRECTAW(CreateSymbolicLink);

	ADD_REDIRECT(OpenFile);

	ADD_REDIRECTAW(GetPrivateProfileSection);
	ADD_REDIRECTAW(GetPrivateProfileString);
	ADD_REDIRECTAW(GetPrivateProfileInt);
	ADD_REDIRECTAW(GetPrivateProfileStruct);
	ADD_REDIRECTAW(GetPrivateProfileSectionNames);

	ADD_REDIRECTAW(WritePrivateProfileSection);
	ADD_REDIRECTAW(WritePrivateProfileString);
	ADD_REDIRECTAW(WritePrivateProfileStruct);

	ADD_REDIRECTAW(GetFileAttributes);
	ADD_REDIRECTAW(GetFileAttributesEx);
	ADD_REDIRECTAW(SetFileAttributes);
}

#undef ADD_REDIRECTAW
#undef ADD_REDIRECT

SR_Redirection* SR_GetRedirections()
{
	if (Redirections == NULL) CreateRedirections();
	return Redirections;
}

static void FreePaths()
{
	free(UserConfigA.Ini);
	UserConfigA.Ini = NULL;

	free(UserConfigA.PrefsIni);
	UserConfigA.PrefsIni = NULL;

	free(UserConfigA.CustomIni);
	UserConfigA.CustomIni = NULL;

	free(UserConfigA.Plugins);
	UserConfigA.Plugins = NULL;

	free(SkyrimPluginsPathW);
	SkyrimPluginsPathW = NULL;

	free(SkyrimPluginsPathA);
	SkyrimPluginsPathA = NULL;

}

void SR_FreeRedirections()
{
	while (Redirections != NULL)
	{
		SR_Redirection* previous = Redirections;
		Redirections = Redirections->Next;
		free(previous);
	}

	FreePaths();
}
