#pragma once

#include "Wasabi.h"

class WSprite : public WBase {
	virtual std::string GetTypeName() const;

	bool			m_hidden;
	class WImage*	m_img;
	WVector2		m_pos;
	WVector2		m_size;
	WVector2		m_rotationCenter;
	float			m_angle;
	unsigned int	m_priority;
	float			m_alpha;

public:
	WSprite(Wasabi* const app, unsigned int ID = 0);
	~WSprite();

	void			SetImage(WImage* img);
	void			SetPosition(float x, float y);
	void			SetPosition(WVector2 pos);
	void			Move(float units);
	void			SetSize(float sizeX, float sizeY);
	void			SetSize(WVector2 size);
	void			SetAngle(float fAngle);
	void			Rotate(float fAngle);
	void			SetRotationCenter(float x, float y);
	void			SetRotationCenter(WVector2 center);
	void			SetPriority(unsigned int priority);
	void			SetAlpha(float fAlpha);

	void			Render(class WRenderTarget* rt);

	void			Show();
	void			Hide();
	bool			Hidden() const;

	float			GetAngle() const;
	float			GetPositionX() const;
	float			GetPositionY() const;
	WVector2		GetPosition() const;
	float			GetSizeX() const;
	float			GetSizeY() const;
	WVector2		GetSize() const;
	unsigned int	GetPriority() const;

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

	WError Load();

	void Render(class WRenderTarget* rt);
};
