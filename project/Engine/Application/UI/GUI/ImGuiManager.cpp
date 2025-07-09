#include "ImGuiManager.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Foundation/Utility/Func/DxFunc.h>
#include <Engine/Graphics/Descriptor/SrvLocator.h>
#include <Engine/Graphics/Device/DxCore.h>

// externals
#ifdef _DEBUG
#include "imgui/ImGuizmo.h"
#endif // _DEBUG
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>


void ImGuiManager::Initialize(WinApp* winApp, const DxCore* dxCore){
	pDxCore_ = dxCore;

	srvHeap_ = CreateDescriptorHeap(pDxCore_->GetDevice().Get(),
									D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
									12800,
									true
	);


	//srvの設定
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();


	// Dockingのみ有効、Viewportsは無効
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplWin32_Init(winApp->GetHWND());
	ImGui_ImplDX12_Init(pDxCore_->GetDevice().Get(),
						pDxCore_->GetSwapChain().GetSwapChainDesc().BufferCount,
						pDxCore_->GetFormat(),
						srvHeap_.Get(),
						srvHeap_.Get()->GetCPUDescriptorHandleForHeapStart(),
						srvHeap_.Get()->GetGPUDescriptorHandleForHeapStart()
	);
	ImGui::StyleColorsDark(); // ダークテーマを適用

	// fontの設定
	ImFont* font = io.Fonts->AddFontFromFileTTF("Resources/Assets/fonts/FiraMono.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	io.FontDefault = font;
	CustomizeImGuiStyle();

	//先頭にimguiが入ったsrvを管理クラスに移す
	SrvLocator::Provide(srvHeap_, pDxCore_->GetDevice());
}


void ImGuiManager::Finalize(){
	//後始末
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	srvHeap_.Reset();
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
	ID3D12DescriptorHeap* descriptorHeaps[] = {SrvLocator::GetSrvHeap().Get()};
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
	style.WindowPadding = ImVec2(8, 8);
	ImVec4* colors = style.Colors;

	// 全体背景
	colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.0f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.0f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.0f);

	// テキスト
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.92f, 0.95f, 1.0f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.58f, 1.0f);

	// フレーム
	colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.20f, 0.22f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.27f, 0.30f, 1.0f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.33f, 0.36f, 1.0f);

	// ボタン
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.25f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.30f, 0.33f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.35f, 0.38f, 1.0f);

	// タブ
	colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.18f, 0.20f, 1.0f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.27f, 0.30f, 1.0f);
	colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.30f, 0.33f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.20f, 0.22f, 1.0f);

	// ヘッダー（例: 選択リストやツリー）
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.25f, 1.0f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.27f, 0.30f, 1.0f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.33f, 0.36f, 1.0f);

	// タイトルバー
	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.20f, 0.22f, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);

	// スクロールバー
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.37f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.42f, 1.0f);

	// ボーダー・セパレーター
	colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.26f, 1.0f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.34f, 0.34f, 0.36f, 1.0f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.42f, 1.0f);

	// プロット（折れ線/棒）
	colors[ImGuiCol_PlotLines] = ImVec4(0.70f, 0.75f, 0.85f, 1.0f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.90f, 0.95f, 1.0f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.65f, 0.70f, 1.0f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.75f, 0.78f, 0.80f, 1.0f);

	// 丸みなどのスタイル
	style.WindowRounding = 4.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;
}
