/** @file WDeferredRenderer.h
 *  @brief Implementation of deferred rendering
 *
 *  Deferred rendering is a rendering technique in which objects are not
 *  rendered directly to the intended render target, but are rendered (in
 *  several stages) to off-screen buffers and then finally composed onto the
 *  final destination (render target).
 *
 *  TODO: implement this and document the behavior
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"
#include "WRenderer.h"
#include "../Materials/WMaterial.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Sprites/WSprite.h"

/**
 * @ingroup engineclass
 *
 * Implementation of a deferred renderer.
 *
 * To set the engine's renderer, override Wasabi::SetupComponents() and add
 * this line:
 * @code
 * Renderer = new WDeferredRenderer(this);
 * @endcode
 */
class WDeferredRenderer : public WRenderer {
	/** Represents the GBuffer rendering stage in a WDeferredRenderer */
	class GBufferStage {
		friend class WDeferredRenderer;

		/** Pointer to the owner renderer */
		WDeferredRenderer* m_renderer;
		/** GBuffer to store normals, depth and color */
		WRenderTarget* m_renderTarget;
		/** Depth attachment in the GBuffer */
		WImage* m_depthTarget;
		/** Color attachment in the GBuffer */
		WImage* m_colorTarget;
		/** Normal attachment in the GBuffer */
		WImage* m_normalTarget;

	public:
		GBufferStage(WDeferredRenderer* renderer);
		~GBufferStage();

		WError Initialize(unsigned int width, unsigned int height);
		WError Render(class WCamera* cam);
		void Cleanup();
		WError Resize(unsigned int width, unsigned int height);
	} m_GBuffer;

	/** Represents the LightBuffer rendering stage in a WDeferredRenderer */
	class LightBufferStage {
		friend class WDeferredRenderer;

		/** Pointer to the owner renderer */
		WDeferredRenderer* m_renderer;
		/** LightBuffer to store lighting information */
		WRenderTarget* m_renderTarget;
		/** Light buffer attachment that will hold the rendered lighting output */
		WImage* m_outputTarget;

	public:
		LightBufferStage(WDeferredRenderer* renderer);
		~LightBufferStage();

		WError Initialize(unsigned int width, unsigned int height);
		WError Render(class WCamera* cam);
		void Cleanup();
		WError Resize(unsigned int width, unsigned int height);
	} m_LightBuffer;

	/** Default Vulkan sampler */
	VkSampler m_sampler;
	/** Default effect used to create materials */
	class WEffect* m_default_fx;
	/** Sprite used to render the final composition */
	WSprite* m_masterRenderSprite;
	/** Final composition material */
	WMaterial* m_compositionMaterial;

public:
	WDeferredRenderer(Wasabi* const app);

	virtual WError Initiailize();
	virtual WError LoadDependantResources();
	virtual void Render(class WRenderTarget* rt, unsigned int filter = -1);
	virtual void Cleanup();
	virtual WError Resize(unsigned int width, unsigned int height);

	void RenderLight(class WLight* light);

	virtual class WMaterial* CreateDefaultMaterial();
	virtual VkSampler GetDefaultSampler() const;
};
