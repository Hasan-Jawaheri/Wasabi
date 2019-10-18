/** @file WLight.h
 *  @brief Lights control in Wasabi
 *
 *  Lighting in Wasabi is designed such that lights are independent from the
 *  renderer, and the renderer has to make sure it supports the lights that
 *  represented by WLight objects.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/Core/WCore.hpp"

/** Describes a light type */
enum W_LIGHT_TYPE: uint8_t {
	W_LIGHT_DIRECTIONAL = 0,
	W_LIGHT_POINT = 1,
	W_LIGHT_SPOT = 2,
};

/**
 * @ingroup engineclass
 * This represents a light to be rendered by the used renderer. Lights have 3
 * types:
 * * Directional: This is light that has infinite range, and always strikes in
 * 	 the same direction (the L vector of the WLight's WOrientation base class).
 * 	 This is suitable to mimic sunlight.
 * * Point: This is light that is illuminated from a point (the position of the
 * 	 WLight's WOrientation base class) whose range is specified by SetRange().
 * 	 This is suitable for light bulbs or flame torches etc...
 * * Spot: This is light that is illuminated from a point (the position of the
 * 	 WLight's WOrientation base class) pointing at a direction (the L vector
 * 	 of the WLight's WOrientation base class) in a cone shape whose height (
 * 	 or length) is specified by SetRange() and whose emission angle is
 * 	 specified by SetEmittingAngle(). This is suitable for car headlights,
 * 	 torch spotlight, etc...
 */
class WLight : public WOrientation, public WFileAsset {
protected:
	virtual ~WLight();

public:
	/**
	 * Returns "Light" string.
	 * @return Returns "Light" string
	 */
	static std::string _GetTypeName();
	virtual std::string GetTypeName() const override;
	virtual void SetID(uint32_t newID) override;
	virtual void SetName(std::string newName) override;

	WLight(class Wasabi* const app, W_LIGHT_TYPE type = W_LIGHT_DIRECTIONAL, uint32_t ID = 0);

	/**
	 * Sets the color of this light
	 * @param col New color, alpha component will be used as light specular power
	 */
	void SetColor(WColor col);

	/**
	 * Sets the range of the light. In case of a directional light, this is
	 * ignored. In case of a point light, this is the radius of the sphere. In
	 * case of a spot light, this is the length of the cone.
	 * @param fRange New range
	 */
	void SetRange(float fRange);

	/**
	 * Sets the intensity of the light.
	 * @param fIntensity New intensity
	 */
	void SetIntensity(float fIntensity);

	/**
	 * Sets the angle at which the spot light emits (angle for the cone). This
	 * is ignored in other light types.
	 * @param fAngle Angle of emission, in degrees
	 */
	void SetEmittingAngle(float fAngle);

	/**
	 * Retrieves the type of the light.
	 * @return Type of the light
	 */
	W_LIGHT_TYPE GetType() const;

	/**
	 * Retrieves the color of the light
	 * @return Color of the light
	 */
	WColor GetColor() const;

	/**
	 * Retrieves the range of the light.
	 * @return Range of the light
	 */
	float GetRange() const;

	/**
	 * Retrieves the intensity of the light.
	 * @return Intensity of the light
	 */
	float GetIntensity() const;

	/**
	 * Retrieves the cosine of half the emission angle set.
	 * @return Cosine of half the emission angle set.
	 */
	float GetMinCosAngle() const;

	/**
	 * Shows the light so that it can be rendered.
	 */
	void Show();

	/**
	 * Hides the light.
	 */
	void Hide();

	/**
	 * Checks if the light is hidden.
	 * @return true if the light is hidden, false otherwise
	 */
	bool Hidden() const;

	/**
	 * Checks if the light appears anywhere in the view of the camera
	 * @param  cam Camera to check against
	 * @return     true if the light is in the viewing frustum of cam, false
	 *             otherwise
	 */
	bool InCameraView(class WCamera* cam) const;

	/**
	 * Retrieves the world matrix for the light.
	 * @return World matrix for the light
	 */
	WMatrix GetWorldMatrix();

	/**
	 * Updates the world matrix of the light.
	 * @return true if the matrix has changed, false otherwise
	 */
	bool UpdateLocals();

	/**
	 * Always true.
	 * @return true
	 */
	virtual bool Valid() const override;

	virtual void OnStateChange(STATE_CHANGE_TYPE type) override;

	static std::vector<void*> LoadArgs();
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream) override;
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) override;

private:
	/** true if the light is hidden, false otherwise */
	bool m_hidden;
	/** true if the matrix needs to be updated, false otherwise */
	bool m_bAltered;
	/** World matrix for the light */
	WMatrix m_WorldM;
	/** Type of the light */
	W_LIGHT_TYPE m_type;
	/** Color of the light */
	WColor m_color;
	/** Range of the light */
	float m_range;
	/** Intensity of the light */
	float m_intensity;
	/** Cosine of half the emission angle */
	float m_cosAngle;
};

class WDirectionalLight : public WLight {
public:
	WDirectionalLight(class Wasabi* const app, uint32_t ID = 0)
		: WLight(app, W_LIGHT_DIRECTIONAL, ID) {}
};

class WPointLight : public WLight {
public:
	WPointLight(class Wasabi* const app, uint32_t ID = 0)
		: WLight(app, W_LIGHT_POINT, ID) {}
};

class WSpotLight : public WLight {
public:
	WSpotLight(class Wasabi* const app, uint32_t ID = 0)
		: WLight(app, W_LIGHT_SPOT, ID) {}
};

/**
 * @ingroup engineclass
 * Manager class for WLight.
 */
class WLightManager : public WManager<WLight> {
	friend class WLight;

	/**
	 * Returns "Light" string.
	 * @return Returns "Light" string
	 */
	virtual std::string GetTypeName() const;

	/** Default directional light, created by the manager. */
	WLight* m_defaultLight;

public:
	WLightManager(class Wasabi* const app);
	~WLightManager();

	/**
	 * Loads the manager and creates the default directional light.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Retrieves the default (directional) light.
	 * @return Pointer to the default light
	 */
	WLight* GetDefaultLight() const;
};
