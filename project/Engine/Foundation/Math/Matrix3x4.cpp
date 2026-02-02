#include "Matrix3x4.h"
#include "Matrix4x4.h"

CalyxMath::Matrix3x4 CalyxMath::Matrix3x4::ToMatrix3x4(const CalyxMath::Matrix4x4& m) {
	CalyxMath::Matrix3x4 out{};
	// Transpose 3x3 rotation/scale part
	out.m[0][0] = m.m[0][0];
	out.m[0][1] = m.m[1][0];
	out.m[0][2] = m.m[2][0];
	out.m[1][0] = m.m[0][1];
	out.m[1][1] = m.m[1][1];
	out.m[1][2] = m.m[2][1];
	out.m[2][0] = m.m[0][2];
	out.m[2][1] = m.m[1][2];
	out.m[2][2] = m.m[2][2];

	// Move translation (Row 3 -> Column 3)
	out.m[0][3] = m.m[3][0];
	out.m[1][3] = m.m[3][1];
	out.m[2][3] = m.m[3][2];
	return out;
}