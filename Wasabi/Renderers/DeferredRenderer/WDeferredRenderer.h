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

#include "../WRenderer.h"
#include "../../Core/WCore.h"

#include <unordered_map>
using std::unordered_map;

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
public:
	WDeferredRenderer(Wasabi* const app);

	virtual WError Initiailize();
	virtual WError LoadDependantResources();
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);

	virtual VkSampler GetTextureSampler(W_TEXTURE_SAMPLER_TYPE type = TEXTURE_SAMPLER_DEFAULT) const;

private:
	/** Default Vulkan sampler */
	VkSampler m_sampler;
};
