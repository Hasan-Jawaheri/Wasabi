/** @file WEffect.h
 *  @brief Shaders and rendering effects implementation
 *
 *  An effect in Wasabi is a collection of shaders and rendering states (
 *  components of a Vulkan pipeline). An effect controls the way geometry is
 *  drawn on the screen.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 * Type of a shader.
 */
enum W_SHADER_TYPE {
	/** Vertex shader */
	W_VERTEX_SHADER = VK_SHADER_STAGE_VERTEX_BIT,
	/** Pixel (or fragment) shader */
	W_FRAGMENT_SHADER = VK_SHADER_STAGE_FRAGMENT_BIT,
	/** Pixel (or fragment) shader */
	W_PIXEL_SHADER = VK_SHADER_STAGE_FRAGMENT_BIT,
	/** Geometry shader */
	W_GEOMETRY_SHADER = VK_SHADER_STAGE_GEOMETRY_BIT,
};

/**
 * @ingroup engineclass
 * Type of a shader variable (or attribute).
 */
enum W_SHADER_VARIABLE_TYPE {
	/** A floating-point type */
	W_TYPE_FLOAT = 0,
	/** An integer type */
	W_TYPE_INT = 1,
	/** An unsigned int type */
	W_TYPE_UINT = 2,
	/** A half float */
	W_TYPE_HALF = 3,
};

/**
 * @ingroup engineclass
 * Type of a bound resource to a shader.
 */
enum W_SHADER_BOUND_RESOURCE_TYPE {
	/** Bound resource is a UBO */
	W_TYPE_UBO = 0,
	/** Bound resource is a combined sampler (texture) */
	W_TYPE_SAMPLER = 1,
};

/**
 * @ingroup engineclass
 * Rate at which a vertex buffer is passed to the vertex shader
 */
enum W_VERTEX_INPUT_RATE {
	/** A unit of the buffer (of a size provided by the stride given to the draw
			call) will be passed to the vertex shader for every vertex passed. */
	W_INPUT_RATE_PER_VERTEX = 0,
	/** A unit of the buffer (of a size provided by the stride given to the draw
			call) will be passed to the vertex shader for every instance passed. */
	W_INPUT_RATE_PER_INSTANCE = 1,
};

/**
 * @ingroup engineclass
 * Description of a shader variable in a UBO or vertex attribute.
 */
typedef struct W_SHADER_VARIABLE_INFO {
	W_SHADER_VARIABLE_INFO(
		W_SHADER_VARIABLE_TYPE _type,
		int _num_elems,
		std::string _name = ""
	);

	/** Type of the variable */
	W_SHADER_VARIABLE_TYPE type;
	/** Number of elements in the variable (vec2 has 2 elements, etc...) */
	int num_elems;
	/** Given name for the variable, which is used by materials to set its
	 *  value */
	std::string name;
	/** Cached size of the variable */
	size_t _size;

	/**
	 * Retrieves the size of the variable, in bytes. This is calculated as the
	 * size of type multiplied by num_elems.
	 * @return Size, in bytes, of this shader variable
	 */
	size_t GetSize() const;

	/**
	 * Returns the alignment of this variable, according to Vulkan:
	 * - A scalar of size N has a base alignment of N
	 * - A two-component vector, with components of size N, has a base
	 *   alignment of 2 N
	 * - A three- or four-component vector, with components of size N, has a
	 *   base alignment of 4 N
	 * - An array has a base alignment equal to the base alignment of its
	 *   element type, rounded up to a multiple of 16
	 * - A structure has a base alignment equal to the largest base alignment
	 *   of any of its members, rounded up to a multiple of 16
	 * - A row-major matrix of C columns has a base alignment equal to the
	 *   base alignment of a vector of C matrix components
	 * - A column-major matrix has a base alignment equal to the base alignment
	 *   of the matrix column type
	 * @return Alignment of this variable
	 */
	size_t GetAlignment() const;

	/**
	 * Retrieves the Vulkan format corresponding to this variable.
	 * @return Format of the the variable
	 */
	VkFormat GetFormat() const;
} W_SHADER_VARIABLE_INFO;

/**
 * @ingroup engineclass
 * Description of a resource bound to a shader. A resource can be anything
 * listed by W_SHADER_BOUND_RESOURCE_TYPE. An example of a bound resource is a
 * UBO.
 */
