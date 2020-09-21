#include "../include/pdm.h"
#include "gatherer.h"

namespace PDM
{
	const PDMData& RetrievePDMData(std::string applicationName, std::string applicationVersion)
	{
		static PDMData data{};
		static bool hasData = false;

		if (!hasData)
		{
			data = GatherData(applicationName, applicationVersion);
			hasData = true;
		}

		return data;
	}
}
