#include "Audio.h"
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <stdexcept>
#include <iostream>   // デバッグ用などに
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
	std::filesystem::path ResolveAudioFile(const std::string& filename) {
		const std::filesystem::path requested(filename);
		if(requested.has_parent_path() && std::filesystem::exists(requested)) return requested;

		auto* database = AssetDatabase::GetInstance();
		for(const AssetRecord* record : database->GetView()) {
			if(record && record->type == AssetType::Audio &&
			   record->sourcePath.filename().generic_string() == requested.filename().generic_string()) {
				return record->sourcePath;
			}
		}

		// オーディオをAssets外に置いている既存プロジェクトとの互換性を維持する。
		return std::filesystem::path("Resources/sounds") / requested.filename();
	}
}

Audio::~Audio() = default;

/////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Initialize(){
	// SourceVoiceとMasteringVoiceの基盤となるXAudio2インターフェースを先に生成する。
	HRESULT hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 全再生Voiceの最終出力先となるMasteringVoiceをXAudio2より後に生成する。
	hr = xAudio2_->CreateMasteringVoice(&masteringVoice_);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// MP3/M4Aデコードを利用可能にするためMedia Foundationを起動する。
	hr = InitializeMediaFoundation();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 起動時常駐対象が追加された場合も初期化順序を統一できるよう専用処理へ委譲する。
	StartUpLoad();
}

/////////////////////////////////////////////////////////////////////////////////////
// StartUpLoad
/////////////////////////////////////////////////////////////////////////////////////
void Audio::StartUpLoad(){
}

void Audio::Finalize(){
	// SourceVoiceをMasteringVoiceとXAudio2本体より先に破棄し、参照先の解放順序を守る。
	UnloadAllAudio();
	// SourceVoiceを全て破棄した後で、出力先となるMasteringVoiceを解放する。
	if(masteringVoice_) {
		masteringVoice_->DestroyVoice();
		masteringVoice_ = nullptr;
	}
	// MasteringVoiceが参照しなくなってからXAudio2本体を解放する。
	xAudio2_.Reset();
	// Media Foundationで保持されるデコーダー基盤を最後に終了する。
	MFShutdown();
}

