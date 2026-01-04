#pragma once

#include "Vector3.h"
#include "Matrix4x4.h"
#include <stdexcept>

struct Quaternion{
	float x; //< X成分
	float y; //< Y成分
	float z; //< Z成分
	float w; //< W成分

	Quaternion();
	Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w){}

	// 単位クォータニオンで初期化する
	void Initialize();

	bool NotIdentity() const;

	// 2つのクォータニオンのドット積（内積）を求める
	static float Dot(const Quaternion& q1, const Quaternion& q2);

	// ========================== 演算子オーバーロード ==========================

	// クォータニオン同士の加算
	Quaternion operator+(const Quaternion& other) const;

	// クォータニオン同士の乗算
	Quaternion operator*(const Quaternion& other) const;

	Quaternion operator-() const;

	// クォータニオン同士の除算
	Quaternion operator/(const Quaternion& q) const;

	Quaternion operator*(float scalar) const;
	friend Quaternion operator*(float scalar, const Quaternion& q);


	// ========================== 静的関数 ==========================

	// 球面線形補間(SLERP)を行う
	static Quaternion Slerp(Quaternion q0, const Quaternion& q1, float t);

	// 単位クォータニオンを生成する
	static Quaternion MakeIdentity();

	// クォータニオンの共役を求める
	static Quaternion Conjugate(const Quaternion& q);

	// クォータニオンのノルム(大きさ)を求める
	static float Norm(const Quaternion& q);

	// クォータニオンを正規化する
	static Quaternion Normalize(const Quaternion& q);

	// クォータニオンの逆数(逆クォータニオン)を求める
	static Quaternion Inverse(const Quaternion& q);

	// 任意軸回転を表すクォータニオンを生成する
	static Quaternion MakeRotateAxisQuaternion(const Vector3& axis, float angle);

	// クォータニオンから回転行列を生成する
	static Matrix4x4 ToMatrix(const Quaternion& q);

	static Quaternion FromMatrix(const Matrix4x4& m);

	// クォータニオンでベクトルを回転させる
	static Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);

	// 2つのクォータニオンの積を計算する
	static Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);

	// クォータニオンからオイラー角を求める
	static Vector3 ToEuler(const Quaternion& q);

	// オイラー角からクォータニオンを生成する
	static Quaternion FromToQuaternion(const Vector3& from, const Vector3& to);
	static Quaternion EulerToQuaternion(const Vector3& euler);

	// X軸回転
	static Quaternion MakeRotateX(float radian);
	// Y軸回転
	static Quaternion MakeRotateY(float radian);
	// Z軸回転
	static Quaternion MakeRotateZ(float radian);

	static Quaternion LookAt(const Vector3& eye,
							 const Vector3& target,
							 const Vector3& worldUp = {0.0f, 1.0f, 0.0f});

};
	//--------- serializer ---------------------------------------------------
inline void to_json(nlohmann::json& j, const Quaternion& q) {
	j = nlohmann::json{ {"x", q.x}, {"y", q.y}, {"z", q.z}, {"w", q.w} };
}

inline void from_json(const nlohmann::json& j, Quaternion& q) {
	j.at("x").get_to(q.x);
	j.at("y").get_to(q.y);
	j.at("z").get_to(q.z);
	j.at("w").get_to(q.w);

	const float n = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
	if (n > 0.0f) {
		q.w /= n; q.x /= n; q.y /= n; q.z /= n;
	} else {
		q = Quaternion::MakeIdentity();
	}
}