#include "ImGuiManager.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Foundation/Utility/Func/DxFunc.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/Device/DxCore.h>

// externals
#ifdef _DEBUG
#include "imgui/ImGuizmo.h"
#endif // _DEBUG
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>


void ImGuiManager::Initialize(WinApp* winApp, const DxCore* dxCore){
	pDxCore_ = dxCore;

	ID3D12DescriptorHeap* heap = DescriptorAllocator::GetHeap(DescriptorUsage::CbvSrvUav);
	//srvの設定
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui_ImplWin32_Init(winApp->GetHWND());
	ImGui_ImplDX12_Init(pDxCore_->GetDevice().Get(),
						pDxCore_->GetSwapChain().GetSwapChainDesc().BufferCount,
						pDxCore_->GetFormat(),
						heap,
						DescriptorAllocator::GetCpuHandleStart(DescriptorUsage::CbvSrvUav),  // 0番目
						DescriptorAllocator::GetGpuHandleStart(DescriptorUsage::CbvSrvUav)); // 0番目
	ImGui::StyleColorsDark(); // ダークテーマを適用

	// fontの設定
	ImFont* font = io.Fonts->AddFontFromFileTTF("Resources/Assets/fonts/FiraMono.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	io.FontDefault = font;
	CustomizeImGuiStyle();
}


void ImGuiManager::Finalize(){
	//後始末
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

}

void ImGuiManager::Begin(){
	//フレーム開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#ifdef _DEBUG
	ImGuizmo::BeginFrame();

#endif // _DEBUG	


	ComPtr<ID3D12GraphicsCommandList> commandList = pDxCore_->GetCommandList();
	//でスクリプタヒープの配列をセットする
	ID3D12DescriptorHeap* descriptorHeaps[] = {
	DescriptorAllocator::GetHeap(DescriptorUsage::CbvSrvUav)
	};
	commandList->SetDescriptorHeaps(1, descriptorHeaps);
}

void ImGuiManager::End(){
	//描画前準備
	ImGui::Render();

}

void ImGuiManager::Draw(){
	ComPtr<ID3D12GraphicsCommandList> commandList = pDxCore_->GetCommandList();
	//描画コマンドを発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void ImGuiManager::CustomizeImGuiStyle(){
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.04f, 0.04f, 0.04f, 0.99f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.03f, 0.03f, 0.04f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.43f, 0.90f, 0.11f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.21f, 0.22f, 0.23f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.55f, 0.55f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.16f, 0.16f, 0.16f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.23f, 0.23f, 0.24f, 0.80f);
	colors[ImGuiCol_Tab] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.14f, 0.87f, 0.05f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.23f, 0.78f, 0.02f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.46f, 0.47f, 0.46f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.78f, 0.69f, 0.69f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	style.WindowRounding = 4.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 3.0f;
	style.PopupRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ScrollbarSize = 10.0f;
	style.GrabMinSize = 10.0f;
	style.DockingSeparatorSize = 1.0f;
	style.SeparatorTextBorderSize = 2.0f;
}