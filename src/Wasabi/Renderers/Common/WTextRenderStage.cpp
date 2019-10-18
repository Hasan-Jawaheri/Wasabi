#include "Wasabi/Renderers/Common/WTextRenderStage.hpp"
#include "Wasabi/Core/WCore.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Texts/WText.hpp"


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