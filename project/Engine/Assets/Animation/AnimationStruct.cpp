#include "AnimationStruct.h"

void Skeleton::JointDraw(const CalyxEngine::Matrix4x4& m, const CalyxEngine::Vector4& color /* = white */) {
	constexpr float kJointCubeHalf = 0.03f;
	CalyxEngine::Vector3 jointCube[8] = {
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
		v = CalyxEngine::Vector3::Transform(v, m);
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


void Skeleton::Draw(const CalyxEngine::Matrix4x4& world,
					int highlightIndex,
					const CalyxEngine::Vector4& hiCol) {
	const CalyxEngine::Vector4 white{ 1,1,1,1 };

	for (const Joint& joint : joints) {
		CalyxEngine::Matrix4x4 ws = joint.skeletonSpaceMatrix * world;

		CalyxEngine::Vector3 jointPos{ ws.m[3][0], ws.m[3][1], ws.m[3][2] };

		CalyxEngine::Vector4 cubeCol = (joint.index == highlightIndex) ? hiCol : white;
		JointDraw(ws, cubeCol);

		// 親とのライン
		if (joint.parent) {
			CalyxEngine::Matrix4x4 pws = joints[*joint.parent].skeletonSpaceMatrix * world;
			CalyxEngine::Vector3 parentPos{ pws.m[3][0], pws.m[3][1], pws.m[3][2] };
			PrimitiveDrawer::GetInstance()->DrawLine3d(
				jointPos, parentPos, cubeCol); // ラインも同色に
		}

	}
}
