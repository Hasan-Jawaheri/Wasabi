/** @file WGeometry.h
 *  @brief Geometry (mesh) implementation
 *
 *  A WGeometry is a flexible way to represent any geometry that is needed for
 *  rendering or any general computation or usage. WGeometry can be derived
 *  such that one can modify the composition of the geometry (the attributes).
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

#define W_ATTRIBUTE_POSITION	W_VERTEX_ATTRIBUTE("position", 3)
#define W_ATTRIBUTE_TANGENT		W_VERTEX_ATTRIBUTE("tangent", 3)
#define W_ATTRIBUTE_NORMAL		W_VERTEX_ATTRIBUTE("normal", 3)
#define W_ATTRIBUTE_UV			W_VERTEX_ATTRIBUTE("uv", 2)
#define W_ATTRIBUTE_BONE_INDEX	W_VERTEX_ATTRIBUTE("bone_index", 4)
#define W_ATTRIBUTE_BONE_WEIGHT	W_VERTEX_ATTRIBUTE("bone_weight", 4)

/**
 * @ingroup engineclass
 *
 * Represents a single vertex attribute.
 */
struct W_VERTEX_ATTRIBUTE {
	W_VERTEX_ATTRIBUTE() : num_components(0) {}
	W_VERTEX_ATTRIBUTE(std::string n, unsigned char s)
		: name(n), num_components(s) {}

	/** Attribute name */
	std::string name;
	/** Number of components of the attribute */
	unsigned char num_components;
};

/**
 * @ingroup engineclass
 *
 * Represents the description of a vertex. A vertex is described by a list of
 * its attributes.
 */
struct W_VERTEX_DESCRIPTION {
	W_VERTEX_DESCRIPTION(std::vector<W_VERTEX_ATTRIBUTE> attribs = {})
		: attributes(attribs), _size(-1) {}

	/** A list of attributes for the vertex */
	std::vector<W_VERTEX_ATTRIBUTE> attributes;
	/** Cached size of a vertex */
	mutable size_t _size;

	/**
	 * Retrieves the size (in bytes) of a vertex of this description.
	 * @return Size (in bytes) of a vertex of this description
	 */
	size_t GetSize() const;

	/**
	 * Retrieves the offset (in bytes) to a certain attribute.
	 * @param  attrib_index Index of the attribute to get its offset
	 * @return              The offset of the attribute at attrib_index
	 */
	size_t GetOffset(unsigned int attrib_index) const;

	/**
	 * Retrieves the offset (in bytes) to a certain attribute.
	 * @param  attrib_name Name of the attribute to get its offset
	 * @return             The offset of the attribute at attrib_index
	 */
	size_t GetOffset(std::string attrib_name) const;

	/**
	 * Find the index of an attribute given its name.
	 * @param  attrib_name Attribute name to look for
	 * @return             Index of the attribute whose name is attrib_name, -1
	 *                     if it cannot be found
	 */
	unsigned int GetIndex(std::string attrib_name) const;
};

/**
 * @ingroup engineclass
 *
 * This represents the default vertex structure used by the engine.
 */
struct WDefaultVertex {
	WDefaultVertex() {}
	WDefaultVertex( float x, float y, float z,
					float tx, float ty, float tz,
					float nx, float ny, float nz,
					float u, float v)
					: pos(x, y, z), norm(nx, ny, nz), tang(tx, ty, tz), texC(u, v) {
	};

	/** Position attribute */
	WVector3 pos;
	/** Tangent attribute */
	WVector3 tang;
	/** Normal attribute */
	WVector3 norm;
	/** Texture coordinate attribute (UV) */
	WVector2 texC;
};

/**
 * @ingroup engineclass
 *
 * This represents the default animation vertex used by the engine.
 */
struct WDefaultVertex_Animation {
	WDefaultVertex_Animation() {}

	/** 4 bone IDs that this vertex is bound to */
	int boneIDs[4];
	/** Weight of each bone this vertex is bound to */
	float weights[4];
};

