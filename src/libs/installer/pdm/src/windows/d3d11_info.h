#pragma once

#if _WIN32

#include "../../include/pdm.h"

#include <d3d11.h>
#include <atlcomcli.h>
#include <dxgi.h>
#include <string>
#include <vector>

namespace PDM
{
	struct D3D11Info
	{
		D3D_FEATURE_LEVEL maxSupportedFeatureLevel{ static_cast<D3D_FEATURE_LEVEL>(0) };
		std::vector<GPUInfo> adapters;
		std::vector<MonitorInfo> monitors;
	};

	D3D11Info& GetD3DInfo();
}

#endif
