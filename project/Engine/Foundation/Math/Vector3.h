#pragma once

#include <externals/nlohmann/json.hpp>

namespace CalyxMath {
	struct Matrix4x4;
	struct Quaternion;

	/// <summary>
	/// 3次元ベクトル
	/// </summary>
	struct Vector3 final {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		Vector3(float scaler);
		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
		Vector3() : x(0.0f), y(0.0f), z(0.0f) {}

		//--------- function ---------------------------------------------------
#pragma region function
		void				 Initialize(const Vector3& value);
		void				 Initialize(float v);
		static Vector3		 Forward();
		Vector3				 Right();
		static const Vector3 Zero();
		static Vector3		 One();
		static const Vector3 Up();
		bool				 HasValue() const;
		float				 Length() const;
		Vector3				 Abs();
		Vector3				 Normalize() const;
		float				 LengthSquared() const;
		static float		 Dot(const Vector3& v1, const Vector3& v2);
		static Vector3		 Cross(const Vector3& v0, const Vector3& v1);
		static Vector3		 Lerp(const Vector3& v1, const Vector3& v2, float t);
		static Vector3		 Transform(const Vector3& vector, const Matrix4x4& matrix);
		static Vector3		 Transform(const Vector3& v, const Quaternion& q);

		static Vector3 Min(const Vector3& a, const Vector3& b);

		static Vector3 Max(const Vector3& a, const Vector3& b);
#pragma endregion

		//--------- operator ---------------------------------------------------
#pragma region operator
		// == 演算子のオーバーロード
		bool operator==(const Vector3& other) const {
			return x == other.x && y == other.y && z == other.z;
		}

		// != 演算子のオーバーロード（オプション）
		bool operator!=(const Vector3& other) const {
			return !(*this == other);
		}

		Vector3		   operator*(const float& scalar) const;
		Vector3		   operator*=(const float& scalar);
		Vector3		   operator*(const Vector3& other) const;
		Vector3		   operator*=(const Vector3& other);
		friend Vector3 operator*(float scalar, const Vector3& v);

		Vector3		   operator/(const float& scalar) const;
		Vector3		   operator/=(const float& scalar);
		Vector3		   operator/(const Vector3& other) const;
		Vector3		   operator/=(const Vector3& other);
		friend Vector3 operator/(float scalar, const Vector3& v);

		// ベクトルの加算
		Vector3		   operator+(const float& scalar) const;
		Vector3		   operator+=(const float& scalar);
		Vector3		   operator+(const Vector3& other) const;
		Vector3		   operator+=(const Vector3& other);
		friend Vector3 operator+(float scalar, const Vector3& v);

		// ベクトルの減算
		Vector3		   operator-(const float& scalar) const;
		Vector3		   operator-=(const float& scalar);
		Vector3		   operator-(const Vector3& other) const;
		Vector3		   operator-=(const Vector3& other);
		friend Vector3 operator-(float scalar, const Vector3& v);
		Vector3		   operator-() const;
		float&		   operator[](int index);

		const float& operator[](int index) const;
#pragma endregion
	};

	//--------- serializer ---------------------------------------------------
#pragma region serializer
	inline void to_json(nlohmann::json& j, const CalyxMath::Vector3& v) {
		j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
	}
	inline void from_json(const nlohmann::json& j, CalyxMath::Vector3& v) {
		v.x = j.value("x", 0.0f);
		v.y = j.value("y", 0.0f);
		v.z = j.value("z", 0.0f);
	}

#pragma endregion
} // namespace CalyxMath
