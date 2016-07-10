#pragma once

#include "Wasabi.h"

class WLight : public WBase {
	virtual std::string GetTypeName() const;

public:
	WLight(class Wasabi* const app, unsigned int ID);
	~WLight();

	virtual bool Valid() const;
};

class WLightManager : public WManager<WLight> {
	friend class WLight;

	virtual std::string GetTypeName() const;

public:
	WLightManager(class Wasabi* const app);
	~WLightManager();

	WError Load();
};
