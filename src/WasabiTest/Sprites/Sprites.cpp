#include "Sprites/Sprites.hpp"
#include <Renderers/Common/WSpritesRenderStage.h>

class CustomSpritePS : public WShader {
public:
	CustomSpritePS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
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
			"	outFragColor = vec4(inUV.xy, 0, 0.65);\n"
			"}\n"
		, bSaveData);
	}
};

SpritesDemo::SpritesDemo(Wasabi* const app) : WTestState(app) {
}

void SpritesDemo::Load() {
	WImage* img;

	img = m_app->ImageManager->CreateImage("Media/dummy.bmp");
	m_sprites[0] = m_app->SpriteManager->CreateSprite(img);
	img->RemoveReference();
	m_sprites[0]->SetPriority(1);

	img = m_app->ImageManager->CreateImage("Media/checker.bmp");
	m_sprites[1] = m_app->SpriteManager->CreateSprite(img);
	m_sprites[1]->SetPosition(WVector2(100, 100));
	m_sprites[1]->GetMaterial()->SetVariableFloat("alpha", 0.5f);
	img->RemoveReference();
	m_sprites[1]->SetPriority(2);

	
	WShader* pixel_shader = new CustomSpritePS(m_app);
	pixel_shader->Load();
	WEffect* spriteFX = m_app->SpriteManager->CreateSpriteEffect(nullptr, pixel_shader);
	pixel_shader->RemoveReference();

	m_sprites[2] = m_app->SpriteManager->CreateSprite(img);
	m_sprites[2]->RemoveEffect(m_sprites[2]->GetDefaultEffect());
	m_sprites[2]->AddEffect(spriteFX);
	spriteFX->RemoveReference();
	m_sprites[2]->SetPosition(WVector2(200, 200));
	m_sprites[2]->SetSize(WVector2(400, 200));
}

void SpritesDemo::Update(float fDeltaTime) {
}

void SpritesDemo::Cleanup() {
	for (int i = 0; i < sizeof(m_sprites) / sizeof(WSprite*); i++)
		W_SAFE_REMOVEREF(m_sprites[i]);
}