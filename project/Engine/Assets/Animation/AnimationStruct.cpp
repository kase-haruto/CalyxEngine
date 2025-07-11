#include "AnimationStruct.h"

void Skeleton::JointDraw(const Matrix4x4& m){
	Vector3 jointCube[8] = {
		Vector3(0.075f, 0.075f, 0.075f),
		Vector3(-0.075f, 0.075f, 0.075f),
		Vector3(-0.075f, 0.075f,-0.075f),
		Vector3(0.075f, 0.075f,-0.075f),

		Vector3(0.075f,-0.075f, 0.075f),
		Vector3(-0.075f,-0.075f, 0.075f),
		Vector3(-0.075f,-0.075f,-0.075f),
		Vector3(0.075f,-0.075f,-0.075f),
	};

	for (int i = 0; i < 8; i++){
		jointCube[i] = Vector3::Transform(jointCube[i], m);
	}

	int p1 = 0;
	int p2 = 1;
	for (int i = 0; i < 4; i++){
		PrimitiveDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], {1.0f,1.0f,1.0f,1.0f});

		p1++;
		p2++;
		p1 = int(fmod(p1, 4));
		p2 = int(fmod(p2, 4));
	}
	p1 = 4;
	p2 = 5;
	for (int i = 0; i < 4; i++){
		PrimitiveDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], {1.0f,1.0f,1.0f,1.0f});

		p1++;
		p2++;
		p1 = 4 + int(fmod(p1, 4));
		p2 = 4 + int(fmod(p2, 4));
	}
	p1 = 0;
	p2 = 4;
	for (int i = 0; i < 4; i++){
		PrimitiveDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], {1.0f,1.0f,1.0f,1.0f});

		p1++;
		p2++;
	}
}

void Skeleton::Draw(){
	for (Joint& joint : joints){
		Vector3 jointPos = {joint.skeletonSpaceMatrix.m[3][0],joint.skeletonSpaceMatrix.m[3][1],joint.skeletonSpaceMatrix.m[3][2]};
		JointDraw(joint.skeletonSpaceMatrix);
		if (joint.parent){
			Vector3 parentPos = {joints[*joint.parent].skeletonSpaceMatrix.m[3][0],joints[*joint.parent].skeletonSpaceMatrix.m[3][1] ,joints[*joint.parent].skeletonSpaceMatrix.m[3][2]};
			PrimitiveDrawer::GetInstance()->DrawLine3d(jointPos, parentPos, {1.0f,1.0f,1.0f,1.0f});
		}
	}
}
