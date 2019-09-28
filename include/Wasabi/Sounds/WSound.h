/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016-2019 by Hassan Al-Jawaheri
desc.: Wasabi Engine sounds interface to be implemented
*********************************************************************/

#pragma once

#include "Wasabi/Core/WCore.h"

class WSound;
class WBase;

class WSoundComponent {
	friend class WSound;

public:
	WSoundComponent(class Wasabi* const app) : m_app(app), SoundManager(nullptr) {}
	~WSoundComponent() {}

	class WSoundManager* SoundManager;

	/**
	 * Initializes the sound component.
	 * @return Error code, see WError.h
	 */
	virtual WError Initialize() = 0;

	/**
	 * Frees all resources allocated by the sounds component.
	 */
	virtual void Cleanup() = 0;

	/**
	 * Creates a new sound instance. This is equivalent to calling
	 * new WSound(app, ID), so care must be taken to free the resource
	 * when done with it.
	 * @param ID  ID for the created sound
	 * @return    A newly allocated sound instance
	 */
	virtual WSound* CreateSound(uint32_t ID = 0) const = 0;

	virtual void SetSoundSpeed(float fSpeed) {
		UNREFERENCED_PARAMETER(fSpeed);
	}

	virtual void SetDopplerFactor(float fFactor) {
		UNREFERENCED_PARAMETER(fFactor);
	}

	virtual void SetMasterGain(float fGain) {
		UNREFERENCED_PARAMETER(fGain);
	}

	virtual void SetListenerPosition(float x, float y, float z) {
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(z);
	}

	virtual void SetListenerPosition(WVector3 pos) {
		UNREFERENCED_PARAMETER(pos);
	}

	virtual void SetListenerPosition(class WOrientation* pos) {
		UNREFERENCED_PARAMETER(pos);
	}

	virtual void SetListenerVelocity(float x, float y, float z) {
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(z);
	}

	virtual void SetListenerVelocity(WVector3 vel) {
		UNREFERENCED_PARAMETER(vel);
	}

	virtual void SetListenerOrientation(WVector3 look, WVector3 up) {
		UNREFERENCED_PARAMETER(look);
		UNREFERENCED_PARAMETER(up);
	}

	virtual void SetListenerOrientation(class WOrientation* ori) {
		UNREFERENCED_PARAMETER(ori);
	}

	virtual void SetListenerToOrientation(class WOrientation* ori) {
		UNREFERENCED_PARAMETER(ori);
	}

protected:
	/** A pointer to the Wasabi application */
	class Wasabi* m_app;
};

/*********************************************************************
*********************************WSound******************************
Sound class interface
*********************************************************************/
class WSound : public WFileAsset {
protected:
	virtual ~WSound() {}

public:

	WSound(Wasabi* const app, uint32_t ID = 0) : WFileAsset(app, ID) {}

	virtual WError LoadWAV(std::string filename, uint32_t buffer, bool bSaveData = false) = 0;
	virtual void Play() = 0;
	virtual void Loop() = 0;
	virtual void Pause() = 0;
	virtual void Reset() = 0;
	virtual void SetTime(uint32_t time) = 0;
	virtual bool Playing() const = 0;
	virtual bool Looping() const = 0;

	virtual void SetVolume(float volume) = 0;

	virtual void SetPitch(float fPitch) {
		UNREFERENCED_PARAMETER(fPitch);
	}

	virtual void SetFrequency(uint32_t buffer, uint32_t frequency) {
		UNREFERENCED_PARAMETER(buffer);
		UNREFERENCED_PARAMETER(frequency);
	}

	virtual uint32_t GetNumChannels(uint32_t buffer) const {
		UNREFERENCED_PARAMETER(buffer);
		return 0;
	}

	virtual uint32_t GetBitDepth(uint32_t buffer) const {
		UNREFERENCED_PARAMETER(buffer);
		return 0;
	}

	virtual void SetPosition(float x, float y, float z) {
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(z);
	}

	virtual void SetPosition(WVector3 pos) {
		UNREFERENCED_PARAMETER(pos);
	}

	virtual void SetPosition(WOrientation* pos) {
		UNREFERENCED_PARAMETER(pos);
	}

	virtual void SetVelocity(float x, float y, float z) {
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(z);
	}

	virtual void SetVelocity(WVector3 vel) {
		UNREFERENCED_PARAMETER(vel);
	}

	virtual void SetDirection(WVector3 look) {
		UNREFERENCED_PARAMETER(look);
	}

	virtual void SetDirection(class WOrientation* look) {
		UNREFERENCED_PARAMETER(look);
	}

	virtual void SetToOrientation(class WOrientation* oriDev) {
		UNREFERENCED_PARAMETER(oriDev);
	}

	virtual WError SaveToStream(WFile* file, std::ostream& outputStream) = 0;
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) = 0;
};

/**
 * @ingroup engineclass
 * Manager class for WSound.
 */
class WSoundManager : public WManager<WSound> {
public:
	WSoundManager(class Wasabi* const app) : WManager<WSound>(app) {}
	~WSoundManager() {}
};
