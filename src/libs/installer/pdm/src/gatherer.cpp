#include "gatherer.h"
#include "defines.h"
#include "../include/pdm.h"

#include <string>
#include <thread>
#include <time.h>

namespace PDM
{
	struct CPUInfo
	{
		int model{ 0 };
		int stepping{ 0 };
		std::string vendor;
		std::string brand;
		Bitness bitness;
	};

	Bitness GetOSBitnessInternal();


	constexpr const char* GetVersion()
	{
        return "1.0.6";
	}

	// Only works on x86/x64/IA32/IA64 architectures
	CPUInfo GetCPUInfo()
	{
		Bitness bitness;

		CPUID id8(CPUID::CPUID_FLAG);
		if (id8.EAX() >= CPUID::CPUID_EXTENDED_FLAG)
		{
			CPUID id81(CPUID::CPUID_EXTENDED_FLAG);
			bitness = id81.EDX() & CPUID::CPUID_X64_FLAG ? Bitness::BITNESS_64 : Bitness::BITNESS_32;
		}
		else
		{
			bitness = Bitness::BITNESS_32;
		}
		
		std::string brand;

		if (id8.EAX() >= CPUID::CPUID_MODEL_NAME_FLAG)
		{
			for (unsigned i = 0; i < 3; i++)
			{
				CPUID id(CPUID::CPUID_MODEL_NAME_OFFSET_FLAG + i);
				brand += id.get_eax_string() + id.get_ebx_string() + id.get_ecx_string() + id.get_edx_string();
			}

			trim(brand);
		}

		CPUID id0(0);
		std::string vendor = id0.get_ebx_string() + id0.get_edx_string() + id0.get_ecx_string();
		trim(vendor);

		int model = 0;
		int stepping = 0;

		if (id0.EAX() > 0)
		{
			CPUID id1(1);
			model = (id1.EAX() >> 4) & 0xf;
			stepping = id1.EAX() & 0xf;
		}

		return { model, stepping, vendor, brand, bitness };
	}

	size_t GetTimingCycles()
	{
		size_t time1 = 0;
		size_t time2 = 0;

#if _WIN64
		time1 = __rdtsc();
		time2 = __rdtsc();
#elif _WIN32
		__asm
		{
			RDTSC
			MOV time1, EAX
			RDTSC
			MOV time2, EAX
		}
#else
		asm volatile("RDTSC" : "=a" (time1));
		asm volatile("RDTSC" : "=a" (time2));
#endif
		return time2 - time1;
	}

	bool HasVMExecutionTiming()
	{
		static bool _finished, _ret;
		if (_finished) return _ret;
		_finished = true;

		// Average time on a modern processor natively is around 25 cycles
		// Time on paravirtualized VM is over 5000 cycles
		const unsigned THRESHOLD_CYCLES = 100;
		const unsigned RUNS = 1024;

		unsigned thresholdCrossings = 0;
		
		for (unsigned i = 0; i < RUNS; i++)
		{
			if (GetTimingCycles() > THRESHOLD_CYCLES) thresholdCrossings++;
		}

		_ret = thresholdCrossings > (RUNS / 2);
		return _ret;
	}

	bool HasHypervisorBit()
	{
		return CPUID(1).ECX() & CPUID::HYPERVISOR_PRESENT_FLAG;
	}

	std::string GetHypervisorName()
	{
		if (!HasHypervisorBit()) return "";

		CPUID id(CPUID::HYPERVISOR_INFO_FLAG);
		auto str = id.get_ebx_string() + id.get_ecx_string() + id.get_edx_string();
		trim(str);
		return str;
	}

	bool IsHyperVGuestOS()
	{
		if (GetHypervisorName() != HYPER_V_NAME) return false;

		// TODO: More checks?
		if (CPUID(CPUID::HYPERVISOR_INFO_FLAG + 128).has_data() ||
			CPUID(CPUID::HYPERVISOR_INFO_FLAG + 129).has_data() ||
			CPUID(CPUID::HYPERVISOR_INFO_FLAG + 130).has_data())
			return true;

		return false;
	}