typedef struct W_BOUND_RESOURCE {
	W_BOUND_RESOURCE(
		W_SHADER_BOUND_RESOURCE_TYPE t,
		unsigned int index,
		std::vector<W_SHADER_VARIABLE_INFO> v =
			std::vector<W_SHADER_VARIABLE_INFO>()
	);

	/** Type of this resource */
	W_SHADER_BOUND_RESOURCE_TYPE type;
	/** Index in the shader at which the resource is bound */
	unsigned int binding_index;
	/** Variables of this resource (in case of a UBO), which is empty for
		textures */
	std::vector<W_SHADER_VARIABLE_INFO> variables;
	/** Cached size of the variables, after automatically padding variables
	    to be 16-byte-aligned */
	size_t _size;
	/** Aligned offsets of variables elements in the UBO */
	std::vector<size_t> _offsets;

	/**
	 * Retrieves the total size, in bytes, of the variables it contains. This
	 * is not relevant for texture resources. The size aligns variables to 16
	 * bytes (as required by Vulkan)
	 * @return Size of all variables, in bytes
	 */
	size_t GetSize() const;

	/**
	 * Retrieves the (aligned) offset of a given variable
	 * @param variable_index  Index into the variables array for which to get
	 *                        the offset
	 * @return                Aligned offset of the given variable in the UBO
	 */
	size_t OffsetAtVariable(unsigned int variable_index) const;

	/**
	 * Checks if this resource has the same layout as another resource.
	 * @param resource  The resource to check against
	 * @return          True iff this resource and the one provided have the
	 *                  same binding index and type and variables
	 */
	bool IsSimilarTo(W_BOUND_RESOURCE resource);
} W_BOUND_RESOURCE;

/**
 * @ingroup engineclass
 * Description of an input layout for a shader. An input layout describes how
 * a vertex buffer should look like to be compatible with a shader. A vertex
 * buffer bound to the pipeline has to have the same layout as described by
 * the attributes per instance or per vertex, depending on the rate specified.
 */
typedef struct W_INPUT_LAYOUT {
	W_INPUT_LAYOUT(std::vector<W_SHADER_VARIABLE_INFO> a,
				   W_VERTEX_INPUT_RATE r = W_INPUT_RATE_PER_VERTEX)
		: attributes(a), input_rate(r), _size(-1) {
	}
	W_INPUT_LAYOUT() : attributes({}), _size(-1) {}

	/** Attributes of a single "vertex" (or instance, depending on input_rate) */
	std::vector<W_SHADER_VARIABLE_INFO> attributes;
	/** Rate at which memory is read from the vertex buffer to use as attributes
	 */
	W_VERTEX_INPUT_RATE input_rate;
	/** Cached size of the input layout */
	mutable size_t _size;

	/**
	 * Retrieves the size (or stride), in bytes, of this input layout. The size
	 * is calculated as the sum of the sizes of the attributes.
	 * @return The size (or stride), in bytes, of this input layout
	 */
	size_t GetSize() const;
} W_INPUT_LAYOUT;

/**
 * @ingroup engineclass
 * Description of a shader to be bound to an effect.
 */
typedef struct W_SHADER_DESC {
	W_SHADER_DESC()
		: animation_texture_index(-1), instancing_texture_index(-1) {
	}
	/** Type of the shader */
	W_SHADER_TYPE type;
	/** A list of bound resources (including UBOs and samplers/textures) */
	std::vector<W_BOUND_RESOURCE> bound_resources;
	/** A list of input layouts that bind to the shader. There should be one
			vertex buffer present to bind to every input layout in the shader */
	std::vector<W_INPUT_LAYOUT> input_layouts;
	/** Binding index of a texture (which must exist in bound_resources) that
			has been specifically marked to be used for animation */
	unsigned int animation_texture_index;
	/** Binding index of a texture (which must exist in bound_resources) that
			has been specifically marked to be used for instancing */
	unsigned int instancing_texture_index;
} W_SHADER_DESC;

