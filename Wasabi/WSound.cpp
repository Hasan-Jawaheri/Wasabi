#include "WSound.h"
#include <windows.h>

template<typename T>
T BytesTo(BYTE* bytes, bool bLittleEndian = false) {
	T ret = 0;

	if (bLittleEndian)
		for (uint i = sizeof(bytes) - 1; i >= 0 && i != UINT_MAX; i--)
			ret = (ret << 8) + bytes[i];
	else
		for (uint i = 0; i < sizeof bytes; i++)
			ret = (ret << 8) + bytes[i];

	return ret;
};

WSoundComponent::WSoundComponent(Wasabi* app) {
	m_app = app;
	m_oalDevice = nullptr;
	m_oalContext = nullptr;

	//
	//Initialize open AL
	//
	m_oalDevice = alcOpenDevice(nullptr); // select the "preferred device"
	if (m_oalDevice) {
		m_oalContext = alcCreateContext(m_oalDevice, nullptr);
		alcMakeContextCurrent(m_oalContext);
	}
}
WSoundComponent::~WSoundComponent() {
	for (uint i = 0; i < m_soundV.size(); i)
		delete (m_soundV[i]);

	// Exit open AL
	m_oalContext = alcGetCurrentContext();
	m_oalDevice = alcGetContextsDevice(m_oalContext);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(m_oalContext);
	alcCloseDevice(m_oalDevice);
}
ALCdevice* WSoundComponent::GetALSoundDevice(void) const {
	return m_oalDevice;
}
ALCcontext* WSoundComponent::GetALSoundDeviceContext(void) const {
	return m_oalContext;
}
void WSoundComponent::SetSoundSpeed(float fSpeed) {
	alSpeedOfSound(fSpeed);
}
void WSoundComponent::SetDopplerFactor(float fFactor) {
	alDopplerFactor(fFactor);
}
void WSoundComponent::SetMasterGain(float fGain) {
	alListenerf(AL_GAIN, fGain);
}
void WSoundComponent::SetPosition(float x, float y, float z) {
	alListener3f(AL_POSITION, x, y, z);
}
void WSoundComponent::SetPosition(WVector3 pos) {
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}
void WSoundComponent::SetPosition(WOrientation* pos) {
	alListener3f(AL_POSITION, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ());
}
void WSoundComponent::SetVelocity(float x, float y, float z) {
	alListener3f(AL_VELOCITY, x, y, z);
}
void WSoundComponent::SetVelocity(WVector3 vel) {
	alListener3f(AL_POSITION, vel.x, vel.y, vel.z);
}
void WSoundComponent::SetOrientation(WVector3 look, WVector3 up) {
	float fVals[6];
	for (uint i = 0; i < 6; i++) {
		if (i < 3)
			fVals[i] = look[i];
		else
			fVals[i] = up[i - 3];
	}
	alListenerfv(AL_ORIENTATION, fVals);
}
void WSoundComponent::SetOrientation(WOrientation* ori) {
	float fVals[6];
	for (uint i = 0; i < 6; i++) {
		if (i < 3)
			fVals[i] = ori->GetLVector()[i];
		else
			fVals[i] = ori->GetUVector()[i];
	}
	alListenerfv(AL_ORIENTATION, fVals);
}
void WSoundComponent::SetToOrientationDevice(WOrientation* ori) {
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
WSound* WSoundComponent::GetSoundHandle(uint ID) const {
	//return a handle to a sound if it exists
	for (uint i = 0; i < m_soundV.size(); i++)
		if (m_soundV[i]->GetID() == ID)
			return m_soundV[i];
	return nullptr;
}
WSound* WSoundComponent::GetSoundHandleByIndex(uint index) const {
	//return a handle to a sound if it exists, do not count sounds with ID = 0
	for (uint i = 0; i < m_soundV.size(); i++)
		if (m_soundV[i]->GetID())
			if (i == index)
				return m_soundV[i];
	return nullptr;
}
uint WSoundComponent::GetSoundsCount(void) const {
	return m_soundV.size();
}
void WSoundComponent::m_RegisterSound(WSound* sound) {
	//sign a sound in the sound's list, delete a sound if it already exists with the same ID (this should be used by the system only)
	if (sound->GetID())
		for (uint i = 0; i < m_soundV.size(); i++)
			if (m_soundV[i]->GetID() == sound->GetID()) {
				WSound* temp = m_soundV[i];
				W_SAFE_DELETE(temp);
			}
	m_soundV.push_back(sound);
}
void WSoundComponent::m_UnRegisterSound(WBase* base) {
	//remove a sound from the list
	for (uint i = 0; i < m_soundV.size(); i++)
		if (m_soundV[i] == base) {
			m_soundV.erase(m_soundV.begin() + i);
			break;
		}
}


WSound::WSound(Wasabi* const app, uint ID) : WBase(app) {
	m_valid = false;
	m_numBuffers = W_NUM_SOUND_BUFFERS_PER_SOUND;
	m_buffers = new ALuint[m_numBuffers];
	ZeroMemory(m_buffers, m_numBuffers * sizeof ALuint);

	// Generate Buffers
	alGetError(); // clear error code
	alGenBuffers(m_numBuffers, m_buffers);
	ALenum err = alGetError();

	// Generate Source
	alGetError(); //clear errors
	alGenSources(1, &m_source);

	//initiate the locals
	SetID(ID);

	char name[256];
	static uint i = 0;
	sprintf_s(name, 256, "Sound%3u", i++);
	SetName(name);

	GetAppPtr()->SoundComponent->m_RegisterSound(this);
}
WSound::~WSound(void) {
	alDeleteBuffers(m_numBuffers, m_buffers);
	alDeleteSources(1, &m_source);

	W_SAFE_DELETE_ARRAY(m_buffers);

	for (uint i = 0; i < m_dataV.size(); i++) {
		W_SAFE_FREE(m_dataV[i].data);
	}
	m_dataV.clear();

	m_valid = false;

	//unregister
	GetAppPtr()->SoundComponent->m_UnRegisterSound(this);
}
std::string WSound::GetTypeName(void) const {
	return "Sound";
}

WError WSound::LoadFromMemory(uint buffer, void* data, size_t dataSize, ALenum format, uint frequency, bool bSaveData) {
	if (!m_bCheck(true)) return WError(W_ERRORUNK);

	alBufferData(m_buffers[buffer], format, data, dataSize, frequency);

	// Attach buffer to source
	alSourcei(m_source, AL_BUFFER, m_buffers[buffer]);

	//save data if required
	if (bSaveData) {
		__SAVEDATA saveData;
		saveData.buffer = buffer;
		saveData.dataSize = dataSize;
		saveData.format = format;
		saveData.data = W_SAFE_ALLOC(dataSize);
		memcpy(saveData.data, data, dataSize);
		m_dataV.push_back(saveData);
	}

	return WError(W_SUCCEEDED);
}
WError WSound::LoadWAV(std::string Filename, uint buffer, bool bSaveData) {
	if (!m_bCheck(true)) return WError(W_ERRORUNK);

	fstream file(Filename, ios::in | ios::binary);
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
	freq = BytesTo<__int32>((BYTE*)buffer4Bytes, bLittleEndian);

	//data size at offset 22 (2 bytes long)
	file.seekg(22);
	file.read(buffer2Bytes, 2);
	uint numChannels = BytesTo<__int16>((BYTE*)buffer2Bytes, bLittleEndian);

	//bits per sample at offset 34 (2 bytes long)
	file.seekg(34);
	file.read(buffer2Bytes, 2);
	uint bps = BytesTo<__int16>((BYTE*)buffer2Bytes, bLittleEndian);

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
	dataSize = BytesTo<__int32>((BYTE*)buffer4Bytes, bLittleEndian);

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
		saveData.format = format;
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
void WSound::Play(void) {
	if (!m_bCheck()) return;

	alSourcePlay(m_source);
	alSourcei(m_source, AL_LOOPING, AL_FALSE); //looping off
}
void WSound::Loop(void) {
	if (!m_bCheck()) return;

	alSourcePlay(m_source);
	alSourcei(m_source, AL_LOOPING, AL_TRUE);
}
void WSound::Pause(void) {
	if (!m_bCheck()) return;

	alSourcePause(m_source);
}
void WSound::Reset(void) {
	if (!m_bCheck()) return;

	alSourceRewind(m_source);
}
void WSound::SetTime(uint time) {
	if (!m_bCheck()) return;

	alSourcef(m_source, AL_SEC_OFFSET, time / 1000.0f);
}
bool WSound::Playing(void) const {
	if (!m_bCheck()) return false;

	ALint out = 0;
	alGetSourcei(m_source, AL_SOURCE_STATE, &out);
	return out == AL_PLAYING;
}
bool WSound::Looping(void) const {
	if (!m_bCheck()) return false;

	ALint out = 0;
	alGetSourcei(m_source, AL_SOURCE_STATE, &out);
	return out == AL_LOOPING;
}
void WSound::SetVolume(float volume) {
	if (!m_bCheck()) return;

	alSourcef(m_source, AL_GAIN, volume / 100.0f);
}
void WSound::SetPitch(float fPitch) {
	if (!m_bCheck()) return;

	alSourcei(m_source, AL_PITCH, fPitch);
}
void WSound::SetFrequency(uint buffer, uint frequency) {
	if (!m_bCheck() || buffer >= m_numBuffers) return;

	alBufferi(m_buffers[buffer], AL_FREQUENCY, frequency);
}

uint WSound::GetNumChannels(uint buffer) const {
	if (!m_bCheck() || buffer >= m_numBuffers) return 0;

	ALint out = 0;
	alGetBufferi(m_buffers[buffer], AL_CHANNELS, &out);
	return out;
}
uint WSound::GetBitDepth(uint buffer) const {
	if (!m_bCheck() || buffer >= m_numBuffers) return 0;

	ALint out = 0;
	alGetBufferi(m_buffers[buffer], AL_BITS, &out);
	return out;
}
ALuint WSound::GetALBuffer(uint buffer) const {
	if (!m_bCheck() || buffer >= m_numBuffers) return 0;

	return m_buffers[buffer];
}
ALuint WSound::GetALSource(void) const {
	if (!m_bCheck()) return 0;

	return m_source;
}

void WSound::SetPosition(float x, float y, float z) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, x, y, z);
}
void WSound::SetPosition(WVector3 pos) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, pos.x, pos.y, pos.z);
}
void WSound::SetPosition(WOrientation* pos) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionX());
}
void WSound::SetVelocity(float x, float y, float z) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_VELOCITY, x, y, z);
}
void WSound::SetVelocity(WVector3 vel) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_VELOCITY, vel.x, vel.y, vel.z);
}
void WSound::SetDirection(WVector3 look) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_DIRECTION, look.x, look.y, look.z);
}
void WSound::SetDirection(WOrientation* look) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_DIRECTION, look->GetLVector()[0], look->GetLVector()[1], look->GetLVector()[2]);
}
void WSound::SetToOrientationDevice(WOrientation* oriDev) {
	if (!m_bCheck()) return;

	alSource3f(m_source, AL_POSITION, oriDev->GetPositionX(), oriDev->GetPositionY(), oriDev->GetPositionX());
	alSource3f(m_source, AL_DIRECTION, oriDev->GetLVector()[0], oriDev->GetLVector()[1], oriDev->GetLVector()[2]);
}
bool WSound::Valid(void) const {
	return m_valid;
}
WError WSound::LoadFromWS(std::string filename, bool bSaveData) {
	fstream file;
	file.open(filename, ios::in | ios::binary);
	if (file.fail())
		return WError(W_FILENOTFOUND);

	float temp[3];

	//read pitch & volume
	file.read((char*)temp, 2 * sizeof(float));
	alSourcef(m_source, AL_PITCH, temp[0]);
	alSourcef(m_source, AL_GAIN, temp[1]);

	//read position & direction
	file.read((char*)temp, 3 * sizeof(float));
	alSource3f(m_source, AL_POSITION, temp[0], temp[1], temp[2]);
	file.read((char*)temp, 3 * sizeof(float));
	alSource3f(m_source, AL_DIRECTION, temp[0], temp[1], temp[2]);

	char numBuffers = 0;
	file.read((char*)&numBuffers, 1);
	for (uint i = 0; i < numBuffers; i++) {
		__SAVEDATA data;
		file.read((char*)&data.buffer, 4); //buffer index
		file.read((char*)&data.format, sizeof ALenum); //buffer format
		file.read((char*)&temp[0], 4); //frequency
		file.read((char*)&data.dataSize, 4); //size of data
		data.data = W_SAFE_ALLOC(data.dataSize);
		file.read((char*)&data.data, m_dataV[i].dataSize); //data

		LoadFromMemory(data.buffer, data.data, data.dataSize, data.format, temp[0], bSaveData);

		W_SAFE_FREE(data.data);
	}

	file.close();

	return WError(W_SUCCEEDED);
}
WError WSound::LoadFromWS(basic_filebuf<char>* buff, uint pos, bool bSaveData) {
	//use the given stream
	fstream file;
	if (!buff)
		return WError(W_INVALIDPARAM);
	file.set_rdbuf(buff);
	file.seekg(pos);

	float temp[3];

	//read pitch & volume
	file.read((char*)temp, 2 * sizeof(float));
	alSourcef(m_source, AL_PITCH, temp[0]);
	alSourcef(m_source, AL_GAIN, temp[1]);

	//read position & direction
	file.read((char*)temp, 3 * sizeof(float));
	alSource3f(m_source, AL_POSITION, temp[0], temp[1], temp[2]);
	file.read((char*)temp, 3 * sizeof(float));
	alSource3f(m_source, AL_DIRECTION, temp[0], temp[1], temp[2]);

	char numBuffers = 0;
	file.read((char*)&numBuffers, 1);
	for (uint i = 0; i < numBuffers; i++) {
		__SAVEDATA data;
		file.read((char*)&data.buffer, 4); //buffer index
		file.read((char*)&data.format, sizeof ALenum); //buffer format
		file.read((char*)&temp[0], 4); //frequency
		file.read((char*)&data.dataSize, 4); //size of data
		data.data = W_SAFE_ALLOC(data.dataSize);
		file.read((char*)&data.data, m_dataV[i].dataSize); //data

		WError err = LoadFromMemory(data.buffer, data.data, data.dataSize, data.format, temp[0], bSaveData);

		W_SAFE_FREE(data.data);

		return WError(err);
	}

	return WError(W_SUCCEEDED);
}
WError WSound::SaveToWS(std::string filename) const {
	if (Valid() && m_dataV.size()) //only attempt to save if the sound is valid and there is something to save
	{
		//open the file for writing
		fstream file;
		file.open(filename, ios::out | ios::binary);

		if (!file.is_open())
			return WError(W_FILENOTFOUND);

		float temp[3];

		//write pitch & volume
		alGetSourcef(m_source, AL_PITCH, &temp[0]);
		alGetSourcef(m_source, AL_GAIN, &temp[1]);
		file.write((char*)temp, 2 * sizeof(float));

		//write position & direction
		alGetSource3f(m_source, AL_POSITION, &temp[0], &temp[1], &temp[2]);
		file.write((char*)temp, 3 * sizeof(float));
		alGetSource3f(m_source, AL_DIRECTION, &temp[0], &temp[1], &temp[2]);
		file.write((char*)temp, 3 * sizeof(float));

		char numBuffers = m_dataV.size();
		file.write((char*)&numBuffers, 1);
		for (uint i = 0; i < numBuffers; i++) {
			file.write((char*)&m_dataV[i].buffer, 4); //buffer index
			file.write((char*)&m_dataV[i].format, sizeof ALenum); //buffer format
			alGetBufferf(m_buffers[m_dataV[i].buffer], AL_FREQUENCY, &temp[0]);
			file.write((char*)&temp[0], 4); //frequency
			file.write((char*)&m_dataV[i].dataSize, 4); //size of data
			file.write((char*)&m_dataV[i].data, m_dataV[i].dataSize); //data
		}

		file.close();
	}
	else
		return WError(W_NOTVALID);

	return WError(W_SUCCEEDED);

}
WError WSound::SaveToWS(basic_filebuf<char>* buff, uint pos) const {
	if (Valid() && m_dataV.size()) //only attempt to save if the sound is valid and there is something to save
	{
		//use the given stream
		fstream file;
		if (!buff)
			return WError(W_INVALIDPARAM);
		file.set_rdbuf(buff);
		file.seekp(pos);

		float temp[3];

		//write pitch & volume
		alGetSourcef(m_source, AL_PITCH, &temp[0]);
		alGetSourcef(m_source, AL_GAIN, &temp[1]);
		file.write((char*)temp, 2 * sizeof(float));

		//write position & direction
		alGetSource3f(m_source, AL_POSITION, &temp[0], &temp[1], &temp[2]);
		file.write((char*)temp, 3 * sizeof(float));
		alGetSource3f(m_source, AL_DIRECTION, &temp[0], &temp[1], &temp[2]);
		file.write((char*)temp, 3 * sizeof(float));

		char numBuffers = m_dataV.size();
		file.write((char*)&numBuffers, 1);
		for (uint i = 0; i < numBuffers; i++) {
			file.write((char*)&m_dataV[i].buffer, 4); //buffer index
			file.write((char*)&m_dataV[i].format, sizeof ALenum); //buffer format
			alGetBufferf(m_buffers[m_dataV[i].buffer], AL_FREQUENCY, &temp[0]);
			file.write((char*)&temp[0], 4); //frequency
			file.write((char*)&m_dataV[i].dataSize, 4); //size of data
			file.write((char*)&m_dataV[i].data, m_dataV[i].dataSize); //data
		}
	}
	else
		return WError(W_NOTVALID);

	return WError(W_SUCCEEDED);
}
bool WSound::m_bCheck(bool ignoreValidness) const {
	return (m_valid || ignoreValidness) && m_source && m_buffers;
}