#pragma once

#include <memory>

namespace CalyxEngine {

	class FreeTypeLibrary {
	public:
		FreeTypeLibrary();
		~FreeTypeLibrary();
		FreeTypeLibrary(const FreeTypeLibrary&) = delete;
		FreeTypeLibrary& operator=(const FreeTypeLibrary&) = delete;

		[[nodiscard]] bool IsValid() const noexcept { return library_ != nullptr; }
		[[nodiscard]] void* GetNativeLibrary() const noexcept { return library_; }

	private:
		void* library_ = nullptr; //< FT_Library
	};

} // namespace CalyxEngine
