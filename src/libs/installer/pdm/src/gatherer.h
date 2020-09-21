#pragma once

#include "defines.h"
#include "../include/pdm_data.h"

#if _WIN32
#include <intrin.h>
#endif

namespace PDM
{
	constexpr auto HYPER_V_NAME = "Microsoft Hv";

	class CPUID
	{
		uint32_t regs[4];

	public:
		explicit CPUID(unsigned funcId)
		{
		#if _WIN32
			__cpuidex(reinterpret_cast<int*>(regs), static_cast<int>(funcId), 0);
		#else
			asm volatile
			(
				"cpuid" :
				"=a" (regs[0]),
				"=b" (regs[1]),
				"=c" (regs[2]),
				"=d" (regs[3]) :
				"a" (funcId),
				"c" (0)
			);
		#endif
		}

		const uint32_t& EAX() const { return regs[0]; }
		const uint32_t& EBX() const { return regs[1]; }
		const uint32_t& ECX() const { return regs[2]; }
		const uint32_t& EDX() const { return regs[3]; }

		bool has_data() const
		{
			return EAX() || EBX() || ECX() || EDX();
		}

		std::string register_to_string(uint32_t reg) const
		{
			return std::string(reinterpret_cast<const char*>(&reg), 4);
		}

		std::string get_eax_string() const { return register_to_string(EAX()); }
		std::string get_ebx_string() const { return register_to_string(EBX()); }
		std::string get_ecx_string() const { return register_to_string(ECX()); }
		std::string get_edx_string() const { return register_to_string(EDX()); }

		static constexpr uint32_t CPUID_FLAG                   = 0x80000000;
		static constexpr uint32_t CPUID_EXTENDED_FLAG          = 0x80000001;
		static constexpr uint32_t CPUID_X64_FLAG               = 0x20000000;
		static constexpr uint32_t CPUID_MODEL_NAME_FLAG        = 0x80000004;
		static constexpr uint32_t CPUID_MODEL_NAME_OFFSET_FLAG = 0x80000002;
		static constexpr uint32_t HYPERVISOR_PRESENT_FLAG      = 0x80000000;
		static constexpr uint32_t HYPERVISOR_INFO_FLAG         = 0x40000000;
	};

	PDMData GatherData(std::string applicationName, std::string applicationVersion);
}
