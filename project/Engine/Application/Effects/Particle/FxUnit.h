#pragma once

#include <Engine/Objects/Transform/Transform.h>

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

namespace CalyxEffect {

	/*-----------------------------------------------------------------------------------------
	 * FxUnit
	 * - CPUパーティクル1個分のデータ構造体
	 * - 位置・速度・寿命・色・スケール等のパーティクル情報を保持
	 *---------------------------------------------------------------------------------------*/
	struct FxUnit {
		CalyxMath::Vector3 position;                        //< 座標
		CalyxMath::Vector3 rotationEuler;                   //< オイラー回転
		float              spinSpeed;                       // < スピン速度
		CalyxMath::Vector3 velocity;                        //< 速度
		CalyxMath::Vector3 initialScale = {1.0f,1.0f,1.0f}; // 初期スケール
		CalyxMath::Vector3 scale;                           //< スケール
		float              lifetime = 1.0f;                 //< 寿命
		float              age;                             //< 経過時間
		CalyxMath::Vector4 color;                           //< 色
		bool               alive = true;                    //< 生存フラグ
		float              lifeT = 0.0f;                    //< 補完の01
		//UVオフセット・スケール
		Transform2D        uvTransform;
		bool               followEmitter = false;                              // エミッタ追従フラグ
		CalyxMath::Vector3 followOffset  = CalyxMath::Vector3(0.0f,0.0f,0.0f); // エミッタからのオフセット

	};

	/*-----------------------------------------------------------------------------------------
	 * ParticleCS
	 * - GPUパーティクル用のデータ構造体
	 * - ComputeShader向けにアライメントされたパーティクル情報
	 *---------------------------------------------------------------------------------------*/
	struct ParticleCS {
		CalyxMath::Vector3 translate;
		CalyxMath::Vector3 scale;
		float              lifeTime;
		CalyxMath::Vector3 velocity;
		float              currentTIme;
		CalyxMath::Vector4 color;
	};
}