#include "IScene.h"

/* core */
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>


IScene::IScene(){
}

IScene::IScene(CalyxGraphics::DxCore* dxCore){
	pDxCore_ = dxCore;

}

