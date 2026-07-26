#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CalyxEngine {
	enum class ShaderResourceKind : int32_t {
		Unknown,
		CBuffer,
		Texture,
		Sampler,
		UAV,
		StructuredBuffer,
		RaytracingAccelerationStructure,
	};

	/*-----------------------------------------------------------------------------------------
	 * ShaderResourceBinding
	 * - Shader Reflectionで取得した一つのResource Bindingを表すデータ構造
	 * - 種別、Register番号、配列数、Register Spaceを保持する
	 *---------------------------------------------------------------------------------------*/
	struct ShaderResourceBinding {
		std::string name;
		ShaderResourceKind kind = ShaderResourceKind::Unknown;
		uint32_t bindPoint = 0;
		uint32_t bindCount = 0;
		uint32_t space = 0;
	};

	/*-----------------------------------------------------------------------------------------
	 * ShaderCBufferVariable
	 * - Constant Buffer内の一変数のReflection情報を保持するデータ構造
	 * - 変数名、バイトオフセット、バイトサイズを管理する
	 *---------------------------------------------------------------------------------------*/
	struct ShaderCBufferVariable {
		std::string name;
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	/*-----------------------------------------------------------------------------------------
	 * ShaderCBufferLayout
	 * - Constant Buffer全体のReflectionレイアウトを保持するデータ構造
	 * - Binding位置、総サイズ、内部変数の配置を管理する
	 *---------------------------------------------------------------------------------------*/
	struct ShaderCBufferLayout {
		std::string name;
		uint32_t size = 0;
		uint32_t bindPoint = 0;
		uint32_t space = 0;
		std::vector<ShaderCBufferVariable> variables;
	};

	/*-----------------------------------------------------------------------------------------
	 * ShaderReflectionInfo
	 * - 一つのShaderから取得したReflection結果をまとめるデータ構造
	 * - Entry Point、Profile、Resource、Constant Buffer情報を保持する
	 *---------------------------------------------------------------------------------------*/
	struct ShaderReflectionInfo {
		std::string entryPoint;
		std::string profile;
		std::vector<ShaderResourceBinding> resources;
		std::vector<ShaderCBufferLayout> cbuffers;
	};
}