/**
 * @ingroup engineclass
 *
 * A WGeometry is a flexible way to represent any geometry that is needed for
 * rendering or any general computation or usage. WGeometry can be derived
 * such that one can modify the composition of the geometry (the attributes).
 *
 * A geometry can be made immutable, making it use less memory, at the cost
 * of having several functions not work (functions that require access to
 * the geometry's vertex data).
 *
 * WGeometry can hold several vertex buffers. By default, Wasabi uses the
 * first buffer to render (with indices) and uses the second buffer (if
 * available) for animation data.
 */
class WGeometry : public WBase {
	/**
	 * Returns "Geometry" string.
	 * @return Returns "Geometry" string
	 */
	virtual std::string GetTypeName() const;

public:
	WGeometry(Wasabi* const app, unsigned int ID = 0);
	~WGeometry();

	/**
	 * This function should return the number of vertex descriptions that
	 * GetVertexDescription() can return, which represents the number of vertex
	 * buffers this geometry class can hold.
	 * @return The number of vertex buffers this geometry can hold
	 */
	virtual unsigned int GetVertexBufferCount() const {
		return 2; // one vertex buffer, one animation buffer
	}

	/**
	 * Retrieves the vertex description of a vertex buffer. Should return valid
	 * descriptions for all <a>layout_index</a> values between 0 and
	 * GetVertexBufferCount()-1.
	 *
	 * The default vertex description (for default geometry) is:
	 * - layout 0: (vertex buffer)
	 *   - W_ATTRIBUTE_POSITION
	 *   - W_ATTRIBUTE_TANGENT
	 *   - W_ATTRIBUTE_NORMAL
	 *   - W_ATTRIBUTE_UV
	 * - layout 1: (animation buffer)
	 *   - W_ATTRIBUTE_BONE_INDEX
	 *   - W_ATTRIBUTE_BONE_WEIGHT
	 * @param  layout_index Index of the vertex buffer
	 * @return              The description of a vertex in the <a>layout_index
	 *                      </a>'th vertex buffer
	 */
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(
		unsigned int layout_index = 0) const;

	/**
	* Retrieves the size of the vertex description at the given index.
	*
	* @param  layout_index Index of the vertex buffer
	* @return              The size of the vertex description at the given index
	*/
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const;

	/**
	 * Creates a geometry from buffers in memory. The buffers must be formatted
	 * such that they match what GetVertexDescription() returns. This function
	 * fills up only the first (0th) vertex buffer that is accompanied with an
	 * index buffer for rendering use.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 *
	 * Examples:
	 * Create a triangle:
	 * @code
	 * WDefaultVertex vertices[] = {
	 *   WDefaultVertex(-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
	 *                  0.0f, 0.0f, -1.0f, 0.0f, 1.0f),
	 *   WDefaultVertex(-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
	 *                  0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
	 *   WDefaultVertex(0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
	 *                  0.0f, 0.0f, -1.0f, 1.0f, 0.0f)
	 * };
	 * uint indices[] = {0, 1, 2};
	 * WGeomery* geometry = new WGeometry(this);
	 * geometry->CreateFromData(vertices, 3, indices, 3);
	 * @endcode
	 * 
	 * @param  vb            A pointer to the memory containing the vertex data,
	 *                       which must be a valid contiguous memory of size
	 *                       <a>num_verts*GetVertexDescription(0).GetSize()</a>
	 * @param  num_verts     Number of vertices in vb
	 * @param  ib            A pointer to the memory containing the index data,
	 *                       which must be a valid contiguous memory of size
	 *                       <a>num_indices*sizeof(uint)</a>
	 * @param  num_indices   Number of indices in ab
	 * @param  bDynamic      true if the geometry will require frequent
	 *                       modifications, false otherwise
	 * @param  bCalcNormals  Setting this to true will cause this function to
	 *                       automatically calculate normals (if there is an
	 *                       attribute named "normal" in
	 *                       GetVertexDescription(0)) for the geometry
	 *                       (hard-normals)
	 * @param  bCalcTangents Setting this to true will cause this function to
	 *                       automatically calculate tangents (if there is an
	 *                       attribute named "tangent" in
	 *                       GetVertexDescription(0)) for the geometry
	 * @return               Error code, see WError.h
	 */
	WError CreateFromData(void* vb, unsigned int num_verts,
						  void* ib, unsigned int num_indices, bool bDynamic = false,
						  bool bCalcNormals = false, bool bCalcTangents = false);

