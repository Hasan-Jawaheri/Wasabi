#pragma once

#include "../Core/Core.h"

enum W_PROJECTIONTYPE { PROJECTION_PERSPECTIVE = 0, PROJECTION_ORTHOGONAL = 1 };

class WCamera : public WBase, public WOrientation {
	virtual std::string GetTypeName() const;

	bool				m_bAltered;
	WMatrix				m_ViewM;
	WMatrix				m_ProjM;
	WMatrix				m_orthoMatrix;
	float				m_maxRange;
	float				m_minRange;
	float				m_fFOV;
	float				m_fAspect;
	bool				m_bAutoAspect;
	W_PROJECTIONTYPE	m_projType;
	WPlane				m_frustumPlanes[6];

	unsigned int		m_lastWidth, m_lastHeight;

public:
	WCamera(Wasabi* const app, unsigned int ID = 0);
	~WCamera();

	void		OnStateChange(STATE_CHANGE_TYPE type);
	void		UpdateInternals();

	void		EnableAutoAspect();
	void		DisableAutoAspect();
	void		SetRange(float min, float max);
	void		SetFOV(float fValue);
	void		SetAspect(float aspect);
	void		SetProjectionType(W_PROJECTIONTYPE type);

	void		Render(unsigned int width, unsigned int height);

	WMatrix		GetViewMatrix() const;
	WMatrix		GetProjectionMatrix(bool bForceOrtho = false) const;
	float		GetMinRange() const;
	float		GetMaxRange() const;
	float		GetFOV() const;
	float		GetAspect() const;

	bool		CheckPointInFrustum(float x, float y, float z) const;
	bool		CheckPointInFrustum(WVector3 point) const;
	bool		CheckCubeInFrustum(float x, float y, float z, float radius) const;
	bool		CheckCubeInFrustum(WVector3 pos, float radius) const;
	bool		CheckSphereInFrustum(float x, float y, float z, float radius) const;
	bool		CheckSphereInFrustum(WVector3 pos, float radius) const;
	bool		CheckBoxInFrustum(float x, float y, float z, float xSize, float ySize, float zSize) const;
	bool		CheckBoxInFrustum(WVector3 pos, WVector3 size) const;

	virtual bool Valid() const;
};

class WCameraManager : public WManager<WCamera> {
	virtual std::string GetTypeName() const;

	WCamera* m_default_camera;

public:
	WCameraManager(Wasabi* const app);
	~WCameraManager();

	WError	Load();

	WCamera* GetDefaultCamera() const;
};
