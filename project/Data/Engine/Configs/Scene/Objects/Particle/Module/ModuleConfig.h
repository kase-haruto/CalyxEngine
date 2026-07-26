#pragma once

#include "Engine/Foundation/Utility/Guid/Guid.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Foundation/Curve/ParticleCurve.h>
#include <Engine/Application/Effects/Particle/Module/ParticleModuleStage.h>
#include <externals/nlohmann/json.hpp>
#include <string>
#include <algorithm>

namespace CalyxEngine {
	class OverLifetimeModule;

	/*-----------------------------------------------------------------------------------------
	 * BaseModuleConfig
	 * - Particle Module設定の共通インターフェースとなる基底データ構造
	 * - GUID、表示名、有効状態、実行StageとJSON変換契約を管理する
	 * - Runtime ParticleとGPUリソースは所有しない
	 *---------------------------------------------------------------------------------------*/
	struct BaseModuleConfig {
		Guid		guid;
		std::string name;
		bool		enabled = true;
		ParticleModuleStage stage = ParticleModuleStage::Update;

		BaseModuleConfig() = default;

		BaseModuleConfig(const std::string& name_, bool enabled_)
			: name(name_), enabled(enabled_) {}

		virtual ~BaseModuleConfig() = default;

		virtual nlohmann::json ToJson() const					 = 0;
		virtual void		   FromJson(const nlohmann::json& j) = 0;
	};

	enum class LifetimeModuleTarget : uint8_t {
		Color,
		Alpha,
		Size,
		Rotation,
		Velocity,
		Emissive,
	};

	/*-----------------------------------------------------------------------------------------
	 * LifetimeModuleConfig
	 * - Particle Lifetime Moduleの編集・保存設定を保持するデータ構造
	 * - 対象属性とFloat、Vector、Colorの時間変化カーブを管理する
	 * - Runtime ParticleとGPUリソースは所有しない
	 *---------------------------------------------------------------------------------------*/
	struct LifetimeModuleConfig : public BaseModuleConfig {
		LifetimeModuleTarget target = LifetimeModuleTarget::Alpha;
		FloatCurve floatCurve{};
		Vector3Curve vectorCurve{};
		ColorGradient gradient{};

		LifetimeModuleConfig() { name = "AlphaOverLifetimeModule"; }
		explicit LifetimeModuleConfig(const std::string& typeName, LifetimeModuleTarget valueTarget)
			: target(valueTarget) { name = typeName; }

		nlohmann::json ToJson() const override;
		void FromJson(const nlohmann::json& j) override;
	};

	/*-----------------------------------------------------------------------------------------
	 * SimpleModuleConfig
	 * - 追加パラメータを持たないParticle Moduleの設定データ構造
	 * - 共通の名前と有効状態だけをJSONへ保存する
	 *---------------------------------------------------------------------------------------*/
	struct SimpleModuleConfig : public BaseModuleConfig {
		SimpleModuleConfig(const std::string& name_, bool enabled_)
			: BaseModuleConfig(name_, enabled_) {}

		nlohmann::json ToJson() const override {
			return {
				{"name", name},
				{"enabled", enabled}};
		}

		void FromJson(const nlohmann::json& j) override {
			if(j.contains("enabled")) j.at("enabled").get_to(enabled);
			if(j.contains("name")) j.at("name").get_to(name);
		}
	};

	/*-----------------------------------------------------------------------------------------
	 * GravityModuleConfig
	 * - Particleへ適用する重力加速度を保持するModule設定データ構造
	 * - 1秒ごとの速度変化に使用する三次元加速度を管理する
	 *---------------------------------------------------------------------------------------*/
	struct GravityModuleConfig : public BaseModuleConfig {
		CalyxEngine::Vector3 gravity{0.0f, -9.8f, 0.0f};

		GravityModuleConfig() {
			name = "GravityModule";
		}

		GravityModuleConfig(const std::string& _name, bool _enable)
			: BaseModuleConfig(_name, _enable) {}

		nlohmann::json ToJson() const override {
			return {
				{"name", name},
				{"enabled", enabled},
				{"gravity", gravity}};
		}

		void FromJson(const nlohmann::json& j) override {
			if(j.contains("enabled")) j.at("enabled").get_to(enabled);
			if(j.contains("gravity")) j.at("gravity").get_to(gravity);
		}
	};

	/*-----------------------------------------------------------------------------------------
	 * AccelerationModuleConfig
	 * - Particleへ一定加速度を適用するModule設定データ構造
	 * - 三次元加速度と共通Module設定を保存する
	 *---------------------------------------------------------------------------------------*/
	struct AccelerationModuleConfig : public BaseModuleConfig {
		Vector3 acceleration{0.0f, 0.0f, 0.0f};
		AccelerationModuleConfig() { name = "AccelerationModule"; }
		nlohmann::json ToJson() const override {
			return {{"guid", guid}, {"name", name}, {"enabled", enabled}, {"acceleration", acceleration}};
		}
		void FromJson(const nlohmann::json& j) override {
			guid = j.value("guid", Guid::Empty());
			enabled = j.value("enabled", true);
			acceleration = j.value("acceleration", Vector3{});
		}
	};

