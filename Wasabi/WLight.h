#pragma once

#include "Wasabi.h"

class WLight : public WBase, public WOrientation {
	virtual std::string GetTypeName() const;

public:
	WLight(class Wasabi* const app, unsigned int ID);
	~WLight();

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
};

class WLightManager : public WManager<WLight> {
	friend class WLight;

	virtual std::string GetTypeName() const;

public:
	WLightManager(class Wasabi* const app);
	~WLightManager();

	WError Load();
};
