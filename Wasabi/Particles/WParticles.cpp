#include "WParticles.h"
#include "../Cameras/WCamera.h"
#include "../Images/WRenderTarget.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Renderers/WRenderer.h"

class WParticlesGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({
		W_ATTRIBUTE_POSITION,
		W_VERTEX_ATTRIBUTE("particleSize", 3),
		W_VERTEX_ATTRIBUTE("particleAlpha", 1),
	});

public:
	WParticlesGeometry(Wasabi* const app, unsigned int ID = 0) : WGeometry(app, ID) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int layout_index = 0) const {
		return m_desc;
	}
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const {
		return m_desc.GetSize();
	}
};

class ParticlesVS : public WShader {
public:
	ParticlesVS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, "uboPerParticles", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			}),
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // size
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT), // alpha
		})};
		vector<byte> code = {
			#include "Shaders/particles.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

class ParticlesGS : public WShader {
public:
	ParticlesGS(class Wasabi* const app) : WShader(app) {}

	static vector<W_BOUND_RESOURCE> GetBoundResources() {
		return {
			ParticlesVS::GetBoundResources()[0],
		};
	}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_GEOMETRY_SHADER;
		m_desc.bound_resources = GetBoundResources();
		vector<byte> code = {
			#include "Shaders/particles.geom.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
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
		vector<byte> code = {
			#include "Shaders/particles.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), code.size(), bSaveData);
	}
};

void WParticlesBehavior::Emit(void* particle) {
	if (m_numParticles < m_maxParticles) {
		memcpy((char*)m_buffer + (m_numParticles*m_particleSize), particle, m_particleSize);
		m_numParticles++;
	}
}

WParticlesBehavior::WParticlesBehavior(unsigned int max_particles, unsigned int particle_size) {
	m_maxParticles = max_particles;
	m_particleSize = particle_size;
	m_numParticles = 0;
	m_buffer = (void*)new char[max_particles*particle_size];
}

WParticlesBehavior::~WParticlesBehavior() {
	delete[] m_buffer;
}

unsigned int WParticlesBehavior::UpdateAndCopyToVB(float cur_time, void* vb, unsigned int max_particles) {
	UpdateSystem(cur_time);

	unsigned int cur_offset_in_vb = 0;
	for (unsigned int i = 0; i < m_numParticles; i++) {
		void* cur_particle = (char*)m_buffer + (m_particleSize*i);
		if (!UpdateParticle(cur_time, cur_particle)) {
			memcpy(cur_particle, (char*)m_buffer + (m_particleSize*(m_numParticles-1)), m_particleSize);
			m_numParticles--;
			i--;
		} else {
			memcpy((char*)vb + cur_offset_in_vb, cur_particle, sizeof(WParticlesVertex));
			cur_offset_in_vb += sizeof(WParticlesVertex);
		}
	}

	return cur_offset_in_vb / sizeof(WParticlesVertex);
}

WDefaultParticleBehavior::WDefaultParticleBehavior(unsigned int max_particles)
	: WParticlesBehavior(max_particles, sizeof(WDefaultParticleBehavior::Particle)) {
	m_lastEmit = 0;
	m_emissionPosition = WVector3(0, 0, 0);
	m_emissionRandomness = WVector3(1, 1, 1);
	m_particleLife = 3;
	m_particleSpawnVelocity = WVector3(0, 2, 0);
	m_emissionFrequency = 20;
	m_emissionSize = 1;
	m_deathSize = 3;
	m_type = BILLBOARD;
}

void WDefaultParticleBehavior::UpdateSystem(float cur_time) {
	int num_emitted = 0;
	while (cur_time > m_lastEmit + 1.0f / m_emissionFrequency && num_emitted++ < 10) {
		m_lastEmit += 1.0f / m_emissionFrequency;
		Particle p = {};
		p.initialPos = m_emissionPosition + m_emissionRandomness * WVector3(WUtil::frand_0_1() - 0.5f, WUtil::frand_0_1() - 0.5f, WUtil::frand_0_1() - 0.5f);
		p.velocity = m_particleSpawnVelocity;
		p.spawnTime = m_lastEmit;
		Emit((void*)&p);
	}
}

inline bool WDefaultParticleBehavior::UpdateParticle(float cur_time, void* particle) {
	Particle* p = (Particle*)particle;
	float lifePercentage = (cur_time - p->spawnTime) / m_particleLife;
	float size = WUtil::flerp(m_emissionSize, m_deathSize, lifePercentage);

	p->vtx.pos = p->initialPos + lifePercentage * m_particleLife * p->velocity;
	p->vtx.size = m_type == BILLBOARD ? WVector3(0, size, 0) : WVector3(size, 0, 0);
	p->vtx.alpha = fmin(fmax(0.8f - fabs(lifePercentage * 2.0f - 1.0f), 0.0f), 1.0f);
	return lifePercentage <= 1.0f;
}

WParticlesManager::WParticlesManager(class Wasabi* const app)
	: WManager<WParticles>(app) {
	m_vertexShader = nullptr;
	m_geometryShader = nullptr;
	m_fragmentShader = nullptr;
}

WParticlesManager::~WParticlesManager() {
	W_SAFE_REMOVEREF(m_vertexShader);
	W_SAFE_REMOVEREF(m_geometryShader);
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
	m_geometryShader = new ParticlesGS(m_app);
	m_geometryShader->SetName("DefaultParticlesGS");
	m_app->FileManager->AddDefaultAsset(m_geometryShader->GetName(), m_geometryShader);
	m_geometryShader->Load();
	m_fragmentShader = new ParticlesPS(m_app);
	m_fragmentShader->SetName("DefaultParticlesPS");
	m_app->FileManager->AddDefaultAsset(m_fragmentShader->GetName(), m_fragmentShader);
	m_fragmentShader->Load();

	return WError(W_SUCCEEDED);
}

class WEffect* WParticlesManager::CreateParticlesEffect(W_DEFAULT_PARTICLE_EFFECT_TYPE type) const {
	VkPipelineColorBlendAttachmentState blend_state = {};
	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};

	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.depthTestEnable = VK_TRUE;
	depth_stencil_state.depthWriteEnable = VK_FALSE;
	depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
	depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
	depth_stencil_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_stencil_state.stencilTestEnable = VK_FALSE;
	depth_stencil_state.front = depth_stencil_state.back;

	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state.cullMode = VK_CULL_MODE_NONE;
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.depthBiasEnable = VK_FALSE;
	rasterization_state.lineWidth = 1.0f;

	blend_state.colorWriteMask = 0xff;
	blend_state.blendEnable = VK_TRUE;
	blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	switch (type) {
	case W_DEFAULT_PARTICLES_ADDITIVE:
		blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.colorBlendOp = VK_BLEND_OP_ADD;
		blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case W_DEFAULT_PARTICLES_ALPHA:
		blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blend_state.colorBlendOp = VK_BLEND_OP_ADD;
		blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	case W_DEFAULT_PARTICLES_SUBTRACTIVE:
		blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
		blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
	}

	WEffect* fx = new WEffect(m_app);

	WError werr = fx->BindShader(m_vertexShader);
	if (werr) {
		werr = fx->BindShader(m_geometryShader);
		if (werr) {
			werr = fx->BindShader(m_fragmentShader);
			if (werr) {
				fx->SetBlendingState(blend_state);
				fx->SetDepthStencilState(depth_stencil_state);
				fx->SetRasterizationState(rasterization_state);
				fx->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
				werr = fx->BuildPipeline(m_app->Renderer->GetRenderTarget(m_app->Renderer->GetParticlesRenderStageName()));
			}
		}
	}

	if (!werr)
		W_SAFE_REMOVEREF(fx);

	return fx;
}

WParticles* WParticlesManager::CreateParticles(W_DEFAULT_PARTICLE_EFFECT_TYPE type, unsigned int maxParticles, WParticlesBehavior* behavior, unsigned int ID) const {
	WParticles* particles = new WParticles(m_app, type, ID);
	WError werr = particles->Create(maxParticles, behavior);
	if (!werr)
		W_SAFE_REMOVEREF(particles);
	return particles;
}

WParticles::WParticles(class Wasabi* const app, W_DEFAULT_PARTICLE_EFFECT_TYPE type, unsigned int ID) : WBase(app, ID) {
	m_type = type;
	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();

	m_behavior = nullptr;
	m_geometry = nullptr;

	app->ParticlesManager->AddEntity(this);
}

WParticles::~WParticles() {
	_DestroyResources();

	ClearEffects();

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

WError WParticles::Create(unsigned int maxParticles, WParticlesBehavior* behavior) {
	_DestroyResources();
	if (maxParticles == 0)
		return WError(W_INVALIDPARAM);

	m_geometry = new WParticlesGeometry(m_app);
	WError err = m_geometry->CreateFromData(nullptr, maxParticles, nullptr, 0, W_GEOMETRY_CREATE_VB_DYNAMIC | W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME);
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
	if (material) {
		WCamera* cam = rt->GetCamera();
		material->SetVariableMatrix("worldMatrix", GetWorldMatrix());
		material->SetVariableMatrix("viewMatrix", cam->GetViewMatrix());
		material->SetVariableMatrix("projectionMatrix", cam->GetProjectionMatrix());
		material->Bind(rt);
	}

	// update the geometry
	WParticlesVertex* vb;
	m_geometry->MapVertexBuffer((void**)&vb, W_MAP_WRITE);
	float cur_time = m_app->Timer.GetElapsedTime();
	unsigned int num_particles = m_behavior->UpdateAndCopyToVB(cur_time, vb, m_maxParticles);
	m_geometry->UnmapVertexBuffer(false);

	m_geometry->Draw(rt, num_particles, 1, false);
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

