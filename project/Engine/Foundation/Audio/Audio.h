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
struct ChunkHeader{
    char id[4];
    int32_t size;
};

struct RiffHeader{
    ChunkHeader chunk;
    char type[4];
};

struct FormatChunk{
    ChunkHeader chunk;
    WAVEFORMATEX fmt;
};

/////////////////////////////////////////////////////////////////////////////////////
//  サウンドデータを格納する構造体
/////////////////////////////////////////////////////////////////////////////////////
struct SoundData{
    // 波形フォーマット
    WAVEFORMATEX wfex {};
    // バッファの先頭アドレス
    BYTE* pBuffer = nullptr;
    // バッファのサイズ
    uint32_t bufferSize = 0;
};


/////////////////////////////////////////////////////////////////////////////////////
//  Audio クラス
/////////////////////////////////////////////////////////////////////////////////////
class Audio{

private:
    // privateコンストラクタ
    Audio() = default;

    // コピー禁止
    Audio(const Audio&) = delete;
    void operator=(const Audio&) = delete;

public:
    // デストラクタ
    ~Audio();

    // インスタンスの取得
    static const Audio* GetInstance();


public:// 初期化に関する関数
    static void Initialize();
    static void StartUpLoad();
	static void Finalize();
    HRESULT InitializeMediaFoundation();

public:// エンジンで利用できる関数
    static void Play(const std::string& filename, bool loop, float volume = 1.0f);
    static void EndAudio(const std::string& filename);
    static void PauseAudio(const std::string& filename);
    static void RestertAudio(const std::string& filename);
    static void SetAudioVolume(const std::string& filename, float volume);
    static bool IsPlayingAudio(const std::string& filename);
    static void Load(const std::string& filename);
    static void UnloadAudio(const std::string& filename);
    static void UnloadAllAudio();

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
    // シングルトンインスタンス
    static Audio* instance_;

    // XAudio2インターフェース
    ComPtr<IXAudio2> xAudio2_;
    // マスターボイス
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;

    // ロード済み音声データ (filename -> SoundData)
    std::unordered_map<std::string, SoundData> audios_;
    // 再生中のソースボイス (filename -> SourceVoice)
    std::unordered_map<std::string, IXAudio2SourceVoice*> sourceVoices_;
    // 再生中かどうか (filename -> bool)
    std::unordered_map<std::string, bool> isPlaying_;

private:
    // 音声ファイルディレクトリパス
    static const std::string directoryPath_;
};
