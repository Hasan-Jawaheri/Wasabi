#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderer.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Renderers/ForwardRenderer/WForwardRenderStage.hpp"
#include "Wasabi/Renderers/Common/WSpritesRenderStage.hpp"
#include "Wasabi/Renderers/Common/WParticlesRenderStage.hpp"
#include "Wasabi/Renderers/Common/WTextRenderStage.hpp"

WError WInitializeForwardRenderer(Wasabi* app) {
	return app->Renderer->SetRenderingStages({
		new WForwardRenderStage(app),
		new WParticlesRenderStage(app),
		new WSpritesRenderStage(app),
		new WTextsRenderStage(app),
	});
}
