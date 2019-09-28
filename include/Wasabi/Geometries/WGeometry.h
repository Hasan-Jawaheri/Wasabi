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

#include "Wasabi/Core/WCore.h"

#define W_ATTRIBUTE_POSITION	W_VERTEX_ATTRIBUTE("position", 3)
#define W_ATTRIBUTE_TANGENT		W_VERTEX_ATTRIBUTE("tangent", 3)
#define W_ATTRIBUTE_NORMAL		W_VERTEX_ATTRIBUTE("normal", 3)
#define W_ATTRIBUTE_UV			W_VERTEX_ATTRIBUTE("uv", 2)
#define W_ATTRIBUTE_TEX_INDEX	W_VERTEX_ATTRIBUTE("texture_index", 1)
#define W_ATTRIBUTE_BONE_INDEX	W_VERTEX_ATTRIBUTE("bone_index", 4)
#define W_ATTRIBUTE_BONE_WEIGHT	W_VERTEX_ATTRIBUTE("bone_weight", 4)

/**
 * @ingroup engineclass
 *
 * Represents a single vertex attribute.
 */
struct W_VERTEX_ATTRIBUTE {
	W_VERTEX_ATTRIBUTE() : numComponents(0) {}
	W_VERTEX_ATTRIBUTE(std::string n, unsigned char s)
		: name(n), numComponents(s) {}

	/** Attribute name */
	std::string name;
	/** Number of components of the attribute */
	unsigned char numComponents;
};

/**
 * @ingroup engineclass
 *
 * Represents the description of a vertex. A vertex is described by a list of
 * its attributes.
 */
struct W_VERTEX_DESCRIPTION {
	W_VERTEX_DESCRIPTION(std::vector<W_VERTEX_ATTRIBUTE> attribs = {})
		: attributes(attribs), _size((size_t)-1) {}

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
	 * @return             The offset of the attribute at attrib_indexm -1 if
	 *                     it cannot be found
	 */
	size_t GetOffset(uint32_t attrib_index) const;

	/**
	 * Retrieves the offset (in bytes) to a certain attribute.
	 * @param  attrib_name Name of the attribute to get its offset
	 * @return             The offset of the attribute at attrib_indexm -1 if
	 *                     it cannot be found
	 */
	size_t GetOffset(std::string attrib_name) const;

	/**
	 * Find the index of an attribute given its name.
	 * @param  attrib_name Attribute name to look for
	 * @return             Index of the attribute whose name is attrib_name, -1
	 *                     if it cannot be found
	 */
	uint32_t GetIndex(std::string attrib_name) const;

	/**
	 * Checks if this vertex description is equal to another one
	 * @param other  Other vertex description to compare against
	 * @return       true if both descriptions are the same, false otherwise
	 */
	bool isEqualTo(W_VERTEX_DESCRIPTION other) const;
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
					float u, float v, uint32_t t = 0)
					: pos(x, y, z), norm(nx, ny, nz), tang(tx, ty, tz), texC(u, v), textureIndex(t) {
	};

	/** Position attribute */
	WVector3 pos;
	/** Tangent attribute */
	WVector3 tang;
	/** Normal attribute */
	WVector3 norm;
	/** Texture coordinate attribute (UV) */
	WVector2 texC;
	/** Index of the texture to be used for this vertex */
	uint32_t textureIndex;
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

enum W_GEOMETRY_CREATE_FLAGS: uint32_t {
	W_GEOMETRY_CREATE_VB_CPU_READABLE = 1,
	W_GEOMETRY_CREATE_VB_DYNAMIC = 2,
	W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME = 4,
	W_GEOMETRY_CREATE_IB_CPU_READABLE = 8,
	W_GEOMETRY_CREATE_IB_DYNAMIC = 16,
	W_GEOMETRY_CREATE_IB_REWRITE_EVERY_FRAME = 32,
	W_GEOMETRY_CREATE_AB_CPU_READABLE = 64,
	W_GEOMETRY_CREATE_AB_DYNAMIC = 128,
	W_GEOMETRY_CREATE_AB_REWRITE_EVERY_FRAME = 256,

	W_GEOMETRY_CREATE_STATIC = 0,
	W_GEOMETRY_CREATE_CPU_READABLE = 1 | 8 | 64,
	W_GEOMETRY_CREATE_DYNAMIC = 2 | 16 | 128,
	W_GEOMETRY_CREATE_REWRITE_EVERY_FRAME = 4 | 32 | 256,

	W_GEOMETRY_CREATE_CALCULATE_NORMALS = 512,
	W_GEOMETRY_CREATE_CALCULATE_TANGENTS = 1024,
};

