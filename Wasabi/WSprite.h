#pragma once

#include "Wasabi.h"

class WSprite : public WBase {
	virtual std::string GetTypeName() const;

	bool			m_hidden;

public:
	WSprite(Wasabi* const app, unsigned int ID = 0);
	~WSprite();

	void Render();

	void			Show();
	void			Hide();
	bool			Hidden() const;

	virtual bool	Valid() const;
};

class WSpriteManager : public WManager<WSprite> {
	friend class WSprite;

	virtual std::string GetTypeName() const;

	class WGeometry* m_spriteGeometry;
	class WMaterial* m_spriteMaterial;

public:
	WSpriteManager(class Wasabi* const app);
	~WSpriteManager();

	void Load();

	void Render();
};