	bool IsSuspectedVM()
	{
		if (HasVMExecutionTiming() || IsHyperVGuestOS()) return true;
		if (!HasHypervisorBit()) return false;
		if (GetHypervisorName() != HYPER_V_NAME) return true;

		return false;
	}

	constexpr Bitness GetProcessBitness()
	{
		switch (sizeof(void*))
		{
		case 8:
			return Bitness::BITNESS_64;
		case 4:
			return Bitness::BITNESS_32;
		default:
			return Bitness::BITNESS_UNKNOWN;
		}
	}

	Bitness GetOSBitness()
	{
		if (GetProcessBitness() == Bitness::BITNESS_64) return Bitness::BITNESS_64;

		return GetOSBitnessInternal();
	}

	Bitness GetCPUBitness(CPUInfo info)
	{
		if (GetOSBitness() == Bitness::BITNESS_64) return Bitness::BITNESS_64;

		return info.bitness;
	}

	TimeStamp GetCurrentTime()
	{

		time_t rawtime;
		time(&rawtime);
		TimeStamp time{ {0} };

#if _WIN32
		localtime_s(&time, &rawtime);
#else
		localtime_r(&rawtime, &time);
#endif

		return time;
	}

	constexpr const char* BitnessToString(Bitness bitness)
	{
		switch (bitness)
		{
		case Bitness::BITNESS_64:
			return "x64";
		case Bitness::BITNESS_32:
			return "x32";
		case Bitness::BITNESS_UNKNOWN:
		default:
			return "Unknown";
		}
	}

	constexpr const char* OSToString(OS os)
	{
		switch (os)
		{
		case OS::WINDOWS:
			return "Windows";
		case OS::MACOS:
			return "macOS";
		case OS::WINE:
			return "Wine";
		case OS::UNKNOWN:
		default:
			return "Unknown";
		}
	}

	constexpr const char* VulkanSupportToString(VulkanSupport support)
	{
		switch (support)
		{
		case VulkanSupport::SUPPORTED:
			return "YES";
		case VulkanSupport::UNSUPPORTED:
			return "NO";
		case VulkanSupport::UNKNOWN:
		default:
			return "UNKNOWN";
		}
	}

	std::string TimestampToString(const TimeStamp& timestamp)
	{
		const int MAX_SIZE = 20;
		char time[MAX_SIZE];
		strftime(time, MAX_SIZE, "%F %T", &timestamp);
		return time;
	}