inline W_GEOMETRY_CREATE_FLAGS operator | (W_GEOMETRY_CREATE_FLAGS lhs, W_GEOMETRY_CREATE_FLAGS rhs) {
	using T = std::underlying_type_t <W_GEOMETRY_CREATE_FLAGS>;
	return static_cast<W_GEOMETRY_CREATE_FLAGS>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline W_GEOMETRY_CREATE_FLAGS operator & (W_GEOMETRY_CREATE_FLAGS lhs, W_GEOMETRY_CREATE_FLAGS rhs) {
	using T = std::underlying_type_t <W_GEOMETRY_CREATE_FLAGS>;
	return static_cast<W_GEOMETRY_CREATE_FLAGS>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline W_GEOMETRY_CREATE_FLAGS& operator |= (W_GEOMETRY_CREATE_FLAGS& lhs, W_GEOMETRY_CREATE_FLAGS rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline W_GEOMETRY_CREATE_FLAGS& operator &= (W_GEOMETRY_CREATE_FLAGS& lhs, W_GEOMETRY_CREATE_FLAGS rhs) {
	lhs = lhs & rhs;
	return lhs;
}

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
class WGeometry : public WFileAsset {
	friend class WGeometryManager;

protected:
	virtual ~WGeometry();

public:
	/**
	 * Returns "Geometry" string.
	 * @return Returns "Geometry" string
	 */
	virtual std::string GetTypeName() const;
	static std::string _GetTypeName();

	WGeometry(Wasabi* const app, uint32_t ID = 0);

	/**
	 * This function should return the number of vertex descriptions that
	 * GetVertexDescription() can return, which represents the number of vertex
	 * buffers this geometry class can hold.
	 * @return The number of vertex buffers this geometry can hold
	 */
	virtual uint32_t GetVertexBufferCount() const {
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
	 * @param  layoutIndex  Index of the vertex buffer
	 * @return              The description of a vertex in the <a>layout_index
	 *                      </a>'th vertex buffer
	 */
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(
		uint32_t layoutIndex = 0) const;

	/**
	* Retrieves the size of the vertex description at the given index.
	*
	* @param  layoutIndex  Index of the vertex buffer
	* @return              The size of the vertex description at the given index
	*/
	virtual size_t GetVertexDescriptionSize(uint32_t layoutIndex = 0) const;

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
	 * uint32_t indices[] = {0, 1, 2};
	 * WGeomery* geometry = GeometryManager->CreateFromData(vertices, 3, indices, 3);
	 * @endcode
	 * 
	 * @param  vb            A pointer to the memory containing the vertex data,
	 *                       which must be a valid contiguous memory of size
	 *                       <a>num_verts*GetVertexDescription(0).GetSize()</a>
	 * @param  numVerts      Number of vertices in vb
	 * @param  ib            A pointer to the memory containing the index data,
	 *                       which must be a valid contiguous memory of size
	 *                       <a>num_indices*sizeof(uint)</a>
	 * @param  numIndices    Number of indices in ab
	 * @param  flags         Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return               Error code, see WError.h
	 */
	WError CreateFromData(void* vb, uint32_t numVerts, void* ib, uint32_t numIndices, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Called by CreateBox, CreateSphere, etc... to convert the given default vertices
	 * (of type WDefaultVertex) to the custom type for this geometry. This can be
	 * overrided to utilize the Create* functions while using a custom vertex format
	 */
	virtual WError CreateFromDefaultVerticesData(vector<WDefaultVertex>& default_vertices, vector<uint32_t>& indices, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

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
	 * @param  size    Dimension of the cube
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CreateCube(float size, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Creates a box geometry.
	 * 
	 * @param  dimensions Dimensions of the box
	 * @param  flags      Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return            Error code, see WError.h
	 */
	WError CreateBox(WVector3 dimensions, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Creates a plain geometry that is segmented. The plain will be split into
	 * (xsegs+1)*(zsegs+1) squares, which will have 2 triangles each.
	 * 
	 * @param  size    Size (dimension) of the plain
	 * @param  xsegs    Number of segments on the X axis, minimum is 0
	 * @param  zsegs    Number of segments on the Z axis, minimum is 0
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CreatePlain(float size, int xsegs, int zsegs, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Creates a plain geometry that is segmented. The plain will be split into
	 * (xsegs+1)*(zsegs+1) squares, which will have 2 triangles each.
	 *
	 * @param  sizeX    Size (dimension) of the plain on the X axis
	 * @param  sizeZ    Size (dimension) of the plain on the Z axis
	 * @param  xsegs    Number of segments on the X axis, minimum is 0
	 * @param  zsegs    Number of segments on the Z axis, minimum is 0
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CreateRectanglePlain(float sizeX, float sizeZ, int xsegs, int zsegs, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Creates a sphere geometry, with VRes vertical segments and URes horizontal
	 * segments.
	 * 
	 * @param  radius   Radius of the sphere
	 * @param  vres     Vertical resolution, or number of vertical splits,
	 *                  minimum is 3
	 * @param  ures     Horizontal resolution, or number of horizontal splits,
	 *                  minimum is 2
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CreateSphere(float radius, uint32_t vres = 12, uint32_t ures = 12, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Creates a cone geometry, with csegs segments at the bottom circle and
	 * hsegs segments along the cone's height.
	 * 
	 * @param  radius   Radius of the bottom circle
	 * @param  height   Height of the cone
	 * @param  hsegs    Number of segments along the height, minimum is 0
	 * @param  csegs    Number of segments at the bottom circle, minimum is 3
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CreateCone(float radius, float height, uint32_t hsegs, uint32_t csegs, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Creates a cylinder geometry, with csegs segments at the bottom and top
	 * circles and hsegs segments along the cylinder's height.
	 *
	 * @param  radius   Radius of the cylinder
	 * @param  height   Height of the cylinder
	 * @param  hsegs    Number of segments along the height, minimum is 0
	 * @param  csegs    Number of segments at the bottom and top circles,
	 *                  minimum is 3
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CreateCylinder(float radius, float height, uint32_t hsegs, uint32_t csegs, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

	/**
	 * Copy another (non-immutable) geometry.
	 * 
	 * @param  from     Geometry to copy
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError CopyFrom(WGeometry* const from, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_VB_CPU_READABLE | W_GEOMETRY_CREATE_IB_CPU_READABLE);

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
	 * @param  flags   Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return         Error code, see WError.h
	 */
	WError CreateAnimationData(void* animBuf, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_STATIC);

	/**
	 * Load a geometry from an HXM file. The geometry in the HXM file may be of
	 * a different format, in which case this function will try to fill in the
	 * attributes that the two formats share, discarding the rest. If no normal
	 * or tangent data are found in the file, but are available in this
	 * geometry's description, the function will attempt to fill them in
	 * automatically.
	 * @param  filename Name of the file to load
	 * @param  flags    Creation flags, see W_GEOMETRY_CREATE_FLAGS
	 * @return          Error code, see WError.h
	 */
	WError LoadFromHXM(std::string filename, W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);

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
	 * @param  flags     Map flags (bitwise OR'd), specifying read/write intention
	 * @return           Error code, see WError.h
	 */
	WError MapVertexBuffer(void** const vb, W_MAP_FLAGS mapFlags);

	/**
	 * Map the index buffer of this geometry. This will fail if there is no
	 * geometry, the geometry is dynamic or if the geometry is immutable. Indices
	 * are stored in 32-bits-per-index (uint32_t).
	 *
	 * Examples:
	 * Flip this first triangle
	 * @code
	 * // Assuming geometry is valid and dynamic
	 * uint* indices;
	 * geometry->MapIndexBuffer((void**)&indices);
	 * uint32_t temp = indices[0];
	 * indices[0] = indices[2];
	 * indices[2] = temp;
	 * geometry->UnmapIndexBuffer();
	 * @endcode
	 * 
	 * @param  ib        The address of a pointer to have it point to the mapped
	 *                   memory of the indices
	 * @param  flags     Map flags (bitwise OR'd), specifying read/write intention
	 * @return           Error code, see WError.h
	 */
	WError MapIndexBuffer(void** const ib, W_MAP_FLAGS mapFlags);

	/**
	 * Map the animation buffer of this geometry. This will fail if there is no
	 * animation data, the geometry is dynamic or if the geometry is immutable.
	 * @param  ab        The address of a pointer to have it point to the mapped
	 *                   memory of the animation vertices
	 * @param  flags     Map flags (bitwise OR'd), specifying read/write intention
	 * @return           Error code, see WError.h
	 */
	WError MapAnimationBuffer(void** const ab, W_MAP_FLAGS mapFlags);

	/**
	 * Unmap vertices from a previous MapVertexBuffer() call. If
	 * MapVertexBuffer() was called with bReadOnly == false, the this will apply
	 * the changes to the geometry.
	 * @param recalculateBoundingBox Whether or not to recalculate the geometry's
	 *                               bounding box information
	 */
	void UnmapVertexBuffer(bool recalculateBoundingBox = true);

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
	bool Intersect(WVector3 p1, WVector3 p2, WVector3* pt = nullptr, WVector2* uv = nullptr, uint32_t* triangleIndex = nullptr);

	/**
	 * Draw the geometry to the render target. This function will bind the
	 * geometry buffer to vertex buffer slot 0 in Vulkan, and will bind the
	 * animation buffer (if available and requested) to slot 1. The render
	 * target must have its Begin() function called before this function is
	 * called.
	 * @param  rt             Render target to draw to
	 * @param  numIndices     Number of indices to draw, -1 for all. If the
	 *                        geometry has no indices, this is the number of
	 *                        vertices to draw, -1 for all
	 * @param  numInstances   Number of instances to draw
	 * @param  bindAnimation  true to bind the animation buffer (if not
	 *                        available, the geometry buffer will be bound
	 *                        twice), false otherwise
	 * @return                [description]
	 */
	WError Draw(class WRenderTarget* rt, uint32_t numIndices = -1, uint32_t numInstances = 1, bool bindAnimation = true);

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
	uint32_t GetNumVertices() const;

	/**
	 * Retrieves the number of indices in the geometry.
	 * @return Number of indices
	 */
	uint32_t GetNumIndices() const;

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

	static std::vector<void*> LoadArgs(W_GEOMETRY_CREATE_FLAGS flags = W_GEOMETRY_CREATE_CPU_READABLE);
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix);

private:
	/** Vertex buffer */
	WBufferedBuffer m_vertices;
	/** Index buffer */
	WBufferedBuffer m_indices;
	/** Animation vertex buffer */
	WBufferedBuffer m_animationbuf;
	/** Number of vertices */
	uint32_t m_numVertices;
	/** Number of indices */
	uint32_t m_numIndices;
	/** Currently mapped vertex buffer (only valid if mapped for writing), used to recalculate min/max points */
	void* m_mappedVertexBufferForWrite;
	/** An array of buffered maps to perform, one per buffered buffer */
	std::map<WBufferedBuffer*, std::vector<void*>> m_pendingBufferedMaps;

	/** Maximum boundary */
	WVector3 m_maxPt;
	/** Minimum boundary */
	WVector3 m_minPt;

	/**
	 * Destroys all the geometry resources.
	 */
	void _DestroyResources();

	/**
	 * Calculates m_minPt.
	 * @param vb       Vertex buffer to calculate from
	 * @param numVerts Number of vertices in vb
	 */
	void _CalcMinMax(void* vb, uint32_t numVerts);

	/**
	 * Calculates the vertex normals in vb and stores them in vb (if possible).
	 * @param vb          Vertex buffer to calculate normals for
	 * @param numVerts    Number of vertices in vb
	 * @param ib          Index buffer for vb
	 * @param numIndices  Number of indices in ib
	 */
	void _CalcNormals(void* vb, uint32_t numVerts, void* ib, uint32_t numIndices);

	/**
	 * Calculates the vertex tangents in vb and stores them in vb (if possible).
	 * @param vb        Vertex buffer to calculate tangents for
	 * @param numVerts  Number of vertices in vb
	 */
	void _CalcTangents(void* vb, uint32_t numVerts);

	/**
	 * Performs all pending maps for the given buffer index
	 */
	void _PerformPendingMaps(uint32_t bufferIndex);

	/**
	 * Performs an update necessary to a buffered buffer at a given index after it gets mapped
	 */
	void _UpdatePendingMap(WBufferedBuffer* buffer, void* mappedData, uint32_t bufferIndex, W_MAP_FLAGS mapFlags);

	/**
	 * Performs an update necessary to a buffered buffer at a given index before it gets unmapped
	 */
	void _UpdatePendingUnmap(WBufferedBuffer* buffer, uint32_t bufferIndex);
};

/**
 * @ingroup engineclass
 * Manager class for WGeometry.
 */
class WGeometryManager : public WManager<WGeometry> {
	friend class WGeometry;

	/** A container of the dynamic geometries that need to be (possibly) updated
		per-frame for buffered mapping/unmapping */
	std::unordered_map<WGeometry*, bool> m_dynamicGeometries;

	/**
	 * Returns "Geometry" string.
	 * @return Returns "Geometry" string
	 */
	virtual std::string GetTypeName() const;

public:
	WGeometryManager(class Wasabi* const app);
	~WGeometryManager();

	/**
	 * Makes sure all Map call results are propagated to the buffered geometries at
	 * the given buffer index.
	 */
	void UpdateDynamicGeometries(uint32_t bufferIndex) const;
};

