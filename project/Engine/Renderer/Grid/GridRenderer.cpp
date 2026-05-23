#include "GridRenderer.h"

#include <Engine\Graphics\Camera\Base\BaseCamera.h>
#include <Engine\Graphics\Context\GraphicsGroup.h>
#include <Engine\Graphics\Pipeline\Service\PipelineService.h>

namespace CalyxEngine {

	void GridRenderer::Initialize() {
		if(initialized_) return;
		settingsBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice());
		initialized_ = true;
	}

	void GridRenderer::Render(ID3D12GraphicsCommandList* cmd, PipelineService* pso, BaseCamera* camera) {
		if(!cmd || !pso || !camera) return;
		Initialize();

		settingsBuffer_.TransferData(settings_);

		auto set = pso->GetPipelineSet(PipelineTag::Object::EditorInfiniteGrid, BlendMode::NONE);
		set.SetCommand(cmd);

		camera->SetRootCommand(cmd, 0);
		settingsBuffer_.SetCommand(cmd, 1);

		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(3, 1, 0, 0);
	}

} // namespace CalyxEngine
