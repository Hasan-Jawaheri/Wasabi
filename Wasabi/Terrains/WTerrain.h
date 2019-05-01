/** @file WTerrain.h
 *  @brief Terrain system for Wasabi
 *
 *  Terrain systems offer an efficient way to render large landscapes.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 * This represents a terrain object and is responsible for rendering it.
 */
class WTerrain : public WBase, public WOrientation {
protected:
	virtual ~WTerrain();

public:
	/**
	 * Returns "Terrain" string.
	 * @return Returns "Terrain" string
	 */
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();

	WTerrain(class Wasabi* const app, unsigned int ID = 0);

	/**
	 * Initializes the terrain.
	 * @param N    Dimension (in number of vertices) of each block in the
	 *             terrain, must be a power of 2 greater than 1
	 * @param size Size of each square in the highest resolution block, must be
	 *             greater than 0
	 * @return Error code, see WError.h
	 */
	WError Create(unsigned int N = 256, float size = 1.0f);

	/**
	 * Renders the terrain to the given render target.
	 * @params rt  Render target to render to
	 */
	void Render(class WRenderTarget* rt);

	/**
	 * Shows the terrain so that it can be rendered.
	 */
	void Show();

	/**
	 * Hides the terrain.
	 */
	void Hide();

	/**
	 * Checks if the terrain hidden.
	 * @return true if the terrain is hidden, false otherwise
	 */
	bool Hidden() const;

	/**
	 * Retrieves the world matrix for the terrain.
	 * @return World matrix for the terrain
	 */
	WMatrix GetWorldMatrix();

	/**
	 * Updates the world matrix of the terrain.
	 * @return true if the matrix has changed, false otherwise
	 */
	bool UpdateLocals();

	/**
	 * @return Whether or not the terrain will render properly
	 */
	virtual bool Valid() const;

	virtual void OnStateChange(STATE_CHANGE_TYPE type);

private:
	/** true if the world matrix needs to be updated, false otherwise */
	bool m_bAltered;
	/** true if the terrain is hidden, false otherwise */
	bool m_hidden;
	/** Local world matrix */
	WMatrix m_WorldM;

	class WGeometry* m_blockGeometry;

	/**
	 * Destroys all resources held by this terrain
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Manager class for WTerrain.
 */
class WTerrainManager : public WManager<WTerrain> {
	friend class WTerrain;

	/**
	 * Returns "Terrain" string.
	 * @return Returns "Terrain" string
	 */
	virtual std::string GetTypeName() const;

public:
	WTerrainManager(class Wasabi* const app);
	~WTerrainManager();

	/**
	 * Loads the manager.
	 * @return Error code, see WError.h
	 */
	WError Load();

	/**
	 * Renders all terrains registered by this manager (by calling
	 * their WTerrain::Render() function).
	 * @param rt Render target to render to
	 */
	void Render(class WRenderTarget* rt);

private:
};
