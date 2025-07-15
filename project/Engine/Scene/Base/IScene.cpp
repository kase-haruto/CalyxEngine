#include "IScene.h"

/* core */
#include <Engine/Graphics/Device/DxCore.h>


IScene::IScene(){


}

IScene::IScene(DxCore* dxCore){
	pDxCore_ = dxCore;

}

