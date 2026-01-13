#include "SerializableFieldBuilder.h"

#include "SerializableField.h"

namespace CalyxEngine {

	FieldBuilder::FieldBuilder(SerializableField& f):field_(f){}

	FieldBuilder& FieldBuilder::Category(const std::string& c)  {
		field_.category = c;
		return *this;
	}

	FieldBuilder& FieldBuilder::Tooltip(const std::string& t){
		field_.tooltip = t;
		return *this;
	}

	FieldBuilder& FieldBuilder::Speed(float s)  {
		field_.speed = s;
		return *this;
	}

	FieldBuilder& FieldBuilder::Range(float mn, float mx) {
		field_.hasRange = true;
		field_.min      = mn;
		field_.max      = mx;
		return *this;
	}

	FieldBuilder& FieldBuilder::ReadOnly() {
		field_.readOnly = true;
		return *this;
	}

	FieldBuilder& FieldBuilder::Hidden() {
		field_.hidden = true;
		return *this;
	}


}