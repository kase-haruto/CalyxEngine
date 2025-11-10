#pragma once

#include <Engine/Objects/Transform/Transform.h>

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

struct FxUnit{
	Vector3 position;		//< 座標
	Vector3 rotationEuler;	//< オイラー回転
	float spinSpeed;		// < スピン速度
	Vector3 velocity;		//< 速度
	Vector3 initialScale = {1.0f, 1.0f, 1.0f}; // 初期スケール
	Vector3 scale;			//< スケール
	float lifetime = 1.0f;	//< 寿命
	float age;				//< 経過時間
	Vector4 color;			//< 色
	bool alive = true;		//< 生存フラグ
	float lifeT = 0.0f;		//< 補完の01
	//UVオフセット・スケール
	Transform2D uvTransform;
};

struct ParticleCS {
	Vector3 translate;
	Vector3 scale;
	float lifeTime;
	Vector3 velocity;
	float currentTIme;
	Vector4 color;
};