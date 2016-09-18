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

#include "../Core/WCore.h"
#include "WRenderer.h"
#include "../Materials/WMaterial.h"

/**
 * @ingroup engineclass
 *
 * Implementation of a forward renderer. Forward rendering is the simplest
 * rendering technique in graphics. Forward rendering renders every object
 * (or terrain, particles, etc...) straight to the backbuffer, once, in one
 * pass. Forward rendering is usually fast as it has a low overhead of
 * rendering.
 *
 * To set the engine's renderer, override Wasabi::SetupComponents() and add
 * this line:
 * @code
 * Renderer = new WForwardRenderer(this);
 * @endcode
 */
class WForwardRenderer : public WRenderer {
	friend class WFRMaterial; // TODO: can be removed

public:
	WForwardRenderer(class Wasabi* const app);

	/**
	 * Initializes the forward renderer by creating the default effect/material,
	 * default sampler and setting up lights.
	 * @return Error code, see WError.h
	 */
	virtual WError Initiailize();

	/**
	 * Renders the scene using the forward rendering technique.
	 * @param rt     Render target to render the scene to
	 * @param filter A filter, represented by a bitfield whose bits are any
	 *               combination of W_RENDER_FILTER flags
	 */
	virtual void Render(class WRenderTarget* rt, unsigned int filter = -1);

	/**
	 * Frees all allocated resources.
	 */
	virtual void Cleanup();

	virtual WError Resize(unsigned int width, unsigned int height);

	/**
	 * Creates a forward rendering material. This material simply renders
	 * objects in their final form to the screen.  The returned material should
	 * be freed (via WMaterial::RemoveReference()) by the caller.
	 * @return Newly allocated material
	 */
	virtual class WMaterial* CreateDefaultMaterial();

	virtual VkSampler GetDefaultSampler() const;

private:
	/** Default Vulkan sampler */
	VkSampler m_sampler;
	/** Lights descriptions that will be passed on to the shaders */
	struct LightStruct* m_lights;
	/** Number of lights in m_lights */
	int m_numLights;
	/** Default effect to used to create materials */
	class WEffect* m_default_fx;
};

/**
 * @ingroup engineclass
 *
 * A WFRMaterial is a material compatible with the WForwardRenderer. It is
 * intended to be used to render geometry directly in one stage.
 *
 * An object rendered with WFRMaterial can either be textured or rendered with
 * a solid color.
 */
class WFRMaterial : public WMaterial {
public:
	WFRMaterial(class Wasabi* const app, unsigned int ID = 0);

	virtual WError Bind(class WRenderTarget* rt,
						unsigned int num_vertex_buffers = -1);

	/**
	 * Sets the texture of the object rendered with this material. Setting this
	 * will make the object render with the provided texture, rather than a
	 * solid color.
	 * @param  img Texture to use
	 * @return     Error code, see WError.h
	 */
	WError Texture(class WImage* img);

	/**
	 * Sets the solid color used to render objects with this material. Setting
	 * this will make the object render with the flat color provided, rather than
	 * a texture.
	 * @param  col Solid color to use
	 * @return     Error code see WError.h
	 */
	WError SetColor(WColor col);
};
