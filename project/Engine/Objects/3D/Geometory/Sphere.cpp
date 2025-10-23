//#include "Sphere.h"
//
//#include "../physics/DirectionalLight.h"
//#include "../graphics/GraphicsGroup.h"
//#include "../graphics/camera/CameraManager.h"
//
//#include <Engine/Foundation/Utility/Func/MyFunc.h>
//
////* externals *//
//#include <Engine/objects/TextureManager.h"
//#include <Externals/imgui/imgui.h"
//
///* c++ */
//#include<numbers>
//
//Sphere::Sphere(){}
//
//Sphere::~Sphere(){}
//
//
//void Sphere::Initialize(){
//	device_ = GraphicsGroup::GetInstance()->GetDevice();
//	commandList_ = GraphicsGroup::GetInstance()->GetCommandList();
//	rootSignature_ = GraphicsGroup::GetInstance()->GetRootSignature(Object3D);
//	pipelineState_ = GraphicsGroup::GetInstance()->GetPipelineState(Object3D);
//
//	handle = TextureManager::GetInstance()->LoadTexture("Textures/MonsterBall.png");
//
//	RGBa = {1.0f,1.0f,1.0f,1.0f};
//	transform = {{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}};
//
//	///=================================================
//	///		リソースの生成と書き込み
//	///=================================================
//	CreateBuffer();
//	Map();
//}
//
//void Sphere::Update(){
//	materialData->color = Vector4(RGBa.x, RGBa.y, RGBa.z, RGBa.w);
//	MatrixInitialize();	
//
//}
//
//void Sphere::UpdateImGui(std::string lavel){
//#ifdef _DEBUG
//	ImGui::Begin(lavel.c_str());
//	ImGui::DragFloat3("translation", &transform.translate.x, 0.01f);
//	ImGui::DragFloat3("rotation", &transform.rotate.x, 0.01f);
//	ImGui::ColorEdit4("color", &RGBa.x);
//	ImGui::DragFloat("shininess", &materialData->shininess, 0.01f);
//	
//	const char* lightingModes[] = {"Half-Lambert", "Lambert","SpecularReflection", "No Lighting"};
//
//	if (ImGui::BeginCombo("Lighting Mode", lightingModes[currentLightingMode])){
//		for (int n = 0; n < IM_ARRAYSIZE(lightingModes); n++){
//			const bool is_selected = (currentLightingMode == n);
//			if (ImGui::Selectable(lightingModes[n], is_selected)){
//				currentLightingMode = n;
//				materialData->enableLighting = currentLightingMode;
//			}
//			if (is_selected)
//				ImGui::SetItemDefaultFocus();
//		}
//		ImGui::EndCombo();
//	}
//	ImGui::End();
//#endif // _DEBUG
//}
//
//void Sphere::Draw(){
//	commandList_->SetPipelineState(pipelineState_.Get());
//	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
//	commandList_->IASetVertexBuffers(0, 1, &vertexBufferView);
//	//形状を設定。psoに設定しているものとはまた別。同じものを設定すると考える
//	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//	//マテリアルCBufferの場所を設定
//	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
//	//wvp用のCBufferの場所を設定
//	commandList_->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
//	//srvのdescriptorTableの先頭を設定。3はrootParamenter[3]
//	commandList_->SetGraphicsRootDescriptorTable(3, handle);
//	//描画　3頂点で1つのインスタンス
//	commandList_->DrawInstanced(1536, 1, 0, 0);
//}
//
//void Sphere::CreateBuffer(){
//	CreateVertexBuffer();
//	CreateMaterialBuffer();
//	CreateMatrixBuffer();
//}
//
//void Sphere::CreateVertexBuffer(){
//	vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * 1536);
//	vertexBufferView.BufferLocation = vertexResource_->GetGPUVirtualAddress();
//	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * 1536);
//	vertexBufferView.StrideInBytes = sizeof(VertexData);
//}
//
//void Sphere::CreateMaterialBuffer(){
//	materialResource_ = CreateBufferResource(device_, sizeof(Material));
//}
//
//void Sphere::CreateMatrixBuffer(){
//	wvpResource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
//}
//
//
//void Sphere::Map(){
//	//リソースにデータを書き込む
//	VertexBufferMap();
//	MaterialBufferMap();
//	MatrixBufferMap();
//}
//
//void Sphere::VertexBufferMap(){
//	VertexData* vertexData = nullptr;
//	//書き込むためのアドレスを取得
//	vertexResource_->Map(0, nullptr,
//						 reinterpret_cast< void** >(&vertexData));
//	// 分割数
//	const uint32_t kSubdivision = 16; // 任意の適切な値を設定
//	// 軽度分割1つ分の角度
//	const float kLonEvery = 2 * static_cast<float>(std::numbers::pi) / kSubdivision;
//	// 緯度分割1つ分の角度
//	const float kLatEvery = static_cast< float >(std::numbers::pi) / kSubdivision;
//
//	// 緯度の方向に分割 -n/2 ~ n/2
//	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex){
//		float lat = -static_cast< float >(std::numbers::pi) / 2.0f + kLatEvery * latIndex;
//		// 経度の方向に分割 0~2π
//		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex){
//			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
//			float lon = lonIndex * kLonEvery;
//
//			//uv座標の計算
//			float u0 = float(lonIndex) / float(kSubdivision);
//			float u1 = float(lonIndex + 1) / float(kSubdivision);
//			float v0 = 1.0f - float(latIndex) / float(kSubdivision);
//			float v1 = 1.0f - float(latIndex + 1) / float(kSubdivision);
//
//			//===========================================================
//			//	三角形二枚重ねて面を作る
//			//===========================================================
//
//			//-----------------------------------------------------------
//			//1枚目-------------------------------------------------------
//
//			// a
//			vertexData[start].position.x = std::cos(lat) * std::cos(lon);
//			vertexData[start].position.y = std::sin(lat);
//			vertexData[start].position.z = std::cos(lat) * std::sin(lon);
//			vertexData[start].position.w = 1.0f;
//			vertexData[start].texcoord = {u0, v0};
//			vertexData[start].normal.x = vertexData[start].position.x;
//			vertexData[start].normal.y = vertexData[start].position.y;
//			vertexData[start].normal.z = vertexData[start].position.z;
//
//			// b
//			vertexData[start + 1].position.x = std::cos(lat + kLatEvery) * std::cos(lon);
//			vertexData[start + 1].position.y = std::sin(lat + kLatEvery);
//			vertexData[start + 1].position.z = std::cos(lat + kLatEvery) * std::sin(lon);
//			vertexData[start + 1].position.w = 1.0f;
//			vertexData[start + 1].texcoord = {u0, v1};
//			vertexData[start + 1].normal.x = vertexData[start + 1].position.x;
//			vertexData[start + 1].normal.y = vertexData[start + 1].position.y;
//			vertexData[start + 1].normal.z = vertexData[start + 1].position.z;
//
//			// c
//			vertexData[start + 2].position.x = std::cos(lat) * std::cos(lon + kLonEvery);
//			vertexData[start + 2].position.y = std::sin(lat);
//			vertexData[start + 2].position.z = std::cos(lat) * std::sin(lon + kLonEvery);
//			vertexData[start + 2].position.w = 1.0f;
//			vertexData[start + 2].texcoord = {u1, v0};
//			vertexData[start + 2].normal.x = vertexData[start + 2].position.x;
//			vertexData[start + 2].normal.y = vertexData[start + 2].position.y;
//			vertexData[start + 2].normal.z = vertexData[start + 2].position.z;
//
//			//-----------------------------------------------------------
//
//
//			//-----------------------------------------------------------
//			//2枚目-------------------------------------------------------
//
//			// c2
//			vertexData[start + 3].position = vertexData[start + 2].position;
//			vertexData[start + 3].texcoord = {u1, v0};
//			vertexData[start + 3].normal.x = vertexData[start + 3].position.x;
//			vertexData[start + 3].normal.y = vertexData[start + 3].position.y;
//			vertexData[start + 3].normal.z = vertexData[start + 3].position.z;
//
//			// b2
//			vertexData[start + 4].position = vertexData[start + 1].position;
//			vertexData[start + 4].texcoord = {u0, v1};
//			vertexData[start + 4].normal.x = vertexData[start + 4].position.x;
//			vertexData[start + 4].normal.y = vertexData[start + 4].position.y;
//			vertexData[start + 4].normal.z = vertexData[start + 4].position.z;
//
//			// d
//			vertexData[start + 5].position.x = std::cos(lat + kLatEvery) * std::cos(lon + kLonEvery);
//			vertexData[start + 5].position.y = std::sin(lat + kLatEvery);
//			vertexData[start + 5].position.z = std::cos(lat + kLatEvery) * std::sin(lon + kLonEvery);
//			vertexData[start + 5].position.w = 1.0f;
//			vertexData[start + 5].texcoord = {u1, v1};
//			vertexData[start + 5].normal.x = vertexData[start + 5].position.x;
//			vertexData[start + 5].normal.y = vertexData[start + 5].position.y;
//			vertexData[start + 5].normal.z = vertexData[start + 5].position.z;
//
//			//------------------------------------------------------------
//
//			//// 緯度の方向に分割 -n/2 ~ n/2
//			//for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex){
//			//	// 経度の方向に分割 0~2π
//			//	for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex){
//			//		uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
//
//			//		indexData[start] = start;
//			//		indexData[start + 1] = start + 1;
//			//		indexData[start + 2] = start + 2;
//			//		indexData[start + 3] = start + 1;
//			//		indexData[start + 4] = start + 3;
//			//		indexData[start + 5] = start + 2;
//			//	}
//			//}
//		}
//	}
//	vertexResource_->Unmap(0,nullptr);
//}
//
//void Sphere::MaterialBufferMap(){
//	materialResource_->Map(0, nullptr, reinterpret_cast< void** >(&materialData));
//	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
//	materialData->enableLighting = HalfLambert;
//	materialData->uvTransform = Matrix4x4::MakeIdentity();
//	materialData->shininess = 20.0f;
//	materialResource_->Unmap(0, nullptr);
//}
//
//void Sphere::MatrixBufferMap(){
//	wvpResource_->Map(0, nullptr, reinterpret_cast< void** >(&matrixData));
//	//単位行列を書き込んでおく
//	MatrixInitialize();
//}
//
//void Sphere::MatrixInitialize(){
//	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale,
//											 transform.rotate,
//											 transform.translate
//	);
//
//	Matrix4x4 worldViewProjectionMatrix = Matrix4x4::Multiply(worldMatrix, CameraManager::GetCamera3d()->GetViewProjectionMatrix());
//	matrixData->world = worldMatrix;
//	matrixData->WVP = worldViewProjectionMatrix;
//}