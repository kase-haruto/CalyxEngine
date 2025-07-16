#include "IScene.h"

/* core */
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>


IScene::IScene(){


}

IScene::IScene(DxCore* dxCore){
	pDxCore_ = dxCore;

}

