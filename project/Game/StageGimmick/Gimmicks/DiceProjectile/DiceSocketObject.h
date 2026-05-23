#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// サイコロの収納箱オブジェクト
/// </summary>
class DiceSocketObject : public StageGimmickObjectBase
{
public:

	DiceSocketObject() = default;
	DiceSocketObject(const std::string& modelName,
					std::optional<std::string> objectName = std::nullopt);
	~DiceSocketObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "DiceSocketObject";
	}

	// クリアの数をセット
	void SetClearCount(uint32_t count) {
		clearCount_ = count;
	}
	// 収納箱の数を加算する
	void AddDiceSocketCount() {
		diceSocketCount_++;
	}
	// 収納箱の数を取得
	const uint32_t GetDiceSocketCount() {
		return diceSocketCount_;
	}
	// 今収納可能な座標を取得
	const CalyxEngine::Vector3 GetSocketPos();
	// ゾロ目用の回転を取得する
	const CalyxEngine::Quaternion GetSameNumberRotation();


protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// 揃えるゾロ目の数字
	uint32_t sameNumbers_ = 1;

	// 扉が開く数
	uint32_t clearCount_ = 1;

	// 今ハマっているサイコロの数
	uint32_t diceSocketCount_ = 0;

};
