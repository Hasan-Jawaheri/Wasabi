#include "Files.hpp"
#include <Physics/Bullet/WBulletRigidBody.h>
#include <iostream>

class VS : public WShader {
public:
	VS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, "ubo", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
			}),
		};
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
			W_SHADER_VARIABLE_INFO(W_TYPE_UINT, 1), // texture index
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
				mat4 worldMatrix;\n\
				mat4 viewMatrix;\n\
			} ubo;\n\
			layout(location = 0) out vec2 outUV;\n\
			void main() {\n\
				outUV = inUV;\n\
				gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.worldMatrix * vec4(inPos.xyz, 1.0);\n\
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
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 3, "diffuseTexture"),
			W_BOUND_RESOURCE(W_TYPE_UBO, 4, "uboPS", {
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "color"),
			}),
		};
		LoadCodeGLSL("\
			#version 450\n\
			#extension GL_ARB_separate_shader_objects : enable\n\
			#extension GL_ARB_shading_language_420pack : enable\n\
			layout(binding = 3) uniform sampler2D diffuseTexture;\n\
			layout(location = 0) in vec2 inUV;\n\
			layout(location = 0) out vec4 outFragColor;\n\
			layout(binding = 4) uniform UBO {\n\
				vec4 color;\n\
			} ubo;\n\
			void main() {\n\
				outFragColor = texture(diffuseTexture, inUV) + ubo.color;\n\
			}\n\
		", bSaveData);
	}
};

FilesDemo::FilesDemo(Wasabi* const app) : WTestState(app) {
	m_material = nullptr;
	m_object = nullptr;
	m_cam = nullptr;
}

void FilesDemo::Load() {
	WImage* img = new WImage(m_app);
	img->Load("Media/dummy.bmp", W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_TEXTURE);

	WGeometry* geometry = new WGeometry(m_app);
	geometry->CreateCube(1, W_GEOMETRY_CREATE_DYNAMIC);

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
	fx->BuildPipeline(m_app->Renderer->GetRenderTarget());

	m_object = m_app->ObjectManager->CreateObject();
	m_object->SetGeometry(geometry);
	m_object->ClearEffects();
	m_object->AddEffect(fx);
	m_object->GetMaterial()->SetVariableColor("color", WColor(0, 0, 1));
	m_object->GetMaterial()->SetTexture("diffuseTexture", img);

	WRigidBody* rb = m_app->PhysicsComponent->CreateRigidBody();
	rb->Create(W_RIGID_BODY_CREATE_INFO::ForGeometry(geometry, 1.0f, nullptr, WVector3(0, 10, 0)), true);

	// empty out the file
	std::fstream f;
	f.open("Media/WFile.WSBI", ios::out);
	f.close();

	WFile file(m_app);
	file.Open("Media/WFile.WSBI");

	std::string objName = m_object->GetName();
	std::string rbName = rb->GetName();
	std::string imgName = img->GetName();

	file.SaveAsset(m_object);
	file.SaveAsset(rb);
	W_SAFE_REMOVEREF(img);
	W_SAFE_REMOVEREF(geometry);
	W_SAFE_REMOVEREF(fx);
	W_SAFE_REMOVEREF(m_object);
	W_SAFE_REMOVEREF(rb);

	file.Close();
	file.Open("Media/WFile.WSBI");

	file.LoadAsset<WObject>(objName, &m_object, WObject::LoadArgs());
	file.LoadAsset<WBulletRigidBody>(rbName, (WBulletRigidBody**)&rb, WBulletRigidBody::LoadArgs());
	file.LoadAsset<WImage>(imgName, &img, WImage::LoadArgs());

	file.Close();

	WSprite* spr = m_app->SpriteManager->CreateSprite(img);

	rb->BindObject(m_object, m_object);

	m_app->PhysicsComponent->Start();

	m_cam = m_app->CameraManager->GetDefaultCamera();
	m_material = m_object->GetMaterial();
}

void FilesDemo::Update(float fDeltaTime) {
	m_material->SetVariableMatrix("projectionMatrix", m_cam->GetProjectionMatrix());
	m_material->SetVariableMatrix("worldMatrix", m_object->GetWorldMatrix());
	m_material->SetVariableMatrix("viewMatrix", m_cam->GetViewMatrix());
}

void FilesDemo::Cleanup() {
}