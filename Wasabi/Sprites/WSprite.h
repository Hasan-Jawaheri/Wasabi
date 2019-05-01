/** @file WSprite.h
 *  @brief Implementation of sprites
 *
 * 	Sprites are used to render 2D graphics on a screen. A sprite provides an
 * 	interface to control the position and the orientation for an image to be
 * 	rendered.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug Only one sprite will render at a time (because Draw uses one geometry)
 *  @bug Sprite priority is not implemented
 */

#pragma once

#include "../Core/WCore.h"
#include "../Materials/WMaterialsStore.h"
#include "../Materials/WEffect.h"

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

/**
 * @ingroup engineclass
 *
 * Implementation of a sprite. A sprite provides an interface to control the
 * position, orientation, and other properties for an image to be drawn on
 * screen.
 */
class WSprite : public WBase, public WMaterialsStore {
	friend class WSpriteManager;

protected:
	WSprite(Wasabi* const app, unsigned int ID = 0);
	~WSprite();

public:
	/**
	 * Returns "Sprite" string.
	 * @return Returns "Sprite" string
	 */
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();

	/**
	 * Checks whether a call to Render() will cause any rendering (draw call) to
	 * happen.
	 */
	bool WillRender(class WRenderTarget* rt);

	/**
	 * Renders the sprite. The sprite will not render if its hidden or invalid
	 * (see Valid()).
	 * @param rt        Render target to render to
	 * @param material  Material to use to render the sprite
	 */
	void Render(class WRenderTarget* rt);

	/**
	 * Sets the position of the sprite in screen-space.
	 * @param pos 2D coordinates in screen-space
	 */
	void SetPosition(WVector2 pos);

	/**
	 * Moves the sprite forward towards the angle its facing, 0 being to the
	 * right.
	 * @param units Units to move forward by
	 */
	void Move(float units);

	/**
	 * Sets the size of the sprite.
	 * @param size Width and height of the sprite, in pixels
	 */
	void SetSize(WVector2 size);

	/**
	 * Sets the size of the sprite to the size of an image
	 * @param image Image to use its size
	 */
	void SetSize(class WImage* image);

	/**
	 * Sets the angle (clockwise rotation) of the sprite.
	 * @param fAngle Angle of the sprite, in degrees
	 */
	void SetAngle(float fAngle);

	/**
	 * Rotates the sprite in the clockwise direction.
	 * @param fAngle Angle to rotate by
	 */
	void Rotate(float fAngle);

	/**
	 * Sets the center of rotation of the sprite. The default center of rotation
	 * is (0,0) meaning the top-left corner.
	 * @param x The x-coordinate for the center of rotation relative to the
	 *          position of the sprite
	 * @param y The y-coordinate of the center of rotation relative to the
	 *          position of the sprite
	 */
	void SetRotationCenter(float x, float y);

	/**
	 * Sets the center of rotation of the sprite. The default center of rotation
	 * is (0,0) meaning the top-left corner.
	 * @param center The center of rotation relative to the position of the
	 *               sprite
	 */
	void SetRotationCenter(WVector2 center);

	/**
	 * Sets the priority of the sprite. Sprites with higher priority will render
	 * on top of those with lower priority.
	 * @param priority New priority to set
	 */
	void SetPriority(unsigned int priority);

	/**
	 * Show the sprite.
	 */
	void Show();

	/**
	 * Hide the sprite.
	 */
	void Hide();

	/**
	 * Checks if the sprite is hidden.
	 * @return true if the sprite is hidden, false otherwise
	 */
	bool Hidden() const;

	/**
	 * Retrieves the current angle of the sprite.
	 * @return The current angle of the sprite
	 */
	float GetAngle() const;

	/**
	 * Retrieves the position of the sprite.
	 * @return XY-coordinates of the sprite, in screen-space
	 */
	WVector2 GetPosition() const;

	/**
	 * Retrieves the size of the sprite.
	 * @return Width and height of the sprite, in pixels
	 */
	WVector2 GetSize() const;

	/**
	 * Retrieves the current priority of the sprite. See SetPriority().
	 * @return The current priority of the sprite
	 */
	unsigned int GetPriority() const;

	/**
	 * Checks the validity of the sprite. A sprite is valid if it has a texture
	 * and its' size is greater than zero (both width and height).
	 * @return true if the sprite is valid, false otherwise
	 */
	virtual bool Valid() const;

private:
	/** true if the sprite is hidden (will no render), false otherwise */
	bool m_hidden;
	/** Screen-space position of the sprite */
	WVector2 m_pos;
	/** Size of the sprite, in pixels */
	WVector2 m_size;
	/** Rotation center of the sprite relative to top-left corner, in pixels */
	WVector2 m_rotationCenter;
	/** Angle of the sprite, in radians */
	float m_angle;
	/** Priority of the sprite, higher priority sprites render over lower ones */
	unsigned int m_priority;
	/** true if position/size/etc... changed and geometry needs to be rebuilt */
	bool m_geometryChanged;

	/** Rendering geometry */
	class WGeometry* m_geometry;
};

/**
 * @ingroup engineclass
 * Manager class for WSprite.
 */
class WSpriteManager : public WManager<WSprite> {
	friend class WSprite;

	/**
	 * Returns "Sprite" string.
	 * @return Returns "Sprite" string
	 */
	virtual std::string GetTypeName() const;

public:
	WSpriteManager(class Wasabi* const app);
	~WSpriteManager();

	/**
	 * Loads the manager and creates the default geometry and material.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Called when the application window is resized.
	 * @param width  New screen width
	 * @param height New screen height
	 * @return Error code, see WError.h
	 */
	WError Resize(unsigned int width, unsigned int height);

	/**
	 * Allocates and initializes a new sprite. If an image is supplied, it
	 * will be set to the default sprite's material texture "diffuseTexture".
	 * @param img Image to set to the new sprite
	 * @param ID  Id of the new image
	 * @return    New sprite
	 */
	WSprite* CreateSprite(class WImage* img = nullptr, unsigned int ID = 0) const;

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
	 * @param rt  Render target intended for this effect. If this is null,
	 *            the render target will be the backbuffer
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
		class WRenderTarget* rt = nullptr,
		class WShader* ps = nullptr,
		VkPipelineColorBlendAttachmentState bs = {},
		VkPipelineDepthStencilStateCreateInfo dss = {},
		VkPipelineRasterizationStateCreateInfo rs = {}
	) const;

private:
	/** Vertex shader used by all sprites */
	class WShader* m_spriteVertexShader;
	/** Default pixel shader used by sprites */
	class WShader* m_spritePixelShader;
	/** Geometry used to render full-screen sprites */
	class WGeometry* m_spriteFullscreenGeometry;

	class WGeometry* CreateSpriteGeometry() const;
};
