#include "Files.hpp"
#include <iostream>

class VS : public WShader {
public:
	VS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "gProjection"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "gWorld"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "gView"), // view
			}),
		};
		m_desc.animation_texture_index = 1;
		m_desc.instancing_texture_index = 2;
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
		}), W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 4), // bone indices
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4), // bone weights
		}) };
		LoadCodeGLSL("\
			#version 450\n\
			#extension GL_ARB_separate_shader_objects : enable\n\
			#extension GL_ARB_shading_language_420pack : enable\n\
			layout(location = 0) in vec3 inPos;\n\
			layout(location = 1) in vec3 inTang;\n\
			layout(location = 2) in vec3 inNorm;\n\
			layout(location = 3) in vec2 inUV;\n\
			\n\
			layout(binding = 0) uniform UBO {\n\
				mat4 projectionMatrix;\n\
				mat4 modelMatrix;\n\
				mat4 viewMatrix;\n\
			} ubo;\n\
			layout(location = 0) out vec2 outUV;\n\
			void main() {\n\
				outUV = inUV;\n\
				gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);\n\
			}\n\
		", bSaveData);
	}
};

class PS : public WShader {
public:
	PS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 3),
			W_BOUND_RESOURCE(W_TYPE_UBO, 4, {
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"),
			}),
		};
		LoadCodeGLSL("\
			#version 450\n\
			#extension GL_ARB_separate_shader_objects : enable\n\
			#extension GL_ARB_shading_language_420pack : enable\n\
			layout(binding = 3) uniform sampler2D samplerColor;\n\
			layout(location = 0) in vec2 inUV;\n\
			layout(location = 0) out vec4 outFragColor;\n\
			layout(binding = 4) uniform UBO {\n\
				vec4 color;\n\
			} ubo;\n\
			void main() {\n\
				outFragColor = texture(samplerColor, inUV) + ubo.color;\n\
			}\n\
		", bSaveData);
	}
};

FilesDemo::FilesDemo(Wasabi* const app) : WTestState(app) {
}

void FilesDemo::Load() {
	WImage* img = new WImage(m_app);
	img->Load("Media/dummy.bmp", true);

	WGeometry* geometry = new WGeometry(m_app);
	geometry->CreateCube(1, true);

	WPointLight* light = new WPointLight(m_app);
	light->SetPosition(2, 2, 2);
	light->SetRange(5);
	light->SetColor(WColor(1, 0, 0));

	WShader* ps = new PS(m_app);
	WShader* vs = new VS(m_app);
	ps->Load(true);
	vs->Load(true);
	WEffect* fx = new WEffect(m_app);
	fx->BindShader(vs);
	fx->BindShader(ps);
	fx->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());
	WMaterial* mat = new WMaterial(m_app);
	mat->SetEffect(fx);
	mat->SetVariableColor("color", WColor(0, 0, 1));
	mat->SetTexture(3, img);

	// empty out the file
	std::fstream f;
	f.open("WFile.WSBI", ios::out);
	f.close();

	WFile file(m_app);
	file.Open("WFile.WSBI");

	uint imgId, geoId, fxId, matId;
	file.SaveAsset(img, &imgId);
	file.SaveAsset(geometry, &geoId);
	file.SaveAsset(fx, &fxId);
	file.SaveAsset(mat, &matId);
	W_SAFE_REMOVEREF(img);
	W_SAFE_REMOVEREF(geometry);
	W_SAFE_REMOVEREF(fx);
	W_SAFE_REMOVEREF(mat);

	file.LoadAsset<WImage>(imgId, &img);
	file.LoadAsset<WGeometry>(geoId, &geometry);
	file.LoadAsset<WEffect>(fxId, &fx);
	file.LoadAsset<WMaterial>(matId, &mat);

	file.Close();

	WSprite* spr = new WSprite(m_app);
	spr->SetImage(img);

	WObject* obj = new WObject(m_app);
	obj->SetGeometry(geometry);
	obj->SetMaterial(mat);
}

void FilesDemo::Update(float fDeltaTime) {
}

void FilesDemo::Cleanup() {
}