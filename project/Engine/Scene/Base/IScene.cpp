#include "IScene.h"

/* core */
#include <Engine/Graphics/Device/DxCore.h>


IScene::IScene(){
}

IScene::IScene(CalyxEngine::DxCore* dxCore){
	pDxCore_ = dxCore;

}