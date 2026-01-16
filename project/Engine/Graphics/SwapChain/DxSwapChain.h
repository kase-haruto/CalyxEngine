#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <cassert>
#include <chrono>

/*-----------------------------------------------------------------------------------------
 * DxSwapChain
 * - DirectX12スワップチェーン管理クラス
 * - バックバッファの管理、画面への表示（Present）を担当
 *---------------------------------------------------------------------------------------*/
class DxSwapChain{
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    /**
     * \brief 初期化
     * \param dxgiFactory DXGIファクトリー
     * \param commandQueue コマンドキュー
     * \param hwnd ウィンドウハンドル
     * \param width 画面幅
     * \param height 画面高さ
     */
    void Initialize(ComPtr<IDXGIFactory7> dxgiFactory, ComPtr<ID3D12CommandQueue> commandQueue, HWND hwnd, uint32_t width, uint32_t height);

    /**
     * \brief 画面表示（フリップ）
     */
    void Present();

    /**
     * \brief リサイズ
     * \param width 幅
     * \param height 高さ
     */
    void Resize(uint32_t width, uint32_t height);

    //===================================================================*/
    //                    accessor
    //===================================================================*/
public:
    /**
     * \brief 現在のバックバッファインデックスを取得
     * \return インデックス
     */
    UINT GetCurrentBackBufferIndex() const{ return swapChain_->GetCurrentBackBufferIndex(); }

    /**
     * \brief スワップチェーン本体を取得
     * \return スワップチェーン
     */
    ComPtr<IDXGISwapChain4> GetSwapChain() const{ return swapChain_; }

    /**
     * \brief 指定インデックスのバックバッファリソースを取得
     * \param index インデックス
     * \return バックバッファリソース
     */
    ComPtr<ID3D12Resource> GetBackBuffer(UINT index) const{ return backBuffers_[index]; }

    /**
     * \brief バックバッファをセット
     * \param index インデックス
     * \param bb バックバッファリソース
     */
    void SetBackBuffer(uint32_t index, ComPtr<ID3D12Resource>bb){ backBuffers_[index] = bb; }

    /**
     * \brief スワップチェーンの設定情報を取得
     * \return 設定情報
     */
    DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc()const{ return swapChainDesc_; }

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
    ComPtr<IDXGISwapChain4> swapChain_ = nullptr; //< スワップチェーン
    std::array<ComPtr<ID3D12Resource>, 2> backBuffers_; //< バックバッファリソース
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc_ {}; //< スワップチェーン設定情報

    float refreshRate_ = 60.0f; //< リフレッシュレート
	UINT syncInterval_ = 1; //< 垂直同期の間隔
};
