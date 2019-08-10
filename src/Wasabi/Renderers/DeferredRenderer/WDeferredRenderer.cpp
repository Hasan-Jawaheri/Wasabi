#include "Wasabi/Renderers/DeferredRenderer/WDeferredRenderer.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Renderers/DeferredRenderer/WGBufferRenderStage.h"
#include "Wasabi/Renderers/DeferredRenderer/WLightBufferRenderStage.h"
#include "Wasabi/Renderers/DeferredRenderer/WSceneCompositionRenderStage.h"
#include "Wasabi/Renderers/Common/WSpritesRenderStage.h"
#include "Wasabi/Renderers/Common/WTextRenderStage.h"
#include "Wasabi/Renderers/Common/WParticlesRenderStage.h"
#include "Wasabi/Renderers/Common/WBackfaceDepthRenderStage.h"

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
