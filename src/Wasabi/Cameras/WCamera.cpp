#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"

WCameraManager::WCameraManager(Wasabi* const app) : WManager<WCamera>(app) {
	m_default_camera = nullptr;
}

WCameraManager::~WCameraManager() {
	W_SAFE_REMOVEREF(m_default_camera);
}

WError WCameraManager::Load() {
	m_default_camera = new WCamera(m_app);

	return WError(W_SUCCEEDED);
}

WCamera* WCameraManager::GetDefaultCamera() const {
	return m_default_camera;
}

std::string WCameraManager::GetTypeName() const {
	return "Camera";
}

WCamera::WCamera(Wasabi* const app, uint32_t ID) : WBase(app, ID) {
	//initialize default values
	m_lastWidth = m_app->WindowAndInputComponent->GetWindowWidth();
	m_lastHeight = m_app->WindowAndInputComponent->GetWindowHeight();
	m_bAutoAspect = true;
	m_fFOV = W_DEGTORAD(45);
	m_fAspect = float((float)m_lastWidth / (float)m_lastHeight);
	m_minRange = 1.0f;
	m_maxRange = 5000.0f;
	m_projType = PROJECTION_PERSPECTIVE;
	m_ViewM = WMatrix();
	m_ProjM = WMatrix();
	m_bAltered = true;

	//register the camera
	m_app->CameraManager->AddEntity(this);
}

WCamera::~WCamera() {
	m_app->CameraManager->RemoveEntity(this);
}

std::string WCamera::_GetTypeName() {
	return "Camera";
}

std::string WCamera::GetTypeName() const {
	return _GetTypeName();
}

void WCamera::SetID(uint32_t newID) {
	m_app->CameraManager->RemoveEntity(this);
	m_ID = newID;
	m_app->CameraManager->AddEntity(this);
}

void WCamera::SetName(std::string newName) {
	m_name = newName;
	m_app->CameraManager->OnEntityNameChanged(this, newName);
}

void WCamera::EnableAutoAspect() {
	m_bAutoAspect = true;
}

void WCamera::DisableAutoAspect() {
	m_bAutoAspect = false;
}

void WCamera::SetRange(float min, float max) {
	//adjust camera range
	m_minRange = min;
	m_maxRange = max;
	m_bAltered = true;
}

void WCamera::SetFOV(float fValue) {
	m_fFOV = W_DEGTORAD(fValue);
	m_bAltered = true;
}

void WCamera::SetAspect(float aspect) {
	m_fAspect = aspect;
	m_bAltered = true;
}

void WCamera::SetProjectionType(W_PROJECTIONTYPE type) {
	m_projType = type;
	m_bAltered = true;
}

WMatrix WCamera::GetViewMatrix() const {
	return m_ViewM;
}

WMatrix WCamera::GetProjectionMatrix(bool bForceOrtho) const {
	if (bForceOrtho)
		return m_orthoMatrix;
	return m_ProjM;
}

float WCamera::GetMinRange() const {
	return m_minRange;
}

float WCamera::GetMaxRange() const {
	return m_maxRange;
}

float WCamera::GetFOV() const {
	return W_RADTODEG(m_fFOV);
}

float WCamera::GetAspect() const {
	return m_fAspect;
}

void WCamera::Render(uint32_t width, uint32_t height) {
	if (m_lastWidth != width || m_lastHeight != height) {
		m_bAltered = true;
		m_lastWidth = width;
		m_lastHeight = height;
		if (m_bAutoAspect)
			m_fAspect = (float)width / (float)height;
	}

	UpdateInternals();
}

bool WCamera::CheckPointInFrustum(float x, float y, float z) const {
	// Check if the point is inside all six planes of the view m_frustum.
	for (uint32_t i = 0; i < 6; i++)
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3(x, y, z)) < 0.0f)
			return false;

	return true;
}

bool WCamera::CheckPointInFrustum(WVector3 point) const {
	return CheckPointInFrustum(point.x, point.y, point.z);
}

bool WCamera::CheckCubeInFrustum(float xCenter, float yCenter, float zCenter, float radius) const {
	// Check if any one point of the cube is in the view m_frustum.
	for (uint32_t i = 0; i < 6; i++) {
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - radius), (yCenter - radius), (zCenter - radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + radius), (yCenter - radius), (zCenter - radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - radius), (yCenter + radius), (zCenter - radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + radius), (yCenter + radius), (zCenter - radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - radius), (yCenter - radius), (zCenter + radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + radius), (yCenter - radius), (zCenter + radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - radius), (yCenter + radius), (zCenter + radius))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + radius), (yCenter + radius), (zCenter + radius))) >= 0.0f)
			continue;

		return false;
	}

	return true;
}

bool WCamera::CheckCubeInFrustum(WVector3 center, float radius) const {
	return CheckCubeInFrustum(center.x, center.y, center.z, radius);
}

bool WCamera::CheckSphereInFrustum(float xCenter, float yCenter, float zCenter, float radius) const {
	// Check if the radius of the sphere is inside the view m_frustum.
	for (uint32_t i = 0; i < 6; i++)
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3(xCenter, yCenter, zCenter)) < -radius)
			return false;

	return true;
}

