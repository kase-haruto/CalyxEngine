#include "DemoParticle.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Utility/Random/Random.h>

#include <Engine/Graphics/Context/GraphicsGroup.h>


DemoParticle::DemoParticle(){

	Particle::SetName("demoParticle");


}

void DemoParticle::Initialize(const std::string& modelName, const std::string& texturePath, int32_t count = 1){

	//50個性性
	particleNum_ = 10;

	Particle::Initialize(modelName, texturePath, count);

}

void DemoParticle::Update(){
	
	// 座標などの更新
	BaseParticle::Update();

}

