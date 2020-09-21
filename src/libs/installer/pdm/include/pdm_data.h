#pragma once

#include <vector>
#include <string>
#include <ctime>

namespace PDM
{
	enum class Bitness
	{
		BITNESS_UNKNOWN =  0,
		BITNESS_32      = 32,
		BITNESS_64      = 64
	};

	enum class OS
	{
		WINDOWS,
		MACOS,
		WINE,
		UNKNOWN
	};

	struct MonitorInfo
	{
		std::string name;
		uint32_t width{};
		uint32_t height{};
		uint32_t bitsPerColor{};
		uint32_t refreshRate{};
		uint32_t dpiScaling{};
	};

	struct GPUInfo
	{
		std::string description;
		uint32_t vendorID{};
		uint32_t deviceID{};
		uint32_t revision{};
		uint64_t memory{};
		std::string driverVersionString;
		std::string driverDate;
		std::string driverVendor;
	};

	struct NetworkAdapterInfo
	{
		std::string name;
		std::string macAddress;
		std::string uuid;
	};

	enum class VulkanSupport
	{
		UNKNOWN,
		SUPPORTED,
		UNSUPPORTED,
	};

	struct VulkanProperties
	{
		VulkanSupport support{ VulkanSupport::UNKNOWN };
		std::string version;
	};

	struct TimeStamp : tm
	{
		bool operator ==(const TimeStamp& other) const
		{
			return
				tm_sec   == other.tm_sec  &&
				tm_min   == other.tm_min  &&
				tm_hour  == other.tm_hour &&
				tm_mday  == other.tm_mday &&
				tm_mon   == other.tm_mon  &&
				tm_year  == other.tm_year &&
				tm_yday  == other.tm_yday &&
				tm_isdst == other.tm_isdst;
		}
	};

	struct DataField
	{
		bool operator ==(const DataField& other) const
		{
			return name == other.name && value == other.value;
		}

		std::string name;
		std::string value;
	};

	struct SubItem
	{
		bool operator ==(const SubItem& other) const
		{
			return name == other.name &&
				subitems == other.subitems &&
				items == other.items;
		}

		std::string name;
		std::vector<SubItem> subitems;
		std::vector<DataField> items;
	};

	struct PDMData
	{
		bool operator ==(const PDMData& other) const
		{
			return data == other.data && timestamp == other.timestamp;
		}

		SubItem data;
		TimeStamp timestamp;
	};
}