	/*-----------------------------------------------------------------------------------------
	 * DragModuleConfig
	 * - Particle速度へ抵抗を適用するModule設定データ構造
	 * - 0以上のDrag係数と共通Module設定を保存する
	 *---------------------------------------------------------------------------------------*/
	struct DragModuleConfig : public BaseModuleConfig {
		float drag = 0.0f;
		DragModuleConfig() { name = "DragModule"; }
		nlohmann::json ToJson() const override {
			return {{"guid", guid}, {"name", name}, {"enabled", enabled}, {"drag", drag}};
		}
		void FromJson(const nlohmann::json& j) override {
			guid = j.value("guid", Guid::Empty());
			enabled = j.value("enabled", true);
			drag = (std::max)(j.value("drag", 0.0f), 0.0f);
		}
	};

	/*-----------------------------------------------------------------------------------------
	 * SizeOverLifetimeConfig
	 * - Particle寿命に応じたSize変化を保持するModule設定データ構造
	 * - 拡大縮小方向と補間Easeを管理する
	 *---------------------------------------------------------------------------------------*/
	struct SizeOverLifetimeConfig
		: public BaseModuleConfig {
		bool			   isGrowing = true;
		CalyxEngine::EaseType easeType	 = CalyxEngine::EaseType::EaseInOutCubic;

		SizeOverLifetimeConfig() {
			name = "SizeOverLifetimeModule";
		}

		SizeOverLifetimeConfig(const std::string& _name, bool _enable)
			: BaseModuleConfig(_name, _enable) {}

		nlohmann::json ToJson() const override {
			return {
				{"name", name},
				{"enabled", enabled},
				{"isGrowing", isGrowing},
				{"easeType", static_cast<int>(easeType)}};
		}

		void FromJson(const nlohmann::json& j) override {
			if(j.contains("enabled")) j.at("enabled").get_to(enabled);
			if(j.contains("isGrowing")) j.at("isGrowing").get_to(isGrowing);
			if(j.contains("easeType")) {
				int ease = 0;
				j.at("easeType").get_to(ease);
				easeType = static_cast<CalyxEngine::EaseType>(ease);
			}
		}
	};

	/*-----------------------------------------------------------------------------------------
	 * OverLifetimeModuleConfig
	 * - Particle寿命に応じた汎用属性変化を保持するModule設定データ構造
	 * - 対象、合成方式、補間、開始値と終了値を管理する
	 *---------------------------------------------------------------------------------------*/
	struct OverLifetimeModuleConfig
		: public BaseModuleConfig {
		OverLifetimeModuleConfig() { name = "OverLifetimeModule"; }
		OverLifetimeModuleConfig(const std::string& _name, bool _enable)
			: BaseModuleConfig(_name, _enable) {}
		// Target/Blend/Ease は int で保存（モジュール側の enum と対応）
		int	 target	 = 0; // 0:Scale, 1:RotX, 2:RotY, 3:RotZ, 4:ColorRGBA, 5:AlphaOnly
		int	 blend	 = 0; // 0:Set, 1:Add, 2:Multiply
		int	 ease	 = static_cast<int>(CalyxEngine::EaseType::EaseInOutCubic);
		bool clamp01 = true;
		bool invert	 = false;

		// start/end は CalyxEngine::Vector4
		CalyxEngine::Vector4 start{0, 0, 0, 1};
		CalyxEngine::Vector4 end{1, 1, 1, 1};

		nlohmann::json ToJson() const override;

		void FromJson(const nlohmann::json& j) override;

		void ApplyTo(OverLifetimeModule& m) const;

		void ExtractFrom(const OverLifetimeModule& m);
	};

	/*-----------------------------------------------------------------------------------------
	 * TextureSheetAnimationConfig
	 * - Particle Texture Sheet Animationの保存設定を保持するデータ構造
	 * - 行列数、Loop、再生速度、Custom Frame利用状態を管理する
	 *---------------------------------------------------------------------------------------*/
	struct TextureSheetAnimationConfig
		: public BaseModuleConfig {
		int	  rows			  = 4;
		int	  cols			  = 4;
		bool  loop			  = true;
		float animationSpeed  = 10.0f;
		bool  useCustomFrames = false;

		TextureSheetAnimationConfig() {
			name = "TextureSheetAnimationModule";
		}

		TextureSheetAnimationConfig(const std::string& _name, bool _enable)
			: BaseModuleConfig(_name, _enable) {}

		nlohmann::json ToJson() const override {
			return {
				{"name", name},
				{"enabled", enabled},
				{"rows", rows},
				{"cols", cols},
				{"loop", loop},
				{"animationSpeed", animationSpeed},
				{"useCustomFrames", useCustomFrames}
				// カスタムUVリスト（オプション）があればここに追加可能
			};
		}

		void FromJson(const nlohmann::json& j) override {
			if(j.contains("enabled")) j.at("enabled").get_to(enabled);
			if(j.contains("rows")) j.at("rows").get_to(rows);
			if(j.contains("cols")) j.at("cols").get_to(cols);
			if(j.contains("loop")) j.at("loop").get_to(loop);
			if(j.contains("animationSpeed")) j.at("animationSpeed").get_to(animationSpeed);
			if(j.contains("useCustomFrames")) j.at("useCustomFrames").get_to(useCustomFrames);
		}
	};
} // namespace CalyxEngine
