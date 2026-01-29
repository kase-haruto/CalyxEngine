#include "MeshData.h"

void MeshBuffers::SetCommand(ID3D12GraphicsCommandList* cmdList) const{
	vertexBuffer.SetCommand(cmdList);
	indexBuffer.SetCommand(cmdList);
}

void MeshResource::SetCommand(ID3D12GraphicsCommandList* cmdList)const {
	buffers.SetCommand(cmdList);
}