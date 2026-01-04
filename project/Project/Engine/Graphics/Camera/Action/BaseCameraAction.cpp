#include "BaseCameraAction.h"

BaseCameraAction::BaseCameraAction() = default;

BaseCameraAction::~BaseCameraAction() = default;

void BaseCameraAction::SetActionName(const std::string& name) { actionName_ = name; }

const std::string& BaseCameraAction::GetActionName()const { return actionName_; }
