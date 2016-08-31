#include "WObject.h"

std::string WObjectManager::GetTypeName() const {
	return "Object";
}

WObjectManager::WObjectManager(class Wasabi* const app) : WManager<WObject>(app) {
}

void WObjectManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

WError WObjectManager::Load() {
	VkDevice device = m_app->GetVulkanDevice();
	VkMemoryAllocateInfo memAlloc = {};
	VkMemoryRequirements memReqs;
	VkBufferCreateInfo dummyBufferInfo = {};
	VkBufferCopy copyRegion = {};
	void *data;
	VkResult err;

	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	// Instance buffer
	dummyBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	dummyBufferInfo.size = 1;
	dummyBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	err = vkCreateBuffer(device, &dummyBufferInfo, nullptr, &m_dummyBuf.buf);
	if (err)
		goto destroy_resources;
	vkGetBufferMemoryRequirements(device, m_dummyBuf.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits,
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &m_dummyBuf.mem);
	if (err)
		goto destroy_resources;
	err = vkBindBufferMemory(device, m_dummyBuf.buf, m_dummyBuf.mem, 0);

destroy_resources:
	if (err) {
		m_dummyBuf.Destroy(device);
		return WError(W_OUTOFMEMORY);
	}

	return WError(W_SUCCEEDED);
}

VkBuffer* WObjectManager::GetDummyBuffer() {
	return &m_dummyBuf.buf;
}

WObject* WObjectManager::PickObject(int x, int y, bool bAnyHit, unsigned int iObjStartID, unsigned int iObjEndID,
									WVector3* _pt, WVector2* uv, unsigned int* faceIndex) const {
	struct pickStruct {
		WObject* obj;
		WVector3 pos;
		WVector2 uv;
		unsigned int face;
	};
	vector<pickStruct> pickedObjects;

	WCamera* cam = m_app->Renderer->GetDefaultRenderTarget()->GetCamera();
	if (!cam)
		return nullptr;

	int Width = m_app->WindowComponent->GetWindowWidth();
	int Height = m_app->WindowComponent->GetWindowHeight();
	cam->Render(Width, Height);

	WMatrix P = cam->GetProjectionMatrix();
	WMatrix V = cam->GetViewMatrix();
	WMatrix inverseV = WMatrixInverse(V);

	// Compute picking ray in view space.
	float vx = (+2.0f*x / Width  - 1.0f) / P(0, 0);
	float vy = (+2.0f*y / Height - 1.0f) / P(1, 1);

	WVector3 pos(0.0f, 0.0f, 0.0f);
	WVector3 dir(vx, vy, 1.0f);

	pos = WVec3TransformCoord(pos, inverseV);
	dir = WVec3TransformNormal(dir, inverseV);

	for (unsigned int j = 0; j < W_HASHTABLESIZE; j++) {
		for (unsigned int i = m_entities[j].size() - 1; i >= 0 && i != UINT_MAX; i--) {
			if (((m_entities[j][i]->GetID() >= iObjStartID && m_entities[j][i]->GetID() <= iObjEndID) ||
				(!iObjStartID && !iObjEndID)) && m_entities[j][i]->Valid()) {
				if (m_entities[j][i]->Hidden())
					continue;

				unsigned int hit = 0;
				WGeometry* temp = m_entities[j][i]->GetGeometry();
				if (temp) {
					//these calculations are per-subset
					WMatrix inverseW = WMatrixInverse(m_entities[j][i]->GetWorldMatrix());

					WVector3 subsetPos = WVec3TransformCoord(pos, inverseW);
					WVector3 subsetDir = WVec3TransformNormal(dir, inverseW);

					WVector3 boxPos = (temp->GetMaxPoint() + temp->GetMinPoint()) / 2.0f;
					WVector3 boxSize = (temp->GetMaxPoint() - temp->GetMinPoint()) / 2.0f;

					pickStruct p;
					p.obj = m_entities[j][i];
					if (WUtil::RayIntersectBox(boxSize, subsetPos, subsetDir, boxPos)) {
						WVector3 pt;
						bool b = temp->Intersect(subsetPos, subsetDir, &pt, &p.uv, &p.face);
						WMatrix m = m_entities[j][i]->GetWorldMatrix();
						p.pos.x = (pt.x * m(0, 0)) + (pt.y * m(1, 0)) + (pt.z * m(2, 0)) + (1 * m(3, 0));
						p.pos.y = (pt.x * m(0, 1)) + (pt.y * m(1, 1)) + (pt.z * m(2, 1)) + (1 * m(3, 1));
						p.pos.z = (pt.x * m(0, 2)) + (pt.y * m(1, 2)) + (pt.z * m(2, 2)) + (1 * m(3, 2));

						if (b) {
							if (bAnyHit) {
								if (faceIndex) *faceIndex = p.face;
								if (_pt) *_pt = pt;
								if (uv) *uv = p.uv;
								return m_entities[j][i];
							}
							pickedObjects.push_back(p);
						}
					}
				}
			}
		}
	}

	if (!pickedObjects.size())
		return 0;

	unsigned int nearest = 0;
	float distance = FLT_MAX;
	for (unsigned int i = 0; i < pickedObjects.size(); i++) {
		float fCurDist = WVec3Length(pickedObjects[i].pos - cam->GetPosition());
		if (fCurDist < distance) {
			nearest = i;
			distance = fCurDist;
		}
	}

	if (_pt) *_pt = pickedObjects[nearest].pos;
	if (uv) *uv = pickedObjects[nearest].uv;
	if (faceIndex) *faceIndex = pickedObjects[nearest].face;
	return pickedObjects[nearest].obj;
}

