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
#include "../Materials/WMaterialsStore.h"

/**
 * @ingroup engineclass
 * This represents a terrain object and is responsible for rendering it.
 */
class WTerrain : public WBase, public WOrientation, public WMaterialsStore {
	struct InstanceData {
		WVector2 offset;
	};

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
	 * @param N        Dimension (in number of vertices) of each block in the
	 *                 terrain, must be a power of 2 greater than 1
	 * @param size     Size of each square in the highest resolution block, must be
	 *                 greater than 0
	 * @param numRings Number of LOD rings drawn around the origin
	 * @return Error code, see WError.h
	 */
	WError Create(unsigned int N = 256, float size = 1.0f, unsigned int numRings = 7);

	/**
	 * Sets the point around which the terrain is loaded. The terrain will not
	 * immediately load around the point.
	 * @param point Point to load the terrain around
	 */
	void SetViewpoint(WVector3 point);

	/**
	 * Checks whether a call to Render() will cause any rendering (draw call) to
	 * happen.
	 */
	bool WillRender(class WRenderTarget* rt);

	/**
	 * Renders the terrain to the given render target.
	 * @params rt      Render target to render to
	 * @param material Material to use for rendering
	 */
	void Render(class WRenderTarget* rt, WMaterial* material);

	/**
	 * Retrieves the height of the terrain at a given point on the xz plain
	 * @param pos2D x and z coordinates to retrieve the height at
	 * @return      the height at pos2D
	 */
	float GetHeight(WVector2 pos2D);

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
	/** N and M of the clipmap */
	int m_N, m_M;
	/** Terrain size multiplier */
	float m_size;
	/** Number of rings to draw */
	int m_LOD;
	/** Viewpoint to load terrain around */
	WVector3 m_viewpoint;

	class WGeometry* m_MxMGeometry;
	class WGeometry* m_Mx3Geometry;
	class WGeometry* m_2Mp1Geometry;
	class WImage* m_instanceTexture;
	class WImage* m_heightTexture;

	struct LODRing;
	struct RingPiece {
		struct LODRing* ring;
		WGeometry* geometry;
		WVector2 offsetFromCenter;
		float orientation;
		WVector3 maxPoint, minPoint;

		RingPiece(struct LODRing* r, WGeometry* g, float o, WVector2 off) {
			geometry = g;
			ring = r;
			orientation = o;
			offsetFromCenter = off;
		}
	};

	struct LODRing {
		int level;
		WVector2 center;
		std::vector<RingPiece*> pieces;

		bool Update(WVector3 viewpoint);
	};
	std::vector<LODRing*> m_rings;
	std::map<WGeometry*, std::vector<RingPiece*>> m_pieces;

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
	 * Initializes the terrain.
	 * @param N        Dimension (in number of vertices) of each block in the
	 *                 terrain, must be a power of 2 greater than 1
	 * @param size     Size of each square in the highest resolution block, must be
	 *                 greater than 0
	 * @param numRings Number of LOD rings drawn around the origin
	 * @param ID       ID of the newly created terrain
	 * @return Newly allocated terrain, or nullptr on failure
	 */
	WTerrain* CreateTerrain(unsigned int N = 256, float size = 1.0f, unsigned int numRings = 7, unsigned int ID = 0);

private:
};
