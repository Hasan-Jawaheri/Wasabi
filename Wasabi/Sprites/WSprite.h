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

/**
 * @ingroup engineclass
 *
 * Implementation of a sprite. A sprite provides an interface to control the
 * position, orientation, and other properties for an image to be drawn on
 * screen.
 */
class WSprite : public WBase {
	/**
	 * Returns "Sprite" string.
	 * @return Returns "Sprite" string
	 */
	virtual std::string GetTypeName() const;

	/** True if the last call to Load() failed */
	bool m_lastLoadFailed;
	/** true if the sprite is hidden (will no render), false otherwise */
	bool m_hidden;
	/** Texture of the sprite */
	class WImage* m_img;
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
	/** Alpha (transparency) of the sprite, 0 being fully transparent and 1 being
	    fully opaque */
	float m_alpha;
	/** true if position/size/etc... changed and geometry needs to be rebuilt */
	bool m_geometry_changed;

	/** Rendering material */
	class WMaterial* m_material;
	/** Rendering geometry */
	class WGeometry* m_geometry;

public:
	WSprite(Wasabi* const app, unsigned int ID = 0);
	~WSprite();

	/**
	 * Loads the sprite's resources (material and geometry). If material is
	 * already set (via SetMaterial()) then it will not be changed.
	 * @param bCreateMaterial  If set to true, the sprite will create a
	 *					       material for itself. Otherwise none will
	 *                         be created and the sprite will depend on user
	 *                         to set one before rendering
	 * @return Error code, see WError.h
	 */
	WError Load(bool bCreateMaterial = true);

	/**
	 * Sets the texture of the sprite.
	 * @param img Texture to use
	 */
	void SetImage(class WImage* img);

	/**
	 * Sets a custom material to be used for rendering the sprite. The material's
	 * effect should use a vertex shader with the following input layout:
	 * Layout:
	 * - W_VERTEX_ATTRIBUTE("pos2", 2)
	 * - W_ATTRIBUTE_UV
	 * The default vertex shader can be used from
	 * WSpriteManager::GetSpriteVertexShader().
	 * The input geometry is 2 triangles with 4 vertices.
	 *
	 * @param mat Material to set
	 */
	void SetMaterial(class WMaterial* mat);

	/**
	 * Sets the position of the sprite in screen-space.
	 * @param x X-coordinate in screen-space
	 * @param y Y-coordinate in screen-space
	 */
	void SetPosition(float x, float y);

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
	 * @param sizeX Width of the sprite, in pixels
	 * @param sizeY Height of the sprite, in pixels
	 */
	void SetSize(float sizeX, float sizeY);

	/**
	 * Sets the size of the sprite.
	 * @param size Width and height of the sprite, in pixels
	 */
	void SetSize(WVector2 size);

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
	 * Sets the alpha value for the sprite. Alpha represents the transparency of
	 * the sprite, 0 being fully transparent and 1 being fully opaque.
	 * @param fAlpha New alpha value for the sprite
	 */
	void SetAlpha(float fAlpha);

	/**
	 * Renders the sprite. The sprite will not render if its hidden or invalid
	 * (see Valid()).
	 * Material settings during render:
	 * - Variable "alpha" will be set to the sprite's alpha
	 * - Texture slot 1 will be set to the sprite's current texture
	 * @param rt Render target to render to
	 */
	void Render(class WRenderTarget* rt);

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
	 * Retrieves the x-position of the sprite.
	 * @return X-position of the sprite, in screen-space
	 */
	float GetPositionX() const;

	/**
	 * Retrieves the y-position of the sprite.
	 * @return Y-position of the sprite, in screen-space
	 */
	float GetPositionY() const;

	/**
	 * Retrieves the position of the sprite.
	 * @return XY-coordinates of the sprite, in screen-space
	 */
	WVector2 GetPosition() const;

	/**
	 * Retrieves the width of the sprite.
	 * @return Width of the sprite, in pixels
	 */
	float GetSizeX() const;

	/**
	 * Retrieves the height of the sprite.
	 * @return Height of the sprite, in pixels
	 */
	float GetSizeY() const;

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

	/** Geometry used to render full-screen sprites */
	class WGeometry* m_spriteFullscreenGeometry;
	/** Vertex shader used by all sprites */
	class WShader* m_spriteVertexShader;
	/** Default pixel shader used by sprites */
	class WShader* m_spritePixelShader;

	class WGeometry* CreateSpriteGeometry() const;

public:
	WSpriteManager(class Wasabi* const app);
	~WSpriteManager();

	/**
	 * Loads the manager and creates the default geometry and material.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Renders the sprites to a render target.
	 * @param rt Render target to render to
	 */
	void Render(class WRenderTarget* rt);

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