/**
 * @ingroup engineclass
 * Encapsulation of a shader object. A shader is a small program bound to
 * the graphics (or compute) Vulkan pipelines that does its work on the GPU.
 *
 * To create a WShader, one must define a child class which implements
 * WShader::Load(). The load function needs to fill in the m_desc and m_module
 * protected variables. m_module is automatically filled by a call to one of
 * the LoadCode* functions (which needs to be called during the Load() call).
 * The m_desc structure should be initialized to describe the shader and the
 * resources that can/need to be bound to it.
 *
 * Examples:
 * 
 * Creating a simple vertex shader type that is compatible with the default
 * vertex layout of Wasabi (see WDefaultVertex).
 * @code
 * class SimpleVS : public WShader {
 * public:
 * 	SimpleVS(class Wasabi* const app) : WShader(app) {}
 *
 * 	// We need to implement Load(), which loads the shader.
 * 	virtual void Load() {
 * 		m_desc.type = W_VERTEX_SHADER; // Creating a vertex shader
 * 		m_desc.bound_resources = {
 * 			// We have 1 UBO, bound to "location = 0" (in the shader code below)
 * 			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
 * 				// projection matrix
 * 				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gProjection"),
 * 				// world matrix
 * 				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gWorld"),
 * 				// view matrix
 * 				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4 * 4, "gView"),
 * 			}),
 * 		};
 * 		// We have 1 per-vertex input layout
 * 		// This layout corresponds to the layout of vertices in WDefaultVertex
 * 		// which WGeometry uses by default.
 * 		m_desc.input_layouts = {W_INPUT_LAYOUT({
 * 			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // position
 * 			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // tangent
 * 			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 3), // normal
 * 			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
 * 		})};
 * 		LoadCodeGLSL(
 * 			"#version 450\n"
 * 			""
 * 			"#extension GL_ARB_separate_shader_objects : enable\n"
 * 			"#extension GL_ARB_shading_language_420pack : enable\n"
 * 			""
 * 			"layout(location = 0) in vec3 inPos;\n"
 * 			"layout(location = 1) in vec3 inTang;\n"
 * 			"layout(location = 2) in vec3 inNorm;\n"
 * 			"layout(location = 3) in vec2 inUV;\n"
 * 			""
 * 			"layout(binding = 0) uniform UBO {\n"
 * 			"	mat4 projectionMatrix;\n"
 * 			"	mat4 modelMatrix;\n"
 * 			"	mat4 viewMatrix;\n"
 * 			"} ubo;\n"
 * 			""
 * 			"layout(location = 0) out vec2 outUV;\n"
 * 			""
 * 			"void main() {\n"
 * 			"	outUV = inUV;\n"
 * 			"	gl_Position = ubo.projectionMatrix * \n"
 * 			"               ubo.viewMatrix * \n"
 * 			"               ubo.modelMatrix * \n"
 * 			"               vec4(outWorldPos, 1.0);\n"
 * 			"}\n"
 * 		);
 * 	}
 * };
 * @endcode
 *
 * Simple pixel (or fragment) shader, compatible with the above vertex shader:
 * @code
 * class SimplePS : public WShader {
 * public:
 * 	SimplePS(class Wasabi* const app) : WShader(app) {}
 * 
 * 	virtual void Load() {
 * 		m_desc.type = W_FRAGMENT_SHADER; // Fragment (or pixel) shader
 * 		m_desc.bound_resources = {}; // no bound resources
 * 		LoadCodeGLSL(
 * 			"#version 450\n"
 * 			""
 * 			"#extension GL_ARB_separate_shader_objects : enable\n"
 * 			"#extension GL_ARB_shading_language_420pack : enable\n"
 * 			"" // The UV coordinates are passed in from the vertex shader
 * 			"layout(location = 0) in vec2 inUV;\n"
 * 			"" // We need to output a color using this variable in main()
 * 			"layout(location = 0) out vec4 outFragColor;\n"
 * 			""
 * 			"void main() {\n"
 * 			"		outFragColor = vec4(inUV.x, inUV.y, 0, 1);\n"
 * 			"}\n"
 * 		);
 * 	}
 * };
 * @endcode
 *
 * Using the above shaders to create an effect:
 * @code
 * WShader* vs = new SimpleVS(this);
 * vs->Load();
 * WShader* ps = new SimplePS(this);
 * ps->Load();
 * WEffect* FX = new WEffect(this);
 * FX->BindShader(vs);
 * FX->BindShader(ps);
 * FX->BuildPipeline(Renderer->GetDefaultRenderTarget());
 * vs->RemoveReference();
 * ps->RemoveReference();
 * @endcode
 */
class WShader : public WBase {
	friend class WEffect;
	friend class WMaterial;

	/**
	 * Returns "Shader" string.
	 * @return Returns "Shader" string
	 */
	virtual std::string GetTypeName() const;

protected:
	/** Shader description */
	W_SHADER_DESC m_desc;
	/** Compiled shader code object */
	VkShaderModule m_module;

