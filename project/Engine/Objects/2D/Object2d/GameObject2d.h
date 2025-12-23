#pragma once

#include "../../base/BaseObject.h"
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/System/Component/IComponent.h>

#include <memory>
#include <type_traits>
#include <vector>

namespace Calyx2D {

	/*----------------------------------------------------------------------------------------
	 * 2Dゲームオブジェクト基底クラス
	 * - コンポーネントを持てるようにする
	 *---------------------------------------------------------------------------------------*/
	class GameObject2D
		: public BaseObject {
	public:
		virtual ~GameObject2D() override;

		template <class T, class... Args>
		T* AddComponent(Args&&... args) {
			static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");
			auto comp	= std::make_unique<T>(std::forward<Args>(args)...);
			comp->owner = this;
			T* ptr		= comp.get();
			components_.push_back(std::move(comp));
			return ptr;
		}

		template <class T>
		T* GetComponent() const {
			for(auto& c : components_) {
				if(auto p = dynamic_cast<T*>(c.get())) return p;
			}
			return nullptr;
		}

	public:
		Transform2D transform;

	private:
		std::vector<std::unique_ptr<IComponent>> components_;
	};

} // namespace Calyx2D
