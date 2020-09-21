#if _WIN32

#include "d3d11_info.h"
#include "../defines.h"
#include "../utilities.h"

#include <functional>
#include <comdef.h>
#include <dxgi1_6.h>

namespace PDM
{
	bool GetHexIdFromDeviceId(const char* deviceId, uint32_t& deviceIdHex)
	{
		constexpr auto deviceIdPrefix = "DEV_";

		auto found = strstr(deviceId, deviceIdPrefix);
		if (!found) return false;

		return sscanf_s(found + strlen(deviceIdPrefix), "%x", &deviceIdHex) == 1;
	}

	const char* GetRegistryPathToLocalMachine(const char* registryPath)
	{
		constexpr auto rootPath = "\\Registry\\Machine\\";
		if (strncmp(registryPath, rootPath, strlen(rootPath)) == 0)
			return registryPath + strlen(rootPath);
		else
			return registryPath;
	}

	bool GetDeviceRegistryKey(uint32_t deviceId, std::string& keyPath)
	{
        DISPLAY_DEVICEA dd;
        dd.cb = sizeof(DISPLAY_DEVICEA);

        for (int i = 0; EnumDisplayDevicesA(nullptr, i, &dd, 0); ++i)
		{
			uint32_t device;
			if (GetHexIdFromDeviceId(dd.DeviceID, device) && device == deviceId)
			{
				keyPath = GetRegistryPathToLocalMachine(dd.DeviceKey);
				return true;
			}
		}
		return false;
	}

	bool GetRegistryValue(HKEY key, const char* name, std::string& value)
	{
		char buffer[256];
		DWORD dwcb_data = sizeof(buffer);

        if (LONG result = RegQueryValueExA(key, name, nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &dwcb_data); result == ERROR_SUCCESS)
        {
			value = buffer;
			return true;
		}
		value = "";
		return false;
	}

	void PopulateAdapterDriverVersion(GPUInfo& adapter)
	{
		std::string keyPath;
		if (!GetDeviceRegistryKey(adapter.deviceID, keyPath)) return;

		HKEY key;
        if (LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_QUERY_VALUE, &key); result != ERROR_SUCCESS) return;

		GetRegistryValue(key, "DriverVersion", adapter.driverVersionString);
		GetRegistryValue(key, "DriverDate",    adapter.driverDate);
		GetRegistryValue(key, "ProviderName",  adapter.driverVendor);

