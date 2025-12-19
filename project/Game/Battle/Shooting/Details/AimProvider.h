#pragma once

#include <Engine/Foundation/Math/Vector3.h>

struct AimContext {
	CxMath::Vector3 origin{};
	CxMath::Vector3 selfForward{0,0,1};
	CxMath::Vector3 targetPos{};
	CxMath::Vector3 targetVel{};
};

/////////////////////////////////////////////////////////////////////////////////////////
//		プロバイダ
/////////////////////////////////////////////////////////////////////////////////////////
struct IAimProvider {
	virtual ~IAimProvider() = default;
	virtual CxMath::Vector3 GetForwardN(const AimContext& ctx) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////
//		targetに向けて発射
/////////////////////////////////////////////////////////////////////////////////////////
class AimAtTarget :
	public IAimProvider {
public:
	CxMath::Vector3 GetForwardN(const AimContext& ctx) override {
		CxMath::Vector3 d = ctx.targetPos - ctx.origin;
		float L2 = d.LengthSquared();
		return (L2>1e-12f) ? d.Normalize() : CxMath::Vector3{0,0,1};
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
//		前方発射
/////////////////////////////////////////////////////////////////////////////////////////
class FixedForward :
	public IAimProvider {
public:
	CxMath::Vector3 GetForwardN(const AimContext& ctx) override {
		return ctx.selfForward.LengthSquared()>0 ? ctx.selfForward.Normalize() : CxMath::Vector3{0,0,1};
	}
};