#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Windows.h>

namespace Calyx {

	class Application;

	CALYX_API int Run(HINSTANCE hInstance, Application& application);
	CALYX_API int Run(HINSTANCE hInstance, Application& application, const char* commandLine);

} // namespace Calyx
