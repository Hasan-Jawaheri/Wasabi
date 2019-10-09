#include "Wasabi/Particles/WParticles.h"
#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Geometries/WGeometry.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WImage.h"

class PositionOnlyGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({
		W_ATTRIBUTE_POSITION,
	});

public:
	PositionOnlyGeometry(Wasabi* const app, uint32_t ID = 0) : WGeometry(app, ID) {}

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
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projection"),
				W_SHADER_VARIABLE_INFO(W_TYPE_UINT, "instancingTextureWidth"), // width of the instancing texture
			}),
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 1, "instancingTexture"),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // local position
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
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 2, "diffuseTexture"),
		};
		vector<uint8_t> code {
			#include "Shaders/particles.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

inline void WParticlesInstance::SetParameters(const WMatrix& WVP, WColor color, WVector2 uvTopLeft, WVector2 uvBottomRight) {
	mat1 = WVector4(WVP(0, 0), WVP(0, 1), WVP(0, 2), WVP(3, 0));
	mat2 = WVector4(WVP(1, 0), WVP(1, 1), WVP(1, 2), WVP(3, 1));
	mat3 = WVector4(WVP(2, 0), WVP(2, 1), WVP(2, 2), WVP(3, 2));
	colorAndUVs = WVector4(color.r, color.g, color.b, color.a) * 255.1f;
	colorAndUVs.x = std::floor(colorAndUVs.x) + uvTopLeft.x * 0.98f + 0.01f;
	colorAndUVs.y = std::floor(colorAndUVs.y) + uvTopLeft.y * 0.98f + 0.01f;
	colorAndUVs.z = std::floor(colorAndUVs.z) + uvBottomRight.x * 0.98f + 0.01f;
	colorAndUVs.w = std::floor(colorAndUVs.w) + uvBottomRight.y * 0.98f + 0.01f;
}

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

uint32_t WParticlesBehavior::UpdateAndCopyToBuffer(float curTime, void* buffer, uint32_t maxParticles, const WMatrix& worldMatrix, WCamera* camera) {
	UNREFERENCED_PARAMETER(maxParticles);

	UpdateSystem(curTime, worldMatrix, camera);

	for (uint32_t i = 0; i < m_numParticles; i++) {
		void* curParticleData = (char*)m_particlesData + (m_particleSize * i);
		WParticlesInstance* outputStart = (reinterpret_cast<WParticlesInstance*>(buffer)) + i;
		if (!UpdateParticle(curTime, curParticleData, outputStart, worldMatrix, camera)) {
			if (m_numParticles > 1) {
				// put the last particle in this particle's place, then decrementing m_numParticles effectively removes this particle
				memcpy(curParticleData, (char*)m_particlesData + (m_particleSize * (m_numParticles - 1)), m_particleSize);
				memcpy(outputStart, reinterpret_cast<WParticlesInstance*>(buffer) + (m_numParticles - 1), sizeof(WParticlesInstance));
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
	// initially infinite bounding box
	m_minPoint = WVector3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	m_maxPoint = WVector3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
}

void WDefaultParticleBehavior::UpdateSystem(float curTime, const WMatrix& worldMatrix, WCamera* camera) {
	UNREFERENCED_PARAMETER(worldMatrix);
	UNREFERENCED_PARAMETER(camera);

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

inline bool WDefaultParticleBehavior::UpdateParticle(float curTime, void* particleData, WParticlesInstance* outputInstance, const WMatrix& worldMatrix, WCamera* camera) {
	WDefaultParticleBehavior::Particle* p = (WDefaultParticleBehavior::Particle*)particleData;
	float lifePercentage = (curTime - p->spawnTime) / m_particleLife;
	if (lifePercentage >= 1.0f)
		return false;

	float size = WUtil::flerp(m_emissionSize, m_deathSize, lifePercentage);
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

	/*
	 * The geometry is a nova by default (a plain facing up, pointing to Z+).
	 * To make it a billboard
	 */
	WMatrix view = camera->GetViewMatrix();
	WMatrix transformation;
	transformation = WScalingMatrix(WVector3(size, size, size)) * WTranslationMatrix(particlePosition) * worldMatrix;
	// if (m_type == WDefaultParticleBehavior::Type::BILLBOARD)
	// 	transformation *=
	transformation = transformation * view;
	outputInstance->SetParameters(transformation, particleColor, p->UVTopLeft, p->UVBottomRight);

	// for (uint8_t i = 0; i < 4; i++) {
	// 	// set output vertex UV
	// 	vertices[i].UV = WVector2(
	// 		i / 2 == 0 ? p->UVTopLeft.x : p->UVBottomRight.x,
	// 		i % 2 == 0 ? p->UVTopLeft.y : p->UVBottomRight.y
	// 	);
	// 	// set output vertex color
	// 	memcpy(&(vertices[i].color), &particleColor, sizeof(WColor));
	// 	// set output vertex position
	// 	WVector3 localPos = particlePosition + WVector3(i / 2 == 0 ? -sizeNova : sizeNova, 0.0f, i % 2 == 0 ? sizeNova : -sizeNova);
	// 	WVector3 worldPos = WVec3TransformCoord(localPos, worldMatrix);
	// 	vertices[i].viewPos = WVec3TransformCoord(worldPos, viewMatrix);
	// 	vertices[i].viewPos += WVector3(i / 2 == 0 ? -sizeBillboard : sizeBillboard, i % 2 == 0 ? sizeBillboard : -sizeBillboard, 0.0f);
	// }

	return true;
}

inline WVector3& WDefaultParticleBehavior::GetMinPoint() {
	return m_minPoint;
}

inline WVector3& WDefaultParticleBehavior::GetMaxPoint() {
	return m_maxPoint;
}

WParticlesManager::WParticlesManager(class Wasabi* const app)
	: WManager<WParticles>(app) {
	m_vertexShader = nullptr;
	m_fragmentShader = nullptr;
	m_plainGeometry = nullptr;
}

WParticlesManager::~WParticlesManager() {
	W_SAFE_REMOVEREF(m_vertexShader);
	W_SAFE_REMOVEREF(m_fragmentShader);
	W_SAFE_REMOVEREF(m_plainGeometry);
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

	m_plainGeometry = new PositionOnlyGeometry(m_app);
	WParticlesVertex vertices[4];
	vertices[0].pos = WVector3(-1.0f, 0.0f,  1.0f) * 0.5f;
	vertices[1].pos = WVector3( 1.0f, 0.0f,  1.0f) * 0.5f;
	vertices[2].pos = WVector3(-1.0f, 0.0f, -1.0f) * 0.5f;
	vertices[3].pos = WVector3( 1.0f, 0.0f, -1.0f) * 0.5f;
	WError err = m_plainGeometry->CreateFromData(static_cast<void*>(vertices), 4, nullptr, 0, W_GEOMETRY_CREATE_STATIC);
	if (err != W_SUCCEEDED) {
		return err;
	}

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
	m_effectType = type;
	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;
	m_priority = 0;

	m_WorldM = WMatrix();

	m_behavior = nullptr;
	m_instancesTexture = nullptr;

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
	W_SAFE_REMOVEREF(m_instancesTexture);
}

WParticlesBehavior* WParticles::GetBehavior() const {
	return m_behavior;
}

bool WParticles::Valid() const {
	return m_behavior && m_instancesTexture;
}

W_DEFAULT_PARTICLE_EFFECT_TYPE WParticles::GetEffectType() const {
	return m_effectType;
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
	// @TODO: this is not a good check
	WVector3 min = WVec3TransformCoord(m_behavior->GetMinPoint(), m_WorldM);
	WVector3 max = WVec3TransformCoord(m_behavior->GetMaxPoint(), m_WorldM);
	WVector3 pos = (max + min) / 2.0f;
	WVector3 size = (max - min) / 2.0f;
	return cam->CheckBoxInFrustum(pos, size);
}

WError WParticles::Create(uint32_t maxParticles, WParticlesBehavior* behavior) {
	_DestroyResources();
	if (maxParticles == 0)
		return WError(W_INVALIDPARAM);

	uint32_t numRequiredPixels = maxParticles * sizeof(WParticlesInstance) / (4*4);
	uint32_t imgSize = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<double>(numRequiredPixels)))+0.01); // find the first power of 2 greater than the square of required pixels
	imgSize--;
    imgSize |= imgSize >> 1;
    imgSize |= imgSize >> 2;
    imgSize |= imgSize >> 4;
    imgSize |= imgSize >> 8;
    imgSize |= imgSize >> 16;
    imgSize++;
	m_instancesTexture = m_app->ImageManager->CreateImage(nullptr, imgSize, imgSize, VK_FORMAT_R32G32B32A32_SFLOAT, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_REWRITE_EVERY_FRAME);
	if (!m_instancesTexture) {
		_DestroyResources();
		return WError(W_OUTOFMEMORY);
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
	if (material) {
		material->SetTexture("instancingTexture", m_instancesTexture);
		material->SetVariable<uint32_t>("instancingTextureWidth", m_instancesTexture->GetWidth());
		material->SetVariable<WMatrix>("projection", rt->GetCamera()->GetProjectionMatrix());
		material->Bind(rt);
	}

	// update the geometry
	void* instances;
	m_instancesTexture->MapPixels(&instances, W_MAP_WRITE);
	float curTime = m_app->Timer.GetElapsedTime();
	uint32_t numParticles = m_behavior->UpdateAndCopyToBuffer(curTime, instances, m_maxParticles, m_WorldM, rt->GetCamera());
	m_instancesTexture->UnmapPixels();

	if (numParticles > 0)
		m_app->ParticlesManager->m_plainGeometry->Draw(rt, UINT32_MAX, numParticles, false);
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