	/**
	 * Called by CreateBox, CreateSphere, etc... to convert the given default vertices
	 * (of type WDefaultVertex) to the custom type for this geometry. This can be
	 * overrided to utilize the Create* functions while using a custom vertex format
	 */
	virtual WError CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices,
												 vector<uint>& indices, bool bDynamic);

	/**
	 * Creates a cube geometry.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  fSize    Dimension of the cube
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError CreateCube(float fSize, bool bDynamic = false);

	/**
	 * Creates a box geometry.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  dimensions Dimensions of the box
	 * @param  bDynamic   true if the geometry will require frequent
	 *                    modifications, false otherwise
	 * @return            Error code, see WError.h
	 */
	WError CreateBox(WVector3 dimensions, bool bDynamic = false);

	/**
	 * Creates a plain geometry that is segmented. The plain will be split into
	 * (xsegs+1)*(zsegs+1) squares, which will have 2 triangles each.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  fSize    Size (dimension) of the plain
	 * @param  xsegs    Number of segments on the X axis, minimum is 0
	 * @param  zsegs    Number of segments on the Z axis, minimum is 0
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError CreatePlain(float fSize, int xsegs, int zsegs, bool bDynamic = false);

	/**
	 * Creates a sphere geometry, with VRes vertical segments and URes horizontal
	 * segments.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  Radius   Radius of the sphere
	 * @param  VRes     Vertical resolution, or number of vertical splits,
	 *                  minimum is 3
	 * @param  URes     Horizontal resolution, or number of horizontal splits,
	 *                  minimum is 2
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError CreateSphere(float Radius, unsigned int VRes = 12, unsigned int URes = 12,
						bool bDynamic = false);

	/**
	 * Creates a cone geometry, with csegs segments at the bottom circle and
	 * hsegs segments along the cone's height.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  fRadius  Radius of the bottom circle
	 * @param  fHeight  Height of the cone
	 * @param  hsegs    Number of segments along the height, minimum is 0
	 * @param  csegs    Number of segments at the bottom circle, minimum is 3
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError CreateCone(float fRadius, float fHeight, unsigned int hsegs,
					  unsigned int csegs, bool bDynamic = false);

	/**
	 * Creates a cylinder geometry, with csegs segments at the bottom and top
	 * circles and hsegs segments along the cylinder's height.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  fRadius  Radius of the cylinder
	 * @param  fHeight  Height of the cylinder
	 * @param  hsegs    Number of segments along the height, minimum is 0
	 * @param  csegs    Number of segments at the bottom and top circles,
	 *                  minimum is 3
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError CreateCylinder(float fRadius, float fHeight, unsigned int hsegs,
						  unsigned int csegs, bool bDynamic = false);

	/**
	 * Copy another (non-immutable) geometry.
	 *
	 * If this function is called while the engine parameter "geometryImmutable"
	 * (see Wasabi::engineParams) is set to true, then the geometry will be made
	 * immutable, regardless of the bDynamic parameter. Immutable geometry cannot
	 * have the following function calls work: SaveToWGM(), all map and unmap
	 * functions, all scaling functions, all offset functions and Intersect().
	 * Immutable geometry cannot be copied. Immutable geometry uses less memory.
	 * 
	 * @param  from     Geometry to copy
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError CopyFrom(WGeometry* const from, bool bDynamic = false);

	/**
	 * Creates animation data vertex buffer. The geometry must have
	 * GetVertexBufferCount() > 1. The buffer will be dynamic if the first
	 * vertex buffer (created from a Load*, CreateFromData() or CopyFrom() call)
	 * was dynamic, false otherwise. If the geometry of this object is immutable,
	 * the the animation buffer will also be.
	 * 
	 * @param  animBuf A pointer to the memory to create the animation buffer
	 *                 from, which must be a valid contiguous memory of size
	 *                 <a>GetNumVertices()*GetVertexDescription(1).GetSize()</a>
	 * @return         Error code, see WError.h
	 */
	WError CreateAnimationData(void* animBuf);

	/**
	 * Load a geometry from a WGM file. The geometry in the WGM file may be of
	 * a different format, in which case this function will try to fill in the
	 * attributes that the two formats share, discarding the rest. If no normal
	 * or tangent data are found in the file, but are available in this
	 * geometry's description, the function will attempt to fill them in
	 * automatically. The function will also attempt to load animation data, if
	 * available. However, animation data will only be loaded if it matches
	 * exactly with this geometry's animation vertex buffer structure.
	 * @param  filename Name of the file to load
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError LoadFromWGM(std::string filename, bool bDynamic = false);

	/**
	 * Saves this geometry's data in a WGM file. The data will be saved in the
	 * same format of this geometry. Animation data will also be stored.
	 * @param  filename Name of the file
	 * @return          Error code, see WError.h
	 */
	WError SaveToWGM(std::string filename);

	/**
	 * Load a geometry from an HXM file. The geometry in the HXM file may be of
	 * a different format, in which case this function will try to fill in the
	 * attributes that the two formats share, discarding the rest. If no normal
	 * or tangent data are found in the file, but are available in this
	 * geometry's description, the function will attempt to fill them in
	 * automatically.
	 * @param  filename Name of the file to load
	 * @param  bDynamic true if the geometry will require frequent
	 *                  modifications, false otherwise
	 * @return          Error code, see WError.h
	 */
	WError LoadFromHXM(std::string filename, bool bDynamic = false);

	/**
	 * Map the vertex buffer of this geometry. This will fail if there is no
	 * geometry, the geometry is dynamic or if the geometry is immutable.
	 *
	 * Examples:
	 * Move the first vertex a bit
	 * @code
	 * // Assuming geometry is valid and dynamic
	 * WDefaultVertex* vertices;
	 * geometry->MapVertexBuffer((void**)&vertices);
	 * vertices[0].pos.x += 5;
	 * geometry->UnmapVertexBuffer();
	 * @endcode
	 * 
	 * @param  vb        The address of a pointer to have it point to the mapped
	 *                   memory of the vertices
	 * @param  bReadOnly Set to true if you intend to only read from the vertex
	 *                   array, false if you want to modify the vertices
	 * @return           Error code, see WError.h
	 */
	WError MapVertexBuffer(void** const vb, bool bReadOnly = false);

	/**
	 * Map the index buffer of this geometry. This will fail if there is no
	 * geometry, the geometry is dynamic or if the geometry is immutable.
	 *
	 * Examples:
	 * Flip this first triangle
	 * @code
	 * // Assuming geometry is valid and dynamic
	 * uint* indices;
	 * geometry->MapIndexBuffer(&indices);
	 * uint temp = indices[0];
	 * indices[0] = indices[2];
	 * indices[2] = temp;
	 * geometry->UnmapIndexBuffer();
	 * @endcode
	 * 
	 * @param  ib        The address of a pointer to have it point to the mapped
	 *                   memory of the indices
	 * @param  bReadOnly Set to true if you intend to only read from the index
	 *                   array, false if you want to modify the indices
	 * @return           Error code, see WError.h
	 */
	WError MapIndexBuffer(uint** const ib, bool bReadOnly = false);

	/**
	 * Map the animation buffer of this geometry. This will fail if there is no
	 * animation data, the geometry is dynamic or if the geometry is immutable.
	 * @param  ab        The address of a pointer to have it point to the mapped
	 *                   memory of the animation vertices
	 * @param  bReadOnly Set to true if you intend to only read from the
	 *                   animation vertex array, false if you want to modify the
	 *                   animation vertices
	 * @return           Error code, see WError.h
	 */
	WError MapAnimationBuffer(void** const ab, bool bReadOnly = false);

	/**
	 * Unmap vertices from a previous MapVertexBuffer() call. If
	 * MapVertexBuffer() was called with bReadOnly == false, the this will apply
	 * the changes to the geometry.
	 */
	void UnmapVertexBuffer();

	/**
	 * Unmap indices from a previous MapIndexBuffer() call. If MapIndexBuffer()
	 * was called with bReadOnly == false, the this will apply the changes to
	 * the geometry.
	 */
	void UnmapIndexBuffer();

	/**
	 * Unmap animation vertices from a previous MapAnimationBuffer() call. If
	 * MapAnimationBuffer() was called with bReadOnly == false, the this will
	 * apply the changes to the animation data.
	 */
	void UnmapAnimationBuffer();

	/**
	 * Scale the geometry. The geometry must be valid and must have an attribute
	 * in its geometry buffer named "position". The geometry must be dynamic and
	 * not immutable.
	 * @param  mulFactor Scale factor
	 * @return           Error code, see WError.h
	 */
	WError Scale(float mulFactor);

	/**
	 * Scale the geometry on the X axis. The geometry must be valid and must
	 * have an attribute in its geometry buffer named "position". The geometry
	 * must be dynamic and not immutable.
	 * @param  mulFactor X scale factor
	 * @return           Error code, see WError.h
	 */
	WError ScaleX(float mulFactor);

	/**
	 * Scale the geometry on the Y axis. The geometry must be valid and must
	 * have an attribute in its geometry buffer named "position". The geometry
	 * must be dynamic and not immutable.
	 * @param  mulFactor Y scale factor
	 * @return           Error code, see WError.h
	 */
	WError ScaleY(float mulFactor);

	/**
	 * Scale the geometry on the Z axis. The geometry must be valid and must
	 * have an attribute in its geometry buffer named "position". The geometry
	 * must be dynamic and not immutable.
	 * @param  mulFactor Z scale factor
	 * @return           Error code, see WError.h
	 */
	WError ScaleZ(float mulFactor);

	/**
	 * Applies an offset to all vertices in the geometry. The geometry must be
	 * valid and must have an attribute in its geometry buffer named "position".
	 * The geometry must be dynamic and not immutable.
	 * @param  x x offset to apply
	 * @param  y y offset to apply
	 * @param  z z offset to apply
	 * @return   Error code, see WError.h
	 */
	WError ApplyOffset(float x, float y, float z);

	/**
	 * Applies an offset to all vertices in the geometry. The geometry must be
	 * valid and must have an attribute in its geometry buffer named "position".
	 * The geometry must be dynamic and not immutable.
	 * @param  offset Offset to apply
	 * @return   			Error code, see WError.h
	 */
	WError ApplyOffset(WVector3 offset);

	/**
	 * Applies a transformation matrix to all vertices in the geometry. The
	 * geometry must be valid and must have an attribute in its geometry buffer
	 * named "position". The geometry must be dynamic and not immutable.
	 * @param  mtx Transformation matrix to apply
	 * @return     Error code, see WError.h
	 */
	WError ApplyTransformation(WMatrix mtx);

	/**
	 * Checks if a ray intersects the geometry. The geometry must be valid and
	 * must have an attribute in its geometry buffer named "position". The
	 * geometry must be dynamic and not immutable.
	 * @param  p1            Origin of the ray
	 * @param  p2            A vector that points in the same direction as the
	 *                       ray
	 * @param  pt            A pointer to a 3D vector to be populated with the
	 *                       point of intersection, if any
	 * @param  uv            A pointer to a 2D vector to be populated with the
	 *                       UV coordinates at the intersection point, if any.
	 *                       If the vertices don't have a an attribute named uv
	 *                       (with at least two components) then this will be the
	 *                       UV coordinates local to the triangle intersected
	 * @param  triangleIndex A pointer to an integer to be populated with the
	 *                       index of the triangle that was intersected, if any
	 * @return               true if there was an intersection, false otherwise
	 */
	bool Intersect(WVector3 p1, WVector3 p2,
				   WVector3* pt = nullptr, WVector2* uv = nullptr,
				   unsigned int* triangleIndex = nullptr);

	/**
	 * Draw the geometry to the render target. This function will bind the
	 * geometry buffer to vertex buffer slot 0 in Vulkan, and will bind the
	 * animation buffer (if available and requested) to slot 1. The render
	 * target must have its Begin() function called before this function is
	 * called.
	 * @param  rt             Render target to draw to
	 * @param  num_indices    Number of indices to draw, -1 for all
	 * @param  num_instances  Number of instances to draw
	 * @param  bind_animation true to bind the animation buffer (if not
	 *                        available, the geometry buffer will be bound
	 *                        twice), false otherwise
	 * @return                [description]
	 */
	WError Draw(class WRenderTarget* rt, unsigned int num_indices = -1,
				unsigned int num_instances = 1, bool bind_animation = true);

	/**
	 * Retrieves the point that represents the minimum boundary of the geometry.
	 * @return The minimum boundary for the geometry
	 */
	WVector3 GetMinPoint() const;

	/**
	 * Retrieves the point that represents the maximum boundary of the geometry.
	 * @return The maximum boundary for the geometry
	 */
	WVector3 GetMaxPoint() const;

	/**
	 * Retrieves the number of vertices in the geometry.
	 * @return Number of vertices
	 */
	unsigned int GetNumVertices() const;

	/**
	 * Retrieves the number of indices in the geometry.
	 * @return Number of indices
	 */
	unsigned int GetNumIndices() const;

	/**
	 * Checks if the geometry has an animation vertex buffer.
	 * @return true if the geometry has animation data, false otherwise
	 */
	bool IsRigged() const;

	/**
	 * Checks the validity of the geometry. A geometry is valid if it has a
	 * vertex and an index buffer.
	 * @return true if the geometry is valid, false otherwise
	 */
	virtual bool Valid() const;

