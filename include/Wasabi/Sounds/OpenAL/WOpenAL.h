/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine OpenAL wrapper
*********************************************************************/

#pragma once

#include "Sounds/WSound.h"

#include <al.h>				//openAL sound header
#include <alc.h>			//openAL sound header

#pragma comment(lib, "OpenAL32.lib") //openAL library

#define W_NUM_SOUND_BUFFERS_PER_SOUND 10

//forward declaration
class WSound;
class WBase;

class WOpenALSoundComponent : public WSoundComponent {
	friend class WSound;

public:
	WOpenALSoundComponent(class Wasabi* const app);
	~WOpenALSoundComponent();

	virtual WError Initialize();
	virtual void Cleanup();
	virtual WSound* CreateSound(unsigned int ID = 0) const;

	ALCdevice* GetALSoundDevice() const;
	ALCcontext* GetALSoundDeviceContext() const;

	virtual void SetSoundSpeed(float fSpeed);
	virtual void SetDopplerFactor(float fFactor);
	virtual void SetMasterGain(float fGain);

	virtual void SetListenerPosition(float x, float y, float z);
	virtual void SetListenerPosition(WVector3 pos);
	virtual void SetListenerPosition(class WOrientation* pos);
	virtual void SetListenerVelocity(float x, float y, float z);
	virtual void SetListenerVelocity(WVector3 vel);
	virtual void SetListenerOrientation(WVector3 look, WVector3 up);
	virtual void SetListenerOrientation(class WOrientation* ori);
	virtual void SetListenerToOrientation(class WOrientation* ori);

protected:
	class Wasabi* m_app;

private:
	ALCdevice* m_oalDevice;
	ALCcontext* m_oalContext;
};

/*********************************************************************
*********************************WSound******************************
Sound class
*********************************************************************/
class WOpenALSound : public WSound {
protected:
	virtual ~WOpenALSound();

public:
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();
	
	WOpenALSound(class Wasabi* const app, uint ID = 0);

	virtual bool Valid() const;
	virtual WError LoadWAV(std::string Filename, uint buffer, bool bSaveData = false);
	WError LoadFromMemory(uint buffer, void* data, size_t dataSize,
					ALenum format, uint frequency, bool bSaveData = false);
	virtual void Play();
	virtual void Loop();
	virtual void Pause();
	virtual void Reset();
	virtual void SetTime(uint time);
	virtual bool Playing() const;
	virtual bool Looping() const;
	virtual void SetVolume(float volume);
	virtual void SetPitch(float fPitch);

	virtual void SetFrequency(uint buffer, uint frequency);
	virtual uint GetNumChannels(uint buffer) const;
	virtual uint GetBitDepth(uint buffer) const;
	ALuint GetALBuffer(uint buffer) const;
	ALuint GetALSource() const;

	virtual void SetPosition(float x, float y, float z);
	virtual void SetPosition(WVector3 pos);
	virtual void SetPosition(WOrientation* pos);
	virtual void SetVelocity(float x, float y, float z);
	virtual void SetVelocity(WVector3 vel);
	virtual void SetDirection(WVector3 look);
	virtual void SetDirection(class WOrientation* look);
	virtual void SetToOrientation(class WOrientation* oriDev);

	static std::vector<void*> LoadArgs();
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args);

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

/**
 * @ingroup engineclass
 * OpenAL's manager class for WSound.
 */
class WOpenALSoundManager : public WSoundManager {
	friend class WRigidBody;

	/**
	 * Returns "Sound" string.
	 * @return Returns "Sound" string
	 */
	virtual std::string GetTypeName() const;

public:
	WOpenALSoundManager(class Wasabi* const app);
	~WOpenALSoundManager();
};
