#include "WParticles.h"
#include "../Cameras/WCamera.h"
#include "../Images/WRenderTarget.h"
#include "../Geometries/WGeometry.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Renderers/WRenderer.h"

class WParticlesGeometry : public WGeometry {
	const W_VERTEX_DESCRIPTION m_desc = W_VERTEX_DESCRIPTION({ W_ATTRIBUTE_POSITION });

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
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "wvp"), // world * view * projection
			}),
		};
	}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = GetBoundResources();
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
		}) };
		LoadCodeGLSL(
			#include "Shaders/particles_vs.glsl"
		);
	}
};

class ParticlesPS : public WShader {
public:
	ParticlesPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
		};
		LoadCodeGLSL(
			#include "Shaders/particles_ps.glsl"
		);
	}
};

WParticlesManager::WParticlesManager(class Wasabi* const app) : WManager<WParticles>(app), m_blendState({}), m_rasterizationState({}), m_depthStencilState({}) {
	m_particlesEffect = nullptr;
}

WParticlesManager::~WParticlesManager() {
	W_SAFE_REMOVEREF(m_particlesEffect);
}

void WParticlesManager::Render(WRenderTarget* rt) {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < m_entities[i].size(); j++) {
			m_entities[i][j]->Render(rt);
		}
	}
}

std::string WParticlesManager::GetTypeName() const {
	return "Particles";
}

WError WParticlesManager::Load() {
	m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilState.depthTestEnable = VK_TRUE;
	m_depthStencilState.depthWriteEnable = VK_FALSE;
	m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
	m_depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	m_depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	m_depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	m_depthStencilState.stencilTestEnable = VK_FALSE;
	m_depthStencilState.front = m_depthStencilState.back;

	m_blendState.colorWriteMask = 0xff;
	m_blendState.blendEnable = VK_TRUE;
	m_blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.colorBlendOp = VK_BLEND_OP_ADD;
	m_blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_blendState.alphaBlendOp = VK_BLEND_OP_ADD;

	m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizationState.polygonMode = VK_POLYGON_MODE_POINT;
	m_rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	m_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizationState.depthClampEnable = VK_FALSE;
	m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationState.depthBiasEnable = VK_FALSE;
	m_rasterizationState.lineWidth = 1.0f;

	WShader* vertex_shader = new ParticlesVS(m_app);
	vertex_shader->Load();
	WShader* pixel_shader = new ParticlesPS(m_app);
	pixel_shader->Load();
	m_particlesEffect = new WEffect(m_app);
	WError werr = m_particlesEffect->BindShader(vertex_shader);
	if (!werr)
		goto error;

	werr = m_particlesEffect->BindShader(pixel_shader);
	if (!werr)
		goto error;

	m_particlesEffect->SetBlendingState(m_blendState);
	m_particlesEffect->SetDepthStencilState(m_depthStencilState);
	m_particlesEffect->SetRasterizationState(m_rasterizationState);
	m_particlesEffect->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

	werr = m_particlesEffect->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());
	if (!werr)
		goto error;

error:
	W_SAFE_REMOVEREF(vertex_shader);
	W_SAFE_REMOVEREF(pixel_shader);

	return WError(W_SUCCEEDED);
}

class WEffect* WParticlesManager::GetDefaultEffect() const {
	return m_particlesEffect;
}


WParticles::WParticles(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;
	m_bAltered = true;
	m_bFrustumCull = true;

	m_WorldM = WMatrix();

	m_geometry = nullptr;
	m_material = nullptr;

	app->ParticlesManager->AddEntity(this);
}

WParticles::~WParticles() {
	_DestroyResources();

	m_app->ParticlesManager->RemoveEntity(this);
}

std::string WParticles::GetTypeName() const {
	return "Particles";
}

void WParticles::_DestroyResources() {
	W_SAFE_REMOVEREF(m_geometry);
	W_SAFE_REMOVEREF(m_material);
}

bool WParticles::Valid() const {
	return m_geometry && m_material;
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

bool WParticles::InCameraView(class WCamera* cam) {
	WMatrix worldM = GetWorldMatrix();
	WVector3 min = WVec3TransformCoord(m_geometry->GetMinPoint(), worldM) + WVector3(2, 2, 2); // @TODO: use particle size instead
	WVector3 max = WVec3TransformCoord(m_geometry->GetMaxPoint(), worldM) - WVector3(2, 2, 2); // @TODO: use particle size instead
	WVector3 pos = (max + min) / 2.0f;
	WVector3 size = (max - min) / 2.0f;
	return cam->CheckBoxInFrustum(pos, size);
}

WError WParticles::Create(unsigned int max_particles) {
	_DestroyResources();
	if (max_particles == 0)
		return WError(W_INVALIDPARAM);

	m_geometry = new WParticlesGeometry(m_app);
	m_material = new WMaterial(m_app);

	m_material->SetEffect(m_app->ParticlesManager->GetDefaultEffect());

	WParticlesVertex* vertices = new WParticlesVertex[max_particles];
	vertices[0].pos = WVector3(0, 0, 0);
	WError err = m_geometry->CreateFromData(vertices, max_particles, nullptr, 0, true);
	delete[] vertices;

	if (err != W_SUCCEEDED) {
		_DestroyResources();
		return err;
	}

	return WError(W_SUCCEEDED);
}

void WParticles::Render(class WRenderTarget* const rt) {
	if (Valid() && !m_hidden) {
		WCamera* cam = rt->GetCamera();
		WMatrix worldM = GetWorldMatrix();
		if (m_bFrustumCull) {
			if (!InCameraView(cam))
				return;

			m_material->Bind(rt);
			m_geometry->Draw(rt, 1, 1, false);
		}
	}
}

WMatrix WParticles::GetWorldMatrix() {
	UpdateLocals();
	return m_WorldM;
}

bool WParticles::UpdateLocals() {
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
		float x = -WVec3Dot(_right, _pos); //apply offset
		float y = -WVec3Dot(_up, _pos); //apply offset
		float z = -WVec3Dot(_look, _pos); //apply offset
		(m_WorldM)(0, 0) = _right.x; (m_WorldM)(0, 1) = _up.x; (m_WorldM)(0, 2) = _look.x; (m_WorldM)(0, 3) = 0.0f;
		(m_WorldM)(1, 0) = _right.y; (m_WorldM)(1, 1) = _up.y; (m_WorldM)(1, 2) = _look.y; (m_WorldM)(1, 3) = 0.0f;
		(m_WorldM)(2, 0) = _right.z; (m_WorldM)(2, 1) = _up.z; (m_WorldM)(2, 2) = _look.z; (m_WorldM)(2, 3) = 0.0f;
		(m_WorldM)(3, 0) = x;        (m_WorldM)(3, 1) = y;     (m_WorldM)(3, 2) = z;       (m_WorldM)(3, 3) = 1.0f;
		m_WorldM = WMatrixInverse(m_WorldM);

		return true;
	}

	return false;
}

void WParticles::OnStateChange(STATE_CHANGE_TYPE type) {
	WOrientation::OnStateChange(type); //do the default OnStateChange first
	m_bAltered = true;
}