		RegCloseKey(key);
	}

	std::string GetMonitorName(HMONITOR monitor)
	{
		MONITORINFOEXW info;
		info.cbSize = sizeof(info);
		GetMonitorInfoW(monitor, &info);

		UINT32 requiredPaths, requiredModes;
		GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &requiredPaths, &requiredModes);
		std::vector<DISPLAYCONFIG_PATH_INFO> paths(requiredPaths);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(requiredModes);
		QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &requiredPaths, paths.data(), &requiredModes, modes.data(), nullptr);

		for (auto& p : paths)
		{
			DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName;
			sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			sourceName.header.size = sizeof(sourceName);
			sourceName.header.adapterId = p.sourceInfo.adapterId;
			sourceName.header.id = p.sourceInfo.id;
			DisplayConfigGetDeviceInfo(&sourceName.header);

			if (!wcscmp(info.szDevice, sourceName.viewGdiDeviceName))
			{
				DISPLAYCONFIG_TARGET_DEVICE_NAME name;
				name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
				name.header.size = sizeof(name);
				name.header.adapterId = p.sourceInfo.adapterId;
				name.header.id = p.targetInfo.id;
				DisplayConfigGetDeviceInfo(&name.header);

				return ws2s(name.monitorFriendlyDeviceName);
			}
		}

		return "";
	}

	uint32_t GetMonitorBPC(CComPtr<IDXGIOutput>& pOutput)
	{
        if (CComQIPtr<IDXGIOutput6> pOutput6(pOutput); pOutput6)
        {
			pOutput = nullptr; // Need to do this explicitly
			DXGI_OUTPUT_DESC1 outpDesc1;
			pOutput6->GetDesc1(&outpDesc1);

			return outpDesc1.BitsPerColor;
		}

		return 0;
	}

	uint32_t GetMonitorMaxRefreshRate(const CComPtr<IDXGIOutput>& pOutput)
	{
		uint32_t refreshRate = 0;
		UINT pNumModes = 0;

		if (SUCCEEDED(pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &pNumModes, nullptr)) && pNumModes > 0)
		{
			std::vector<DXGI_MODE_DESC> pDesc(pNumModes);
			if (SUCCEEDED(pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &pNumModes, &pDesc[0])))
			{
				for (DXGI_MODE_DESC desc : pDesc)
				{
					uint32_t rate = static_cast<uint32_t>(std::round(static_cast<float>(desc.RefreshRate.Numerator) / desc.RefreshRate.Denominator));
					if (rate > refreshRate) refreshRate = rate;
				}
			}
		}

		return refreshRate;
	}

	uint32_t GetMonitorDPIScalingPercent(HMONITOR monitor, HMODULE scalingModuleHandle)
	{
		if (!scalingModuleHandle) return 0;

		typedef enum MONITOR_DPI_TYPE {
			MDT_EFFECTIVE_DPI,
			MDT_ANGULAR_DPI,
			MDT_RAW_DPI,
			MDT_DEFAULT
		} MONITOR_DPI_TYPE;
		typedef HRESULT(STDAPICALLTYPE* LPGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT* dpiX, UINT* dpiY);

		LPGetDpiForMonitor GetDpiForMonitor = reinterpret_cast<LPGetDpiForMonitor>(GetProcAddress(scalingModuleHandle, "GetDpiForMonitor"));
		if (!GetDpiForMonitor) return 0;

		UINT x, y;
		GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &x, &y);

		return x * 100 / 96;
	}

	void SetDPIScalingAware(HMODULE scalingModuleHandle) // Give us physical monitor resolutions
	{
		if (scalingModuleHandle)
		{
			typedef enum PROCESS_DPI_AWARENESS {
				PROCESS_DPI_UNAWARE = 0,
				PROCESS_SYSTEM_DPI_AWARE = 1,
				PROCESS_PER_MONITOR_DPI_AWARE = 2
			} PROCESS_DPI_AWARENESS;
			typedef HRESULT(STDAPICALLTYPE* LPSetProcessDpiAwareness)(_In_ PROCESS_DPI_AWARENESS value);

			LPSetProcessDpiAwareness SetProcessDpiAwareness = reinterpret_cast<LPSetProcessDpiAwareness>(GetProcAddress(scalingModuleHandle, "SetProcessDpiAwareness"));
			if (SetProcessDpiAwareness) SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		}
		
		SetProcessDPIAware(); // For older windows
	}

	////////////////////////////////////////

#pragma warning(disable:26812)
	HRESULT CreateDevice(PFN_D3D11_CREATE_DEVICE d3dCreateDevice, IDXGIAdapter* adapter, D3D_FEATURE_LEVEL& maxSupport)
