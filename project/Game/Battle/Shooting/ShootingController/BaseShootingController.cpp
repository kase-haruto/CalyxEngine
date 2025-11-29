#include "BaseShootingController.h"

BaseShootingController::BaseShootingController() = default;
BaseShootingController::~BaseShootingController() = default;

void BaseShootingController::Update(float dt){ shootCooldown_ -= dt; }
void BaseShootingController::SetCooldown(float cooldown){ shootCooldown_ = cooldown; }
float BaseShootingController::GetCooldown() const{ return shootCooldown_; }
