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
#if defined(_DEBUG) || defined(DEVELOP)
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

void ImGuiManager::CustomizeImGuiStyle() {

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// === Unreal Engine風 カラーパレット ===
	const ImVec4 darkBg      = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	const ImVec4 darkPanel   = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	const ImVec4 midPanel    = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	const ImVec4 darkHover   = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	const ImVec4 darkActive  = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	const ImVec4 borderDim   = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

	const ImVec4 highlight     = ImVec4(0.10f, 0.45f, 0.90f, 1.00f); // ブルーアクセント
	const ImVec4 highlightLow  = ImVec4(0.10f, 0.45f, 0.90f, 0.50f);
	const ImVec4 highlightHigh = ImVec4(0.25f, 0.65f, 1.00f, 1.00f);

	// === 基本色 ===
	colors[ImGuiCol_Text]                 = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]             = darkBg;
	colors[ImGuiCol_ChildBg]              = darkBg;
	colors[ImGuiCol_PopupBg]              = darkPanel;
	colors[ImGuiCol_Border]               = borderDim;     // ← 視認できる細めの枠
	colors[ImGuiCol_BorderShadow]         = ImVec4(0,0,0,0);

	// === Frame / Controls（チェックボックス・入力などの背景）===
	//   背景(darkBg)と同色にしない。少し明るめにして浮かせる。
	colors[ImGuiCol_FrameBg]              = darkPanel;
	colors[ImGuiCol_FrameBgHovered]       = midPanel;
	colors[ImGuiCol_FrameBgActive]        = darkHover;

	// ──────────── タイトルバー ────────────
	colors[ImGuiCol_TitleBg]              = darkBg;
	colors[ImGuiCol_TitleBgActive]        = darkBg;
	colors[ImGuiCol_TitleBgCollapsed]     = darkBg;

	// === Scrollbar / Sliders / Buttons ===
	colors[ImGuiCol_ScrollbarBg]          = darkBg;
	colors[ImGuiCol_ScrollbarGrab]        = darkHover;
	colors[ImGuiCol_ScrollbarGrabHovered] = highlightLow;
	colors[ImGuiCol_ScrollbarGrabActive]  = highlight;
	colors[ImGuiCol_CheckMark]            = highlightHigh; // ← ✓ を明るめに
	colors[ImGuiCol_SliderGrab]           = highlight;
	colors[ImGuiCol_SliderGrabActive]     = highlightHigh;

	colors[ImGuiCol_Button]               = darkPanel;
	colors[ImGuiCol_ButtonHovered]        = highlightLow;
	colors[ImGuiCol_ButtonActive]         = highlight;

	// ──────────── Header / Tree ────────────
	colors[ImGuiCol_Header]               = darkPanel;
	colors[ImGuiCol_HeaderHovered]        = midPanel;
	colors[ImGuiCol_HeaderActive]         = darkHover;
	colors[ImGuiCol_MenuBarBg]            = darkBg;

	// ──────────── Tabs ────────────
	colors[ImGuiCol_Tab]                  = darkPanel;
	colors[ImGuiCol_TabHovered]           = midPanel;
	colors[ImGuiCol_TabActive]            = darkActive;
	colors[ImGuiCol_TabUnfocused]         = darkPanel;
	colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);

	// === その他 ===
	colors[ImGuiCol_ResizeGrip]           = darkPanel;
	colors[ImGuiCol_ResizeGripHovered]    = highlightLow;
	colors[ImGuiCol_ResizeGripActive]     = highlight;

	colors[ImGuiCol_TextSelectedBg]       = highlightLow;
	colors[ImGuiCol_DragDropTarget]       = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavWindowingHighlight]= highlight;
	colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.20f, 0.20f, 0.20f, 0.30f);
	colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);

	// === スタイル設定 ===
	style.WindowRounding           = 2.0f;
	style.FrameRounding            = 2.0f;
	style.GrabRounding             = 2.0f;
	style.TabRounding              = 2.0f;

	style.ScrollbarSize            = 12.0f;
	style.GrabMinSize              = 12.0f;

	// □の枠線を出す（0だと描かれない／背景に溶けやすい）
	style.FrameBorderSize          = 1.0f;  // ← 重要
	style.WindowBorderSize         = 1.0f;

	// クリックしやすく視認しやすいサイズ感
	style.FramePadding             = ImVec2(6.0f, 4.0f);
	style.ItemSpacing              = ImVec2(8.0f, 6.0f);

	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.SeparatorTextBorderSize  = 2.0f;
	style.DockingSeparatorSize     = 1.0f;
}