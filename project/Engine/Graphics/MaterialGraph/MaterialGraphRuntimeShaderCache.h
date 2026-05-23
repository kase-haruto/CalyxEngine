#pragma once

#include <Engine/Assets/DataAsset/MaterialAsset.h>
#include <Engine/Graphics/MaterialGraph/ShaderGraphCodeGenerator.h>
#include <Engine/Graphics/Pipeline/Shader/ShaderCompiler.h>

#include <dxcapi.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <wrl.h>

namespace CalyxEngine {
	struct MaterialGraphRuntimeShader {
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShader;
		std::string hlsl;
		std::string compileMessage;
		std::size_t hash = 0;
		bool cacheHit = false;
		bool usesObjectTexture = false;
		bool compileSucceeded = false;
		bool fallbackUsed = false;
	};

	class MaterialGraphRuntimeShaderCache {
	public:
		MaterialGraphRuntimeShader GetOrCompileObject3DPixelShader(MaterialAsset& material) {
			const GeneratedShaderGraphCode generated = ShaderGraphCodeGenerator::GenerateObject3DRuntimePixelShaderSource(material);
			const std::size_t hash = HashGeneratedShader(generated.hlsl);
			const std::string key = material.GetGuid().ToString() + ":object3d:" + std::to_string(hash);

			auto it = shaders_.find(key);
			if(it != shaders_.end()) {
				MaterialGraphRuntimeShader result = it->second;
				result.cacheHit = true;
				result.compileSucceeded = true;
				result.fallbackUsed = false;
				return result;
			}

			ShaderCompiler compiler;
			compiler.InitializeDXC();
			MaterialGraphRuntimeShader shader;
			shader.hlsl = generated.hlsl;
			shader.hash = hash;
			shader.cacheHit = false;
			shader.usesObjectTexture = generated.usesObjectTexture;
			const ShaderCompileResult compileResult = compiler.TryCompileSource(L"MaterialGraphRuntimeObject3D.PS.hlsl", generated.hlsl, L"ps_6_5");
			shader.pixelShader = compileResult.bytecode;
			shader.compileMessage = compileResult.errors;
			shader.compileSucceeded = compileResult.succeeded;
			if(!compileResult.succeeded) {
				return FallbackOrFailedShader(material.GetGuid().ToString() + ":object3d", shader);
			}
			shaders_[key] = shader;
			latestSuccessfulShaders_[material.GetGuid().ToString() + ":object3d"] = shader;
			return shader;
		}

		MaterialGraphRuntimeShader GetOrCompilePreviewPixelShader(MaterialAsset& material) {
			const GeneratedShaderGraphCode generated = ShaderGraphCodeGenerator::GeneratePreviewPixelShaderSource(material);
			const std::size_t hash = HashGeneratedShader(generated.hlsl);
			const std::string key = material.GetGuid().ToString() + ":" + std::to_string(hash);

			auto it = shaders_.find(key);
			if(it != shaders_.end()) {
				MaterialGraphRuntimeShader result = it->second;
				result.cacheHit = true;
				result.compileSucceeded = true;
				result.fallbackUsed = false;
				return result;
			}

			ShaderCompiler compiler;
			compiler.InitializeDXC();
			MaterialGraphRuntimeShader shader;
			shader.hlsl = generated.hlsl;
			shader.hash = hash;
			shader.cacheHit = false;
			shader.usesObjectTexture = generated.usesObjectTexture;
			const ShaderCompileResult compileResult = compiler.TryCompileSource(L"MaterialGraphRuntimePreview.PS.hlsl", generated.hlsl, L"ps_6_5");
			shader.pixelShader = compileResult.bytecode;
			shader.compileMessage = compileResult.errors;
			shader.compileSucceeded = compileResult.succeeded;
			if(!compileResult.succeeded) {
				return FallbackOrFailedShader(material.GetGuid().ToString() + ":preview", shader);
			}
			shaders_[key] = shader;
			latestSuccessfulShaders_[material.GetGuid().ToString() + ":preview"] = shader;
			return shader;
		}

		void Clear() {
			shaders_.clear();
			latestSuccessfulShaders_.clear();
		}

		std::size_t Size() const {
			return shaders_.size();
		}

	private:
		MaterialGraphRuntimeShader FallbackOrFailedShader(const std::string& fallbackKey, MaterialGraphRuntimeShader failedShader) const {
			auto fallback = latestSuccessfulShaders_.find(fallbackKey);
			if(fallback == latestSuccessfulShaders_.end()) return failedShader;

			MaterialGraphRuntimeShader result = fallback->second;
			result.cacheHit = false;
			result.compileSucceeded = false;
			result.fallbackUsed = true;
			result.compileMessage = failedShader.compileMessage;
			return result;
		}

		static std::size_t HashGeneratedShader(const std::string& hlsl) {
			return std::hash<std::string>{}(hlsl);
		}

		std::unordered_map<std::string, MaterialGraphRuntimeShader> shaders_;
		std::unordered_map<std::string, MaterialGraphRuntimeShader> latestSuccessfulShaders_;
	};
}
