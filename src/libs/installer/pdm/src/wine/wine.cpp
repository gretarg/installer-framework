#if _WIN32

#include <atlbase.h>
#include <string>

namespace PDM
{
	bool IsWine()
	{
		static bool hasCached = false;
		static bool wine = false;

		if (!hasCached)
		{
            HMODULE hMod = GetModuleHandleA("ntdll");
			if (!hMod) return false;
			wine = GetProcAddress(hMod, "wine_get_version") != nullptr;
			hasCached = true;
		}
		return wine;
	}

	const char* GetWineVersion()
	{
		typedef const char* (CDECL* wine_get_version_t)(void);

		static bool hasCached = false;
        static char const* wineVersion = "";

		if (!hasCached)
		{
            HMODULE hMod = GetModuleHandleA("ntdll");
			if (!hMod) return "";
            wine_get_version_t wine_get_version = reinterpret_cast<wine_get_version_t>(GetProcAddress(hMod, "wine_get_version"));

			if (wine_get_version)
				wineVersion = _strdup(wine_get_version());

			hasCached = true;
		}

		return wineVersion;
	}

	const char* GetWineHostOs()
	{
		typedef void (CDECL* wine_get_host_version_t)(const char** sysname, const char** release);

		static bool hasCached = false;
        static char const* hostOs = "";

		if (!hasCached)
		{
            HMODULE hMod = GetModuleHandleA("ntdll");
			if (!hMod) return "";
			wine_get_host_version_t wine_get_host_version = reinterpret_cast<wine_get_host_version_t>(GetProcAddress(hMod, "wine_get_host_version"));

			if (wine_get_host_version)
			{
				const char* sys_name = NULL;
				const char* release_name = NULL;
				wine_get_host_version(&sys_name, &release_name);

				std::string hostOsA = sys_name;
				hostOsA += " ";
				hostOsA += release_name;

				hostOs = _strdup(hostOsA.c_str());
			}

			hasCached = true;
		}

		return hostOs;
	}
}

#else

namespace PDM
{
	bool IsWine() { return false; }
	const char* GetWineVersion() { return ""; }
	const char* GetWineHostOs() { return ""; }
}

#endif
