#include "Matrix4x4.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

/* lib */
#include <cmath>
#include <numbers>

namespace CalyxEngine {

	float cot(float angle) {
		return 1 / std::tan(angle);
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::Multiply(const CalyxEngine::Matrix4x4& m1, const CalyxEngine::Matrix4x4& m2) {
		CalyxEngine::Matrix4x4 result;

		for(int i = 0; i < 4; ++i) {
			for(int j = 0; j < 4; ++j) {
				result.m[i][j] = 0.0f;
				for(int k = 0; k < 4; ++k) {
					result.m[i][j] += m1.m[i][k] * m2.m[k][j];
				}
			}
		}

		return result;
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::MakeIdentity() {
		CalyxEngine::Matrix4x4 result;
		for(int i = 0; i < 4; ++i) {
			for(int j = 0; j < 4; ++j) {
				if(i == j) {
					result.m[i][j] = 1.0f; // 対角成分は1
				} else {
					result.m[i][j] = 0.0f; // 対角以外の成分は0
				}
			}
		}
		return result;
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::Inverse(const CalyxEngine::Matrix4x4& mat) {
		CalyxEngine::Matrix4x4 result;

		// 行列式を求める
#pragma region 行列式
		float bottom =
			mat.m[0][0] * mat.m[1][1] * mat.m[2][2] * mat.m[3][3] +
			mat.m[0][0] * mat.m[1][2] * mat.m[2][3] * mat.m[3][1] +
			mat.m[0][0] * mat.m[1][3] * mat.m[2][1] * mat.m[3][2] -

			mat.m[0][0] * mat.m[1][3] * mat.m[2][2] * mat.m[3][1] -
			mat.m[0][0] * mat.m[1][2] * mat.m[2][1] * mat.m[3][3] -
			mat.m[0][0] * mat.m[1][1] * mat.m[2][3] * mat.m[3][2] -

			mat.m[0][1] * mat.m[1][0] * mat.m[2][2] * mat.m[3][3] -
			mat.m[0][2] * mat.m[1][0] * mat.m[2][3] * mat.m[3][1] -
			mat.m[0][3] * mat.m[1][0] * mat.m[2][1] * mat.m[3][2] +

			mat.m[0][3] * mat.m[1][0] * mat.m[2][2] * mat.m[3][1] +
			mat.m[0][2] * mat.m[1][0] * mat.m[2][1] * mat.m[3][3] +
			mat.m[0][1] * mat.m[1][0] * mat.m[2][3] * mat.m[3][2] +

			mat.m[0][1] * mat.m[1][2] * mat.m[2][0] * mat.m[3][3] +
			mat.m[0][2] * mat.m[1][3] * mat.m[2][0] * mat.m[3][1] +
			mat.m[0][3] * mat.m[1][1] * mat.m[2][0] * mat.m[3][2] -

			mat.m[0][3] * mat.m[1][2] * mat.m[2][0] * mat.m[3][1] -
			mat.m[0][2] * mat.m[1][1] * mat.m[2][0] * mat.m[3][3] -
			mat.m[0][1] * mat.m[1][3] * mat.m[2][0] * mat.m[3][2] -

			mat.m[0][1] * mat.m[1][2] * mat.m[2][3] * mat.m[3][0] -
			mat.m[0][2] * mat.m[1][3] * mat.m[2][1] * mat.m[3][0] -
			mat.m[0][3] * mat.m[1][1] * mat.m[2][2] * mat.m[3][0] +

			mat.m[0][3] * mat.m[1][2] * mat.m[2][1] * mat.m[3][0] +
			mat.m[0][2] * mat.m[1][1] * mat.m[2][3] * mat.m[3][0] +
			mat.m[0][1] * mat.m[1][3] * mat.m[2][2] * mat.m[3][0];
#pragma endregion

		float determinant = 1 / bottom;

		// 逆行列を求める
#pragma region 1行目
		//======================================================
		result.m[0][0] =
			(mat.m[1][1] * mat.m[2][2] * mat.m[3][3] +
			 mat.m[1][2] * mat.m[2][3] * mat.m[3][1] +
			 mat.m[1][3] * mat.m[2][1] * mat.m[3][2] -
			 mat.m[1][3] * mat.m[2][2] * mat.m[3][1] -
			 mat.m[1][2] * mat.m[2][1] * mat.m[3][3] -
			 mat.m[1][1] * mat.m[2][3] * mat.m[3][2]) *
			determinant;

		result.m[0][1] =
			(-mat.m[0][1] * mat.m[2][2] * mat.m[3][3] -
			 mat.m[0][2] * mat.m[2][3] * mat.m[3][1] -
			 mat.m[0][3] * mat.m[2][1] * mat.m[3][2] +
			 mat.m[0][3] * mat.m[2][2] * mat.m[3][1] +
			 mat.m[0][2] * mat.m[2][1] * mat.m[3][3] +
			 mat.m[0][1] * mat.m[2][3] * mat.m[3][2]) *
			determinant;

		result.m[0][2] =
			(mat.m[0][1] * mat.m[1][2] * mat.m[3][3] +
			 mat.m[0][2] * mat.m[1][3] * mat.m[3][1] +
			 mat.m[0][3] * mat.m[1][1] * mat.m[3][2] -
			 mat.m[0][3] * mat.m[1][2] * mat.m[3][1] -
			 mat.m[0][2] * mat.m[1][1] * mat.m[3][3] -
			 mat.m[0][1] * mat.m[1][3] * mat.m[3][2]) *
			determinant;

		result.m[0][3] =
			(-mat.m[0][1] * mat.m[1][2] * mat.m[2][3] -
			 mat.m[0][2] * mat.m[1][3] * mat.m[2][1] -
			 mat.m[0][3] * mat.m[1][1] * mat.m[2][2] +
			 mat.m[0][3] * mat.m[1][2] * mat.m[2][1] +
			 mat.m[0][2] * mat.m[1][1] * mat.m[2][3] +
			 mat.m[0][1] * mat.m[1][3] * mat.m[2][2]) *
			determinant;
#pragma endregion

#pragma region 2行目
		//======================================================
		result.m[1][0] =
			(-mat.m[1][0] * mat.m[2][2] * mat.m[3][3] -
			 mat.m[1][2] * mat.m[2][3] * mat.m[3][0] -
			 mat.m[1][3] * mat.m[2][0] * mat.m[3][2] +
			 mat.m[1][3] * mat.m[2][2] * mat.m[3][0] +
			 mat.m[1][2] * mat.m[2][0] * mat.m[3][3] +
			 mat.m[1][0] * mat.m[2][3] * mat.m[3][2]) *
			determinant;

		result.m[1][1] =
			(mat.m[0][0] * mat.m[2][2] * mat.m[3][3] +
			 mat.m[0][2] * mat.m[2][3] * mat.m[3][0] +
			 mat.m[0][3] * mat.m[2][0] * mat.m[3][2] -
			 mat.m[0][3] * mat.m[2][2] * mat.m[3][0] -
			 mat.m[0][2] * mat.m[2][0] * mat.m[3][3] -
			 mat.m[0][0] * mat.m[2][3] * mat.m[3][2]) *
			determinant;

		result.m[1][2] =
			(-mat.m[0][0] * mat.m[1][2] * mat.m[3][3] -
			 mat.m[0][2] * mat.m[1][3] * mat.m[3][0] -
			 mat.m[0][3] * mat.m[1][0] * mat.m[3][2] +
			 mat.m[0][3] * mat.m[1][2] * mat.m[3][0] +
			 mat.m[0][2] * mat.m[1][0] * mat.m[3][3] +
			 mat.m[0][0] * mat.m[1][3] * mat.m[3][2]) *
			determinant;

		result.m[1][3] =
			(mat.m[0][0] * mat.m[1][2] * mat.m[2][3] +
			 mat.m[0][2] * mat.m[1][3] * mat.m[2][0] +
			 mat.m[0][3] * mat.m[1][0] * mat.m[2][2] -
			 mat.m[0][3] * mat.m[1][2] * mat.m[2][0] -
			 mat.m[0][2] * mat.m[1][0] * mat.m[2][3] -
			 mat.m[0][0] * mat.m[1][3] * mat.m[2][2]) *
			determinant;
#pragma endregion

#pragma region 3行目
		//======================================================
		result.m[2][0] =
			(mat.m[1][0] * mat.m[2][1] * mat.m[3][3] +
			 mat.m[1][1] * mat.m[2][3] * mat.m[3][0] +
			 mat.m[1][3] * mat.m[2][0] * mat.m[3][1] -
			 mat.m[1][3] * mat.m[2][1] * mat.m[3][0] -
			 mat.m[1][1] * mat.m[2][0] * mat.m[3][3] -
			 mat.m[1][0] * mat.m[2][3] * mat.m[3][1]) *
			determinant;

		result.m[2][1] =
			(-mat.m[0][0] * mat.m[2][1] * mat.m[3][3] -
			 mat.m[0][1] * mat.m[2][3] * mat.m[3][0] -
			 mat.m[0][3] * mat.m[2][0] * mat.m[3][1] +
			 mat.m[0][3] * mat.m[2][1] * mat.m[3][0] +
			 mat.m[0][1] * mat.m[2][0] * mat.m[3][3] +
			 mat.m[0][0] * mat.m[2][3] * mat.m[3][1]) *
			determinant;

		result.m[2][2] =
			(mat.m[0][0] * mat.m[1][1] * mat.m[3][3] +
			 mat.m[0][1] * mat.m[1][3] * mat.m[3][0] +
			 mat.m[0][3] * mat.m[1][0] * mat.m[3][1] -
			 mat.m[0][3] * mat.m[1][1] * mat.m[3][0] -
			 mat.m[0][1] * mat.m[1][0] * mat.m[3][3] -
			 mat.m[0][0] * mat.m[1][3] * mat.m[3][1]) *
			determinant;

		result.m[2][3] =
			(-mat.m[0][0] * mat.m[1][1] * mat.m[2][3] -
			 mat.m[0][1] * mat.m[1][3] * mat.m[2][0] -
			 mat.m[0][3] * mat.m[1][0] * mat.m[2][1] +
			 mat.m[0][3] * mat.m[1][1] * mat.m[2][0] +
			 mat.m[0][1] * mat.m[1][0] * mat.m[2][3] +
			 mat.m[0][0] * mat.m[1][3] * mat.m[2][1]) *
			determinant;
#pragma endregion

#pragma region 4行目
		//======================================================
		result.m[3][0] =
			(-mat.m[1][0] * mat.m[2][1] * mat.m[3][2] -
			 mat.m[1][1] * mat.m[2][2] * mat.m[3][0] -
			 mat.m[1][2] * mat.m[2][0] * mat.m[3][1] +
			 mat.m[1][2] * mat.m[2][1] * mat.m[3][0] +
			 mat.m[1][1] * mat.m[2][0] * mat.m[3][2] +
			 mat.m[1][0] * mat.m[2][2] * mat.m[3][1]) *
			determinant;

		result.m[3][1] =
			(mat.m[0][0] * mat.m[2][1] * mat.m[3][2] +
			 mat.m[0][1] * mat.m[2][2] * mat.m[3][0] +
			 mat.m[0][2] * mat.m[2][0] * mat.m[3][1] -
			 mat.m[0][2] * mat.m[2][1] * mat.m[3][0] -
			 mat.m[0][1] * mat.m[2][0] * mat.m[3][2] -
			 mat.m[0][0] * mat.m[2][2] * mat.m[3][1]) *
			determinant;

		result.m[3][2] =
			(-mat.m[0][0] * mat.m[1][1] * mat.m[3][2] -
			 mat.m[0][1] * mat.m[1][2] * mat.m[3][0] -
			 mat.m[0][2] * mat.m[1][0] * mat.m[3][1] +
			 mat.m[0][2] * mat.m[1][1] * mat.m[3][0] +
			 mat.m[0][1] * mat.m[1][0] * mat.m[3][2] +
			 mat.m[0][0] * mat.m[1][2] * mat.m[3][1]) *
			determinant;

		result.m[3][3] =
			(mat.m[0][0] * mat.m[1][1] * mat.m[2][2] +
			 mat.m[0][1] * mat.m[1][2] * mat.m[2][0] +
			 mat.m[0][2] * mat.m[1][0] * mat.m[2][1] -
			 mat.m[0][2] * mat.m[1][1] * mat.m[2][0] -
			 mat.m[0][1] * mat.m[1][0] * mat.m[2][2] -
			 mat.m[0][0] * mat.m[1][2] * mat.m[2][1]) *
			determinant;
#pragma endregion

		return result;
	}

	Vector3 CalyxEngine::Matrix4x4::ToEuler(const CalyxEngine::Matrix4x4& matrix) {
		Vector3 euler;

		// YXZ順のオイラー角を計算
		if(matrix.m[0][2] < 1.0f) {
			if(matrix.m[0][2] > -1.0f) {
				euler.y = std::asin(matrix.m[0][2]);
				euler.x = std::atan2(-matrix.m[1][2], matrix.m[2][2]);
				euler.z = std::atan2(-matrix.m[0][1], matrix.m[0][0]);
			} else {
				// matrix.m[0][2] == -1
				euler.y = -static_cast<float>(std::numbers::pi) / 2.0f;
				euler.x = -std::atan2(matrix.m[1][0], matrix.m[1][1]);
				euler.z = 0.0f;
			}
		} else {
			// matrix.m[0][2] == 1
			euler.y = static_cast<float>(std::numbers::pi) / 2.0f;
			euler.x = std::atan2(matrix.m[1][0], matrix.m[1][1]);
			euler.z = 0.0f;
		}

		return euler;
	}

	Vector3 CalyxEngine::Matrix4x4::Translation(const CalyxEngine::Matrix4x4& matrix) {
		return Vector3{
			matrix.m[0][3],
			matrix.m[1][3],
			matrix.m[2][3]};
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::Transpose(const CalyxEngine::Matrix4x4& mat) {
		CalyxEngine::Matrix4x4 result;
		for(int r = 0; r < 4; ++r) {
			for(int c = 0; c < 4; ++c) {
				result.m[r][c] = mat.m[c][r]; // 行と列を入れ替え
			}
		}
		return result;
	}

	Vector3 CalyxEngine::Matrix4x4::Transform(const Vector3& vector, const CalyxEngine::Matrix4x4& matrix) {
		Vector3 result;

		float x =
			vector.x * matrix.m[0][0] +
			vector.y * matrix.m[1][0] +
			vector.z * matrix.m[2][0] +
			matrix.m[3][0];

		float y =
			vector.x * matrix.m[0][1] +
			vector.y * matrix.m[1][1] +
			vector.z * matrix.m[2][1] +
			matrix.m[3][1];

		float z =
			vector.x * matrix.m[0][2] +
			vector.y * matrix.m[1][2] +
			vector.z * matrix.m[2][2] +
			matrix.m[3][2];

		float w =
			vector.x * matrix.m[0][3] +
			vector.y * matrix.m[1][3] +
			vector.z * matrix.m[2][3] +
			matrix.m[3][3];

		if(std::fabs(w) > 1e-6f) {
			result.x = x / w;
			result.y = y / w;
			result.z = z / w;
		} else {
			// w=0 の場合は座標として扱わない（View 行列ではここに来ないのが正常）
			result.x = x;
			result.y = y;
			result.z = z;
		}

		return result;
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::MakeLookRotationMatrix(const Vector3& forward, const Vector3& up) {
		Vector3 zAxis = forward.Normalize();				   // 前方方向
		Vector3 xAxis = Vector3::Cross(up, zAxis).Normalize(); // 右方向
		Vector3 yAxis = Vector3::Cross(zAxis, xAxis);		   // 上方向

		CalyxEngine::Matrix4x4 result = CalyxEngine::Matrix4x4::MakeIdentity();

		result.m[0][0] = xAxis.x;
		result.m[1][0] = xAxis.y;
		result.m[2][0] = xAxis.z;

		result.m[0][1] = yAxis.x;
		result.m[1][1] = yAxis.y;
		result.m[2][1] = yAxis.z;

		result.m[0][2] = zAxis.x;
		result.m[1][2] = zAxis.y;
		result.m[2][2] = zAxis.z;

		return result;
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {

		CalyxEngine::Matrix4x4 matrix = {};

		for(int i = 0; i < 4; i++) {
			for(int j = 0; j < 4; j++) {
				matrix.m[i][j] = 0.0f;
			}
		}

		matrix.m[0][0] = width / 2.0f;
		matrix.m[1][1] = -height / 2.0f;
		matrix.m[2][2] = maxDepth - minDepth;
		matrix.m[3][0] = left + width / 2.0f;
		matrix.m[3][1] = top + height / 2.0f;
		matrix.m[3][2] = minDepth;
		matrix.m[3][3] = 1.0f;

		return matrix;
	}

	CalyxEngine::Matrix4x4 CalyxEngine::Matrix4x4::PerspectiveFovRH(float fovY, float aspect, float nearZ, float farZ) {
		float yScale = 1.0f / std::tan(fovY * 0.5f);
		float xScale = yScale / aspect;
		float zRange = farZ - nearZ;

		CalyxEngine::Matrix4x4 mat = {};

		mat.m[0][0] = xScale;
		mat.m[1][1] = yScale;
		mat.m[2][2] = farZ / zRange;
		mat.m[2][3] = 1.0f;
		mat.m[3][2] = -nearZ * farZ / zRange;
		mat.m[3][3] = 0.0f;

		return mat;
	}
	Matrix4x4 Matrix4x4::MakeLookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
		Vector3 zaxis = (target - eye).Normalize();			   // forward
		Vector3 xaxis = Vector3::Cross(up, zaxis).Normalize(); // right
		Vector3 yaxis = Vector3::Cross(zaxis, xaxis);		   // up

		Matrix4x4 m = Matrix4x4::MakeIdentity();

		m.m[0][0] = xaxis.x;
		m.m[1][0] = xaxis.y;
		m.m[2][0] = xaxis.z;

		m.m[0][1] = yaxis.x;
		m.m[1][1] = yaxis.y;
		m.m[2][1] = yaxis.z;

		m.m[0][2] = zaxis.x;
		m.m[1][2] = zaxis.y;
		m.m[2][2] = zaxis.z;

		m.m[3][0] = -Vector3::Dot(xaxis, eye);
		m.m[3][1] = -Vector3::Dot(yaxis, eye);
		m.m[3][2] = -Vector3::Dot(zaxis, eye);

		return m;
	}

	void CalyxEngine::Matrix4x4::CopyToArray(float out[16]) const {
		// m[row][col] → out[col*4 + row]
		for(int row = 0; row < 4; ++row) {
			for(int col = 0; col < 4; ++col) {
				out[col * 4 + row] = m[row][col];
			}
		}
	}

	Vector3 CalyxEngine::Matrix4x4::GetTranslationMatrix() const {
		return Vector3{
			m[0][3],
			m[1][3],
			m[2][3]};
	}

	CalyxEngine::Vector4 CalyxEngine::Matrix4x4::operator*(const CalyxEngine::Vector4& v) const {
		return CalyxEngine::Vector4{
			m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
			m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
			m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w,
			m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w};
	}

} // namespace CalyxEngine