#include "Wasabi/Renderers/DeferredRenderer/WDeferredRenderer.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Renderers/DeferredRenderer/WGBufferRenderStage.hpp"
#include "Wasabi/Renderers/DeferredRenderer/WLightBufferRenderStage.hpp"
#include "Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.hpp"
#include "Wasabi/Renderers/Common/WSpritesRenderStage.hpp"
#include "Wasabi/Renderers/Common/WTextRenderStage.hpp"
#include "Wasabi/Renderers/Common/WParticlesRenderStage.hpp"
#include "Wasabi/Renderers/Common/WBackfaceDepthRenderStage.hpp"

WError WInitializeDeferredRenderer(Wasabi* app) {
	return app->Renderer->SetRenderingStages({
		new WGBufferRenderStage(app),
		new WBackfaceDepthRenderStage(app),
		new WLightBufferRenderStage(app),
		new WSceneCompositionRenderStage(app),
		new WParticlesRenderStage(app),
		new WSpritesRenderStage(app),
		new WTextsRenderStage(app),
	});
}
