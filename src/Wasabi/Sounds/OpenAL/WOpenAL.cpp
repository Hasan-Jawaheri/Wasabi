#include "Wasabi/Sounds/OpenAL/WOpenAL.h"

#include <AL/al.h>
#include <AL/alc.h>

template<typename T>
T BytesTo(char* bytes, bool bLittleEndian = false) {
	T ret = 0;

	if (bLittleEndian)
		for (uint i = sizeof(bytes) - 1; i >= 0 && i != UINT_MAX; i--)
			ret = (ret << 8) + (unsigned char)bytes[i];
	else
		for (uint i = 0; i < sizeof(bytes); i++)
			ret = (ret << 8) + (unsigned char)bytes[i];

	return ret;
};

WOpenALSoundComponent::WOpenALSoundComponent(Wasabi* const app) : WSoundComponent(app) {
	SoundManager = new WOpenALSoundManager(app);

	m_oalDevice = nullptr;
	m_oalContext = nullptr;
}

WOpenALSoundComponent::~WOpenALSoundComponent() {
	Cleanup();
}

WError WOpenALSoundComponent::Initialize() {
	//
	//Initialize open AL
	//
	m_oalDevice = (void*)alcOpenDevice(nullptr); // select the "preferred device"
	if (m_oalDevice) {
		m_oalContext = (void*)alcCreateContext((ALCdevice*)m_oalDevice, nullptr);
		if (!m_oalContext) {
			alcCloseDevice((ALCdevice*)m_oalDevice);
			m_oalDevice = nullptr;
			return WError(W_ERRORUNK);
		}
		alcMakeContextCurrent((ALCcontext*)m_oalContext);
	} else
		return WError(W_ERRORUNK);

	return WError(W_SUCCEEDED);
}

void WOpenALSoundComponent::Cleanup() {
	W_SAFE_DELETE(SoundManager);

	// Exit open AL
	alcMakeContextCurrent(nullptr);
	alcDestroyContext((ALCcontext*)m_oalContext);
	alcCloseDevice((ALCdevice*)m_oalDevice);
}

WSound* WOpenALSoundComponent::CreateSound(unsigned int ID) const {
	return new WOpenALSound(m_app, ID);
}

void* WOpenALSoundComponent::GetALSoundDevice() const {
	return m_oalDevice;
}

void* WOpenALSoundComponent::GetALSoundDeviceContext() const {
	return m_oalContext;
}

void WOpenALSoundComponent::SetSoundSpeed(float fSpeed) {
	alSpeedOfSound(fSpeed);
}

void WOpenALSoundComponent::SetDopplerFactor(float fFactor) {
	alDopplerFactor(fFactor);
}

void WOpenALSoundComponent::SetMasterGain(float fGain) {
	alListenerf(AL_GAIN, fGain);
}

void WOpenALSoundComponent::SetListenerPosition(float x, float y, float z) {
	alListener3f(AL_POSITION, x, y, z);
}

void WOpenALSoundComponent::SetListenerPosition(WVector3 pos) {
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}

void WOpenALSoundComponent::SetListenerPosition(WOrientation* pos) {
	alListener3f(AL_POSITION, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ());
}

void WOpenALSoundComponent::SetListenerVelocity(float x, float y, float z) {
	alListener3f(AL_VELOCITY, x, y, z);
}

void WOpenALSoundComponent::SetListenerVelocity(WVector3 vel) {
	alListener3f(AL_POSITION, vel.x, vel.y, vel.z);
}

void WOpenALSoundComponent::SetListenerOrientation(WVector3 look, WVector3 up) {
	float fVals[6];
	for (uint i = 0; i < 6; i++) {
		if (i < 3)
			fVals[i] = look[i];
		else
			fVals[i] = up[i - 3];
	}
	alListenerfv(AL_ORIENTATION, fVals);
}

void WOpenALSoundComponent::SetListenerOrientation(WOrientation* ori) {
	float fVals[6];
	for (uint i = 0; i < 6; i++) {
		if (i < 3)
			fVals[i] = ori->GetLVector()[i];
		else
			fVals[i] = ori->GetUVector()[i];
	}
	alListenerfv(AL_ORIENTATION, fVals);
}

