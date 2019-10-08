#include "Wasabi/Particles/WParticles.h"
#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Geometries/WGeometry.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Renderers/WRenderer.h"

class WParticlesGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({
		W_ATTRIBUTE_POSITION,
		W_VERTEX_ATTRIBUTE("particleUV", 2),
		W_VERTEX_ATTRIBUTE("particleColor", 4),
	});

public:
	WParticlesGeometry(Wasabi* const app, uint32_t ID = 0) : WGeometry(app, ID) {}

	virtual uint32_t GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(uint32_t layoutIndex = 0) const {
		UNREFERENCED_PARAMETER(layoutIndex);
		return m_desc;
	}
	virtual size_t GetVertexDescriptionSize(uint32_t layoutIndex = 0) const {
		UNREFERENCED_PARAMETER(layoutIndex);
		return m_desc.GetSize();
	}
};

class ParticlesVS : public WShader {
public:
	ParticlesVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, "uboPerParticles", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			}),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4), // color
		})};
		vector<uint8_t> code {
			#include "Shaders/particles.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

class ParticlesPS : public WShader {
public:
	ParticlesPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, "diffuseTexture"),
		};
		vector<uint8_t> code {
			#include "Shaders/particles.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

WParticlesBehavior::WParticlesBehavior(uint32_t maxParticles, uint32_t particleSize) {
	m_maxParticles = maxParticles;
	m_particleSize = particleSize;
	m_numParticles = 0;
	m_particlesData = (void*)new char[m_maxParticles*m_particleSize];
}

WParticlesBehavior::~WParticlesBehavior() {
	delete[] m_particlesData;
}

void WParticlesBehavior::Emit(void* particle) {
	if (m_numParticles < m_maxParticles) {
		memcpy((char*)m_particlesData + (m_numParticles*m_particleSize), particle, m_particleSize);
		m_numParticles++;
	}
}

uint32_t WParticlesBehavior::UpdateAndCopyToVB(float curTime, void* vb, uint32_t maxParticles, const WMatrix& worldMatrix, const WMatrix& viewMatrix) {
	UNREFERENCED_PARAMETER(maxParticles);

	UpdateSystem(curTime, worldMatrix, viewMatrix);

	for (uint32_t i = 0; i < m_numParticles; i++) {
		void* curParticleData = (char*)m_particlesData + (m_particleSize * i);
		WParticlesVertex* verticesStart = reinterpret_cast<WParticlesVertex*>(vb) + (i * 4);
		if (!UpdateParticleVertices(curTime, curParticleData, verticesStart, worldMatrix, viewMatrix)) {
			if (m_numParticles > 1) {
				// put the last particle in this particle's place, then decrementing m_numParticles effectively removes this particle
				memcpy(curParticleData, (char*)m_particlesData + (m_particleSize * (m_numParticles - 1)), m_particleSize);
				memcpy(verticesStart, reinterpret_cast<WParticlesVertex*>(vb) + ((m_numParticles - 1) * 4), sizeof(WParticlesVertex) * 4);
			}
			m_numParticles--;
			i--;
		}
	}

	return m_numParticles;
}

WDefaultParticleBehavior::WDefaultParticleBehavior(uint32_t maxParticles)
	: WParticlesBehavior(maxParticles, sizeof(WDefaultParticleBehavior::Particle)) {
	m_lastEmit = 0.0f;
	m_emissionPosition = WVector3(0.0f, 0.0f, 0.0f);
	m_emissionRandomness = WVector3(1.0f, 1.0f, 1.0f);
	m_particleLife = 1.5f;
	m_particleSpawnVelocity = WVector3(0.0f, 4.0f, 0.0f);
	m_moveOutwards = false;
	m_emissionFrequency = 30.0f;
	m_emissionSize = 0.3f;
	m_deathSize = 1.5f;
	m_type = BILLBOARD;
	m_numTilesColumns = 1;
	m_numTiles = 1;
	m_colorGradient = {
		std::make_pair(WColor(1.0f, 1.0f, 1.0f, 0.0f), 0.2f),
		std::make_pair(WColor(1.0f, 1.0f, 1.0f, 1.0f), 0.8f),
		std::make_pair(WColor(1.0f, 1.0f, 1.0f, 0.0f), 0.0f)
	};
}

void WDefaultParticleBehavior::UpdateSystem(float curTime, const WMatrix& worldMatrix, const WMatrix& viewMatrix) {
	UNREFERENCED_PARAMETER(worldMatrix);
	UNREFERENCED_PARAMETER(viewMatrix);

	uint32_t numTilesRows = (uint)(ceilf((float)m_numTiles / (float)m_numTilesColumns) + 0.01f);
	int num_emitted = 0;
	while (curTime > m_lastEmit + 1.0f / m_emissionFrequency && num_emitted++ < 10) {
		m_lastEmit += 1.0f / m_emissionFrequency;
		Particle p = {};
		p.initialPos = m_emissionPosition + m_emissionRandomness * WVector3(WUtil::frand_0_1() - 0.5f, WUtil::frand_0_1() - 0.5f, WUtil::frand_0_1() - 0.5f);
		p.velocity = m_particleSpawnVelocity;
		p.spawnTime = m_lastEmit;
		if (m_moveOutwards)
			p.velocity = WVec3Normalize(p.initialPos - m_emissionPosition) * WVec3Length(m_particleSpawnVelocity);

		// calculate tiling
		uint32_t x = rand() % m_numTilesColumns;
		uint32_t y = rand() % numTilesRows;
		p.UVTopLeft = WVector2((float)x / m_numTilesColumns, (float)y / m_numTilesColumns);
		p.UVBottomRight = WVector2((float)(x+1) / m_numTilesColumns, (float)(y + 1) / m_numTilesColumns);
		Emit((void*)&p);
	}
}

inline bool WDefaultParticleBehavior::UpdateParticleVertices(float curTime, void* particle, WParticlesVertex* vertices, const WMatrix& worldMatrix, const WMatrix& viewMatrix) {
	Particle* p = (Particle*)particle;
	float lifePercentage = (curTime - p->spawnTime) / m_particleLife;
	if (lifePercentage >= 1.0f)
		return false;

	float size = WUtil::flerp(m_emissionSize, m_deathSize, lifePercentage);
	float sizeBillboard = m_type == WDefaultParticleBehavior::Type::BILLBOARD ? size : 0.0f;
	float sizeNova = m_type == WDefaultParticleBehavior::Type::NOVA ? size : 0.0f;
	WVector3 particleSize = m_type == BILLBOARD ? WVector3(0, size, 0) : WVector3(size, 0, 0);
	WVector3 particlePosition = p->initialPos + lifePercentage * m_particleLife * p->velocity;
	WColor particleColor;
	float curColorPos = 0.0f;
	for (uint32_t i = 0; i < m_colorGradient.size() - 1; i++) {
		if (lifePercentage < curColorPos + m_colorGradient[i].second) {
			particleColor = WColorLerp(m_colorGradient[i].first, m_colorGradient[i + 1].first, (lifePercentage - curColorPos) / m_colorGradient[i].second);
			break;
		}
		curColorPos += m_colorGradient[i].second;
	}

	/**
	 * Vertices are arranged as follows:
	 * v0-----v1
	 * |       |
	 * |       |
	 * v3-----v2
	 */
	for (uint8_t i = 0; i < 4; i++) {
		// set output vertex UV
		vertices[i].UV = WVector2(
			i / 2 == 0 ? p->UVTopLeft.x : p->UVBottomRight.x,
			i % 2 == 0 ? p->UVTopLeft.y : p->UVBottomRight.y
		);
		// set output vertex color
		memcpy(&(vertices[i].color), &particleColor, sizeof(WColor));
		// set output vertex position
		WVector3 localPos = particlePosition + WVector3(i / 2 == 0 ? -sizeNova : sizeNova, 0.0f, i % 2 == 0 ? sizeNova : -sizeNova);
		WVector3 worldPos = WVec3TransformCoord(localPos, worldMatrix);
		vertices[i].viewPos = WVec3TransformCoord(worldPos, viewMatrix);
		vertices[i].viewPos += WVector3(i / 2 == 0 ? -sizeBillboard : sizeBillboard, i % 2 == 0 ? sizeBillboard : -sizeBillboard, 0.0f);
	}

	return true;
}

WParticlesManager::WParticlesManager(class Wasabi* const app)
	: WManager<WParticles>(app) {
	m_vertexShader = nullptr;
	m_fragmentShader = nullptr;
}

WParticlesManager::~WParticlesManager() {
	W_SAFE_REMOVEREF(m_vertexShader);
	W_SAFE_REMOVEREF(m_fragmentShader);
}

std::string WParticlesManager::GetTypeName() const {
	return "Particles";
}

WError WParticlesManager::Load() {
	m_vertexShader = new ParticlesVS(m_app);
	m_vertexShader->SetName("DefaultParticlesVS");
	m_app->FileManager->AddDefaultAsset(m_vertexShader->GetName(), m_vertexShader);
	m_vertexShader->Load();
	m_fragmentShader = new ParticlesPS(m_app);
	m_fragmentShader->SetName("DefaultParticlesPS");
	m_app->FileManager->AddDefaultAsset(m_fragmentShader->GetName(), m_fragmentShader);
	m_fragmentShader->Load();

	return WError(W_SUCCEEDED);
}

class WEffect* WParticlesManager::CreateParticlesEffect(W_DEFAULT_PARTICLE_EFFECT_TYPE type) const {
	VkPipelineColorBlendAttachmentState blendState = {};
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};

	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = depthStencilState.back;

	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	blendState.colorWriteMask = 0xff;
	blendState.blendEnable = VK_TRUE;
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	switch (type) {
	case W_DEFAULT_PARTICLES_ADDITIVE:
		blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.colorBlendOp = VK_BLEND_OP_ADD;
		blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case W_DEFAULT_PARTICLES_ALPHA:
		blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendState.colorBlendOp = VK_BLEND_OP_ADD;
		blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case W_DEFAULT_PARTICLES_SUBTRACTIVE:
		blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
		blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	}

	WEffect* fx = new WEffect(m_app);

	WError werr = fx->BindShader(m_vertexShader);
	if (werr) {
		werr = fx->BindShader(m_fragmentShader);
		if (werr) {
			fx->SetBlendingState(blendState);
			fx->SetDepthStencilState(depthStencilState);
			fx->SetRasterizationState(rasterizationState);
			fx->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
			werr = fx->BuildPipeline(m_app->Renderer->GetRenderTarget(m_app->Renderer->GetParticlesRenderStageName()));
		}
	}

	if (!werr)
		W_SAFE_REMOVEREF(fx);

	return fx;
}

