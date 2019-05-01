/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016-2019 by Hassan Al-Jawaheri
desc.: Wasabi Engine sounds interface to be implemented
*********************************************************************/

#pragma once

#include "../Core/WCore.h"

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
	virtual WSound* CreateSound(unsigned int ID = 0) const = 0;

	virtual void SetSoundSpeed(float fSpeed) {}
	virtual void SetDopplerFactor(float fFactor) {}
	virtual void SetMasterGain(float fGain) {}

	virtual void SetListenerPosition(float x, float y, float z) {}
	virtual void SetListenerPosition(WVector3 pos) {}
	virtual void SetListenerPosition(class WOrientation* pos) {}
	virtual void SetListenerVelocity(float x, float y, float z) {}
	virtual void SetListenerVelocity(WVector3 vel) {}
	virtual void SetListenerOrientation(WVector3 look, WVector3 up) {}
	virtual void SetListenerOrientation(class WOrientation* ori) {}
	virtual void SetListenerToOrientation(class WOrientation* ori) {}

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

	WSound(Wasabi* const app, unsigned int ID = 0) : WFileAsset(app, ID) {}

	virtual WError LoadWAV(std::string filename, uint buffer, bool bSaveData = false) = 0;
	virtual void Play() = 0;
	virtual void Loop() = 0;
	virtual void Pause() = 0;
	virtual void Reset() = 0;
	virtual void SetTime(uint time) = 0;
	virtual bool Playing() const = 0;
	virtual bool Looping() const = 0;

	virtual void SetVolume(float volume) = 0;
	virtual void SetPitch(float fPitch) {}
	virtual void SetFrequency(uint buffer, uint frequency) {}
	virtual uint GetNumChannels(uint buffer) const { return 0; }
	virtual uint GetBitDepth(uint buffer) const { return 0; }

	virtual void SetPosition(float x, float y, float z) {}
	virtual void SetPosition(WVector3 pos) {}
	virtual void SetPosition(WOrientation* pos) {}
	virtual void SetVelocity(float x, float y, float z) {}
	virtual void SetVelocity(WVector3 vel) {}
	virtual void SetDirection(WVector3 look) {}
	virtual void SetDirection(class WOrientation* look) {}
	virtual void SetToOrientation(class WOrientation* oriDev) {}

	virtual WError SaveToStream(WFile* file, std::ostream& outputStream) = 0;
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args) = 0;
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