/////////////////////////////////////////////////////////////////////////////////////
// InitializeMediaFoundation
/////////////////////////////////////////////////////////////////////////////////////
HRESULT Audio::InitializeMediaFoundation(){
	HRESULT hr = MFStartup(MF_VERSION);
	if (FAILED(hr)){
		return hr;
	}
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////
// PlayAudio(内部実装)
/////////////////////////////////////////////////////////////////////////////////////
void Audio::PlayAudio(
	IXAudio2* xAudio2,
	const SoundData& soundData,
	const std::string& filename,
	bool loop,
	float volume
){
	HRESULT hr;
	std::string tmpFilename = filename;

	// 同じキーのLoop再生を置換できるよう、既存SourceVoiceを先に破棄する。
	if (sourceVoices_[filename] != nullptr){
		sourceVoices_[filename].reset();
	}

	// XAudio2から受け取る生VoiceをDestroyVoiceデリータ付きunique_ptrへ直ちに移す。
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	if (loop){
		hr = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");
		sourceVoices_[filename] = std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>(pSourceVoice);
	} else{
		// One-shotの同時再生を許可するため、既存キーと重ならない連番キーを生成する。
		int count = 0;
		while (sourceVoices_.find(tmpFilename) != sourceVoices_.end()){
			count++;
			tmpFilename = filename + std::to_string(count);
		}
		hr = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");
		sourceVoices_[tmpFilename] = std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>(pSourceVoice);
	}

	// SoundDataのPCM領域を再生完了まで保持したままXAudio2バッファへ関連付ける。
	XAUDIO2_BUFFER buf {};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	if (loop){
		// Loop再生はXAudio2側で繰り返し、毎周回の再投入を不要にする。
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	hr = sourceVoices_[tmpFilename]->SubmitSourceBuffer(&buf);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 再生開始前に音量を設定し、先頭サンプルだけ既定音量で鳴ることを防ぐ。
	hr = sourceVoices_[tmpFilename]->SetVolume(volume);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// Buffer投入と音量設定が成功したVoiceだけを再生開始する。
	hr = sourceVoices_[tmpFilename]->Start();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声を再生する(外部インターフェース)
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Play(const std::string& filename, bool loop, float volume){
	// ロード済みか確認
	CX_CHECK(audios_.find(filename) != audios_.end(), "Assertion failed");

	// 実際の再生を呼び出し
	PlayAudio(
		xAudio2_.Get(),
		audios_[filename],
		filename,
		loop,
		volume
	);

	// 再生フラグを立てる
	isPlaying_[filename] = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声の再生を終了する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::EndAudio(const std::string& filename){
	// SourceVoiceがなければエラー
	CX_CHECK(sourceVoices_.find(filename) != sourceVoices_.end(), "Assertion failed");

	HRESULT hr;
	hr = sourceVoices_[filename]->Stop();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	hr = sourceVoices_[filename]->FlushSourceBuffers();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// ボイス解放
	sourceVoices_[filename].reset();

	// 再生フラグをおろす
	isPlaying_[filename] = false;
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声を一時停止する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::PauseAudio(const std::string& filename){
	// SourceVoiceがなければエラー
	CX_CHECK(sourceVoices_.find(filename) != sourceVoices_.end(), "Assertion failed");

	HRESULT hr = S_OK;
	hr = sourceVoices_[filename]->Stop();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 再生フラグをおろす
	isPlaying_[filename] = false;
}

/////////////////////////////////////////////////////////////////////////////////////
// 一時停止中の音声を再開する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::RestartAudio(const std::string& filename){
	// SourceVoiceがなければエラー
	CX_CHECK(sourceVoices_.find(filename) != sourceVoices_.end(), "Assertion failed");
	HRESULT hr = S_OK;
	if (sourceVoices_[filename] != nullptr){
		hr = sourceVoices_[filename]->Start();
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");

		isPlaying_[filename] = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// 音量を設定する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::SetAudioVolume(const std::string& filename, float volume){
	// SourceVoiceがなければエラー
	CX_CHECK(sourceVoices_.find(filename) != sourceVoices_.end(), "Assertion failed");

	sourceVoices_[filename]->SetVolume(volume);
}

/////////////////////////////////////////////////////////////////////////////////////
// 再生中かどうかを返す
/////////////////////////////////////////////////////////////////////////////////////
bool Audio::IsPlayingAudio(const std::string& filename){
	// フラグがなければエラー
	CX_CHECK(isPlaying_.find(filename) != isPlaying_.end(), "Assertion failed");
	return isPlaying_[filename];
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声をロードする
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Load(const std::string& filename){
	// 同じ論理名のPCMデータを共有し、重複デコードとメモリ消費を避ける。
	if (audios_.find(filename) == audios_.end()){
		// AssetDatabaseを優先し、旧Resources/sounds配置へもフォールバックして互換性を維持する。
		const std::filesystem::path correctPath = ResolveAudioFile(filename);
		CX_CHECK(std::filesystem::exists(correctPath), "Audio file was not found");

		// 大文字小文字を正規化した拡張子だけでデコーダーを選択する。
		size_t pos = filename.find_last_of('.');
		if (pos == std::string::npos || pos == filename.length() - 1){
			CX_CHECK(false && "No valid extension found.", "Assertion failed");
		}
		std::string extension = filename.substr(pos + 1);
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// WAVは直接RIFF解析し、圧縮形式はMedia FoundationでPCMへ展開する。
		if (extension == "wav"){
			audios_[filename] = LoadWave(correctPath.string().c_str());
		} else if (extension == "mp3" || extension == "m4a"){
			audios_[filename] = LoadMP3(correctPath.wstring().c_str());
		} else{
			// 未対応フォーマット
			CX_CHECK(false && "Unsupported audio format.", "Assertion failed");
		}

		// Load直後はVoice未生成のため、外部照会用の再生状態をfalseで登録する。
		isPlaying_[filename] = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// WAVファイルの読み込み
/////////////////////////////////////////////////////////////////////////////////////
SoundData Audio::LoadWave(const char* filename){
	std::ifstream file;

	// バイナリ形式で開く
	file.open(filename, std::ios_base::binary);
	CX_CHECK(file.is_open(), "Assertion failed");

	// RIFF/WAVE識別子を検証し、別形式をWAVとして読み進めないようにする。
	RiffHeader riff;
	file.read(( char* ) &riff, sizeof(riff));
	// チェック
	CX_CHECK(strncmp(riff.chunk.id, "RIFF", 4) == 0, "Assertion failed");
	CX_CHECK(strncmp(riff.type, "WAVE", 4) == 0, "Assertion failed");

	// 可変順序のRIFFチャンクを走査し、SourceVoice生成に必要なfmtチャンクを探す。
	FormatChunk format = {};
	while (true){
		ChunkHeader chunkHeader;
		file.read(( char* ) &chunkHeader, sizeof(ChunkHeader));
		if (strncmp(chunkHeader.id, "fmt ", 4) == 0){
			format.chunk = chunkHeader;
			file.read(( char* ) &format.fmt, format.chunk.size);
			CX_CHECK(format.chunk.size <= sizeof(format.fmt), "Assertion failed");
			break;
		} else{
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}

		if (file.eof()){
			CX_CHECK(0 && "Reached end of file without finding 'fmt ' chunk", "Assertion failed");
			return SoundData {};
		}
	}

	// fmt以外の任意チャンクを読み飛ばし、PCM本体を持つdataチャンクを探す。
	ChunkHeader data;
	while (true){
		file.read(( char* ) &data, sizeof(data));
		if (strncmp(data.id, "data", 4) == 0){
			break;
		} else{
			file.seekg(data.size, std::ios_base::cur);
		}

		if (file.eof()){
			CX_CHECK(0 && "Reached end of file without finding 'data' chunk", "Assertion failed");
			return SoundData {};
		}
	}

	// dataチャンクの宣言サイズだけPCM領域を確保して一括読込する。
	std::vector<BYTE> buffer(data.size);
	file.read(reinterpret_cast<char*>(buffer.data()), data.size);

	// 閉じる
	file.close();

	// XAudio2へ渡すFormatとPCM領域を同じSoundDataへまとめて返す。
	SoundData soundData {};
	soundData.wfex = format.fmt;
	soundData.buffer = std::move(buffer);

	return soundData;
}

/////////////////////////////////////////////////////////////////////////////////////
// MP3/M4Aファイルの読み込み (MediaFoundation)
/////////////////////////////////////////////////////////////////////////////////////
SoundData Audio::LoadMP3(const wchar_t* filename){
	// 圧縮音声を単独ロードする経路でも利用できるようMedia Foundationの起動を確認する。
	HRESULT hr = InitializeMediaFoundation();
	if (FAILED(hr)){
		throw std::runtime_error("Media Foundation initialization failed.");
	}

	IMFSourceReader* pReader = nullptr;
	IMFMediaType* pOutputType = nullptr;

	// Source Reader作成
	hr = MFCreateSourceReaderFromURL(filename, nullptr, &pReader);
	if (FAILED(hr)){
		MFShutdown();
		throw std::runtime_error("Failed to create Source Reader.");
	}

	// XAudio2へ直接投入できるようSource Readerの出力を非圧縮PCMへ固定する。
	hr = MFCreateMediaType(&pOutputType);
	if (FAILED(hr)){
		pReader->Release();
		MFShutdown();
		throw std::runtime_error("Failed to create output media type.");
	}

	hr = pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	hr = pOutputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	hr = pReader->SetCurrentMediaType(( DWORD ) MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pOutputType);
	if (FAILED(hr)){
		pOutputType->Release();
		pReader->Release();
		MFShutdown();
		throw std::runtime_error("Failed to set media type.");
	}

	// 使い終わったのでRelease
	pOutputType->Release();
	pOutputType = nullptr;

	// 実際に設定されたMediaTypeを取得
	pReader->GetCurrentMediaType(( DWORD ) MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutputType);

	// Media Foundationが返す複数Sampleを連続PCMバイト列へ結合する。
	std::vector<BYTE> audioData;
	while (true){
		IMFSample* pMFSample {nullptr};
		DWORD dwStreamFlags {0};
		pReader->ReadSample(
			( DWORD ) MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			0,
			nullptr,
			&dwStreamFlags,
			nullptr,
			&pMFSample
		);

		if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM){
			break;
		}

		// Sample内の分割Bufferを連続Bufferへ変換してからCPUメモリへコピーする。
		if (pMFSample){
			IMFMediaBuffer* pMFMediaBuffer {nullptr};
			pMFSample->ConvertToContiguousBuffer(&pMFMediaBuffer);

			BYTE* pBuffer {nullptr};
			DWORD cbCurrentLength {0};
			pMFMediaBuffer->Lock(&pBuffer, nullptr, &cbCurrentLength);

			size_t oldSize = audioData.size();
			audioData.resize(oldSize + cbCurrentLength);
			memcpy(audioData.data() + oldSize, pBuffer, cbCurrentLength);

			pMFMediaBuffer->Unlock();
			pMFMediaBuffer->Release();
			pMFSample->Release();
		}
	}

	// SoundDataに格納
	SoundData soundData;
	soundData.buffer = std::move(audioData);

	// フォーマット情報を取得
	hr = pOutputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, ( UINT32* ) &soundData.wfex.nChannels);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	hr = pOutputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, ( UINT32* ) &soundData.wfex.nSamplesPerSec);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	hr = pOutputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, ( UINT32* ) &soundData.wfex.wBitsPerSample);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	soundData.wfex.wFormatTag = WAVE_FORMAT_PCM;
	soundData.wfex.nBlockAlign = soundData.wfex.nChannels * soundData.wfex.wBitsPerSample / 8;
	soundData.wfex.nAvgBytesPerSec = soundData.wfex.nSamplesPerSec * soundData.wfex.nBlockAlign;
	soundData.wfex.cbSize = 0;

	// 取得時と逆順にCOMオブジェクトをReleaseし、SoundDataだけを呼出し側へ返す。
	pOutputType->Release();
	pReader->Release();
	MFShutdown();

	return soundData;
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声アンロード
/////////////////////////////////////////////////////////////////////////////////////
void Audio::UnloadAudio(const std::string& filename){
	// まだロードされていなければエラー
	CX_CHECK(audios_.find(filename) != audios_.end(), "Assertion failed");

	// PCMデータを消す前に対象名の登録を外し、以後のPlayで参照されないようにする。
	audios_.erase(filename);

	// unique_ptrのカスタムデリータを通してSourceVoiceをDestroyVoiceする。
	sourceVoices_.erase(filename);
	// 外部照会用の再生フラグも同時に破棄し、3個のMapを同じキー集合に保つ。
	isPlaying_.erase(filename);
}

/////////////////////////////////////////////////////////////////////////////////////
// すべての音声をアンロード
/////////////////////////////////////////////////////////////////////////////////////
void Audio::UnloadAllAudio(){
	for (auto& pair : audios_){
		UnloadAudio(&pair.second);
	}
	audios_.clear();

	sourceVoices_.clear();
	isPlaying_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////
// UnloadAudio(内部実装)
/////////////////////////////////////////////////////////////////////////////////////
void Audio::UnloadAudio(SoundData* soundData){
	soundData->buffer.clear();
	soundData->wfex = {};
}