	/**
	 * Loads SPIR-V formatted shader code from memory. Loaded code will be
	 * compiled into m_module.
	 * @param code Address of the SPIR-V code in memory
	 * @param len  Length of the code, in bytes
	 */
	void LoadCodeSPIRV(const char* const code, int len);

	/**
	 * Loads GLSL shader code from a string. Loaded code will be compiled into
	 * m_module.
	 * @param code String containing GLSL code
	 */
	void LoadCodeGLSL(std::string code);

	/**
	 * Loads SPIR-V formatted shader code from a file. Loaded code will be
	 * compiled into m_module.
	 * @param filename Name of the file to load the code from
	 */
	void LoadCodeSPIRVFromFile(std::string filename);

	/**
	 * Loads GLSL shader code from file. Loaded code will be compiled into
	 * m_module.
	 * @param filename Name of the file to load the code from
	 */
	void LoadCodeGLSLFromFile(std::string filename);

public:
	WShader(class Wasabi* const app, unsigned int ID = 0);
	~WShader();

	/**
	 * Loads the shader into this object. This function must be implemented by a
	 * child class. The implemented function must fill in m_module and m_desc
	 * protected members to fully define the shader. See WShader for example
	 * usage.
	 */
	virtual void Load() = 0;

	/**
	 * Checks the validity of the shader. A shader is valid if m_module is
	 * set.
	 * @return true if the shader is valid, false otherwise
	 */
	virtual bool Valid() const;
};

/**
 * @ingroup engineclass
 * Manager class for WShader.
 */
class WShaderManager : public WManager<WShader> {
	friend class WShader;

	/**
	 * Returns "Shader" string.
	 * @return Returns "Shader" string
	 */
	virtual std::string GetTypeName() const;

public:
	WShaderManager(class Wasabi* const app);
};

/**
 * @ingroup engineclass
 * A WEffect is a container for shaders, rendering states, and the Vulkan
 * graphics (or compute) pipeline. An effect specifies how rendering of
 * geometry is to be done. More generally, an effect controls all stages of a
 * Vulkan pipeline.
 *
 * Example:
 * @code
 * WShader* vs = new MyVertexShader(m_app);
 * vs->Load();
 * WShader* ps = new MyPixelShader(m_app);
 * ps->Load();
 * WEffect* myFX = new WEffect(m_app);
 * myFX->BindShader(vs);
 * myFX->BindShader(ps);
 * myFX->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());
 * vs->RemoveReference();
 * ps->RemoveReference();
 * @endcode
 */
class WEffect : public WBase {
	friend class WMaterial;

	/**
	 * Returns "Effect" string.
	 * @return Returns "Effect" string
	 */
	virtual std::string GetTypeName() const;

public:
	WEffect(class Wasabi* const app, unsigned int ID = 0);
	~WEffect();

	/**
	 * Binds a shader to this effect.
	 * @param  shader New shader to bind
	 * @return        Error code, see WError.h
	 */
	WError BindShader(WShader* shader);

	/**
	 * Unbinds a shader from this effect.
	 * @param  type The type of the shader to unbind
	 * @return      Error code, see WError.h
	 */
	WError UnbindShader(W_SHADER_TYPE type);

	/**
	 * Sets the primitive topology in the Vulkan pipeline. This needs to be
	 * called before BuildPipeline() for changes to be effective.
	 * @param topology The new Vulkan primitive topology
	 */
	void SetPrimitiveTopology(VkPrimitiveTopology topology);

	/**
	 * Sets the blending state in the Vulkan pipeline. This needs to be called
	 * before BuildPipeline() for changes to be effective.
	 * @param state The new Vulkan Blend state
	 */
	void SetBlendingState(VkPipelineColorBlendAttachmentState state);

	/**
	 * Sets multiple blending states in the Vulkan pipeline. Multiple blend states
	 * are used when the render target has multiple output attachments. This needs
	 * to be called before BuildPipeline() for changes to be effective.
	 * @param state The new Vulkan Blend state
	 */
	void SetBlendingStates(vector<VkPipelineColorBlendAttachmentState> states);

	/**
	 * Sets the depth stencil state in the Vulkan pipeline. This needs to be
	 * called before BuildPipeline() for changes to be effective.
	 * @param state The new Vulkan depth stencil state
	 */
	void SetDepthStencilState(VkPipelineDepthStencilStateCreateInfo state);

