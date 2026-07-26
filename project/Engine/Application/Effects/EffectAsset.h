#pragma once

#include <Data/Engine/Configs/Scene/Objects/Particle/EffectConfig.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <filesystem>
#include <string>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * EffectAsset
	 * - SceneObjectではない、再利用可能なエフェクト定義データ
	 * - FxObjectなどの編集用オブジェクトから書き出し、EffectPlayerが実行時に読む
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief EffectAssetの機能を提供するクラスです。
	 */
	class EffectAsset {
	public:
		EffectAsset() = default;
		CALYX_API explicit EffectAsset(EffectAssetData data);

		const EffectAssetData& GetData() const { return data_; }
		EffectAssetData&		  GetData() { return data_; }

		const std::string& GetName() const { return data_.name; }
		void			   SetName(const std::string& name) { data_.name = name; }

		CALYX_API bool Load(const std::filesystem::path& path);
		CALYX_API bool Save(const std::filesystem::path& path) const;

		CALYX_API static EffectAsset FromObjectConfig(const EffectObjectConfig& config);
		CALYX_API EffectObjectConfig ToObjectConfig() const;

	private:
		EffectAssetData data_;
	};
	
} // namespace CalyxEngine