WInstance::WInstance() {
	m_scale = WVector3(100.0f, 100.0f, 100.0f);
}

WInstance::~WInstance() {
}

void WInstance::Scale(float x, float y, float z) {
	m_scale = WVector3(x, y, z);
	m_bAltered = true;
}

void WInstance::Scale(WVector3 scale) {
	m_scale = scale;
	m_bAltered = true;
}

void WInstance::ScaleX(float scale) {
	m_scale.x = scale;
	m_bAltered = true;
}

void WInstance::ScaleY(float scale) {
	m_scale.y = scale;
	m_bAltered = true;
}

void WInstance::ScaleZ(float scale) {
	m_scale.z = scale;
	m_bAltered = true;
}

WVector3 WInstance::GetScale() const {
	return m_scale;
}

float WInstance::GetScaleX() const {
	return m_scale.x;
}

float WInstance::GetScaleY() const {
	return m_scale.y;
}

float WInstance::GetScaleZ() const {
	return m_scale.z;
}

WMatrix WInstance::GetWorldMatrix() {
	UpdateLocals();
	WMatrix m;
	m = m_worldM;
	m(0, 3) = 0;
	m(1, 3) = 0;
	m(2, 3) = 0;
	return m;
}

bool WInstance::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = GetPosition();

		//
		//the world matrix is the view matrix's inverse
		//so we build a normal view matrix and inverse it
		//

		//build world matrix
		float x = -WVec3Dot(_right, _pos);
		float y = -WVec3Dot(_up, _pos);
		float z = -WVec3Dot(_look, _pos);
		(m_worldM)(0, 0) = _right.x; (m_worldM)(0, 1) = _up.x; (m_worldM)(0, 2) = _look.x; (m_worldM)(0, 3) = 0.0f;
		(m_worldM)(1, 0) = _right.y; (m_worldM)(1, 1) = _up.y; (m_worldM)(1, 2) = _look.y; (m_worldM)(1, 3) = 0.0f;
		(m_worldM)(2, 0) = _right.z; (m_worldM)(2, 1) = _up.z; (m_worldM)(2, 2) = _look.z; (m_worldM)(2, 3) = 0.0f;
		(m_worldM)(3, 0) = x;        (m_worldM)(3, 1) = y;     (m_worldM)(3, 2) = z;       (m_worldM)(3, 3) = 1.0f;
		m_worldM = WMatrixInverse(m_worldM);

		//scale matrix
		m_worldM = WScalingMatrix(m_scale.x / 100.0f, m_scale.y / 100.0f, m_scale.z / 100.0f) * m_worldM;

		if (IsBound())
			m_worldM *= GetBindingMatrix();

		m_worldM(0, 3) = m_worldM(3, 0);
		m_worldM(1, 3) = m_worldM(3, 1);
		m_worldM(2, 3) = m_worldM(3, 2);

		return true;
	}

	return false;
}

void WInstance::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type);
	m_bAltered = true;
}

WObject::WObject(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_geometry = nullptr;
	m_material = app->Renderer->CreateDefaultMaterial();
	m_animation = nullptr;

	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();
	m_fScaleX = m_fScaleY = m_fScaleZ = 100.0f;

	m_maxInstances = 0;

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);
	W_SAFE_REMOVEREF(m_animation);

	VkDevice device = m_app->GetVulkanDevice();
	m_instanceBuf.Destroy(device);
	for (int i = 0; i < m_instanceV.size(); i++)
		delete m_instanceV[i];
	m_instanceV.clear();

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	return true; // TODO: put actual implementation
}

