#pragma once

#include "../Core/WCore.h"

class WInstance : public WOrientation {
	friend class WObject;

public:
	WInstance();
	~WInstance();

	void				Scale(float x, float y, float z);
	void				Scale(WVector3 scale);
	void				ScaleX(float scale);
	void				ScaleY(float scale);
	void				ScaleZ(float scale);
	WVector3			GetScale() const;
	float				GetScaleX() const;
	float				GetScaleY() const;
	float				GetScaleZ() const;
	WMatrix				GetWorldMatrix();
	bool				UpdateLocals();
	void				OnStateChange(STATE_CHANGE_TYPE type);

private:
	WVector3 m_scale;
	bool m_bAltered;
	WMatrix m_worldM;
};

class WObject : public WBase, public WOrientation {
	virtual std::string GetTypeName() const;

public:
	WObject(Wasabi* const app, unsigned int ID = 0);
	~WObject();

	void					Render(class WRenderTarget* rt);

	WError					SetGeometry(class WGeometry* geometry);
	WError					SetMaterial(class WMaterial* material);
	WError					SetAnimation(class WAnimation* animation);

	class WGeometry*		GetGeometry() const;
	class WMaterial*		GetMaterial() const;
	class WAnimation*		GetAnimation() const;

	WError					InitInstancing(unsigned int maxInstances);
	WInstance*				CreateInstance();
	WInstance*				GetInstance(unsigned int index) const;
	void					DeleteInstance(WInstance* instance);
	void					DeleteInstance(unsigned int index);

	void					Show();
	void					Hide();
	bool					Hidden() const;
	void					EnableFrustumCulling();
	void					DisableFrustumCulling();
	void					Scale(float x, float y, float z);
	void					Scale(WVector3 scale);
	void					ScaleX(float scale);
	void					ScaleY(float scale);
	void					ScaleZ(float scale);
	WVector3				GetScale() const;
	float					GetScaleX() const;
	float					GetScaleY() const;
	float					GetScaleZ() const;
	WMatrix					GetWorldMatrix();
	bool					UpdateLocals(WVector3 offset = WVector3(0, 0, 0));
	virtual void			OnStateChange(STATE_CHANGE_TYPE type);

	virtual bool			Valid() const;

private:
	class WGeometry*	m_geometry;
	class WMaterial*	m_material;
	class WAnimation*	m_animation;
	
	bool				m_bAltered;
	bool				m_hidden;
	bool				m_bFrustumCull;
	WMatrix				m_WorldM;
	float				m_fScaleX, m_fScaleY, m_fScaleZ;
	class WImage*		m_instanceTexture;
	bool				m_instancesDirty;
	vector<WInstance*>	m_instanceV;

	void _UpdateInstanceBuffer();
};

class WObjectManager : public WManager<WObject> {
	friend class WObject;

	virtual std::string GetTypeName() const;

public:
	WObjectManager(class Wasabi* const app);

	WObject* PickObject(int x, int y, bool bAnyHit, unsigned int iObjStartID = 0, unsigned int iObjEndID = 0,
						WVector3* pt = nullptr, WVector2* uv = nullptr, unsigned int* faceIndex = nullptr) const;

	void Render(class WRenderTarget* rt);
	WError Load();
};
