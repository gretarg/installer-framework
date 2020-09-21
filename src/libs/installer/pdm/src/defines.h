#pragma once

#include <algorithm>
#include <string>
#include <cctype>

namespace PDM
{
	inline void ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
		{
			return !std::isspace(ch) && ch;
		}));
	}

	inline void rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
		{
			return !std::isspace(ch) && ch;
		}).base(), s.end());
	}

	inline void trim(std::string& s)
	{
		ltrim(s);
		rtrim(s);
	}

	inline std::string toupper(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](char c) { return static_cast<char>(std::toupper(c)); });
		return s;
	}

	template <typename F>
	struct ScopeExit
	{
		ScopeExit(F f) : f(f) {}
		~ScopeExit() { f(); }
		F f;
	};

	template <typename F>
	ScopeExit<F> MakeScopeExit(F f)
	{
		return ScopeExit<F>(f);
	};

	#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
	#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
	#define SCOPE_EXIT(code) auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([&](){ code; })
}