void WOpenALSoundComponent::SetListenerToOrientation(WOrientation* ori) {
	float fVals[6];
	for (uint i = 0; i < 6; i++) {
		if (i < 3)
			fVals[i] = ori->GetLVector()[i];
		else
			fVals[i] = ori->GetUVector()[i];
	}
	alListenerfv(AL_ORIENTATION, fVals);
	alListener3f(AL_POSITION, ori->GetPositionX(), ori->GetPositionY(), ori->GetPositionZ());
}

WOpenALSoundManager::WOpenALSoundManager(class Wasabi* const app)
	: WSoundManager(app) {
}

WOpenALSoundManager::~WOpenALSoundManager() {
}

std::string WOpenALSoundManager::GetTypeName() const {
	return "SoundManager";
}


WOpenALSound::WOpenALSound(Wasabi* const app, uint ID) : WSound(app, ID) {
	m_valid = false;
	m_numBuffers = W_NUM_SOUND_BUFFERS_PER_SOUND;
	m_buffers = new ALuint[m_numBuffers];
	ZeroMemory(m_buffers, m_numBuffers * sizeof(ALuint));

	// Generate Buffers
	alGetError(); // clear error code
	alGenBuffers(m_numBuffers, m_buffers);
	ALenum err = alGetError();

	// Generate Source
	alGetError(); //clear errors
	alGenSources(1, &m_source);

	m_app->SoundComponent->SoundManager->AddEntity(this);

}
WOpenALSound::~WOpenALSound() {
	alDeleteBuffers(m_numBuffers, m_buffers);
	alDeleteSources(1, &m_source);

	W_SAFE_DELETE_ARRAY(m_buffers);

	for (uint i = 0; i < m_dataV.size(); i++) {
		W_SAFE_FREE(m_dataV[i].data);
	}
	m_dataV.clear();

	m_valid = false;

	m_app->SoundComponent->SoundManager->RemoveEntity(this);
}

std::string WOpenALSound::_GetTypeName() {
	return "Sound";
}

std::string WOpenALSound::GetTypeName() const {
	return _GetTypeName();
}

WError WOpenALSound::LoadFromMemory(uint buffer, void* data, size_t dataSize, int format, uint frequency, bool bSaveData) {
	if (!m_bCheck(true)) return WError(W_ERRORUNK);

	alBufferData(m_buffers[buffer], format, data, dataSize, frequency);

	// Attach buffer to source
	alSourcei(m_source, AL_BUFFER, m_buffers[buffer]);

	//save data if required
	if (bSaveData) {
		__SAVEDATA saveData;
		saveData.buffer = buffer;
		saveData.dataSize = dataSize;
		saveData.format = (int)format;
		saveData.data = W_SAFE_ALLOC(dataSize);
		memcpy(saveData.data, data, dataSize);
		m_dataV.push_back(saveData);
	}

	return WError(W_SUCCEEDED);
}

WError WOpenALSound::LoadWAV(std::string Filename, uint buffer, bool bSaveData) {
	if (!m_bCheck(true)) return WError(W_ERRORUNK);

	std::fstream file(Filename, ios::in | ios::binary);
	if (!file.is_open())
		return WError(W_FILENOTFOUND);

	ALenum format;
	uint dataSize;
	uint freq;

	bool bLittleEndian = true;
	char buffer4Bytes[4] = { 0 };
	char buffer2Bytes[2] = { 0 };

	//frequency at offset 24 (4 bytes long)
	file.seekg(0);
	file.read(buffer4Bytes, 4);
	if (buffer4Bytes[3] == 'F') //RIFF
		bLittleEndian = true;
	else if (buffer4Bytes[3] == 'X') //RIFX
		bLittleEndian = false;
	else
		return WError(W_INVALIDFILEFORMAT);

	//frequency at offset 24 (4 bytes long)
	file.seekg(24);
	file.read(buffer4Bytes, 4);
	freq = BytesTo<uint>((char*)buffer4Bytes, bLittleEndian);

	//data size at offset 22 (2 bytes long)
	file.seekg(22);
	file.read(buffer2Bytes, 2);
	uint numChannels = BytesTo<short>((char*)buffer2Bytes, bLittleEndian);

	//bits per sample at offset 34 (2 bytes long)
	file.seekg(34);
	file.read(buffer2Bytes, 2);
	uint bps = BytesTo<short>((char*)buffer2Bytes, bLittleEndian);

	if (numChannels == 1)
		format = bps == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
	else if (numChannels == 2)
		format = bps == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
	else {
		file.close();
		return WError(W_INVALIDFILEFORMAT);
	}

	//find "data" word
	int offset = 36;
	while (offset < 260) {
		file.seekg(offset);
		file.read(buffer4Bytes, 4);
		if (buffer4Bytes[0] == 'd' && buffer4Bytes[1] == 'a' &&
			buffer4Bytes[2] == 't' && buffer4Bytes[3] == 'a') {
			offset += 4;
			break;
		}
		offset++;
	}
	if (offset == 60) {
		file.close();
		return WError(W_INVALIDFILEFORMAT);
	}

	//data size at offset (4 bytes long)
	file.seekg(offset);
	file.read(buffer4Bytes, 4);
	dataSize = BytesTo<int>((char*)buffer4Bytes, bLittleEndian);

	//data at offset+4 (dataSize bytes long)
	file.seekg(offset + 4);
	void* data = W_SAFE_ALLOC(dataSize);
	file.read((char*)data, dataSize);

	//set buffer data
	alBufferData(m_buffers[buffer], format, data, dataSize, freq);
	if (alGetError() != AL_NO_ERROR)
		return WError(W_INVALIDFILEFORMAT);

	//release read bytes
	if (!bSaveData) {
		W_SAFE_FREE(data);
	}
	else {
		__SAVEDATA saveData;
		saveData.buffer = buffer;
		saveData.dataSize = dataSize;
		saveData.format = (int)format;
		saveData.data = W_SAFE_ALLOC(dataSize);
		memcpy(saveData.data, data, dataSize);
		m_dataV.push_back(saveData);
	}

	file.close();

	// Attach buffer to source
	alSourcei(m_source, AL_BUFFER, m_buffers[buffer]);

	m_valid = true;

	return WError(W_SUCCEEDED);
}

