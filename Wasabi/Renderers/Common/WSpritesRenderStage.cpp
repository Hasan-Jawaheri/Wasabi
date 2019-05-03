#include "WSpritesRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Sprites/WSprite.h"
#include "../../Materials/WEffect.h"
#include "../../Materials/WMaterial.h"

WSpritesRenderStage::WSpritesRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
	m_stageDescription.flags = RENDER_STAGE_FLAG_SPRITES_RENDER_STAGE;

	m_spritesFragment = nullptr;
}

WError WSpritesRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	WError err = WRenderStage::Initialize(previousStages, width, height);
	if (!err)
		return err;

	WEffect* spritesFX = m_app->SpriteManager->CreateSpriteEffect(m_renderTarget);
	if (!spritesFX)
		return WError(W_ERRORUNK);

	m_spritesFragment = new WSpritesRenderFragment(m_stageDescription.name, spritesFX, m_app);

	return WError(W_SUCCEEDED);
}

void WSpritesRenderStage::Cleanup() {
	WRenderStage::Cleanup();
	W_SAFE_DELETE(m_spritesFragment);
}

WError WSpritesRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_SPRITES) {
		m_spritesFragment->Render(renderer, rt);
	}

	return WError(W_SUCCEEDED);
}

WError WSpritesRenderStage::Resize(uint width, uint height) {
	return WRenderStage::Resize(width, height);
}

