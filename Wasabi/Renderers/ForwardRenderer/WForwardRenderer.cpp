#include "WForwardRenderer.h"
#include "../WRenderer.h"
#include "WForwardLightRenderStage.h"
#include "../Common/WSpritesRenderStage.h"
#include "../Common/WParticlesRenderStage.h"
#include "../Common/WTextRenderStage.h"

WError WInitializeForwardRenderer(Wasabi* app) {
	return app->Renderer->SetRenderingStages({
		new WForwardLightRenderStage(app),
		new WParticlesRenderStage(app),
		new WSpritesRenderStage(app),
		new WTextsRenderStage(app),
	});
}
