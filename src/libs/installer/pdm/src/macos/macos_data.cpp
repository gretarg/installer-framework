#include "../../include/pdm.h"

#if __APPLE__

#include <vector>
#include <sys/sysctl.h>
#include <sys/utsname.h>

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <IOKit/network/IOEthernetInterface.h>
#import <IOKit/network/IOEthernetController.h>

namespace PDM
{
	std::string GetOSString(const char* name)
	{
		char buffer[1024] = { 0 };
		size_t size = sizeof(buffer);
		sysctlbyname(name, buffer, &size, nullptr, 0);
		
		return buffer;
	}

	uint64_t GetOSInteger(const char* name)
	{
		uint64_t result = 0;
		size_t size = sizeof(result);
		sysctlbyname(name, &result, &size, nullptr, 0);
		
		return result;
	}

	Bitness GetOSBitnessInternal()
	{
		struct utsname un;
		int res = uname(&un);
		if (res >= 0)
		{
			std::string machine{un.machine};
			if (machine == "x86_64") return Bitness::BITNESS_64;
			if (machine == "i386"  ) return Bitness::BITNESS_32;
		}
		
		return Bitness::BITNESS_UNKNOWN;
	}

	OS GetOSType()
	{
		return OS::MACOS;
	}

	std::string GetOSName()
	{
		return [[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String];
	}

	std::string GetOSMajorVersion()
	{
		return std::to_string([[NSProcessInfo processInfo] operatingSystemVersion].majorVersion);
	}

	std::string GetOSMinorVersion()
	{
		return std::to_string([[NSProcessInfo processInfo] operatingSystemVersion].minorVersion);
	}

	std::string GetOSBuildNumber()
	{
		return std::to_string([[NSProcessInfo processInfo] operatingSystemVersion].patchVersion);
	}

	std::string GetOSKernelVersion()
	{
		return GetOSString("kern.osrelease");
	}

	std::string GetMachineName()
	{
		return [[[NSProcessInfo processInfo] hostName] UTF8String];
	}

	std::string GetUsername()
	{
		return [[[NSProcessInfo processInfo] userName] UTF8String];
	}

	unsigned GetMonitorCount()
	{
		return [[NSScreen screens] count];
	}

	std::string GetHardwareModel()
	{
		return GetOSString("hw.model");
	}

	uint64_t GetTotalMemory()
	{
		return [[NSProcessInfo processInfo] physicalMemory];
	}

	bool IsRemoteSession()
	{
		return false;
	}

	std::string GetMachineUuid()
	{
		char buffer[128] = { 0 };
		io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
		CFStringRef uuidCf = static_cast<CFStringRef>(IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0));
		IOObjectRelease(ioRegistryRoot);
		CFStringGetCString(uuidCf, buffer, sizeof(buffer), kCFStringEncodingMacRoman);
		CFRelease(uuidCf);
		
		return buffer;
	}

	std::string GetUserLocale()
	{
		return [[[NSLocale currentLocale] localeIdentifier] UTF8String];
	}

	uint32_t GetIntFromID(CFMutableDictionaryRef dict, NSString* name)
	{
		auto val = CFDictionaryGetValue(dict, name);
		if (val == nil) return 0;
		return *static_cast<const uint32_t*>([static_cast<NSData*>(val) bytes]);
	}

	uint64_t GetLongFromID(CFMutableDictionaryRef dict, NSString* name)
	{
		auto val = CFDictionaryGetValue(dict, name);
		if (val == nil) return 0;
		return *static_cast<const uint64_t*>([static_cast<NSData*>(val) bytes]);
	}

	NSString* screenNameForDisplay(CGDirectDisplayID displayID)
	{
		NSString *screenName = @"";

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(CGDisplayIOServicePort(displayID), kIODisplayOnlyPreferredName);
#pragma clang diagnostic pop

		NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];

