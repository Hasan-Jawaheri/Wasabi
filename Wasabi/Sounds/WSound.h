/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine OpenAL wrapper
*********************************************************************/

#pragma once

#include "../Core/WCore.h"

#include <al.h>				//openAL sound header
#include <alc.h>			//openAL sound header

#pragma comment(lib, "OpenAL32.lib") //openAL library

#define W_NUM_SOUND_BUFFERS_PER_SOUND 10

//forward declaration
class WSound;
class WBase;

class WSoundComponent {
	friend class WSound;

public:
	WSoundComponent(class Wasabi* const app);
	~WSoundComponent();

	ALCdevice* GetALSoundDevice() const;
	ALCcontext* GetALSoundDeviceContext() const;

	void SetSoundSpeed(float fSpeed);
	void SetDopplerFactor(float fFactor);
	void SetMasterGain(float fGain);

	void SetPosition(float x, float y, float z);
	void SetPosition(WVector3 pos);
	void SetPosition(class WOrientation* pos);
	void SetVelocity(float x, float y, float z);
	void SetVelocity(WVector3 vel);
	void SetOrientation(WVector3 look, WVector3 up);
	void SetOrientation(class WOrientation* ori);
	void SetToOrientationDevice(class WOrientation* ori);

	WSound* GetSoundHandle(uint ID) const;
	WSound* GetSoundHandleByIndex(uint index) const;
	uint GetSoundsCount() const;

protected:
	class Wasabi* m_app;

private:
	ALCdevice* m_oalDevice;
	ALCcontext* m_oalContext;

	vector<WSound*> m_soundV;

	void m_RegisterSound(WSound* sound);
	void m_UnRegisterSound(WBase* base);
};

/*********************************************************************
*********************************WSound******************************
Sound class
*********************************************************************/
class WSound : public WBase {
public:
	WSound(Wasabi* const app, uint ID = 0);
	~WSound();

	std::string GetTypeName() const;

	bool Valid() const;
	WError LoadWAV(std::string Filename, uint buffer, bool bSaveData = false);
	WError LoadFromMemory(uint buffer, void* data, size_t dataSize,
					ALenum format, uint frequency, bool bSaveData = false);
	void Play();
	void Loop();
	void Pause();
	void Reset();
	void SetTime(uint time);
	bool Playing() const;
	bool Looping() const;
	void SetVolume(float volume);
	void SetPitch(float fPitch);

	WError LoadFromWS(std::string filename, bool bSaveData = false);
	WError LoadFromWS(basic_filebuf<char>* buff, uint pos,
						bool bSaveData = false);
	WError SaveToWS(std::string filename) const;
	WError SaveToWS(basic_filebuf<char>* buff, uint pos) const;

	void SetFrequency(uint buffer, uint frequency);
	uint GetNumChannels(uint buffer) const;
	uint GetBitDepth(uint buffer) const;
	ALuint GetALBuffer(uint buffer) const;
	ALuint GetALSource() const;

	void SetPosition(float x, float y, float z);
	void SetPosition(WVector3 pos);
	void SetPosition(WOrientation* pos);
	void SetVelocity(float x, float y, float z);
	void SetVelocity(WVector3 vel);
	void SetDirection(WVector3 look);
	void SetDirection(class WOrientation* look);
	void SetToOrientationDevice(class WOrientation* oriDev);

private:
	bool m_valid;
	ALuint* m_buffers;
	ALuint m_source;
	uint m_numBuffers;

	struct __SAVEDATA {
		uint buffer;
		ALenum format;
		size_t dataSize;
		void* data;
	};
	vector<__SAVEDATA> m_dataV;
	bool m_bCheck(bool ignoreValidness = false) const;
};
