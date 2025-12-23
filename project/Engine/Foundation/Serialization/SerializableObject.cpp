#include "SerializableObject.h"
#include "ParamStore.h"

namespace CalyxEngine {

	bool SerializableObject::SaveParams() const {
		return ParamStore::Save(*this);
	}

	bool SerializableObject::LoadParams() {
		return ParamStore::Load(*this);
	}

} // namespace CalyxEngine