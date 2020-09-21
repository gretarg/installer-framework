#pragma once

#include "pdm_data.h"

#if _WIN32
#define DllExport __declspec( dllexport )
#else
#define DllExport __attribute__((visibility("default")))
#endif

namespace PDM
{
	DllExport const PDMData& RetrievePDMData(std::string applicationName, std::string applicationVersion);

	OS GetOSType();
	std::string GetOSName();
	std::string GetOSMajorVersion();
	std::string GetOSMinorVersion();
	std::string GetOSBuildNumber();
	std::string GetOSKernelVersion();
	std::string GetHardwareModel();
	std::string GetMachineName();
	std::string GetUsername();
	std::string GetUserLocale();
	unsigned GetMonitorCount();
	uint64_t GetTotalMemory();
	bool IsRemoteSession();
	size_t GetTimingCycles();
	std::string GetMachineUuid();
	std::vector<MonitorInfo> GetMonitorsInfo();
	std::vector<GPUInfo> GetGPUInfo();
	std::vector<NetworkAdapterInfo> GetNetworkAdapterInfo();
	bool GetMetalSupported();
	VulkanProperties GetVulkanProperties();
	std::string GetD3DHighestSupport();
	bool IsWine();
	const char* GetWineVersion();
	const char* GetWineHostOs();
}