void WObject::Render(WRenderTarget* rt) {
	if (m_geometry && m_material && m_geometry->Valid() && m_material->Valid() && !m_hidden) {
		if (m_geometry->GetVertexDescription(0).GetSize() != m_material->GetEffect()->GetInputLayout(0).GetSize())
			return;
		WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			WMatrix wmtx = GetWorldMatrix();
			WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), wmtx);
			WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), wmtx);
			WVector3 pos = (max + min) / 2.0f;
			WVector3 size = (max - min) / 2.0f;
			if (!cam->CheckBoxInFrustum(pos, size))
				return; //outside viewing frustum
		}

		_UpdateInstanceBuffer();

		m_material->SetVariableMatrix("gWorld", worldM);
		m_material->SetVariableMatrix("gProjection", cam->GetProjectionMatrix());
		m_material->SetVariableMatrix("gView", cam->GetViewMatrix());
		m_material->SetVariableVector3("gCamPos", cam->GetPosition());
		// animation variables
		if (m_animation && m_animation->Valid() && m_geometry->IsRigged()) {
			WImage* animTex = m_animation->GetTexture();
			m_material->SetVariableInt("gAnimation", 1);
			m_material->SetVariableInt("gAnimationTextureWidth", animTex->GetWidth());
			m_material->SetAnimationTexture(animTex);
		} else
			m_material->SetVariableInt("gAnimation", 0);
		// instancing variables
		if (m_instanceV.size())
			m_material->SetVariableInt("gInstancing", 1);
		else
			m_material->SetVariableInt("gInstancing", 0);

		WError err;
		// bind the pipeline
		err = m_material->Bind(rt);

		// bind the instance buffer, if its not available bind a dummy one.
		VkDeviceSize offsets[] = { 0 };
		if (m_instanceBuf.buf)
			vkCmdBindVertexBuffers(rt->GetCommnadBuffer(), 2, 1, &m_instanceBuf.buf, offsets);
		else
			vkCmdBindVertexBuffers(rt->GetCommnadBuffer(), 2, 1, m_app->ObjectManager->GetDummyBuffer(), offsets);

		// bind the rest of the buffers and draw
		err = m_geometry->Draw(rt, -1, max(m_instanceV.size(), 1));
	}
}

WError WObject::SetGeometry(class WGeometry* geometry) {
	if (m_geometry)
		m_geometry->RemoveReference();

	m_geometry = geometry;
	if (geometry) {
		m_geometry->AddReference();
	}

	return WError(W_SUCCEEDED);
}

WError WObject::SetMaterial(class WMaterial* material) {
	if (m_material)
		m_material->RemoveReference();

	m_material = material;
	if (material) {
		m_material->AddReference();
	}

	return WError(W_SUCCEEDED);
}

WError WObject::SetAnimation(class WAnimation* animation) {
	if (m_animation)
		m_animation->RemoveReference();

	m_animation = animation;
	if (animation) {
		m_animation->AddReference();
	}

	return WError(W_SUCCEEDED);
}

WError WObject::InitInstancing(unsigned int maxInstances) {
	VkDevice device = m_app->GetVulkanDevice();
	VkMemoryAllocateInfo memAlloc = {};
	VkMemoryRequirements memReqs;
	VkBufferCreateInfo instanceBufferInfo = {};
	VkBufferCopy copyRegion = {};
	void *data;
	VkResult err;

	m_instanceBuf.Destroy(device);
	for (int i = 0; i < m_instanceV.size(); i++)
		delete m_instanceV[i];
	m_instanceV.clear();

	int instanceBufferSize = maxInstances * sizeof(WMatrix);

	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	// Instance buffer
	instanceBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	instanceBufferInfo.size = instanceBufferSize;
	instanceBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	err = vkCreateBuffer(device, &instanceBufferInfo, nullptr, &m_instanceBuf.buf);
	if (err)
		goto destroy_resources;
	vkGetBufferMemoryRequirements(device, m_instanceBuf.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &m_instanceBuf.mem);
	if (err)
		goto destroy_resources;
	err = vkBindBufferMemory(device, m_instanceBuf.buf, m_instanceBuf.mem, 0);
	if (err)
		goto destroy_resources;

destroy_resources:
	if (err) {
		m_instanceBuf.Destroy(device);
		return WError(W_OUTOFMEMORY);
	}

	m_maxInstances = 0;
	m_instancesDirty = true;

	return WError(W_SUCCEEDED);
}

WInstance* WObject::CreateInstance() {
	if (m_instanceBuf.buf == VK_NULL_HANDLE)
		return NULL;

	WInstance* inst = new WInstance();
	m_instanceV.push_back(inst);
	m_instancesDirty = true;
	return inst;
}

