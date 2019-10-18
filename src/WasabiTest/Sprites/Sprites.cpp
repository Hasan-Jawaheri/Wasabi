#include "Sprites/Sprites.hpp"
#include <Wasabi/Renderers/Common/WSpritesRenderStage.hpp>

class CustomSpritePS : public WShader {
public:
	CustomSpritePS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
		};
		std::vector<uint8_t> code = {
			#include "sprite.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

SpritesDemo::SpritesDemo(Wasabi* const app) : WTestState(app) {
}

void SpritesDemo::Load() {
	WImage* img;

	img = m_app->ImageManager->CreateImage("media/dummy.bmp");
	m_sprites[0] = m_app->SpriteManager->CreateSprite(img);
	img->RemoveReference();
	m_sprites[0]->SetPriority(1);

	img = m_app->ImageManager->CreateImage("media/checker.bmp");
	m_sprites[1] = m_app->SpriteManager->CreateSprite(img);
	m_sprites[1]->SetPosition(WVector2(100, 100));
	m_sprites[1]->GetMaterials().SetVariable<float>("alpha", 0.5f);
	img->RemoveReference();
	m_sprites[1]->SetPriority(2);


	WShader* pixelShader = new CustomSpritePS(m_app);
	pixelShader->Load();
	WEffect* spriteFX = m_app->SpriteManager->CreateSpriteEffect(nullptr, pixelShader);
	pixelShader->RemoveReference();

	UNREFERENCED_PARAMETER(spriteFX);
	m_sprites[2] = m_app->SpriteManager->CreateSprite(spriteFX, 0, img);
	spriteFX->RemoveReference();
	m_sprites[2]->SetPosition(WVector2(200, 200));
	m_sprites[2]->SetSize(WVector2(400, 200));
}

void SpritesDemo::Update(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);
}

void SpritesDemo::Cleanup() {
	for (int i = 0; i < sizeof(m_sprites) / sizeof(WSprite*); i++)
		W_SAFE_REMOVEREF(m_sprites[i]);
}
