#include "WForwardRenderer.h"
#include "../Core/WCore.h"
#include "../Cameras/WCamera.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Lights/WLight.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Objects/WObject.h"
#include "../Sprites/WSprite.h"
#include "../Texts/WText.h"
#include "../Particles/WParticles.h"

struct LightStruct {
	WVector4 color;
	WVector4 dir;
	WVector4 pos;
	int type;
	float pad[3];
};

void stringReplaceAll(std::string& source, const std::string& from, const std::string& to)
{
	std::string newString;
	newString.reserve(source.length());  // avoids a few memory allocations

	std::string::size_type lastPos = 0;
	std::string::size_type findPos;

	while (std::string::npos != (findPos = source.find(from, lastPos)))
	{
		newString.append(source, lastPos, findPos - lastPos);
		newString += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	newString += source.substr(lastPos);

	source.swap(newString);
}

class ForwardRendererVS : public WShader {
public:
	ForwardRendererVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gProjection"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gWorld"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gView"), // view
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gAnimationTextureWidth"), // width of the animation texture
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gInstanceTextureWidth"), // width of the instance texture
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gAnimation"), // whether or not animation is enabled
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "gInstancing"), // whether or not instancing is enabled
			}),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 2),
		};
		m_desc.animation_texture_index = 1;
		m_desc.instancing_texture_index = 2;
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		}), W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone indices
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weights
		})};
		LoadCodeGLSL(
			#include "Shaders/Forward/vertex_shader.glsl"
		);
	}
};

class ForwardRendererPS : public WShader {
public:
	ForwardRendererPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		int maxLights = (int)m_app->engineParams["maxLights"];
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 3),
			W_BOUND_RESOURCE(W_TYPE_UBO, 4, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4, "color"),
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "isTextured"),
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 5, { // TODO: make a shared UBO
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "numLights"),
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "pad"),
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, (sizeof(LightStruct) / 4) * maxLights, "lights"),
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 6,{
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "gCamPos"),
			}),
		};
		std::string code =
			#include "Shaders/Forward/pixel_shader.glsl"
		;
		stringReplaceAll(code, "~~~~maxLights~~~~", std::to_string(maxLights));
		LoadCodeGLSL(code);
	}
};

WForwardRenderer::WForwardRenderer(Wasabi* const app) : WRenderer(app) {
	m_sampler = VK_NULL_HANDLE;
	m_default_fx = nullptr;
	m_lights = nullptr;
	m_app->engineParams.insert(std::pair<std::string, void*>("maxLights", (void*)8));
}

WError WForwardRenderer::Initiailize() {
	//
	// Create the texture sampler
	//
	VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Max level-of-detail should match mip level count
	sampler.maxLod = 1;
	// Enable anisotropic filtering
	sampler.maxAnisotropy = 8;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult err = vkCreateSampler(m_device, &sampler, nullptr, &m_sampler);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	//
	// Create the default effect for rendering
	//
	WShader* vs = new ForwardRendererVS(m_app);
	vs->Load();
	WShader* ps = new ForwardRendererPS(m_app);
	ps->Load();
	m_default_fx = new WEffect(m_app);
	m_default_fx->BindShader(vs);
	m_default_fx->BindShader(ps);
	WError werr = m_default_fx->BuildPipeline(m_renderTarget);
	vs->RemoveReference();
	ps->RemoveReference();
	if (!werr)
		return werr;

	m_lights = new LightStruct[(int)m_app->engineParams["maxLights"]];

	return WError(W_SUCCEEDED);
}

WError WForwardRenderer::Resize(unsigned int width, unsigned int height) {
	return WRenderer::Resize(width, height);
}

void WForwardRenderer::Render(WRenderTarget* rt, unsigned int filter) {
	WError werr = rt->Begin();

	if (werr != W_SUCCEEDED)
		return;

	// create the lights UBO data
	int max_lights = (int)m_app->engineParams["maxLights"];
	m_numLights = 0;
	for (int i = 0; m_numLights < max_lights; i++) {
		WLight* light = m_app->LightManager->GetEntityByIndex(i);
		if (!light)
			break;
		if (!light->Hidden() && light->InCameraView(rt->GetCamera())) {
			WColor c = light->GetColor();
			WVector3 l = light->GetLVector();
			WVector3 p = light->GetPosition();
			m_lights[m_numLights].color = WVector4(c.r, c.g, c.b, light->GetIntensity());
			m_lights[m_numLights].dir = WVector4(l.x, l.y, l.z, light->GetRange());
			m_lights[m_numLights].pos = WVector4(p.x, p.y, p.z, light->GetMinCosAngle());
			m_lights[m_numLights].type = (int)light->GetType();
			m_numLights++;
		}
	}

	if (filter & RENDER_FILTER_OBJECTS)
		m_app->ObjectManager->Render(rt);

	if (filter & RENDER_FILTER_OBJECTS)
		m_app->ParticlesManager->Render(rt);

	if (filter & RENDER_FILTER_SPRITES)
		m_app->SpriteManager->Render(rt);

	if (filter & RENDER_FILTER_TEXT)
		m_app->TextComponent->Render(rt);

	rt->End(false);
}

void WForwardRenderer::Cleanup() {
	if (m_sampler)
		vkDestroySampler(m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;

	W_SAFE_REMOVEREF(m_default_fx);
	W_SAFE_DELETE_ARRAY(m_lights);
}

class WMaterial* WForwardRenderer::CreateDefaultMaterial() {
	WFRMaterial* mat = new WFRMaterial(m_app);
	mat->SetEffect(m_default_fx);
	mat->SetVariableInt("isTextured", 1); // default image is being used as texture
	//float r = (float)(rand() % 224 + 32) / 256.0f;
	//float g = (float)(rand() % 224 + 32) / 256.0f;
	//float b = (float)(rand() % 224 + 32) / 256.0f;
	//mat->SetColor(WColor(r, g, b, 1.0f));
	return mat;
}

VkSampler WForwardRenderer::GetDefaultSampler() const {
	return m_sampler;
}

WFRMaterial::WFRMaterial(Wasabi* const app, unsigned int ID) : WMaterial(app, ID) {

}

WError WFRMaterial::Bind(class WRenderTarget* rt, unsigned int num_vertex_buffers) {
	//TODO: remove this and make shared UBO
	int nLights = ((WForwardRenderer*)m_app->Renderer)->m_numLights;
	SetVariableInt("numLights", nLights);
	SetVariableData("lights", ((WForwardRenderer*)m_app->Renderer)->m_lights, nLights * sizeof(LightStruct));

	return WMaterial::Bind(rt, num_vertex_buffers);
}

WError WFRMaterial::Texture(class WImage* img) {
	SetVariableInt("isTextured", 1);
	return SetTexture(3, img);
}

WError WFRMaterial::SetColor(WColor col) {
	SetVariableInt("isTextured", 0);
	return SetVariableColor("color", col);
}
