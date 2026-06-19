#pragma once

#if defined(_WIN32)
#	if defined(CALYX_ENGINE_EXPORTS)
#		define CALYX_API __declspec(dllexport)
#	else
#		define CALYX_API __declspec(dllimport)
#	endif
#else
#	define CALYX_API
#endif
