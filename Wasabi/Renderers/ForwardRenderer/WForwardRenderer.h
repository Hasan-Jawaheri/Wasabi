/** @file WForwardRenderer.h
 *  @brief Implementation of forward rendering
 *
 *  Forward rendering is the simplest rendering technique in graphics. Forward
 *  rendering renders every object (or terrain, particles, etc...) straight to
 *  the backbuffer, once, in one pass. Forward rendering is usually fast as
 *  it has a low overhead of rendering.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../../Core/WCore.h"
#include "../WRenderer.h"
#include "../../Materials/WMaterial.h"

/**
 * @ingroup engineclass
 *
 * Implementation of a forward renderer. Forward rendering is the simplest
 * rendering technique in graphics. Forward rendering renders every object
 * (or terrain, particles, etc...) straight to the backbuffer, once, in one
 * pass. Forward rendering is usually fast as it has a low overhead of
 * rendering.
 * Creating this component adds the following engine parameters:
 * * "maxLights": Maximum number of lights that can be rendered at once (Default is (void*)8)
 *
 * To set the engine's renderer, override Wasabi::CreateRenderer() and return
 * and instance of WForwardRenderer:
 * @code
 * Renderer = new WForwardRenderer(this);
 * @endcode
 */
class WForwardRenderer : public WRenderer {
	friend class WFRMaterial;

public:
	WForwardRenderer(class Wasabi* const app);

	virtual WError Initiailize();
	virtual WError LoadDependantResources();
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);

	virtual VkSampler GetTextureSampler(W_TEXTURE_SAMPLER_TYPE type = TEXTURE_SAMPLER_DEFAULT) const;

private:
	/** Default Vulkan sampler */
	VkSampler m_sampler;
	/** Lights descriptions that will be passed on to the shaders */
	struct LightStruct* m_lights;
	/** Number of lights in m_lights */
	int m_numLights;
};