	/**
	 * Sets the rasterization state in the Vulkan pipeline. This needs to be
	 * called before BuildPipeline() for changes to be effective.
	 * @param state The new Vulkan rasterization state
	 */
	void SetRasterizationState(VkPipelineRasterizationStateCreateInfo state);

	/**
	 * Builds Vulkan pipelines corresponding to the currently bound shaders and
	 * Vulkan states. This function will build several pipelines for different
	 * numbers of input layout supplied by the shaders. For instance, a shader
	 * with two input layouts will have two pipelines, one that only uses one
	 * input layout and another that uses both. This is done to provide
	 * convenience when one wishes to use the same effect without supplying all
	 * required vertex shaders.
	 * @param  rt Render target that the effect plans on rendering to
	 * @return    Error code, see WError.h
	 */
	WError BuildPipeline(class WRenderTarget* rt);

	/**
	 * Binds the effect to render command buffer of the specified render target.
	 * The render target must have its Begin() function called before this
	 * function is called.
	 * @param  rt                 Render target to bind to its command buffer
	 * @param  num_vertex_buffers Number of vertex buffers that the effect
	 *                            should expect to be bound on the pipeline,
	 *                            value values are between 1 and the number of
	 *                            input layouts supplied from the shaders (or 0
	 *                            if no input layouts are present)
	 * @return                    Error code, see WError.h
	 */
	WError Bind(class WRenderTarget* rt, unsigned int num_vertex_buffers = -1);
	
	/**
	 * Retrieves the layout of the pipelines created by this effect.
	 * @return The Vulkan pipeline layout for the effect's pipelines
	 */
	VkPipelineLayout* GetPipelineLayout();

	/**
	 * Retrieves the Vulkan descriptor set layout.
	 * @return The Vulkan descriptor set layout
	 */
	VkDescriptorSetLayout* GetDescriptorSetLayout();

	/**
	 * Retrieves an input layout supplied by one of the bound shaders.
	 * @param  layout_index Index of the layout requested
	 * @return              The input layout at layout_index supplied by bound
	 *                      shaders
	 */
	W_INPUT_LAYOUT GetInputLayout(unsigned int layout_index = 0) const;

	/**
	 * Retrieves the size of the input layout at the given index.
	 * @param  layout_index Index of the layout requested
	 * @return              The size of the input layout at the given index
	 */
	size_t GetInputLayoutSize(unsigned int layout_index = 0) const;

	/**
	 * Checks the validity of the effect. An effect is valid if it has at least
	 * one pipeline created and has a bound vertex shader that supplies a valid
	 * input layout.
	 * @return true if the effect is valid, false otherwise
	 */
	virtual bool Valid() const;

private:
	/** List of pipelines created for this effect */
	std::vector<VkPipeline> m_pipelines;
	/** List of bound shaders */
	std::vector<WShader*> m_shaders;
	/** Index of the bound vertex shader (-1 if none is bound) */
	unsigned int m_vertexShaderIndex;
	/** Vulkan pipeline layout used for the pipelines creation */
	VkPipelineLayout m_pipelineLayout;
	/** Descriptor set layout that can be used to make descriptor sets */
	VkDescriptorSetLayout m_descriptorSetLayout;

	/** Vulkan primitive topology to use for the next pipeline generation */
	VkPrimitiveTopology m_topology;
	/** Vulkan blend state to use for the next pipeline generation */
	vector<VkPipelineColorBlendAttachmentState> m_blendStates;
	/** Vulkan depth stencil state to use for the next pipeline generation */
	VkPipelineDepthStencilStateCreateInfo m_depthStencilState;
	/** Vulkan rasterization state to use for the next pipeline generation */
	VkPipelineRasterizationStateCreateInfo m_rasterizationState;

	/**
	 * Frees all resources allocated by the effect.
	 */
	void _DestroyPipeline();

	/**
	 * Checks the validity of the bound shaders. The bound shaders are valid if
	 * they contain at least one vertex buffer with at least one valid input
	 * layout.
	 * @return true if the bound shaders are valid, false otherwise
	 */
	bool _ValidShaders() const;
};

/**
 * @ingroup engineclass
 * Manager class for WEffect.
 */
class WEffectManager : public WManager<WEffect> {
	friend class WEffect;

	/**
	 * Returns "Effect" string.
	 * @return Returns "Effect" string
	 */
	virtual std::string GetTypeName() const;

public:
	WEffectManager(class Wasabi* const app);
};

