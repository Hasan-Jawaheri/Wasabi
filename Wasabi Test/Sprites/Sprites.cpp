#include "Sprites.hpp"

class CustomSpritePS : public WShader {
public:
	CustomSpritePS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
		};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"void main() {\n"
			"	outFragColor = vec4(inUV.xy, 0, 0.5);\n"
			"}\n"
		);
	}
};

SpritesDemo::SpritesDemo(Wasabi* const app) : WTestState(app) {
}

void SpritesDemo::Load() {
	m_sprites[0] = new WSprite(m_app);
	m_sprites[0]->Load();
	WImage* img = new WImage(m_app);
	img->Load("Media/dummy.bmp");
	m_sprites[0]->SetImage(img);
	img->RemoveReference();

	m_sprites[1] = new WSprite(m_app);
	m_sprites[1]->Load();
	img = new WImage(m_app);
	img->Load("Media/checker.bmp");
	m_sprites[1]->SetImage(img);
	img->RemoveReference();
	m_sprites[1]->SetPosition(100, 100);
	m_sprites[1]->SetAlpha(0.5f);

	m_sprites[2] = new WSprite(m_app);
	WShader* pixel_shader = new CustomSpritePS(m_app);
	pixel_shader->Load();
	WEffect* fx = m_app->SpriteManager->CreateSpriteEffect(pixel_shader);
	pixel_shader->RemoveReference();
	WMaterial* mat = new WMaterial(m_app);
	mat->SetEffect(fx);
	fx->RemoveReference();
	m_sprites[2]->SetMaterial(mat);
	mat->RemoveReference();
	m_sprites[2]->Load();
	m_sprites[2]->SetPosition(200, 200);
	m_sprites[2]->SetSize(200, 200);
}

void SpritesDemo::Update(float fDeltaTime) {
}

void SpritesDemo::Cleanup() {
	for (int i = 0; i < sizeof(m_sprites) / sizeof(WSprite*); i++)
		W_SAFE_REMOVEREF(m_sprites[i]);
}