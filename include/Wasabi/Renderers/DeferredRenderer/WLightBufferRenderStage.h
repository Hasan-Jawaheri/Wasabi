#pragma once

#include "Wasabi/Renderers/WRenderStage.h"

class WShader;



class WLightBufferRenderStage : public WRenderStage {
	/** blend state used for all light renders */
	VkPipelineColorBlendAttachmentState m_blendState;
	/** Rasterization state used for all light renders */
	VkPipelineRasterizationStateCreateInfo m_rasterizationState;

	/** Assets required to render a light onto the light map */
	struct LightTypeAssets {
		/** Light's geometry, only one of fullscreen_sprite and geometry is not null */
		class WGeometry* geometry;
		/** Effect used to build materials */
		class WEffect* effect;
		/** A sprite that renders at full-screen (for directional lights), only one of fullscreen_sprite and geometry is not null */
		class WSprite* fullscreenSprite;
		/** Per-frame material for this light type */
		class WMaterial* perFrameMaterial;
		/** Render materials for all lights of this type */
		unordered_map<class WLight*, class WMaterial*> materialMap;

		LightTypeAssets() : geometry(nullptr), effect(nullptr), fullscreenSprite(nullptr), perFrameMaterial(nullptr) {}

		/** Free the resources of this object */
		void Destroy();
	};
	/** Map of light type -> LightTypeAssets to render that light */
	std::unordered_map<int, LightTypeAssets> m_lightRenderingAssets;

	/** Initializes point lights assets */
	WError LoadPointLightsAssets();
	/** Initializes spot light assets */
	WError LoadSpotLightsAssets();
	/** Initializes directional lights assets */
	WError LoadDirectionalLightsAssets();

	/**
	 * Callback called whenever a light is added/removed from the lights manager
	 */
	void OnLightsChange(class WLight* light, bool is_added);

public:
	WLightBufferRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);
};

