#pragma once
#include <d3d12.h>
#include <string>

namespace CalyxEngine {
	class SpriteAsset {
	public:
		//===================================================================*/
		//                   public methods
		//===================================================================*/
		explicit SpriteAsset(const std::string& filePath);

		/**
		 * \brief パスを取得
		 * \return パス文字列の参照
		 */
		const std::string& GetPath() const { return path_; }
		/**
		 * \brief GPUディスクリプタハンドルを取得
		 * \return ハンドル
		 */
		D3D12_GPU_DESCRIPTOR_HANDLE GetHandle() const { return handle_; }

	private:
		//===================================================================*/
		//                   private members
		//===================================================================*/
		std::string					path_;
		D3D12_GPU_DESCRIPTOR_HANDLE handle_{};
	};

} // namespace CalyxEngine
