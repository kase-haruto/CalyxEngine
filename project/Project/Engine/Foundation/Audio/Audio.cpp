#include "Audio.h"
#include <cassert>
#include <stdexcept>
#include <iostream>   // デバッグ用などに

// 静的メンバ変数の定義
Audio* Audio::instance_ = nullptr;
const std::string Audio::directoryPath_ = "Resources/sounds/";

/////////////////////////////////////////////////////////////////////////////////////
// デストラクタ
/////////////////////////////////////////////////////////////////////////////////////
Audio::~Audio(){

}

/////////////////////////////////////////////////////////////////////////////////////
// インスタンス取得関数
/////////////////////////////////////////////////////////////////////////////////////
const Audio* Audio::GetInstance(){
	if (!instance_){
		instance_ = new Audio();
	}
	return instance_;
}

/////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Initialize(){
	// インスタンスを生成しておく
	GetInstance();

	// XAudio2初期化
	HRESULT hr = XAudio2Create(&instance_->xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));

	hr = instance_->xAudio2_->CreateMasteringVoice(&instance_->masteringVoice_);
	assert(SUCCEEDED(hr));

	// Media Foundation初期化
	hr = instance_->InitializeMediaFoundation();
	assert(SUCCEEDED(hr));

	// 起動時にまとめて読み込む音声の処理
	StartUpLoad();
}

/////////////////////////////////////////////////////////////////////////////////////
// StartUpLoad
/////////////////////////////////////////////////////////////////////////////////////
void Audio::StartUpLoad(){
	Load("bgm.mp3");
	Load("attack.mp3");
	Load("hit.mp3");
}

