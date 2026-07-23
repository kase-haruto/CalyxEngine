#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Windows.h>

namespace Calyx {

	class Application;

	CALYX_API int Run(HINSTANCE hInstance, Application& application);
	CALYX_API int Run(HINSTANCE hInstance, Application& application, const char* commandLine);

	/**
	 * Changes the game window title.
	 *
	 * The title is interpreted as UTF-8. This can be called after engine
	 * initialization, including from Application::OnInitialize and OnUpdate.
	 */
	CALYX_API void SetWindowTitle(const char* title);

} // namespace Calyx
