#pragma once

#include <Engine/Foundation/Math/Vector3.h>

struct AimContext {
	CalyxMath::Vector3 origin{};
	CalyxMath::Vector3 selfForward{0,0,1};
	CalyxMath::Vector3 targetPos{};
	CalyxMath::Vector3 targetVel{};
};

/////////////////////////////////////////////////////////////////////////////////////////
//		プロバイダ
/////////////////////////////////////////////////////////////////////////////////////////
struct IAimProvider {
	virtual ~IAimProvider() = default;
	virtual CalyxMath::Vector3 GetForwardN(const AimContext& ctx) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////
//		targetに向けて発射
/////////////////////////////////////////////////////////////////////////////////////////
class AimAtTarget :
	public IAimProvider {
public:
	CalyxMath::Vector3 GetForwardN(const AimContext& ctx) override {
		CalyxMath::Vector3 d = ctx.targetPos - ctx.origin;
		float L2 = d.LengthSquared();
		return (L2>1e-12f) ? d.Normalize() : CalyxMath::Vector3{0,0,1};
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
//		前方発射
/////////////////////////////////////////////////////////////////////////////////////////
class FixedForward :
	public IAimProvider {
public:
	CalyxMath::Vector3 GetForwardN(const AimContext& ctx) override {
		return ctx.selfForward.LengthSquared()>0 ? ctx.selfForward.Normalize() : CalyxMath::Vector3{0,0,1};
	}
};