void WOpenALSound::Play() {
	if (!m_bCheck()) return;

	alSourcePlay(m_source);
	alSourcei(m_source, AL_LOOPING, AL_FALSE); //looping off
}

void WOpenALSound::Loop() {
	if (!m_bCheck()) return;

	alSourcePlay(m_source);
	alSourcei(m_source, AL_LOOPING, AL_TRUE);
}

void WOpenALSound::Pause() {
	if (!m_bCheck()) return;

	alSourcePause(m_source);
}

void WOpenALSound::Reset() {
	if (!m_bCheck()) return;

	alSourceRewind(m_source);
}

void WOpenALSound::SetTime(uint time) {
	if (!m_bCheck()) return;

	alSourcef(m_source, AL_SEC_OFFSET, time / 1000.0f);
}

bool WOpenALSound::Playing() const {
	if (!m_bCheck()) return false;

	ALint out = 0;
	alGetSourcei(m_source, AL_SOURCE_STATE, &out);
	return out == AL_PLAYING;
}

bool WOpenALSound::Looping() const {
	if (!m_bCheck()) return false;

	ALint out = 0;
	alGetSourcei(m_source, AL_SOURCE_STATE, &out);
	return out == AL_LOOPING;
}

void WOpenALSound::SetVolume(float volume) {
	if (!m_bCheck()) return;

	alSourcef(m_source, AL_GAIN, volume / 100.0f);
}

void WOpenALSound::SetPitch(float fPitch) {
	if (!m_bCheck()) return;

	alSourcei(m_source, AL_PITCH, fPitch);
}

void WOpenALSound::SetFrequency(uint buffer, uint frequency) {
	if (!m_bCheck() || buffer >= m_numBuffers) return;

	alBufferi(m_buffers[buffer], AL_FREQUENCY, frequency);
}

uint WOpenALSound::GetNumChannels(uint buffer) const {
	if (!m_bCheck() || buffer >= m_numBuffers) return 0;

	ALint out = 0;
	alGetBufferi(m_buffers[buffer], AL_CHANNELS, &out);
	return out;
}

uint WOpenALSound::GetBitDepth(uint buffer) const {
	if (!m_bCheck() || buffer >= m_numBuffers) return 0;

	ALint out = 0;
	alGetBufferi(m_buffers[buffer], AL_BITS, &out);
	return out;
}

uint WOpenALSound::GetALBuffer(uint buffer) const {
	if (!m_bCheck() || buffer >= m_numBuffers) return 0;

	return m_buffers[buffer];
}

uint WOpenALSound::GetALSource() const {
	if (!m_bCheck()) return 0;

	return m_source;
}

void WOpenALSound::SetPosition(float x, float y, float z) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, x, y, z);
}

