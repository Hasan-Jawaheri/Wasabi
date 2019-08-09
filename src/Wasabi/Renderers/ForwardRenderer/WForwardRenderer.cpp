#include "Renderers/ForwardRenderer/WForwardRenderer.h"
#include "Renderers/WRenderer.h"
#include "Renderers/ForwardRenderer/WForwardRenderStage.h"
#include "Renderers/Common/WSpritesRenderStage.h"
#include "Renderers/Common/WParticlesRenderStage.h"
#include "Renderers/Common/WTextRenderStage.h"

WError WInitializeForwardRenderer(Wasabi* app) {
	return app->Renderer->SetRenderingStages({
		new WForwardRenderStage(app),
		new WParticlesRenderStage(app),
		new WSpritesRenderStage(app),
		new WTextsRenderStage(app),
	});
}
