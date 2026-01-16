#pragma once
#include "Engine/Foundation/Utility/Guid/Guid.h"

#include <string>

namespace CalyxEffect {

	/*-----------------------------------------------------------------------------------------
	 * BaseFxModule
	 * - パーティクルモジュール基底クラス
	 * - パーティクルの発生時・更新時に適用される処理を定義
	 *---------------------------------------------------------------------------------------*/
	class BaseFxModule {
	public:
		BaseFxModule(const std::string& name)
			: name_(name)
		{}

		virtual ~BaseFxModule() = default;

		virtual void ShowGuiContent() = 0;
		virtual void OnEmit(struct FxUnit&) {}
		virtual void OnUpdate(struct FxUnit&, float) {}

		bool IsEnabled() const { return isEnabled_; }
		void SetEnabled(bool v) { isEnabled_ = v; }

		const std::string& GetName() const { return name_; }
		void SetName(const std::string& s) { name_ = s; }

		const Guid& GetGuid() const { return guid_; }
		void SetGuid(const Guid& g) { guid_ = g; }

		virtual const char* GetTypeName() const = 0;

	protected:
		bool        isEnabled_ = true;
		std::string name_;

		Guid guid_;
	};
}
