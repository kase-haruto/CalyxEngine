#pragma once

#include <Windows.h>

namespace Calyx {

	class Application;

	int Run(HINSTANCE hInstance, Application& application);
	int Run(HINSTANCE hInstance, Application& application, const char* commandLine);

} // namespace Calyx
