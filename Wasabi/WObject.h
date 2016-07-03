#pragma once

#include "Wasabi.h"

class WObject : public WBase {
	virtual std::string GetTypeName() const;

	class WGeometry* m_geometry;
	class WMaterial* m_material;

public:
	WObject(Wasabi* const app, unsigned int ID = 0);
	~WObject();

	void Render();

	WError SetGeometry(class WGeometry* geometry);
	WError SetMaterial(class WMaterial* material);

	virtual bool	Valid() const;
};

class WObjectManager : public WManager<WObject> {
	friend class WObject;

	virtual std::string GetTypeName(void) const;

public:
	WObjectManager(class Wasabi* const app);

	void Render();
};
