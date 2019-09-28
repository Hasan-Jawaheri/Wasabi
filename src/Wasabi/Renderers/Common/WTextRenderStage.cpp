#include "Wasabi/Renderers/Common/WTextRenderStage.h"
#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Texts/WText.h"


WTextsRenderStage::WTextsRenderStage(Wasabi* const app, bool backbuffer) : WRenderStage(app) {
	m_stageDescription.name = __func__;
	m_stageDescription.target = backbuffer ? RENDER_STAGE_TARGET_BACK_BUFFER : RENDER_STAGE_TARGET_PREVIOUS;
	m_stageDescription.flags = RENDER_STAGE_FLAG_TEXTS_RENDER_STAGE;
}

WError WTextsRenderStage::Render(WRenderer* renderer, WRenderTarget* rt, uint32_t filter) {
	UNREFERENCED_PARAMETER(renderer);

	if (filter & RENDER_FILTER_TEXT) {
		m_app->TextComponent->Render(rt);
	}

	return WError(W_SUCCEEDED);
}