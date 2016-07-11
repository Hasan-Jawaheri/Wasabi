#pragma once

#include "Wasabi.h"

enum W_LIGHT_TYPE {
	W_LIGHT_DIRECTIONAL = 0,
	W_LIGHT_POINT = 1,
	W_LIGHT_SPOT = 2,
};

class WLight : public WBase, public WOrientation {
	virtual std::string GetTypeName() const;

public:
	WLight(class Wasabi* const app, unsigned int ID = 0);
	~WLight();

	void			SetType(W_LIGHT_TYPE type);
	void			SetColor(WColor col);
	void			SetRange(float fRange);
	void			SetIntensity(float fIntensity);

	W_LIGHT_TYPE	GetType();
	WColor			GetColor();
	float			GetRange();
	float			GetIntensity();

	void			Show();
	void			Hide();
	bool			Hidden() const;

	WMatrix			GetWorldMatrix();
	bool			UpdateLocals();

	virtual bool	Valid() const;
	virtual void	OnStateChange(STATE_CHANGE_TYPE type);

private:
	bool			m_hidden;
	bool			m_bAltered;
	WMatrix			m_WorldM;
	W_LIGHT_TYPE	m_type;
	WColor			m_color;
	float			m_range;
	float			m_intensity;
};

class WLightManager : public WManager<WLight> {
	friend class WLight;

	virtual std::string GetTypeName() const;

	WLight* m_defaultLight;

public:
	WLightManager(class Wasabi* const app);
	~WLightManager();

	WError Load();

	WLight* GetDefaultLight() const;
};
