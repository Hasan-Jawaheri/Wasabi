#include "WDeferredRenderer.h"
#include "../WRenderer.h"
#include "WGBufferRenderStage.h"
#include "WLightBufferRenderStage.h"
#include "WSceneCompositionRenderStage.h"
#include "../Common/WSpritesRenderStage.h"
#include "../Common/WTextRenderStage.h"
#include "../Common/WParticlesRenderStage.h"
#include "../Common/WBackfaceDepthRenderStage.h"

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
