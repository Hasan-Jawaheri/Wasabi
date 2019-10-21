#include "Files/Files.hpp"
#include <Wasabi/Physics/Bullet/WBulletRigidBody.hpp>
#include <iostream>

class VS : public WShader {
public:
	VS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, "ubo", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "worldMatrix"), // world
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "viewMatrix"), // view
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "projectionMatrix"), // projection
			}),
		};
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // tangent
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // normal
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
			W_SHADER_VARIABLE_INFO(W_TYPE_INT), // texture index
		}) };
		std::vector<uint8_t> code = {
			#include "object.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
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
				W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4, "customColor"),
			}),
		};
		std::vector<uint8_t> code = {
			#include "object.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

FilesDemo::FilesDemo(Wasabi* const app) : WTestState(app) {
	m_object = nullptr;
	m_rb = nullptr;
	m_cam = nullptr;
	m_sprite = nullptr;
}

void FilesDemo::Load() {
	WImage* img = new WImage(m_app);
	img->Load("media/seamless_grass.jpg", W_IMAGE_CREATE_DYNAMIC | W_IMAGE_CREATE_TEXTURE);

	WGeometry* geometry = new WGeometry(m_app);
	geometry->CreateCube(1, W_GEOMETRY_CREATE_CPU_READABLE | W_GEOMETRY_CREATE_DYNAMIC);

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

	m_object = m_app->ObjectManager->CreateObject(fx, 0);
	m_object->SetGeometry(geometry);
	m_object->GetMaterials().SetVariable<WColor>("customColor", WColor(0, 0, 1));
	m_object->GetMaterials().SetTexture("diffuseTexture", img);

	m_rb = m_app->PhysicsComponent->CreateRigidBody();
	m_rb->Create(W_RIGID_BODY_CREATE_INFO::ForGeometry(geometry, 1.0f, nullptr, WVector3(0, 10, 0)), true);

	// empty out the file
	std::fstream f;
	f.open("media/WFile.WSBI", ios::out);
	f.close();

	WFile file(m_app);
	file.Open("media/WFile.WSBI");

	std::string objName = m_object->GetName();
	std::string rbName = m_rb->GetName();
	std::string imgName = img->GetName();

	file.SaveAsset(m_object);
	file.SaveAsset(m_rb);
	W_SAFE_REMOVEREF(img);
	W_SAFE_REMOVEREF(geometry);
	W_SAFE_REMOVEREF(fx);
	W_SAFE_REMOVEREF(m_object);
	W_SAFE_REMOVEREF(m_rb);

	file.Close();
	file.Open("media/WFile.WSBI");

	file.LoadAsset<WObject>(objName, &m_object, WObject::LoadArgs());
	file.LoadAsset<WBulletRigidBody>(rbName, (WBulletRigidBody**)&m_rb, WBulletRigidBody::LoadArgs());
	file.LoadAsset<WImage>(imgName, &img, WImage::LoadArgs());

	file.Close();

	m_sprite = m_app->SpriteManager->CreateSprite(img);
	m_sprite->SetSize(WVector2(50.0f, 50.0f));

	m_rb->BindObject(m_object, m_object);

	m_app->PhysicsComponent->Start();

	m_cam = m_app->CameraManager->GetDefaultCamera();
	m_materials = m_object->GetMaterials();
}

void FilesDemo::Update(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);

	m_materials.SetVariable<WMatrix>("projectionMatrix", m_cam->GetProjectionMatrix());
	m_materials.SetVariable<WMatrix>("worldMatrix", m_object->GetWorldMatrix());
	m_materials.SetVariable<WMatrix>("viewMatrix", m_cam->GetViewMatrix());
}

void FilesDemo::Cleanup() {
	W_SAFE_REMOVEREF(m_object);
	W_SAFE_REMOVEREF(m_rb);
	W_SAFE_REMOVEREF(m_sprite);
}