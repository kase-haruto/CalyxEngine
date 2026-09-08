#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Novel/Asset/NovelEvent.h>

#include <filesystem>
#include <vector>

namespace CalyxEngine {
	class CALYX_API NovelSceneAsset {
	public:
		const std::vector<NovelEvent>& GetEvents() const noexcept { return events_; }
		std::vector<NovelEvent>& GetEvents() noexcept { return events_; }
		NovelEvent* FindEvent(const Guid& guid) noexcept;
		const NovelEvent* FindEvent(const Guid& guid) const noexcept;
		NovelEvent& AddEvent(NovelEventType type);
		bool RemoveEvent(const Guid& guid);
		bool MoveEvent(const Guid& guid, int offset);
		bool Load(const std::filesystem::path& path);
		bool Save(const std::filesystem::path& path) const;
	private:
		std::vector<NovelEvent> events_;
	};
}