WParticles* WParticlesManager::CreateParticles(W_DEFAULT_PARTICLE_EFFECT_TYPE type, uint32_t maxParticles, WParticlesBehavior* behavior, uint32_t ID) const {
	WParticles* particles = new WParticles(m_app, type, ID);
	WError werr = particles->Create(maxParticles, behavior);
	if (!werr)
		W_SAFE_REMOVEREF(particles);
	return particles;
}

WParticles::WParticles(class Wasabi* const app, W_DEFAULT_PARTICLE_EFFECT_TYPE type, uint32_t ID) : WBase(app, ID) {
	m_type = type;
	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;
	m_priority = 0;

	m_WorldM = WMatrix();

	m_behavior = nullptr;
	m_geometry = nullptr;

	app->ParticlesManager->AddEntity(this);
}

WParticles::~WParticles() {
	_DestroyResources();

	m_app->ParticlesManager->RemoveEntity(this);
}

std::string WParticles::_GetTypeName() {
	return "Particles";
}

std::string WParticles::GetTypeName() const {
	return _GetTypeName();
}

void WParticles::_DestroyResources() {
	W_SAFE_DELETE(m_behavior);
	W_SAFE_REMOVEREF(m_geometry);
}

WParticlesBehavior* WParticles::GetBehavior() const {
	return m_behavior;
}