void Audio::Finalize(){
	if (instance_){
		delete instance_;
		instance_ = nullptr;
	}
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
		sourceVoices_[filename]->DestroyVoice();
		sourceVoices_[filename] = nullptr;
	}

	// ソースボイスの生成
	if (loop){
		hr = xAudio2->CreateSourceVoice(&sourceVoices_[filename], &soundData.wfex);
		assert(SUCCEEDED(hr));
	} else{
		// 同じfilenameがあった場合、連番をつける等の処理
		int count = 0;
		while (sourceVoices_.find(tmpFilename) != sourceVoices_.end()){
			count++;
			tmpFilename = filename + std::to_string(count);
		}
		hr = xAudio2->CreateSourceVoice(&sourceVoices_[tmpFilename], &soundData.wfex);
		assert(SUCCEEDED(hr));
	}

	// バッファ設定
	XAUDIO2_BUFFER buf {};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	if (loop){
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	hr = sourceVoices_[tmpFilename]->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(hr));

	// 音量セット
	hr = sourceVoices_[tmpFilename]->SetVolume(volume);
	assert(SUCCEEDED(hr));

	// 再生開始
	hr = sourceVoices_[tmpFilename]->Start();
	assert(SUCCEEDED(hr));
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声を再生する(外部インターフェース)
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Play(const std::string& filename, bool loop, float volume){
	// ロード済みか確認
	assert(instance_->audios_.find(filename) != instance_->audios_.end());

	// 実際の再生を呼び出し
	instance_->PlayAudio(
		instance_->xAudio2_.Get(),
		instance_->audios_[filename],
		filename,
		loop,
		volume
	);

	// 再生フラグを立てる
	instance_->isPlaying_[filename] = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声の再生を終了する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::EndAudio(const std::string& filename){
	// SourceVoiceがなければエラー
	assert(instance_->sourceVoices_.find(filename) != instance_->sourceVoices_.end());

	HRESULT hr;
	hr = instance_->sourceVoices_[filename]->Stop();
	assert(SUCCEEDED(hr));

	hr = instance_->sourceVoices_[filename]->FlushSourceBuffers();
	assert(SUCCEEDED(hr));

	// ボイス解放
	instance_->sourceVoices_[filename]->DestroyVoice();
	instance_->sourceVoices_[filename] = nullptr;

	// 再生フラグをおろす
	instance_->isPlaying_[filename] = false;
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声を一時停止する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::PauseAudio(const std::string& filename){
	// SourceVoiceがなければエラー
	assert(instance_->sourceVoices_.find(filename) != instance_->sourceVoices_.end());

	HRESULT hr = S_OK;
	hr = instance_->sourceVoices_[filename]->Stop();
	assert(SUCCEEDED(hr));

	// 再生フラグをおろす
	instance_->isPlaying_[filename] = false;
}

/////////////////////////////////////////////////////////////////////////////////////
// 一時停止中の音声を再開する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::RestertAudio(const std::string& filename){
	// SourceVoiceがなければエラー
	assert(instance_->sourceVoices_.find(filename) != instance_->sourceVoices_.end());
	HRESULT hr = S_OK;
	if (instance_->sourceVoices_[filename] != nullptr){
		hr = instance_->sourceVoices_[filename]->Start();
		assert(SUCCEEDED(hr));

		instance_->isPlaying_[filename] = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// 音量を設定する
/////////////////////////////////////////////////////////////////////////////////////
void Audio::SetAudioVolume(const std::string& filename, float volume){
	// SourceVoiceがなければエラー
	assert(instance_->sourceVoices_.find(filename) != instance_->sourceVoices_.end());

	instance_->sourceVoices_[filename]->SetVolume(volume);
}

/////////////////////////////////////////////////////////////////////////////////////
// 再生中かどうかを返す
/////////////////////////////////////////////////////////////////////////////////////
bool Audio::IsPlayingAudio(const std::string& filename){
	// フラグがなければエラー
	assert(instance_->isPlaying_.find(filename) != instance_->isPlaying_.end());
	return instance_->isPlaying_[filename];
}

/////////////////////////////////////////////////////////////////////////////////////
// 音声をロードする
/////////////////////////////////////////////////////////////////////////////////////
void Audio::Load(const std::string& filename){
	// 未ロードならロードを実施
	if (instance_->audios_.find(filename) == instance_->audios_.end()){
		// 実際のパス
		std::string correctPath = directoryPath_ + filename;

		// 拡張子を取得
		size_t pos = filename.find_last_of('.');
		if (pos == std::string::npos || pos == filename.length() - 1){
			assert(false && "No valid extension found.");
		}
		std::string extension = filename.substr(pos + 1);

		// 拡張子で振り分け
		if (extension == "wav"){
			instance_->audios_[filename] = instance_->LoadWave(correctPath.c_str());
		} else if (extension == "mp3" || extension == "m4a"){
			std::wstring wpath(correctPath.begin(), correctPath.end());
			instance_->audios_[filename] = instance_->LoadMP3(wpath.c_str());
		} else{
			// 未対応フォーマット
			assert(false && "Unsupported audio format.");
		}

		// 再生フラグ初期化
		instance_->isPlaying_[filename] = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// WAVファイルの読み込み
/////////////////////////////////////////////////////////////////////////////////////
SoundData Audio::LoadWave(const char* filename){
	std::ifstream file;

	// バイナリ形式で開く
	file.open(filename, std::ios_base::binary);
	assert(file.is_open());

	// RIFFヘッダ
	RiffHeader riff;
	file.read(( char* ) &riff, sizeof(riff));
	// チェック
	assert(strncmp(riff.chunk.id, "RIFF", 4) == 0);
	assert(strncmp(riff.type, "WAVE", 4) == 0);

	// フォーマットチャンク
	FormatChunk format = {};
	while (true){
		ChunkHeader chunkHeader;
		file.read(( char* ) &chunkHeader, sizeof(ChunkHeader));
		if (strncmp(chunkHeader.id, "fmt ", 4) == 0){
			format.chunk = chunkHeader;
			file.read(( char* ) &format.fmt, format.chunk.size);
			assert(format.chunk.size <= sizeof(format.fmt));
			break;
		} else{
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}

		if (file.eof()){
			assert(0 && "Reached end of file without finding 'fmt ' chunk");
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
			assert(0 && "Reached end of file without finding 'data' chunk");
			return SoundData {};
		}
	}

	// データ部読み込み
	char* pCbuffer = new char[data.size];
	file.read(pCbuffer, data.size);

	// 閉じる
	file.close();

	// SoundDataへ格納
	SoundData soundData {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast< BYTE* >(pCbuffer);
	soundData.bufferSize = data.size;

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
	assert(SUCCEEDED(hr));
	hr = pOutputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	assert(SUCCEEDED(hr));

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
	soundData.pBuffer = new BYTE[audioData.size()];
	std::copy(audioData.begin(), audioData.end(), soundData.pBuffer);
	soundData.bufferSize = static_cast< uint32_t >(audioData.size());

	// フォーマット情報を取得
	hr = pOutputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, ( UINT32* ) &soundData.wfex.nChannels);
	assert(SUCCEEDED(hr));
	hr = pOutputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, ( UINT32* ) &soundData.wfex.nSamplesPerSec);
	assert(SUCCEEDED(hr));
	hr = pOutputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, ( UINT32* ) &soundData.wfex.wBitsPerSample);
	assert(SUCCEEDED(hr));

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
	assert(instance_->audios_.find(filename) != instance_->audios_.end());

	// SoundData破棄
	instance_->UnloadAudio(&instance_->audios_[filename]);
	instance_->audios_.erase(filename);

	// SourceVoice破棄
	if (instance_->sourceVoices_[filename]){
		instance_->sourceVoices_[filename]->DestroyVoice();
	}
	instance_->sourceVoices_.erase(filename);
	instance_->isPlaying_.erase(filename);
}

/////////////////////////////////////////////////////////////////////////////////////
// すべての音声をアンロード
/////////////////////////////////////////////////////////////////////////////////////
void Audio::UnloadAllAudio(){
	for (auto& pair : instance_->audios_){
		instance_->UnloadAudio(&pair.second);
	}
	instance_->audios_.clear();

	for (auto& voicePair : instance_->sourceVoices_){
		if (voicePair.second){
			voicePair.second->DestroyVoice();
		}
	}
	instance_->sourceVoices_.clear();
	instance_->isPlaying_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////
// UnloadAudio(内部実装)
/////////////////////////////////////////////////////////////////////////////////////
void Audio::UnloadAudio(SoundData* soundData){
	delete[] soundData->pBuffer;
	soundData->pBuffer = nullptr;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}
