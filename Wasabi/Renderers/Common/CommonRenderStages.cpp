#include "CommonRenderStages.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Particles/WParticles.h"
#include "../../Sprites/WSprite.h"
#include "../../Materials/WEffect.h"
#include "../../Texts/WText.h"

WParticlesRenderStage::WParticlesRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
}

WError WParticlesRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_PARTICLES) {
	}

	return WError(W_SUCCEEDED);
}

WTextsRenderStage::WTextsRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
}

WError WTextsRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_TEXT) {
		m_app->TextComponent->Render(rt);
	}

	return WError(W_SUCCEEDED);
}