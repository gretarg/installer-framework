#include "Utilities.h"

#include <atlcomcli.h>

namespace PDM {

    std::string ws2s(const std::wstring& s)
    {
        auto slength = static_cast<int>(s.length());
        auto len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
        std::string r(len, '\0');
        WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
        return r;
    }
}
