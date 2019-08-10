#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderer.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.h"
#include "Wasabi/Renderers/Common/WSpritesRenderStage.h"
#include "Wasabi/Renderers/Common/WParticlesRenderStage.h"
#include "Wasabi/Renderers/Common/WTextRenderStage.h"

WError WInitializeForwardRenderer(Wasabi* app) {
	return app->Renderer->SetRenderingStages({
		new WForwardRenderStage(app),
		new WParticlesRenderStage(app),
		new WSpritesRenderStage(app),
		new WTextsRenderStage(app),
	});
}
