#pragma once

#include "../WRenderStage.h"
#include "../../Materials/WEffect.h"
#include "../../Sprites/WSprite.h"

#include <map>

class WSpriteVS : public WShader {
public:
	WSpriteVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
};

class WSpritePS : public WShader {
public:
	WSpritePS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
};

class WSpritesRenderStage : public WRenderStage {
	/** Vertex shader used by all sprites */
	class WShader* m_spriteVertexShader;
	/** Default pixel shader used by sprites */
	class WShader* m_spritePixelShader;
	/** Default sprite effect */
	class WEffect* m_spriteFX;

	struct SpriteKey {
		uint priority;
		class WEffect* fx;
		class WSprite* sprite;

		SpriteKey(class WSprite* sprite);
		const bool operator< (const SpriteKey& that) const;
	};
	/** Sorted container for all the sprites to be rendered by this stage */
	std::map<SpriteKey, class WSprite*> m_allSprites;

public:
	WSpritesRenderStage(class Wasabi* const app, bool backbuffer = false);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter);
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);

	/**
	 * Retrieves the default vertex shader that sprites use.
	 * @return Default vertex shader that sprites use
	 */
	class WShader* GetSpriteVertexShader() const;

	/**
	 * Retrieves the default pixel shader that sprites can use.
	 * @return Default pixel shader that sprites can use
	 */
	class WShader* GetSpritePixelShader() const;

	/**
	 * Creates a new WEffect using the default sprite vertex shader and a
	 * supplied pixel shader and other states
	 * @param ps  A pixel/fragment shader to use. If none is provided, the
	 *            default pixel shader will be used
	 * @param bs  Blend state to use. If none is provided, the default
	 *            blend state will be used (do not set .colorWriteMask to 0)
	 * @param bs  Depth/stencil state to use. If none is provided, the default
	 *            depth/stencil state will be used
	 * @param bs  Rasterization state to use. If none is provided, the default
	 *            rasterization state will be used
	 * @return    Newly created effect, or nullptr on failure
	 */
	class WEffect* CreateSpriteEffect(
		WShader* ps = nullptr,
		VkPipelineColorBlendAttachmentState bs = {},
		VkPipelineDepthStencilStateCreateInfo dss = {},
		VkPipelineRasterizationStateCreateInfo rs = {}
	) const;
};
