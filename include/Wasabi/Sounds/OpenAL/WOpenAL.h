/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine OpenAL wrapper
*********************************************************************/

#pragma once

#include "Wasabi/Sounds/WSound.h"

#define W_NUM_SOUND_BUFFERS_PER_SOUND 10

//forward declaration
class WSound;
class WBase;

class WOpenALSoundComponent : public WSoundComponent {
	friend class WSound;

public:
	WOpenALSoundComponent(class Wasabi* const app);
	virtual ~WOpenALSoundComponent();

	virtual WError Initialize();
	virtual void Cleanup();
	virtual WSound* CreateSound(uint32_t ID = 0) const;

	void* GetALSoundDevice() const;
	void* GetALSoundDeviceContext() const;

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
	void* m_oalDevice;
	void* m_oalContext;
};

/*********************************************************************
*********************************WSound******************************
Sound class
*********************************************************************/
class WOpenALSound : public WSound {
protected:
	virtual ~WOpenALSound();

public:
	static std::string _GetTypeName();
	virtual std::string GetTypeName() const override;
	virtual void SetID(uint32_t newID) override;
	virtual void SetName(std::string newName) override;

	WOpenALSound(class Wasabi* const app, uint32_t ID = 0);

	virtual bool Valid() const override;
	
	virtual WError LoadWAV(std::string Filename, uint32_t buffer, bool bSaveData = false) override;
	WError LoadFromMemory(uint32_t buffer, void* data, size_t dataSize, int format, uint32_t frequency, bool bSaveData = false);

	virtual void Play() override;
	virtual void Loop() override;
	virtual void Pause() override;
	virtual void Reset() override;
	virtual void SetTime(uint32_t time) override;
	virtual bool Playing() const override;
	virtual bool Looping() const override;
	virtual void SetVolume(float volume) override;
	virtual void SetPitch(int pitch) override;

	virtual void SetFrequency(uint32_t buffer, uint32_t frequency) override;
	virtual uint32_t GetNumChannels(uint32_t buffer) const override;
	virtual uint32_t GetBitDepth(uint32_t buffer) const override;
	uint32_t GetALBuffer(uint32_t buffer) const;
	uint32_t GetALSource() const;

	virtual void SetPosition(float x, float y, float z) override;
	virtual void SetPosition(WVector3 pos) override;
	virtual void SetPosition(WOrientation* pos) override;
	virtual void SetVelocity(float x, float y, float z) override;
	virtual void SetVelocity(WVector3 vel) override;
	virtual void SetDirection(WVector3 look) override;
	virtual void SetDirection(class WOrientation* look) override;
	virtual void SetToOrientation(class WOrientation* oriDev) override;

	static std::vector<void*> LoadArgs();
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream) override;
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) override;

private:
	bool m_valid;
	uint* m_buffers;
	uint32_t m_source;
	uint32_t m_numBuffers;

	struct __SAVEDATA {
		uint32_t buffer;
		int format;
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
