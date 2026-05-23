#pragma once
#include <array>

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Gimmicks\Shoji\ShojiPaperObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 障子のオブジェクトクラス
/// </summary>
class ShojiObject : public StageGimmickObjectBase
{
public:

	ShojiObject() = default;
	ShojiObject(const std::string& modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~ShojiObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "ShojiObject";
	}

	void SetParam(const ShojiParam& param) {
		param_ = param;
	}	
	void SetPaperGuids(const std::array<Guid,12>& guids) {
		paperGuids_ = guids;
	}
	const std::array<std::shared_ptr<ShojiPaperObject>,12>& GetPaperObject() {
		return paperObjs_;
	}
	void SetIsOpen(bool flag) { isOpen_ = flag; }
	const bool GetIsOpen() const { return isOpen_; }

	const uint32_t GetClearCount() const { return clearCount_; }

	// 障子のクリアカウントを加算
	void AddClearCount() { ++clearCount_; }
	// 障子の紙を作成
	void CreatePaperObjects();

protected:

	// 初期化
	void ObjectInitialize() override;
	
	// 更新
	void ObjectUpdate(float dt) override;

	// gui
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;
	void RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) override;

private:

	// 障子の配列
	std::array<std::shared_ptr<ShojiPaperObject>, 12> paperObjs_;
	// 障子紙のGUID
	std::array<Guid, 12> paperGuids_{};
	// パラメータ
	ShojiParam param_;

	// 障子が開いているか
	bool isOpen_ = false;
	bool isStop_ = false;
	// 障子が開くのに必要な数
	uint32_t clearCount_ = 0;
	// 障子の最初の座標
	float offsetX_ = 0.0f;
	// 障子の速度
	float velocityX_ = 0.0f;
};
