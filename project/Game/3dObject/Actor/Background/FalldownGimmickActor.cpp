#include "FalldownGimmickActor.h"
/// ===================================================================== */
///  include space
/// ===================================================================== */

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////

FalldownGimmickActor::FalldownGimmickActor() = default;
FalldownGimmickActor::~FalldownGimmickActor() =default;

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::DerivativeGui() {
	// 落下アニメーションのgui
	falldownAnimation_.ImGui("falldownAnimation");
}

void FalldownGimmickActor::IdleUpdate(float ) { }
void FalldownGimmickActor::OnTriggered() {  }
void FalldownGimmickActor::RunningUpdate(float ) { }
void FalldownGimmickActor::OnFinished() {  }