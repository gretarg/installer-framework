#if _WIN32

#include "vulkan_defs.h"
#include "../defines.h"
#include "../../include/pdm.h"

#include <string>
#include <windows.h>

namespace PDM
{
	VulkanProperties GetVulkanProperties()
	{
		VulkanProperties properties;

        HMODULE vulkanDll = LoadLibraryA("vulkan-1.dll");
		if (!vulkanDll) return properties;
		SCOPE_EXIT(FreeLibrary(vulkanDll));

		PFN_vkCreateInstance vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(GetProcAddress(vulkanDll, "vkCreateInstance"));
		PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(GetProcAddress(vulkanDll, "vkGetPhysicalDeviceProperties"));
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(GetProcAddress(vulkanDll, "vkEnumeratePhysicalDevices"));
		PFN_vkDestroyInstance vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(GetProcAddress(vulkanDll, "vkDestroyInstance"));

		if (!vkCreateInstance || !vkGetPhysicalDeviceProperties || !vkEnumeratePhysicalDevices || !vkDestroyInstance) return properties;

		VkInstance instance;
		VkInstanceCreateInfo vkCreate{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, nullptr, 0, nullptr, 0, nullptr };

		if (vkCreateInstance(&vkCreate, nullptr, &instance) != VK_SUCCESS)
		{
			properties.support = VulkanSupport::UNSUPPORTED;
			return properties;
		}
		SCOPE_EXIT(vkDestroyInstance(instance, nullptr)); // Gets run before FreeLibrary

		properties.support = VulkanSupport::SUPPORTED;

		const unsigned DEVICE_COUNT = 32;
		unsigned count = DEVICE_COUNT;
		VkPhysicalDevice devices[DEVICE_COUNT];

		if (vkEnumeratePhysicalDevices(instance, &count, devices) != VK_SUCCESS) return properties;

		uint32_t maxMajor = 0, maxMinor = 0, maxPatch = 0;
		for (unsigned i = 0; i < count; i++)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(devices[i], &props);

			uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
			uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
			uint32_t patch = VK_VERSION_PATCH(props.apiVersion);

			if (major > maxMajor ||
				(major == maxMajor && minor > maxMinor) ||
				(major == maxMajor && minor == maxMinor && patch > maxPatch))
			{
				maxMajor = major;
				maxMinor = minor;
				maxPatch = patch;
			}
		}

		properties.version = std::to_string(maxMajor) + "." + std::to_string(maxMinor) + "." + std::to_string(maxPatch);

		return properties;
	}
}

#endif
