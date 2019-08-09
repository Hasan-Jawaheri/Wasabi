/** @file WText.h
 *  @brief Text rendering implementation
 *
 *  Text rendering is Wasabi is done by using a 3rd party library called stb.
 *  stb is able to load a font (.ttf) file and create a bitmap of the
 *  characters of that font, with metadata about each character. With the
 *  information from stb, One can easily render text by rendering a quad for
 *  each letter, texturing it with part of the font bitmap (by specifying
 *  the uv coordinates for that letter).
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Core/WCore.h"

/** Represents a text that is to be rendered next frame */
typedef struct W_RENDERING_TEXT {
	/** String to render */
	std::string str;
	/** Color of the text */
	WColor col;
	/** Height of the text */
	float fHeight;
	/** x-coordinate to render at (in screen-space) */
	float x;
	/** y-coordinate to render at (in screen-space) */
	float y;
} W_RENDERING_TEXT;

/**
 * Represents a font created by WTextComponent. A font also tracks the
 * texts that use it for the next frame.
 */
typedef struct W_FONT_OBJECT {
	/** ID of the font */
	unsigned int ID;
	/** Name of the font, as loaded */
	std::string name;
	/** stb library data */
	void* cdata;
	/** Number of characters loaded by stb */
	int num_chars;
	/** Height of each character, as created */
	int char_height;
	/** Geometry to use to render all the texts of this font */
	class WGeometry* textGeometry;
	/** Material to use to render the geometry */
	class WMaterial* textMaterial;
	/** The bitmap created by stb */
	class WImage* img;
	/** A list of texts using this font that will be rendered next frame */
	vector<W_RENDERING_TEXT> texts;
} W_FONT_OBJECT;

/**
 * @ingroup engineclass
 *
 * A WTextComponent provides an interface to allow text rendering.
 */
class WTextComponent {
public:
	WTextComponent(class Wasabi* app);
	~WTextComponent();

	/**
	 * Adds a directory from which fonts can be loaded.
	 * @param dir New directory to load fonts from
	 */
	void AddFontDirectory(std::string dir);

	/**
	 * Initializes the text component and all its resources.
	 * @return Error code, see WError.h
	 */
	WError Initialize();

	/**
	 * Creates a new font.
	 * @param  ID       ID to assign the font to. The ID must be unique
	 * @param  fontName Filename of the font (.ttf file), which can be relative
	 *                  to any of the added font directories (see
	 *                  AddFontDirectory())
	 * @return          Error code, see WError.h
	 */
	WError CreateTextFont(unsigned int ID, std::string fontName);

	/**
	 * Destroys a font.
	 * @param  ID ID of the font to destroy
	 * @return    Error code, see WError.h
	 */
	WError DestroyFont(unsigned int ID);

	/**
	 * Sets the default color for the following text renders (if not specified in
	 * the render call).
	 * @param  col New color to set
	 * @return     Error code, see WError.h
	 */
	WError SetTextColor(WColor col);

	/**
	 * Sets the default font ID for the following text renders (if not specified
	 * in the render call).
	 * @param  ID ID of the font to set default
	 * @return    Error code, see WError.h
	 */
	WError SetFont(unsigned int ID);

	/**
	 * Adds text to be rendered on the next frame.
	 * @param  text    Text to render
	 * @param  x       X-coordinate to render at
	 * @param  y       Y-coordinate to render at
	 * @param  fHeight Height of a text character
	 * @return         Error code, see WError.h
	 */
	WError RenderText(std::string text, float x, float y, float fHeight);

	/**
	 * Adds text to be rendered on the next frame.
	 * @param  text    Text to render
	 * @param  x       X-coordinate to render at
	 * @param  y       Y-coordinate to render at
	 * @param  fHeight Height of a text character
	 * @param  fontID  ID of the font to use
	 * @return         Error code, see WError.h
	 */
	WError RenderText(std::string text, float x, float y, float fHeight, unsigned int fontID);

	/**
	 * Adds text to be rendered on the next frame.
	 * @param  text    Text to render
	 * @param  x       X-coordinate to render at
	 * @param  y       Y-coordinate to render at
	 * @param  fHeight Height of a text character
	 * @param  fontID  ID of the font to use
	 * @param  col     Color to use for rendering
	 * @return         Error code, see WError.h
	 */
	WError RenderText(std::string text, float x, float y, float fHeight, unsigned int fontID, WColor col);

	/**
	 * Renders all text onto the render target.
	 * @param rt Render target to render to
	 */
	void Render(class WRenderTarget* rt);

	/**
	 * Retrieves the width that would be occupied by a text if it was to render.
	 * @param  text    Text to check its width
	 * @param  fHeight Height that will be used for the render call
	 * @param  fontID  Font that will be used for the render call, -1 for default
	 * @return         Width, in pixels, of the text if it was to render using
	 *                 the specified parameters
	 */
	float GetTextWidth(std::string text, float fHeight, unsigned int fontID = 0);

protected:
	/** Pointer to the Wasabi class */
	class Wasabi* m_app;
	/** List of directories in which fonts could be found */
	std::vector<std::string> m_directories;
	/** A hashtable for the fonts, mapping ID -> font */
	std::map<unsigned int, W_FONT_OBJECT> m_fonts;
	/** Current (default) font ID */
	unsigned int m_curFont;
	/** Current (Default) color */
	WColor m_curColor;
	/** Effect to use for rendering texts */
	class WEffect* m_textEffect;
};