bool WParticles::Valid() const {
	return m_behavior && m_geometry;
}

W_DEFAULT_PARTICLE_EFFECT_TYPE WParticles::GetType() const {
	return m_type;
}

void WParticles::Show() {
	m_hidden = false;
}

void WParticles::Hide() {
	m_hidden = true;
}

bool WParticles::Hidden() const {
	return m_hidden;
}

void WParticles::EnableFrustumCulling() {
	m_bFrustumCull = true;
}

void WParticles::DisableFrustumCulling() {
	m_bFrustumCull = false;
}

bool WParticles::InCameraView(class WCamera* cam) {
	WMatrix worldM = GetWorldMatrix();
	WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), worldM) + WVector3(2, 2, 2); // @TODO: use particle size instead
	WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), worldM) - WVector3(2, 2, 2); // @TODO: use particle size instead
	WVector3 pos = (max + min) / 2.0f;
	WVector3 size = (max - min) / 2.0f;
	return cam->CheckBoxInFrustum(pos, size);
}

WError WParticles::Create(uint32_t maxParticles, WParticlesBehavior* behavior) {
	_DestroyResources();
	if (maxParticles == 0)
		return WError(W_INVALIDPARAM);

	m_geometry = new WParticlesGeometry(m_app);
	WError err = m_geometry->CreateFromData(nullptr, maxParticles * 4, nullptr, 0, W_GEOMETRY_CREATE_VB_DYNAMIC | W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME);
	if (err != W_SUCCEEDED) {
		_DestroyResources();
		return err;
	}

	m_behavior = behavior ? behavior : new WDefaultParticleBehavior(maxParticles);
	m_maxParticles = maxParticles;

	return WError(W_SUCCEEDED);
}

