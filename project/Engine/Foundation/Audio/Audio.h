#pragma once

/////////////////////////////////////////////////////////////////////////////////////
// audio
/////////////////////////////////////////////////////////////////////////////////////
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

/////////////////////////////////////////////////////////////////////////////////////
// Media Foundation
/////////////////////////////////////////////////////////////////////////////////////
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

/////////////////////////////////////////////////////////////////////////////////////
// file
/////////////////////////////////////////////////////////////////////////////////////
#include <fstream>

/////////////////////////////////////////////////////////////////////////////////////
// ComPtr
/////////////////////////////////////////////////////////////////////////////////////
#include <wrl/client.h>
#include <Engine/Foundation/Export/CalyxAPI.h>

using Microsoft::WRL::ComPtr;

/////////////////////////////////////////////////////////////////////////////////////
// cstdint
/////////////////////////////////////////////////////////////////////////////////////
#include <cstdint>

/////////////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <unordered_map>
#include <vector>

/////////////////////////////////////////////////////////////////////////////////////
//  読み込みに必要な構造体
/////////////////////////////////////////////////////////////////////////////////////
struct ChunkHeader {
	char	id[4];
	int32_t size;
};

struct RiffHeader {
	ChunkHeader chunk;
	char		type[4];
};

struct FormatChunk {
	ChunkHeader	 chunk;
	WAVEFORMATEX fmt;
};

/////////////////////////////////////////////////////////////////////////////////////
//  サウンドデータを格納する構造体
/////////////////////////////////////////////////////////////////////////////////////
struct SoundData {
	// 波形フォーマット
	WAVEFORMATEX wfex{};
	// バッファ
	std::vector<BYTE> buffer;
};

// SourceVoice用カスタムデリータ
struct SourceVoiceDeleter {
	void operator()(IXAudio2SourceVoice* voice) const {
		if (voice) {
			voice->DestroyVoice();
		}
	}
};

/*-----------------------------------------------------------------------------------------
 * Audio
 * - オーディオ管理クラス
 * - XAudio2を使用した音声再生（WAV, MP3, M4A対応）を管理するシングルトン
 *---------------------------------------------------------------------------------------*/
class Audio {
public:
	// privateコンストラクタ
	Audio() = default;

	/**
	 * \brief デストラクタ
	 */
	~Audio();

public: // 初期化に関する関数
	/**
	 * \brief 初期化
	 */
	void Initialize();
	/**
	 * \brief ロード開始
	 */
	void StartUpLoad();
	/**
	 * \brief 終了処理
	 */
	void Finalize();
	/**
	 * \brief Media Foundationの初期化
	 * \return 成功したか
	 */
	HRESULT		InitializeMediaFoundation();

public: // エンジンで利用できる関数
	/**
	 * \brief 音声を再生
	 * \param filename ファイル名
	 * \param loop ループするか
	 * \param volume 音量
	 */
	CALYX_API void Play(const std::string& filename, bool loop, float volume = 1.0f);
	/**
	 * \brief 音声を停止
	 * \param filename ファイル名
	 */
	CALYX_API void EndAudio(const std::string& filename);
	/**
	 * \brief 音声を一時停止
	 * \param filename ファイル名
	 */
	CALYX_API void PauseAudio(const std::string& filename);
	/**
	 * \brief 音声を再開
	 * \param filename ファイル名
	 */
	CALYX_API void RestartAudio(const std::string& filename);
	/**
	 * \brief 音量を設定
	 * \param filename ファイル名
	 * \param volume 音量
	 */
	CALYX_API void SetAudioVolume(const std::string& filename, float volume);
	/**
	 * \brief 再生中か
	 * \param filename ファイル名
	 * \return 再生中か
	 */
	CALYX_API bool IsPlayingAudio(const std::string& filename);
	/**
	 * \brief 音声をロード
	 * \param filename ファイル名
	 */
	CALYX_API void Load(const std::string& filename);
	/**
	 * \brief 音声をアンロード
	 * \param filename ファイル名
	 */
	CALYX_API void UnloadAudio(const std::string& filename);
	/**
	 * \brief 全ての音声をアンロード
	 */
	CALYX_API void UnloadAllAudio();

private:
	// 内部的に再生処理を行う
	void PlayAudio(IXAudio2* xAudio2, const SoundData& soundData, const std::string& filename, bool loop, float volume);

	// WAVファイルをロードする
	SoundData LoadWave(const char* filename);

	// MP3/M4Aファイルをロードする(MF使用)
	SoundData LoadMP3(const wchar_t* filename);

	// SoundDataの解放処理
	void UnloadAudio(SoundData* soundData);

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	ComPtr<IXAudio2> xAudio2_; //< XAudio2インターフェース
	IXAudio2MasteringVoice* masteringVoice_ = nullptr; //< マスターボイス

	std::unordered_map<std::string, SoundData> audios_; //< ロード済み音声データ
	std::unordered_map<std::string, std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>> sourceVoices_; //< 再生中のソースボイス
	std::unordered_map<std::string, bool> isPlaying_; //< 再生中フラグ

};
