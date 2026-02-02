#include "IScene.h"

/* core */
#include <Engine/Graphics/Device/DxCore.h>


IScene::IScene(){
}

IScene::IScene(CalyxGraphics::DxCore* dxCore){
	pDxCore_ = dxCore;

}