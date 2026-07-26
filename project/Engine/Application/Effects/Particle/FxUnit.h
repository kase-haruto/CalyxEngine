#pragma once

#include <Engine/Objects/Transform/Transform.h>

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

#include <cstdint>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * FxUnit
	 * - CPUパーティクル1個分のデータ構造体
	 * - 位置・速度・寿命・色・スケール等のパーティクル情報を保持
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief FxUnitに関するデータを保持する構造体です。
	 */
	struct FxUnit {
		Vector3 position;                        //< 座標
		Vector3 rotationEuler;                   //< オイラー回転
		Vector3 spinSpeed;                       // < スピン速度
		Vector3 velocity;                        //< 速度
		Vector3 alignDirection{0.0f,0.0f,0.0f};   //< ビルボード板ポリの向き合わせ方向
		bool               alignToDirection = false;          //< 移動方向へ板ポリを回転するか
		Vector3 initialScale = {1.0f,1.0f,1.0f}; // 初期スケール
		Vector3 scale;                           //< スケール
		float              lifetime = 1.0f;                 //< 寿命
		float              age;                             //< 経過時間
		Vector4 color;                           //< 色
		Vector4 vertexColor{1.0f,1.0f,1.0f,1.0f}; //< エディタから設定する頂点色
		Vector4 emissiveColor{1.0f,1.0f,1.0f,1.0f}; //< 発光色
		float emissiveIntensity = 0.0f;             //< 発光強度
		uint32_t randomSeed = 0;                    //< 生成時に確定するRandom Seed
		bool               alive = true;                    //< 生存フラグ
		float              lifeT = 0.0f;                    //< 補完の01
		//UVオフセット・スケール
		Transform2D        uvTransform;
		bool               followEmitter = false;                              // エミッタ追従フラグ
		Vector3 followOffset  = Vector3(0.0f,0.0f,0.0f); // エミッタからのオフセット

	};

	/*-----------------------------------------------------------------------------------------
	 * ParticleCS
	 * - GPUパーティクル用のデータ構造体
	 * - ComputeShader向けにアライメントされたパーティクル情報
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief ParticleCSに関するデータを保持する構造体です。
	 */
	struct ParticleCS {
		Vector3 translate;
		Vector3 scale;
		float              lifeTime;
		Vector3 velocity;
		float              currentTIme;
		Vector4 color;
		uint32_t           isAlive = 0;
		Vector3 initialScale{1.0f, 1.0f, 1.0f};
		Vector3 rotation{0.0f,0.0f,0.0f}; //< 生成時の固定回転（GPU描画ではZ軸を使用）
	};
}