		if ([localizedNames count] > 0) screenName = [[localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]] retain];

		[deviceInfo release];
		return [screenName autorelease];
	}

	std::vector<MonitorInfo> GetMonitorsInfo()
	{
		uint32_t displayCount;
		CGGetOnlineDisplayList(0, nullptr, &displayCount);
		std::vector<CGDirectDisplayID> onlineDisplays(displayCount);
		CGGetOnlineDisplayList(displayCount, &onlineDisplays[0], nullptr);
		
		std::vector<MonitorInfo> monitors;
		
		for (NSScreen* screen in [NSScreen screens])
		{
			uint32_t refreshRate = 0;
			NSString* screenName = [screen respondsToSelector:NSSelectorFromString(@"localizedName")] ? [(id)screen localizedName] : @"";
			
			if (id d = screen.deviceDescription[@"NSScreenNumber"]; d)
			{
				int nr = [d intValue];
				for (CGDirectDisplayID display : onlineDisplays)
				{
					if (display == nr)
					{
						if ([screenName isEqual:@""]) screenName = screenNameForDisplay(display);						
						refreshRate = static_cast<uint32_t>(CGDisplayModeGetRefreshRate(CGDisplayCopyDisplayMode(display)));
						break;
					}
				}
			}
			
			monitors.push_back
			({
				[screenName UTF8String],
				static_cast<uint32_t>(screen.frame.size.width  * screen.backingScaleFactor),
				static_cast<uint32_t>(screen.frame.size.height * screen.backingScaleFactor),
				static_cast<uint32_t>(NSBitsPerSampleFromDepth(screen.depth)),
				refreshRate,
				static_cast<uint32_t>(screen.backingScaleFactor * 100),
			});
		}
		
		return monitors;
	}

	std::string bytesToHexString(NSData* data)
	{
		auto result = [[NSMutableString alloc] initWithCapacity: [data length] << 1];
		auto mbytes = static_cast<const UInt8*>([data bytes]);
		char hBytes[8] = {'\0'};

		for (int i = 0; i < [data length]; i++)
		{
			snprintf(hBytes, 3, "%02X", mbytes[i]);
			if (0 == i) [result appendFormat:@"%s", hBytes];
			else [result appendFormat:@":%s", hBytes];
		}
		
		std::string str = [result UTF8String];
		[result release];
		
		return str;
	}

	std::vector<NetworkAdapterInfo> GetNetworkAdapterInfo()
	{
		std::vector<NetworkAdapterInfo> adapters;
		
		mach_port_t machPort;
		IOMasterPort(MACH_PORT_NULL, &machPort);
		io_iterator_t netIterator = {0};
		IOServiceGetMatchingServices(machPort, IOServiceMatching(kIOEthernetInterfaceClass), &netIterator);
		io_object_t interfaceService, controllerService;
		
		while ( (interfaceService = IOIteratorNext(netIterator)) )
		{
			if (kern_return_t kernResult = IORegistryEntryGetParentEntry( interfaceService, kIOServicePlane, &controllerService ); kernResult == KERN_SUCCESS)
			{
				CFTypeRef MACAddrAsCFData   = IORegistryEntryCreateCFProperty(controllerService, CFSTR(kIOMACAddress), kCFAllocatorDefault, 0);
				CFTypeRef BSDNameAsCFString = IORegistryEntryCreateCFProperty(interfaceService,  CFSTR("BSD Name"),    kCFAllocatorDefault, 0);
				
				if (MACAddrAsCFData && BSDNameAsCFString)
				{
					adapters.push_back
					({
						[static_cast<NSString*>(BSDNameAsCFString) UTF8String],
						bytesToHexString(static_cast<NSData*>(MACAddrAsCFData))
					});
				}
				if (nil != BSDNameAsCFString) CFRelease(BSDNameAsCFString);
				if (nil != MACAddrAsCFData) CFRelease(MACAddrAsCFData);
			}
		}
		IOObjectRelease(netIterator);
		
		return adapters;
	}

	std::vector<GPUInfo> GetGPUInfo()
	{
		std::vector<GPUInfo> gpus;
		
		CFMutableDictionaryRef matchDict = IOServiceMatching("IOPCIDevice");
		io_iterator_t iterator;

		if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchDict, &iterator) == kIOReturnSuccess)
		{
			io_registry_entry_t regEntry;

			while ((regEntry = IOIteratorNext(iterator)))
			{
				CFMutableDictionaryRef serviceDictionary;
				if (IORegistryEntryCreateCFProperties(regEntry, &serviceDictionary, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess)
				{
					IOObjectRelease(regEntry);
					continue;
				}

				if (auto model = static_cast<NSData*>(CFDictionaryGetValue(serviceDictionary, @"model")); model != nil)
				{
					if (CFGetTypeID(model) == CFDataGetTypeID())
					{
						NSString *nsStr = [[NSString alloc] initWithData:model encoding:NSASCIIStringEncoding];
						std::string modelStr = [nsStr UTF8String];
						[nsStr release];
						
						gpus.push_back
						({
							modelStr,
							GetIntFromID(serviceDictionary, @"vendor-id"),
							GetIntFromID(serviceDictionary, @"device-id"),
							GetIntFromID(serviceDictionary, @"revision-id"),
							GetLongFromID(serviceDictionary, @"VRAM,totalsize"),
						});
					}
				}
				
				CFRelease(serviceDictionary);
				IOObjectRelease(regEntry);
			}
			
			IOObjectRelease(iterator);
		}
		
		return gpus;
	}

	bool GetMetalSupported()
	{
		return [MTLCopyAllDevices() count] > 0;
	}

	VulkanProperties GetVulkanProperties()
	{
		return
		{
			// We can safely assume that metal support implies vulkan support (but we might want to probe this in the future anyway)
			GetMetalSupported() ? VulkanSupport::SUPPORTED : VulkanSupport::UNSUPPORTED,
			""
		};
	}
}

#else

namespace PDM
{
	bool GetMetalSupported()
	{
		return false;
	}
}

#endif
