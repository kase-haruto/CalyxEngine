#pragma once
#include <externals/nlohmann/json.hpp>

struct Vector3;
struct Vector4;

/// <summary>
/// 4x4行列
/// </summary>
struct Matrix4x4 final{
	float m[4][4];

	//--------- function ---------------------------------------------------
	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 MakeIdentity();
	static Matrix4x4 Inverse(const Matrix4x4& m);
	static Vector3 ToEuler(const Matrix4x4& matrix);
	static Vector3 Translation(const Matrix4x4& matrix);
	static Matrix4x4 Transpose(const Matrix4x4& mat);
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
	static Matrix4x4 MakeLookRotationMatrix(const Vector3& forward, const Vector3& up);
	static Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
	static Matrix4x4 PerspectiveFovRH(float fovY, float aspect, float nearZ, float farZ);
	void CopyToArray(float out[16]) const;
	Vector3 GetTranslationMatrix()const;

	//--------- operator ---------------------------------------------------
	//operator関数
	Matrix4x4 operator*(const Matrix4x4& other) const{
		return Multiply(*this, other);
	}

	//vector4との積
	Vector4 operator*(const Vector4& v) const;
};
	//--------- serializer ---------------------------------------------------
inline void to_json(nlohmann::json& j, const Matrix4x4& mat) {
	j = nlohmann::json{
		{"m00", mat.m[0][0]}, {"m01", mat.m[0][1]}, {"m02", mat.m[0][2]}, {"m03", mat.m[0][3]},
		{"m10", mat.m[1][0]}, {"m11", mat.m[1][1]}, {"m12", mat.m[1][2]}, {"m13", mat.m[1][3]},
		{"m20", mat.m[2][0]}, {"m21", mat.m[2][1]}, {"m22", mat.m[2][2]}, {"m23", mat.m[2][3]},
		{"m30", mat.m[3][0]}, {"m31", mat.m[3][1]}, {"m32", mat.m[3][2]}, {"m33", mat.m[3][3]}
	};
}

inline void from_json(const nlohmann::json& j,Matrix4x4& mat) {
	j.at("m00").get_to(mat.m[0][0]);
	j.at("m01").get_to(mat.m[0][1]);
	j.at("m02").get_to(mat.m[0][2]);
	j.at("m03").get_to(mat.m[0][3]);
	j.at("m10").get_to(mat.m[1][0]);
	j.at("m11").get_to(mat.m[1][1]);
	j.at("m12").get_to(mat.m[1][2]);
	j.at("m13").get_to(mat.m[1][3]);
	j.at("m20").get_to(mat.m[2][0]);
	j.at("m21").get_to(mat.m[2][1]);
	j.at("m22").get_to(mat.m[2][2]);
	j.at("m23").get_to(mat.m[2][3]);
	j.at("m30").get_to(mat.m[3][0]);
	j.at("m31").get_to(mat.m[3][1]);
	j.at("m32").get_to(mat.m[3][2]);
	j.at("m33").get_to(mat.m[3][3]);
}
