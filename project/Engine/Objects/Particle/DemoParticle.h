#pragma once

#include "Particle.h"

//* c++
#include <memory>

/**
 * @brief DemoParticleの機能を提供するクラスです。
 */
class DemoParticle
	: public Particle{

public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	DemoParticle();
	~DemoParticle() override = default;

	void Initialize(const std::string& modelName, const std::string& texturePath,int32_t count = 1) ;
	void Update() override;

private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	uint32_t particleNum_;
};
