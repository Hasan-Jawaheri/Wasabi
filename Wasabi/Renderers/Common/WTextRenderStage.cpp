#include "WTextRenderStage.h"
#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Images/WRenderTarget.h"
#include "../../Materials/WEffect.h"
#include "../../Texts/WText.h"


WTextsRenderStage::WTextsRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
	m_stageDescription.flags = RENDER_STAGE_FLAG_TEXTS_RENDER_STAGE;
}

WError WTextsRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint filter) {
	if (filter & RENDER_FILTER_TEXT) {
		m_app->TextComponent->Render(rt);
	}

	return WError(W_SUCCEEDED);
}