	PDMData GatherData(std::string applicationName, std::string applicationVersion)
	{
		TimeStamp timestamp = GetCurrentTime();
		CPUInfo cpuinfo = GetCPUInfo();

		std::vector<SubItem> monitors;
		for (auto& monitor : GetMonitorsInfo())
		{
			monitors.push_back
			({
				"MONITOR",
				{},
				{
					{"NAME",                monitor.name},
					{"HORIZONTAL_RES",      std::to_string(monitor.width)},
					{"VERTICAL_RES",        std::to_string(monitor.height)},
					{"BITS_PER_COLOR",      std::to_string(monitor.bitsPerColor)},
					{"REFRESH_RATE",        std::to_string(monitor.refreshRate)},
					{"DPI_SCALING_PERCENT", std::to_string(monitor.dpiScaling)},
				}
			});
		}

		std::vector<SubItem> gpus;
		for (auto& gpu : GetGPUInfo())
		{
			gpus.push_back
			({
				"GPU",
				{},
				{
					{"DESCRIPTION",    gpu.description},
					{"VENDOR_ID",      std::to_string(gpu.vendorID)},
					{"DEVICE_ID",      std::to_string(gpu.deviceID)},
					{"REVISION",       std::to_string(gpu.revision)},
					{"VIDEO_MEMORY",   std::to_string(gpu.memory)},
					{"DRIVER_DATE",    gpu.driverDate},
					{"DRIVER_VENDOR",  gpu.driverVendor},
					{"DRIVER_VERSION", gpu.driverVersionString},
				}
			});
		}

		std::vector<SubItem> networkAdapters;
		for (auto& adapter : GetNetworkAdapterInfo())
		{
			networkAdapters.push_back
			({
				"ADAPTER",
				{},
				{
					{"NAME",        adapter.name},
					{"MAC_ADDRESS", adapter.macAddress},
					{"UUID",        adapter.uuid},
				}
			});
		}
		
		VulkanProperties vulkanProperties = GetVulkanProperties();

		return
		{
			{
				"DATA",
				{
					{
						"GENERAL",
						{
							{
								"APPLICATION",
								{},
								{
									{"NAME",    applicationName},
									{"VERSION", applicationVersion},
								}
							},
							{
								"PROCESS",
								{},
								{
									{"VERSION",   GetVersion()},
									{"TIMESTAMP", TimestampToString(timestamp)},
									{"BITNESS",   BitnessToString(GetProcessBitness())},
								}
							},
							{
								"OS",
								{
									{
										"GRAPHICS_APIS",
										{},
										{
											{"METAL_SUPPORTED",        GetMetalSupported() ? "YES" : "NO"},
											{"VULKAN_SUPPORTED",       VulkanSupportToString(vulkanProperties.support)},
											{"VULKAN_HIGHEST_SUPPORT", vulkanProperties.version},
											{"D3D_HIGHEST_SUPPORT",    GetD3DHighestSupport()},
										},
									},
									{
										"WINE",
										{},
										{
											{"VERSION", GetWineVersion()},
											{"HOST_OS", GetWineHostOs()},
										},
									},
								},
								{
									{"TYPE",              OSToString(GetOSType())},
									{"NAME",              GetOSName()},
									{"BITNESS",           BitnessToString(GetOSBitness())},
									{"MAJOR_VERSION",     GetOSMajorVersion()},
									{"MINOR_VERSION",     GetOSMinorVersion()},
									{"BUILD_NUMBER",      GetOSBuildNumber()},
									{"KERNEL_VERSION",    GetOSKernelVersion()},
									{"USERNAME",          GetUsername()},
									{"USER_LOCALE",       GetUserLocale()},
									{"IS_REMOTE_SESSION", IsRemoteSession() ? "YES" : "NO"},
								}
							},
							{
								"MACHINE",
								{
									{
										"CPU",
										{},
										{
											{"BITNESS",            BitnessToString(GetCPUBitness(cpuinfo))},
											{"LOGICAL_CORE_COUNT", std::to_string(std::thread::hardware_concurrency())},
											{"BRAND",              cpuinfo.brand},
											{"VENDOR",             cpuinfo.vendor},
											{"MODEL",              std::to_string(cpuinfo.model)},
											{"STEPPING",           std::to_string(cpuinfo.stepping)},
										}
									},
									{
										"VM",
										{},
										{
											{"IS_SUSPECTED_VM",         IsSuspectedVM() ? "YES" : "NO"},
											{"HAS_HYPERVISOR_BIT",      HasHypervisorBit() ? "YES" : "NO"},
											{"HYPERVISOR_NAME",         GetHypervisorName()},
											{"IS_HYPERV_GUEST_OS",      IsHyperVGuestOS() ? "YES" : "NO"},
											{"HAS_VM_EXECUTION_TIMING", HasVMExecutionTiming() ? "YES" : "NO"},
										}
									},
									{
										"MONITORS",
										monitors,
										{},
									},
									{
										"GPUS",
										gpus,
										{},
									},
									{
										"NETWORK_ADAPTERS",
										networkAdapters,
										{},
									},
								},
								{
									{"MODEL",         GetHardwareModel()},
									{"NAME",          GetMachineName()},
									{"UUID",          GetMachineUuid()},
									{"TOTAL_MEMORY",  std::to_string(GetTotalMemory())},
									{"MONITOR_COUNT", std::to_string(GetMonitorCount())},
								}
							},
						},
						{}
					},
				},
				{}
			},
			timestamp
		};
	}
}