#pragma warning(default:26812)
	{
		D3D_FEATURE_LEVEL FeatureLevels[] = {
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};

		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;

		HRESULT hr = E_INVALIDARG;
		__try
		{
			int index = 0;
			while ((hr = d3dCreateDevice(
				adapter,
				D3D_DRIVER_TYPE_UNKNOWN,
				0, 0,
				&FeatureLevels[index++],
				1,
				D3D11_SDK_VERSION,
				&device,
				&maxSupport,
				&context
			)) == E_INVALIDARG)
			{
				if (index >= ARRAYSIZE(FeatureLevels)) break;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			hr = E_FAIL;
		}
		if (context) context->Release();
		if (device) device->Release();

		return hr;
	}

	D3D11Info& GetD3DInfo()
	{
		static D3D11Info info;
		static bool initialized = false;
		if (initialized) return info;

		HMODULE dxgiModuleHandle{};
		HMODULE dx11ModuleHandle{};
		CComPtr<IDXGIFactory> dxgiFactory;

		SCOPE_EXIT
		(
			dxgiFactory = nullptr;
			FreeLibrary(dx11ModuleHandle);
			FreeLibrary(dxgiModuleHandle);
		);

        HMODULE scalingModuleHandle = LoadLibraryA("api-ms-win-shcore-scaling-l1-1-1.dll");
		SCOPE_EXIT(FreeLibrary(scalingModuleHandle); );
		SetDPIScalingAware(scalingModuleHandle);

        dxgiModuleHandle = LoadLibraryA("dxgi.dll");
		if (!dxgiModuleHandle) return info;
        dx11ModuleHandle = LoadLibraryA("d3d11.dll");
        if (!dx11ModuleHandle) return info;

		typedef HRESULT(WINAPI* LPCreateDXGIFactory)(REFIID riid, IDXGIFactory** ppFactory);
		LPCreateDXGIFactory createDxgiFactory = reinterpret_cast<LPCreateDXGIFactory>(GetProcAddress(dxgiModuleHandle, "CreateDXGIFactory"));
		if (!createDxgiFactory) return info;
		PFN_D3D11_CREATE_DEVICE createDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(dx11ModuleHandle, "D3D11CreateDevice"));
		if (!createDevice) return info;
		if (FAILED(createDxgiFactory(__uuidof(IDXGIFactory), &dxgiFactory.p))) return info;
		
		initialized = true;

		uint32_t count = 0;
		IDXGIAdapter* pAdapter;

		HMONITOR primaryMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);

		while (dxgiFactory->EnumAdapters(count++, reinterpret_cast<IDXGIAdapter**>(&pAdapter)) != DXGI_ERROR_NOT_FOUND)
		{
			D3D_FEATURE_LEVEL support = info.maxSupportedFeatureLevel;

			CreateDevice(createDevice, pAdapter, support);

			if (support > info.maxSupportedFeatureLevel)
				info.maxSupportedFeatureLevel = support;

			uint32_t index = 0;
			CComPtr<IDXGIOutput> pOutput;
			while (SUCCEEDED(pAdapter->EnumOutputs(index++, reinterpret_cast<IDXGIOutput**>(&pOutput))))
			{
				DXGI_OUTPUT_DESC outpDesc;
				pOutput->GetDesc(&outpDesc);

				uint32_t width = outpDesc.DesktopCoordinates.right - outpDesc.DesktopCoordinates.left;
				uint32_t height = outpDesc.DesktopCoordinates.bottom - outpDesc.DesktopCoordinates.top;

				if (outpDesc.Rotation == DXGI_MODE_ROTATION_ROTATE90 || outpDesc.Rotation == DXGI_MODE_ROTATION_ROTATE270)
					std::swap(width, height);

				std::string name = GetMonitorName(outpDesc.Monitor);
				uint32_t maxRefreshRate = GetMonitorMaxRefreshRate(pOutput);
				uint32_t bpc = GetMonitorBPC(pOutput); // Destroys handle
				uint32_t scaling = GetMonitorDPIScalingPercent(outpDesc.Monitor, scalingModuleHandle);

				MonitorInfo monitor
				{
					name,
					width,
					height,
					bpc,
					maxRefreshRate,
					scaling
				};

				bool isPrimary = outpDesc.Monitor == primaryMonitor;
				if (isPrimary) // Always have primary monitor first
					info.monitors.insert(info.monitors.begin(), monitor);
				else
					info.monitors.push_back(monitor);
			}

			DXGI_ADAPTER_DESC desc{ 0 };
			pAdapter->GetDesc(&desc);

			std::string description(ws2s(desc.Description));
			if (description == "Microsoft Basic Render Driver") continue;

			GPUInfo adapter
			{
				description,
				desc.VendorId,
				desc.DeviceId,
				desc.Revision,
				desc.DedicatedVideoMemory,
			};
			PopulateAdapterDriverVersion(adapter);

			info.adapters.push_back(adapter);
		}

		return info;
	}
}

#endif