private:
	struct {
		/** Number of vertices */
		int count;
		/** Vertex buffer */
		W_BUFFER buffer;
		/** Staging vertex buffer, used for dynamic access */
		W_BUFFER staging;
		/** Set to true if the last vertex map operation was read only */
		bool readOnlyMap;
	} m_vertices;

	struct {
		/** Number of indices */
		int count;
		/** Index buffer */
		W_BUFFER buffer;
		/** Staging index buffer, used for dynamic access */
		W_BUFFER staging;
		/** Set to true if the last index map operation was read only */
		bool readOnlyMap;
	} m_indices;

	struct {
		/** Number of vertices */
		int count;
		/** Animation vertex buffer */
		W_BUFFER buffer;
		/** Staging animation vertex buffer, used for dynamic access */
		W_BUFFER staging;
		/** Set to true if the last animation vertex map operation was read only */
		bool readOnlyMap;
	} m_animationbuf;

	/** Maximum boundary */
	WVector3 m_maxPt;
	/** Minimum boundary */
	WVector3 m_minPt;
	/** true if the geometry is dynamic, false otherwise */
	bool m_dynamic;
	/** true if the geometry is immutable, false otherwise */
	bool m_immutable;

	/**
	 * Destroys all the geometry resources.
	 */
	void _DestroyResources();

	/**
	 * Calculates m_minPt.
	 * @param vb       Vertex buffer to calculate from
	 * @param num_vert Number of vertices in vb
	 */
	void _CalcMinMax(void* vb, unsigned int num_vert);

	/**
	 * Calculates the vertex normals in vb and stores them in vb (if possible).
	 * @param vb          Vertex buffer to calculate normals for
	 * @param num_verts   Number of vertices in vb
	 * @param ib          Index buffer for vb
	 * @param num_indices Number of indices in ib
	 */
	void _CalcNormals(void* vb, unsigned int num_verts, void* ib,
					  unsigned int num_indices);

	/**
	 * Calculates the vertex tangents in vb and stores them in vb (if possible).
	 * @param vb        Vertex buffer to calculate tangents for
	 * @param num_verts Number of vertices in vb
	 */
	void _CalcTangents(void* vb, unsigned int num_verts);
};

/**
 * @ingroup engineclass
 * Manager class for WGeometry.
 */
class WGeometryManager : public WManager<WGeometry> {

	/**
	 * Returns "Geometry" string.
	 * @return Returns "Geometry" string
	 */
	virtual std::string GetTypeName() const;

public:
	WGeometryManager(class Wasabi* const app);
	~WGeometryManager();
};

