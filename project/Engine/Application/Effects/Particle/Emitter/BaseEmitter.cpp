#include "BaseEmitter.h"

#include "Engine/Assets/Model/ModelManager.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"

#include <iostream>

namespace CalyxEffect {
	BaseEmitter::BaseEmitter() =default;

	void BaseEmitter::TransferParticleDataToGPU(){
		if (units_.empty()) return;
		std::vector<ParticleConstantData> gpuUnits;
		for (const auto& fx : units_){
			if (fx.alive){
				gpuUnits.push_back({fx.position, fx.scale, fx.color});
			}
		}
		if (!gpuUnits.empty()){
			instanceBuffer_.TransferVectorData(gpuUnits);
		}
	}

	MeshResource& BaseEmitter::GetMeshResource()  {
		// メッシュを返す
		// プリミティブ形状じゃない場合モデルからメッシュを取得
		if(!primitive_.has_value()) {
			return ModelManager::GetInstance()->GetMeshResource(modelPath);
		}

		// プリミティブ形状の場合は内部メッシュを返す
		// TODO: プリミティブ形状のメッシュ生成を実装する
		// TODO: 形状ごとにメッシュを返すファクトリを実装する
		// NOTE: 仮に plane メッシュを返すようにしておく
		return ModelManager::GetInstance()->GetMeshResource(modelPath);
	}

} // namespace CalyxEffect