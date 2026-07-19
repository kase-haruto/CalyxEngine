#pragma once

#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace CalyxEngine {

	// 一定加速度を速度へ積分するUpdate Module。Particle位置や描画資源は管理しない。
	class AccelerationModule final : public BaseFxModule {
	public:
		explicit AccelerationModule(const std::string& name = "AccelerationModule") : BaseFxModule(name) {}
		void OnUpdate(FxUnit& particle, float deltaTime) override;
		void ShowGuiContent() override;
		const char* GetObjectClassName() const override { return "AccelerationModule"; }
		const Vector3& GetAcceleration() const { return acceleration_; }
		void SetAcceleration(const Vector3& value) { acceleration_ = value; }
	private:
		Vector3 acceleration_{};
	};

	// 指数減衰で速度を減衰するUpdate Module。フレーム単位の係数は保持しない。
	class DragModule final : public BaseFxModule {
	public:
		explicit DragModule(const std::string& name = "DragModule") : BaseFxModule(name) {}
		void OnUpdate(FxUnit& particle, float deltaTime) override;
		void ShowGuiContent() override;
		const char* GetObjectClassName() const override { return "DragModule"; }
		float GetDrag() const { return drag_; }
		void SetDrag(float value);
	private:
		float drag_ = 0.0f;
	};

} // namespace CalyxEngine
