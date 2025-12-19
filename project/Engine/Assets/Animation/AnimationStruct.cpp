#include "AnimationStruct.h"

void Skeleton::JointDraw(const CalyxMath::Matrix4x4& m, const CalyxMath::Vector4& color /* = white */) {
	constexpr float kJointCubeHalf = 0.03f;
	CalyxMath::Vector3 jointCube[8] = {
		{ kJointCubeHalf,  kJointCubeHalf,  kJointCubeHalf},
		{-kJointCubeHalf,  kJointCubeHalf,  kJointCubeHalf},
		{-kJointCubeHalf,  kJointCubeHalf, -kJointCubeHalf},
		{ kJointCubeHalf,  kJointCubeHalf, -kJointCubeHalf},
		{ kJointCubeHalf, -kJointCubeHalf,  kJointCubeHalf},
		{-kJointCubeHalf, -kJointCubeHalf,  kJointCubeHalf},
		{-kJointCubeHalf, -kJointCubeHalf, -kJointCubeHalf},
		{ kJointCubeHalf, -kJointCubeHalf, -kJointCubeHalf},
	};
	for (auto& v : jointCube) {
		v = CalyxMath::Vector3::Transform(v, m);
	}
	// 上面
	for (int i = 0; i < 4; ++i) {
		int p1 = i, p2 = (i + 1) % 4;
		PrimitiveDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], color);
	}
	// 下面
	for (int i = 0; i < 4; ++i) {
		int p1 = 4 + i, p2 = 4 + (i + 1) % 4;
		PrimitiveDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], color);
	}
	// 側面
	for (int i = 0; i < 4; ++i) {
		PrimitiveDrawer::GetInstance()->DrawLine3d(jointCube[i], jointCube[4 + i], color);
	}
}


void Skeleton::Draw(const CalyxMath::Matrix4x4& world,
					int highlightIndex,
					const CalyxMath::Vector4& hiCol) {
	const CalyxMath::Vector4 white{ 1,1,1,1 };

	for (const Joint& joint : joints) {
		CalyxMath::Matrix4x4 ws = joint.skeletonSpaceMatrix * world;

		CalyxMath::Vector3 jointPos{ ws.m[3][0], ws.m[3][1], ws.m[3][2] };

		CalyxMath::Vector4 cubeCol = (joint.index == highlightIndex) ? hiCol : white;
		JointDraw(ws, cubeCol);

		// 親とのライン
		if (joint.parent) {
			CalyxMath::Matrix4x4 pws = joints[*joint.parent].skeletonSpaceMatrix * world;
			CalyxMath::Vector3 parentPos{ pws.m[3][0], pws.m[3][1], pws.m[3][2] };
			PrimitiveDrawer::GetInstance()->DrawLine3d(
				jointPos, parentPos, cubeCol); // ラインも同色に
		}

	}
}