void WOpenALSound::SetPosition(WVector3 pos) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, pos.x, pos.y, pos.z);
}

void WOpenALSound::SetPosition(WOrientation* pos) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionX());
}

void WOpenALSound::SetVelocity(float x, float y, float z) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_VELOCITY, x, y, z);
}

void WOpenALSound::SetVelocity(WVector3 vel) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_VELOCITY, vel.x, vel.y, vel.z);
}

void WOpenALSound::SetDirection(WVector3 look) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_DIRECTION, look.x, look.y, look.z);
}

void WOpenALSound::SetDirection(WOrientation* look) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_DIRECTION, look->GetLVector()[0], look->GetLVector()[1], look->GetLVector()[2]);
}

void WOpenALSound::SetToOrientation(WOrientation* oriDev) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, oriDev->GetPositionX(), oriDev->GetPositionY(), oriDev->GetPositionX());
	alSource3f(m_source, AL_DIRECTION, oriDev->GetLVector()[0], oriDev->GetLVector()[1], oriDev->GetLVector()[2]);
}

bool WOpenALSound::Valid() const {
	return m_valid;
}

bool WOpenALSound::m_bCheck(bool ignoreValidness) const {
	return (m_valid || ignoreValidness) && m_source && m_buffers;
}

WError WOpenALSound::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (Valid() && m_dataV.size()) //only attempt to save if the sound is valid and there is something to save
	{
		float temp[3];

		//write pitch & volume
		alGetSourcef(m_source, AL_PITCH, &temp[0]);
		alGetSourcef(m_source, AL_GAIN, &temp[1]);
		outputStream.write((char*)temp, 2 * sizeof(float));

		//write position & direction
		alGetSource3f(m_source, AL_POSITION, &temp[0], &temp[1], &temp[2]);
		outputStream.write((char*)temp, 3 * sizeof(float));
		alGetSource3f(m_source, AL_DIRECTION, &temp[0], &temp[1], &temp[2]);
		outputStream.write((char*)temp, 3 * sizeof(float));

		char numBuffers = m_dataV.size();
		outputStream.write((char*)&numBuffers, 1);
		for (uint i = 0; i < numBuffers; i++) {
			outputStream.write((char*)&m_dataV[i].buffer, 4); //buffer index
			outputStream.write((char*)&m_dataV[i].format, sizeof(int)); //buffer format
			alGetBufferf(m_buffers[m_dataV[i].buffer], AL_FREQUENCY, &temp[0]);
			outputStream.write((char*)&temp[0], 4); //frequency
			outputStream.write((char*)&m_dataV[i].dataSize, 4); //size of data
			outputStream.write((char*)&m_dataV[i].data, m_dataV[i].dataSize); //data
		}
	} else
		return WError(W_NOTVALID);

	return WError(W_SUCCEEDED);
}

std::vector<void*> WOpenALSound::LoadArgs() {
	return std::vector<void*>();
}

WError WOpenALSound::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) {
	bool bSaveData = false;
	float temp[3];

	//read pitch & volume
	inputStream.read((char*)temp, 2 * sizeof(float));
	alSourcef(m_source, AL_PITCH, temp[0]);
	alSourcef(m_source, AL_GAIN, temp[1]);

	//read position & direction
	inputStream.read((char*)temp, 3 * sizeof(float));
	alSource3f(m_source, AL_POSITION, temp[0], temp[1], temp[2]);
	inputStream.read((char*)temp, 3 * sizeof(float));
	alSource3f(m_source, AL_DIRECTION, temp[0], temp[1], temp[2]);

	char numBuffers = 0;
	inputStream.read((char*)&numBuffers, 1);
	for (uint i = 0; i < numBuffers; i++) {
		__SAVEDATA data;
		inputStream.read((char*)&data.buffer, 4); //buffer index
		inputStream.read((char*)&data.format, sizeof(int)); //buffer format
		inputStream.read((char*)&temp[0], 4); //frequency
		inputStream.read((char*)&data.dataSize, 4); //size of data
		data.data = W_SAFE_ALLOC(data.dataSize);
		inputStream.read((char*)&data.data, m_dataV[i].dataSize); //data

		WError err = LoadFromMemory(data.buffer, data.data, data.dataSize, data.format, temp[0], bSaveData);

		W_SAFE_FREE(data.data);

		return WError(err);
	}

	return WError(W_SUCCEEDED);
}