WInstance* WObject::GetInstance(unsigned int index) const {
	if (index < m_instanceV.size())
		return m_instanceV[index];
	return NULL;
}

void WObject::DeleteInstance(WInstance* instance) {
	for (UINT i = 0; i < m_instanceV.size(); i++) {
		if (m_instanceV[i] == instance) {
			delete m_instanceV[i];
			m_instanceV.erase(m_instanceV.begin() + i);
			m_instancesDirty = true;
		}
	}
}

void WObject::DeleteInstance(unsigned int index) {
	if (index < m_instanceV.size()) {
		delete m_instanceV[index];
		m_instanceV.erase(m_instanceV.begin() + index);
		m_instancesDirty = true;
	}
}

void WObject::_UpdateInstanceBuffer() {
	if (m_instanceBuf.buf && m_instanceV.size()) { //update the instance buffer
		bool bReconstruct = m_instancesDirty;
		for (unsigned int i = 0; i < m_instanceV.size(); i++)
			if (m_instanceV[i]->UpdateLocals())
				bReconstruct = true;
		if (bReconstruct) {
			void* pData;
			VkResult err = vkMapMemory(m_app->GetVulkanDevice(), m_instanceBuf.mem,
									   0, m_maxInstances * sizeof(WMatrix), 0, &pData);
			if (err == VK_SUCCESS) {
				for (unsigned int i = 0; i < m_instanceV.size(); i++)
					memcpy(&((char*)pData)[i * sizeof WMatrix], m_instanceV[i]->m_worldM, sizeof WMatrix);
				vkUnmapMemory(m_app->GetVulkanDevice(), m_instanceBuf.mem);
			}
		}
		m_instancesDirty = false;
	}
}

WGeometry* WObject::GetGeometry() const {
	return m_geometry;
}

WMaterial* WObject::GetMaterial() const {
	return m_material;
}

WAnimation* WObject::GetAnimation() const {
	return m_animation;
}

bool WObject::Hidden() const {
	return m_hidden;
}

void WObject::Show() {
	m_hidden = false;
}

void WObject::Hide() {
	m_hidden = true;
}

void WObject::EnableFrustumCulling() {
	m_bFrustumCull = true;
}

void WObject::DisableFrustumCulling() {
	m_bFrustumCull = false;
}

WVector3 WObject::GetScale() const {
	return WVector3(m_fScaleX, m_fScaleY, m_fScaleZ);
}

float WObject::GetScaleX() const {
	return m_fScaleX;
}

float WObject::GetScaleY() const {
	return m_fScaleY;
}

float WObject::GetScaleZ() const {
	return m_fScaleZ;
}

void WObject::Scale(float x, float y, float z) {
	m_bAltered = true;
	m_fScaleX = x;
	m_fScaleY = y;
	m_fScaleZ = z;
}

void WObject::Scale(WVector3 scale) {
	m_bAltered = true;
	m_fScaleX = scale.x;
	m_fScaleY = scale.y;
	m_fScaleZ = scale.z;
}

void WObject::ScaleX(float scale) {
	m_bAltered = true;
	m_fScaleX = scale;
}

void WObject::ScaleY(float scale) {
	m_bAltered = true;
	m_fScaleY = scale;
}

void WObject::ScaleZ(float scale) {
	m_bAltered = true;
	m_fScaleZ = scale;
}

WMatrix WObject::GetWorldMatrix() {
	UpdateLocals(WVector3(0, 0, 0)); //return matrix without offsets
	return m_WorldM;
}

bool WObject::UpdateLocals(WVector3 offset) {
	if (m_bAltered) {
		m_bAltered = false;
		WVector3 _up = GetUVector();
		WVector3 _look = GetLVector();
		WVector3 _right = GetRVector();
		WVector3 _pos = GetPosition();

		//
		//the world matrix is the view matrix's inverse
		//so we build a normal view matrix and invert it
		//

		//build world matrix
		float x = -WVec3Dot(_right, _pos) - offset.x; //apply offset
		float y = -WVec3Dot(_up, _pos) - offset.y; //apply offset
		float z = -WVec3Dot(_look, _pos) - offset.z; //apply offset
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);

		//scale matrix
		m_WorldM = WScalingMatrix(m_fScaleX / 100.0f, m_fScaleY / 100.0f, m_fScaleZ / 100.0f) * m_WorldM;

		return true;
	}

	return false;
}

void WObject::OnStateChange(STATE_CHANGE_TYPE type) //virtual method of the orientation device
{
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