bool WParticles::WillRender(WRenderTarget* rt) {
	if (Valid() && !m_hidden)
		return !m_bFrustumCull || InCameraView(rt->GetCamera());
	return false;
}

void WParticles::Render(WRenderTarget* const rt, WMaterial* material) {
	WCamera* cam = rt->GetCamera();
	WMatrix viewMatrix = cam->GetViewMatrix();
	if (material) {
		material->SetVariable<WMatrix>("worldMatrix", GetWorldMatrix());
		material->SetVariable<WMatrix>("viewMatrix", viewMatrix);
		material->SetVariable<WMatrix>("projectionMatrix", cam->GetProjectionMatrix());
		material->Bind(rt);
	}

	// update the geometry
	WParticlesVertex* vb;
	m_geometry->MapVertexBuffer((void**)& vb, W_MAP_WRITE);
	float curTime = m_app->Timer.GetElapsedTime();
	uint32_t numParticles = m_behavior->UpdateAndCopyToVB(curTime, vb, m_maxParticles, m_WorldM, viewMatrix);
	m_geometry->UnmapVertexBuffer(false);

	if (numParticles > 0)
		m_geometry->Draw(rt, numParticles * 4, 1, false);
}

void WParticles::SetPriority(uint32_t priority) {
	m_priority = priority;
}

uint32_t WParticles::GetPriority() const {
	return m_priority;
}

WMatrix WParticles::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WParticles::UpdateLocals() {
	if (m_bAltered) {
		m_bAltered = false;
		m_WorldM = ComputeTransformation();

		return true;
	}

	return false;
}

void WParticles::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}