bool WCamera::CheckSphereInFrustum(WVector3 center, float radius) const {
	return CheckSphereInFrustum(center.x, center.y, center.z, radius);
}

bool WCamera::CheckBoxInFrustum(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize) const {
	// Check if any of the 6 planes of the rectangle are inside the view m_frustum.
	for (uint32_t i = 0; i < 6; i++) {
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - xSize), (yCenter - ySize), (zCenter - zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + xSize), (yCenter - ySize), (zCenter - zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - xSize), (yCenter + ySize), (zCenter - zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - xSize), (yCenter - ySize), (zCenter + zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + xSize), (yCenter + ySize), (zCenter - zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + xSize), (yCenter - ySize), (zCenter + zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter - xSize), (yCenter + ySize), (zCenter + zSize))) >= 0.0f)
			continue;
		if (WPlaneDotCoord(m_frustumPlanes[i], WVector3((xCenter + xSize), (yCenter + ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		return false;
	}

	return true;
}

bool WCamera::CheckBoxInFrustum(WVector3 center, WVector3 size) const {
	return CheckBoxInFrustum(center.x, center.y, center.z, size.x, size.y, size.z);
}

void WCamera::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

void WCamera::UpdateInternals() {
	if (m_bAltered) {
		m_bAltered = false;

		m_ViewM = ComputeInverseTransformation(); // the view matrix is the inverse of the world matrix/transformation

		if (IsBound())
			m_ViewM *= GetBindingMatrix();

		//build projection matrix
		m_orthoMatrix = WOrthogonalProjMatrix((float)m_lastWidth, (float)m_lastHeight, m_minRange, m_maxRange);
		if (m_projType == PROJECTION_PERSPECTIVE)
			m_ProjM = WPerspectiveProjMatrixFOV(W_RADTODEG(m_fFOV), m_fAspect, m_minRange, m_maxRange);
		else if (m_projType == PROJECTION_ORTHOGONAL)
			m_ProjM = m_orthoMatrix;

		// fix the coordinate system (we use LHS, Vulkan uses RHS, so flip y coordinate)
		m_ProjM(1, 1) *= -1; // flip Y-axis in the render

		// Create the m_frustum matrix from the view matrix and updated projection matrix.
		WMatrix matrix = m_ViewM * m_ProjM;

		// Calculate near plane of m_frustum.
		m_frustumPlanes[0].a = matrix(0, 3) + matrix(0, 2);
		m_frustumPlanes[0].b = matrix(1, 3) + matrix(1, 2);
		m_frustumPlanes[0].c = matrix(2, 3) + matrix(2, 2);
		m_frustumPlanes[0].d = matrix(3, 3) + matrix(3, 2);
		m_frustumPlanes[0] = WNormalizePlane(m_frustumPlanes[0]);

		// Calculate far plane of m_frustum.
		m_frustumPlanes[1].a = matrix(0, 3) - matrix(0, 2);
		m_frustumPlanes[1].b = matrix(1, 3) - matrix(1, 2);
		m_frustumPlanes[1].c = matrix(2, 3) - matrix(2, 2);
		m_frustumPlanes[1].d = matrix(3, 3) - matrix(3, 2);
		m_frustumPlanes[1] = WNormalizePlane(m_frustumPlanes[1]);

		// Calculate left plane of m_frustum.
		m_frustumPlanes[2].a = matrix(0, 3) + matrix(0, 0);
		m_frustumPlanes[2].b = matrix(1, 3) + matrix(1, 0);
		m_frustumPlanes[2].c = matrix(2, 3) + matrix(2, 0);
		m_frustumPlanes[2].d = matrix(3, 3) + matrix(3, 0);
		m_frustumPlanes[2] = WNormalizePlane(m_frustumPlanes[2]);

		// Calculate right plane of m_frustum.
		m_frustumPlanes[3].a = matrix(0, 3) - matrix(0, 0);
		m_frustumPlanes[3].b = matrix(1, 3) - matrix(1, 0);
		m_frustumPlanes[3].c = matrix(2, 3) - matrix(2, 0);
		m_frustumPlanes[3].d = matrix(3, 3) - matrix(3, 0);
		m_frustumPlanes[3] = WNormalizePlane(m_frustumPlanes[3]);

		// Calculate top plane of m_frustum.
		m_frustumPlanes[4].a = matrix(0, 3) - matrix(0, 1);
		m_frustumPlanes[4].b = matrix(1, 3) - matrix(1, 1);
		m_frustumPlanes[4].c = matrix(2, 3) - matrix(2, 1);
		m_frustumPlanes[4].d = matrix(3, 3) - matrix(3, 1);
		m_frustumPlanes[4] = WNormalizePlane(m_frustumPlanes[4]);

		// Calculate bottom plane of m_frustum.
		m_frustumPlanes[5].a = matrix(0, 3) + matrix(0, 1);
		m_frustumPlanes[5].b = matrix(1, 3) + matrix(1, 1);
		m_frustumPlanes[5].c = matrix(2, 3) + matrix(2, 1);
		m_frustumPlanes[5].d = matrix(3, 3) + matrix(3, 1);
		m_frustumPlanes[5] = WNormalizePlane(m_frustumPlanes[5]);
	}
}

bool WCamera::Valid() const {
	return true; //always true..
}