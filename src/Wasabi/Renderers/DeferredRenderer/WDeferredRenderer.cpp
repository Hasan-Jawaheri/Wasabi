#include "Renderers/DeferredRenderer/WDeferredRenderer.h"
#include "Renderers/WRenderer.h"
#include "Renderers/DeferredRenderer/WGBufferRenderStage.h"
#include "Renderers/DeferredRenderer/WLightBufferRenderStage.h"
#include "Renderers/DeferredRenderer/WSceneCompositionRenderStage.h"
#include "Renderers/Common/WSpritesRenderStage.h"
#include "Renderers/Common/WTextRenderStage.h"
#include "Renderers/Common/WParticlesRenderStage.h"
#include "Renderers/Common/WBackfaceDepthRenderStage.h"

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
