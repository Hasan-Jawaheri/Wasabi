#include "WForwardRenderer.h"

struct LightStruct {
	WVector4 color;
	WVector4 dir;
	WVector4 pos;
	int type;
	float pad[3];
};

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
			}),
		};
		m_desc.animation_texture_index = 1;
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		})};
		LoadCodeGLSL(
			"#version 450\n"
			""
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(location = 0) in vec3 inPos;\n"
			"layout(location = 1) in vec3 inTang;\n"
			"layout(location = 2) in vec3 inNorm;\n"
			"layout(location = 3) in vec2 inUV;\n"
			""
			"layout(binding = 0) uniform UBO {\n"
			"	mat4 projectionMatrix;\n"
			"	mat4 modelMatrix;\n"
			"	mat4 viewMatrix;\n"
			"} ubo;\n"
			""
			"layout(location = 0) out vec2 outUV;\n"
			"layout(location = 1) out vec3 outWorldPos;\n"
			"layout(location = 2) out vec3 outWorldNorm;\n"
			""
			"void main() {\n"
			"	outUV = inUV;\n"
			"	outWorldPos = (ubo.modelMatrix * vec4(inPos.xyz, 1.0)).xyz;\n"
			"	outWorldNorm = (ubo.modelMatrix * vec4(inNorm.xyz, 0.0)).xyz;\n"
			"	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);\n"
			"}\n"
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
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
			W_BOUND_RESOURCE(W_TYPE_UBO, 2, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4, "color"),
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "isTextured"),
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 3, { // TODO: make a shared UBO
				W_SHADER_VARIABLE_INFO(W_TYPE_INT, 1, "numLights"),
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "pad"),
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, (sizeof(LightStruct) / 4) * maxLights, "lights"),
			}),
			W_BOUND_RESOURCE(W_TYPE_UBO, 4,{
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3, "gCamPos"),
			}),
		};
		LoadCodeGLSL(
			"#version 450\n"
			""
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"struct Light {\n"
			"	vec4 color;\n"
			"	vec4 dir;\n"
			"	vec4 pos;\n"
			"	int type;\n"
			"};\n"
			""
			"layout(binding = 1) uniform sampler2D samplerColor;\n"
			"layout(binding = 2) uniform UBO {\n"
			"	vec4 color;\n"
			"	int isTextured;\n"
			"} ubo;\n"
			"layout(binding = 3) uniform LUBO {\n"
			"	int numLights;\n"
			"	Light lights[" + std::to_string(maxLights) + "];\n"
			"} lubo;\n"
			"layout(binding = 4) uniform CAM {\n"
			"	vec3 gCamPos;\n"
			"} cam;\n"
			""
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 1) in vec3 inWorldPos;\n"
			"layout(location = 2) in vec3 inWorldNorm;\n"
			""
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"" // DIRECTIONAL LIGHT CODE
			"vec3 DirectionalLight(in vec3 dir, in vec3 col, in float intensity) {\n"
			""  // N dot L lighting term
			"	vec3 lDir = -normalize(dir); \n"
			"	float nl = clamp(dot(inWorldNorm, lDir), 0, 1);\n"
			"	vec3 camDir = normalize(cam.gCamPos - inWorldPos);\n"
			""  // Calculate specular term
			"	float spec = max(dot(reflect(-lDir, inWorldNorm), camDir), 0.0f);\n"
			"	vec3 unspecced = vec3(col * nl) * intensity;\n"
			"	return unspecced * spec;\n"
			"}\n"
			""
			"" // POINT LIGHT CODE
			"vec3 PointLight(in vec3 pos, in vec3 col, in float intensity, in float range) {\n"
			""  // The distance from surface to light
			"	vec3 lightVec = pos - inWorldPos;\n"
			"	float d = length (lightVec);\n"
			""  // N dot L lighting term
			"	vec3 lDir = normalize(lightVec);\n"
			"	float nl = clamp(dot(inWorldNorm, lDir), 0, 1);\n"
			""  // Calculate specular term
			"	vec3 camDir = normalize(cam.gCamPos - inWorldPos);\n"
			"	vec3 h = normalize(lDir + camDir);\n"
			"	float spec = clamp(dot(inWorldNorm, h), 0, 1);\n"
			""
			"	float xVal = clamp((range-d)/range, 0, 1);\n"
			"	return vec3(col * xVal * spec * nl * intensity);\n"
			"}\n"
			""
			"" // SPOT LIGHT CODE
			"vec3 SpotLight(in vec3 pos, in vec3 dir, in vec3 col, in float intensity, in float range, float minCos) {\n"
			"	vec3 color = PointLight(pos, col, intensity, range);\n"
			""  // The vector from the surface to the light
			"	vec3 lightVec = normalize(pos - inWorldPos);\n"
			""
			"	float cosAngle = dot(-lightVec, dir);\n"
			"	color *= max ((cosAngle - minCos) / (1.0f-minCos), 0);\n"
			""  // Scale color by spotlight factor
			"	return color;\n"
			"}\n"
			""
			"void main() {\n"
			"	if (ubo.isTextured == 1)\n"
			"		outFragColor = texture(samplerColor, inUV);\n"
			"	else\n"
			"		outFragColor = ubo.color;\n"
			"	vec3 lighting = vec3(0,0,0);"
			"	for (int i = 0; i < lubo.numLights; i++) {\n"
			"		if (lubo.lights[i].type == 0)\n"
			"			lighting += DirectionalLight(lubo.lights[i].dir.xyz,\n"
			"										 lubo.lights[i].color.rgb,\n"
			"										 lubo.lights[i].color.a);\n"
			"		else if (lubo.lights[i].type == 1)\n"
			"			lighting += PointLight( lubo.lights[i].pos.xyz,\n"
			"									lubo.lights[i].color.rgb,\n"
			"									lubo.lights[i].color.a,\n"
			"									lubo.lights[i].dir.a);\n"
			"		else if (lubo.lights[i].type == 2)\n"
			"			lighting += SpotLight(  lubo.lights[i].pos.xyz,\n"
			"									lubo.lights[i].dir.xyz,\n"
			"									lubo.lights[i].color.rgb,\n"
			"									lubo.lights[i].color.a,\n"
			"									lubo.lights[i].dir.a,\n"
			"									lubo.lights[i].pos.a);\n"
			"	}\n"
			"	outFragColor.rgb = outFragColor.rgb * 0.2f + outFragColor.rgb * 0.8f * lighting;\n" // ambient 20%
			"}\n"
		);
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

	if (filter & RENDER_FILTER_SPRITES)
		m_app->SpriteManager->Render(rt);

	if (filter & RENDER_FILTER_TEXT)
		m_app->TextComponent->Render(rt);
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
	float r = (float)(rand() % 224 + 32) / 256.0f;
	float g = (float)(rand() % 224 + 32) / 256.0f;
	float b = (float)(rand() % 224 + 32) / 256.0f;
	mat->SetColor(WColor(r, g, b, 1.0f));
	return mat;
}

VkSampler WForwardRenderer::GetDefaultSampler() const {
	return m_sampler;
}

WFRMaterial::WFRMaterial(Wasabi* const app, unsigned int ID) : WMaterial(app, ID) {

}

WError WFRMaterial::Bind(WRenderTarget* rt) {
	//TODO: remove this and make shared UBO
	int nLights = ((WForwardRenderer*)m_app->Renderer)->m_numLights;
	SetVariableInt("numLights", nLights);
	SetVariableData("lights", ((WForwardRenderer*)m_app->Renderer)->m_lights, nLights * sizeof(LightStruct));

	return WMaterial::Bind(rt);
}

WError WFRMaterial::Texture(class WImage* img) {
	int isTextured = 1;
	SetVariableInt("isTextured", isTextured);
	return SetTexture(1, img);
}

WError WFRMaterial::SetColor(WColor col) {
	int isTextured = 0;
	SetVariableInt("isTextured", isTextured);
	return SetVariableColor("color", col);
}

