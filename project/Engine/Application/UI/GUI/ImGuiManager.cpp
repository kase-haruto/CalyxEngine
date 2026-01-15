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
#if defined(_DEBUG) || defined(DEVELOP)
#include <externals/imgui/imgui.h>
#include "imgui/ImGuizmo.h"
#endif // _DEBUG

#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>


void ImGuiManager::Initialize(WinApp* winApp, const CalyxGraphics::DxCore* dxCore){
	pDxCore_ =dxCore;

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
	ImFont* font = io.Fonts->AddFontFromFileTTF("Resources/Assets/fonts/inter.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
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
#if defined(_DEBUG) || defined(DEVELOP)
	ImGuizmo::BeginFrame();

#endif // _DEBUG	


	ComPtr<ID3D12GraphicsCommandList> commandList =pDxCore_->GetCommandList();
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
	ComPtr<ID3D12GraphicsCommandList> commandList =pDxCore_->GetCommandList();
	//描画コマンドを発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void ImGuiManager::CustomizeImGuiStyle() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// === Unreal Engine 5 風 カラーパレット (Dark Theme) ===
	const ImVec4 bgDark       = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	const ImVec4 panelDark    = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // パネル背景
	const ImVec4 panelHover   = ImVec4(0.24f, 0.24f, 0.24f, 1.00f); // ホバー時
	const ImVec4 panelActive  = ImVec4(0.32f, 0.32f, 0.32f, 1.00f); // アクティブ
	
	const ImVec4 accentColor  = ImVec4(1.00f, 0.35f, 0.15f, 1.00f);
	const ImVec4 accentHover  = ImVec4(1.00f, 0.45f, 0.25f, 1.00f);
	const ImVec4 accentActive = ImVec4(0.80f, 0.25f, 0.05f, 1.00f);

	const ImVec4 textMain     = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	const ImVec4 textDim      = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

	// --- Colors ---
	colors[ImGuiCol_Text]                 = textMain;
	colors[ImGuiCol_TextDisabled]         = textDim;
	colors[ImGuiCol_WindowBg]             = bgDark;
	colors[ImGuiCol_ChildBg]              = bgDark;
	colors[ImGuiCol_PopupBg]              = panelDark;
	colors[ImGuiCol_Border]               = ImVec4(0.04f, 0.04f, 0.04f, 1.00f); // ボーダーレス
	colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	
	// Frame (Input fields, checkboxes)
	colors[ImGuiCol_FrameBg]              = ImVec4(0.03f, 0.03f, 0.03f, 1.00f); 
	colors[ImGuiCol_FrameBgHovered]       = panelHover;
	colors[ImGuiCol_FrameBgActive]        = panelActive;

	// Title
	colors[ImGuiCol_TitleBg]              = bgDark;
	colors[ImGuiCol_TitleBgActive]        = bgDark;
	colors[ImGuiCol_TitleBgCollapsed]     = bgDark;

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

	// Slider / Checkmark
	colors[ImGuiCol_CheckMark]            = accentColor;
	colors[ImGuiCol_SliderGrab]           = accentColor;
	colors[ImGuiCol_SliderGrabActive]     = accentActive;

	// Buttons
	colors[ImGuiCol_Button]               = panelDark;
	colors[ImGuiCol_ButtonHovered]        = panelHover;
	colors[ImGuiCol_ButtonActive]         = panelActive;

	// Headers (CollapsingHeader, TreeNodes)
	colors[ImGuiCol_Header]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_HeaderHovered]        = panelHover;
	colors[ImGuiCol_HeaderActive]         = panelActive;

	// Tabs
	colors[ImGuiCol_Tab]                  = bgDark;
	colors[ImGuiCol_TabHovered]           = panelHover;
	colors[ImGuiCol_TabActive]            = panelDark; // アクティブなタブはパネル背景と一体化
	colors[ImGuiCol_TabUnfocused]         = bgDark;
	colors[ImGuiCol_TabUnfocusedActive]   = panelDark;

	// Docking
	colors[ImGuiCol_DockingPreview]       = accentColor;
	colors[ImGuiCol_DockingEmptyBg]       = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

	// Selection
	colors[ImGuiCol_TextSelectedBg]       = ImVec4(1.00f, 0.35f, 0.15f, 0.35f);
	colors[ImGuiCol_DragDropTarget]       = accentColor;
	colors[ImGuiCol_NavWindowingHighlight]= ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	// --- Style Vars ---
	style.WindowPadding     = ImVec2(8.0f, 8.0f);
	style.FramePadding      = ImVec2(5.0f, 5.0f); // 入力欄を少し広めに
	style.CellPadding       = ImVec2(4.0f, 4.0f);
	style.ItemSpacing       = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	style.IndentSpacing     = 21.0f;
	style.ScrollbarSize     = 14.0f; // 少し太く視認しやすく
	style.GrabMinSize       = 10.0f;

	style.WindowBorderSize  = 0.0f; // フラットデザイン
	style.ChildBorderSize   = 0.0f;
	style.PopupBorderSize   = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.TabBorderSize     = 0.0f;

	style.WindowRounding    = 0.0f; // ウィンドウは角ばらせる
	style.ChildRounding     = 0.0f;
	style.FrameRounding     = 3.0f; // UI部品は少しだけ丸める
	style.PopupRounding     = 3.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding      = 3.0f;
	style.LogSliderDeadzone = 4.0f;
	style.TabRounding       = 4.0f; // タブの上部は丸める
	
	// ──────────── Separators (Gaps) ────────────
	colors[ImGuiCol_Separator]            = bgDark;
	colors[ImGuiCol_SeparatorHovered]     = accentHover;
	colors[ImGuiCol_SeparatorActive]      = accentActive;

	// Docking Gaps
	style.DockingSeparatorSize     = 4.0f;
}