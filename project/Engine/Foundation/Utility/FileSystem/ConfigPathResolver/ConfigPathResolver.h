#pragma once

// std
#include <string>
#include <optional>

/* ========================================================================
/*	コンフィグのパス生成
/* ===================================================================== */
/**
 * @brief ConfigPathResolverの機能を提供するクラスです。
 */
class ConfigPathResolver {
public:
	// ベースディレクトリ
	static std::string GetBaseDirectory();

	// オブジェクト名とscene名からパスを生成
	static std::string ResolvePath(const std::string& objectType,
								   const std::string& objectName,
								   const std::string& presetName = "Default");
};

