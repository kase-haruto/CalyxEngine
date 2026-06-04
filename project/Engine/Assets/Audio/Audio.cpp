#include "Audio.h"
#include <Engine/Foundation/Debug/CxAssert.h>
#include <stdexcept>
#include <iostream>   // デバッグ用などに

/////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Initialize(){
	// XAudio2初期化
	HRESULT hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	hr = xAudio2_->CreateMasteringVoice(&masteringVoice_);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// Media Foundation初期化
	hr = InitializeMediaFoundation();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 起動時にまとめて読み込む音声の処理
	StartUpLoad();
}

/////////////////////////////////////////////////////////////////////////////////////
// StartUpLoad
/////////////////////////////////////////////////////////////////////////////////////
void Audio::StartUpLoad(){
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

	// 既にSourceVoiceがあれば解放
	if (sourceVoices_[filename] != nullptr){
		sourceVoices_[filename].reset();
	}

	// ソースボイスの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	if (loop){
		hr = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");
		sourceVoices_[filename] = std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>(pSourceVoice);
	} else{
		// 同じfilenameがあった場合、連番をつける等の処理
		int count = 0;
		while (sourceVoices_.find(tmpFilename) != sourceVoices_.end()){
			count++;
			tmpFilename = filename + std::to_string(count);
		}
		hr = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");
		sourceVoices_[tmpFilename] = std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>(pSourceVoice);
	}

	// バッファ設定
	XAUDIO2_BUFFER buf {};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	if (loop){
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	hr = sourceVoices_[tmpFilename]->SubmitSourceBuffer(&buf);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 音量セット
	hr = sourceVoices_[tmpFilename]->SetVolume(volume);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 再生開始
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
void Audio::RestertAudio(const std::string& filename){
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
	// 未ロードならロードを実施
	if (audios_.find(filename) == audios_.end()){
		// 実際のパス
		std::string correctPath = directoryPath_ + filename;

		// 拡張子を取得
		size_t pos = filename.find_last_of('.');
		if (pos == std::string::npos || pos == filename.length() - 1){
			CX_CHECK(false && "No valid extension found.", "Assertion failed");
		}
		std::string extension = filename.substr(pos + 1);

		// 拡張子で振り分け
		if (extension == "wav"){
			audios_[filename] = LoadWave(correctPath.c_str());
		} else if (extension == "mp3" || extension == "m4a"){
			std::wstring wpath(correctPath.begin(), correctPath.end());
			audios_[filename] = LoadMP3(wpath.c_str());
		} else{
			// 未対応フォーマット
			CX_CHECK(false && "Unsupported audio format.", "Assertion failed");
		}

		// 再生フラグ初期化
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

	// RIFFヘッダ
	RiffHeader riff;
	file.read(( char* ) &riff, sizeof(riff));
	// チェック
	CX_CHECK(strncmp(riff.chunk.id, "RIFF", 4) == 0, "Assertion failed");
	CX_CHECK(strncmp(riff.type, "WAVE", 4) == 0, "Assertion failed");

	// フォーマットチャンク
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

	// データチャンク
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

	// データ部読み込み
	std::vector<BYTE> buffer(data.size);
	file.read(reinterpret_cast<char*>(buffer.data()), data.size);

	// 閉じる
	file.close();

	// SoundDataへ格納
	SoundData soundData {};
	soundData.wfex = format.fmt;
	soundData.buffer = std::move(buffer);

	return soundData;
}

/////////////////////////////////////////////////////////////////////////////////////
// MP3/M4Aファイルの読み込み (MediaFoundation)
/////////////////////////////////////////////////////////////////////////////////////
SoundData Audio::LoadMP3(const wchar_t* filename){
	// 初期化(呼び直してもOK)
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

	// 出力タイプをPCM(WAV)に設定
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

	// サンプルを読み取って音声データを展開
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

		// バッファ取得
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

	// 後始末
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

	// SoundData破棄 (vectorなので自動的に解放される)
	audios_.erase(filename);

	// SourceVoice破棄 (unique_ptrなので自動的に解放される)
	sourceVoices_.erase(filename);
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

	for (auto& voicePair : sourceVoices_){
		if (voicePair.second){
			voicePair.second->DestroyVoice();
		}
	}
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