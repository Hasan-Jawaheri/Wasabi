#pragma once

#include "../WRenderStage.h"

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
		class WSprite* fullscreen_sprite;
		/** Render material */
		unordered_map<class WLight*, class WMaterial*> material_map;

		LightTypeAssets() : geometry(nullptr), fullscreen_sprite(nullptr), effect(nullptr) {}

		/** Free the resources of this object */
		void Destroy();
	};
	/** Map of light type -> LightTypeAssets to render that light */
	std::unordered_map<int, LightTypeAssets> m_lightRenderingAssets;

	/** Initializes point lights assets */
	WError _LoadPointLightsAssets();
	/** Initializes spot light assets */
	WError _LoadSpotLightsAssets();
	/** Initializes directional lights assets */
	WError _LoadDirectionalLightsAssets();

	/**
	 * Callback called whenever a light is added/removed from the lights manager
	 */
	void OnLightsChange(class WLight* light, bool is_added);

public:
	WLightBufferRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter) = 0;
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};

