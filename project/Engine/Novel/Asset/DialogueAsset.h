#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Novel/Asset/DialogueLine.h>

#include <filesystem>
#include <vector>

namespace CalyxEngine {
	class CALYX_API DialogueAsset {
	public:
		[[nodiscard]] const std::vector<DialogueLine>& GetLines() const noexcept { return lines_; }
		[[nodiscard]] std::vector<DialogueLine>& GetLines() noexcept { return lines_; }
		[[nodiscard]] DialogueLine* FindLine(const Guid& guid) noexcept;
		[[nodiscard]] const DialogueLine* FindLine(const Guid& guid) const noexcept;

		DialogueLine& AddLine();
		bool RemoveLine(const Guid& guid);
		bool MoveLine(const Guid& guid, int offset);
		bool Load(const std::filesystem::path& path);
		bool Save(const std::filesystem::path& path) const;

	private:
		std::vector<DialogueLine> lines_;
	};
}
