/** @file WImage.h
 *  @brief Rendering materials implementation
 *
 *  Materials are the way Wasabi passes parameters to effects and their
 *  shaders. Materials provide a simple, convenient interface to assign
 *  textures and shader UBO variables to shaders.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "../Core/WCore.h"

/**
 * @ingroup engineclass
 *
 * A WMaterial contains the information needed to pass shader parameters to
 * WEffects and their shaders. Parameters include all types of variables and
 * textures. WMaterials provide a convenient interface for setting values
 * without having to deal with memory mapping between the host and the client
 * (The RAM and the GPU memory). An effect (WEffect) cannot be used without a
 * material, and thus object rendering requires a material (See
 * WObject::SetMaterial()).
 */
class WMaterial : public WBase {
	/**
	 * Returns "Material" string.
	 * @return Returns "Material" string
	 */
	virtual std::string GetTypeName() const;

public:
	WMaterial(class Wasabi* const app, unsigned int ID = 0);
	~WMaterial();

	/**
	 * Sets the effect that this material will use.
	 * @param  effect Effect to use
	 * @return        Error code, see WError.h
	 */
	WError SetEffect(class WEffect* const effect);

	/**
	 * Binds the material and its effect to the graphics pipeline. A child class
	 * may choose to override this to change the binding procedure, or apply a
	 * certain operation upon binding. The render target must have its Begin()
	 * function called before this function is called.
	 * @param  rt                 Render target whose pipeline is being used
	 * @param  num_vertex_buffers Number of vertex buffers that the material
	 *                            needs to render, valid values are either 0 if
	 *                            the effect uses no vertex buffers, or 1 to the
	 *                            number of input layouts in the effect
	 * @return                    Error code, see WError.h
	 */
	virtual WError Bind(class WRenderTarget* rt,
						unsigned int num_vertex_buffers = -1);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is a float. If multiple variables have the same name, they
	 * will all be set.
	 * @param  varName Name of the variable to set
	 * @param  fVal    Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableFloat(const char* varName, float fVal);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is an array of floats. If multiple variables have the same
	 * name, they will all be set.
	 * @param  varName      Name of the variable to set
	 * @param  fArr         Address of the array to set
	 * @param  num_elements Number of elements in fArr
	 * @return              Error code, see WError.h
	 */
	WError SetVariableFloatArray(const char* varName, float* fArr, int num_elements);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is an integer. If multiple variables have the same name,
	 * they will all be set.
	 * @param  varName Name of the variable to set
	 * @param  iVal    Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableInt(const char* varName, int iVal);
	
	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is an array of integers. If multiple variables have the
	 * same name, they will all be set.
	 * @param  varName      Name of the variable to set
	 * @param  iArr         Address of the array to set
	 * @param  num_elements Number of elements in fArr
	 * @return              Error code, see WError.h
	 */
	WError SetVariableIntArray(const char* varName, int* iArr, int num_elements);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is a matrix. If multiple variables have the same name, they
	 * will all be set.
	 * @param  varName Name of the variable to set
	 * @param  mtx     Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableMatrix(const char* varName, WMatrix mtx);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is a 2D vector. If multiple variables have the same name,
	 * they will all be set.
	 * @param  varName Name of the variable to set
	 * @param  vec     Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableVector2(const char* varName, WVector2 vec);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is a 3D vector. If multiple variables have the same name,
	 * they will all be set.
	 * @param  varName Name of the variable to set
	 * @param  vec     Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableVector3(const char* varName, WVector3 vec);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is a 4D vector. If multiple variables have the same name,
	 * they will all be set.
	 * @param  varName Name of the variable to set
	 * @param  vec     Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableVector4(const char* varName, WVector4 vec);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is a color. If multiple variables have the same name,
	 * they will all be set.
	 * @param  varName Name of the variable to set
	 * @param  col     Value to set
	 * @return         Error code, see WError.h
	 */
	WError SetVariableColor(const char* varName, WColor col);

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type can be anything. The set variable's size must match the len
	 * parameter for successful setting. If multiple variables have the same
	 * name, they will all be set.
	 * @param  varName Name of the variable to set
	 * @param  data    Address of the memory to set the variable's data to
	 * @param  len     Length of data, in bytes
	 * @return         Error code, see WError.h
	 */
	WError SetVariableData(const char* varName, void* data, int len);

	/**
	 * Sets a texture in the bound effect.
	 * @param  binding_index The binding index of the texture
	 * @param  img           The image to set the texture to, can be nullptr
	 * @return               Error code, see WError.h
	 */
	WError SetTexture(int binding_index, class WImage* img);

	/**
	 * Sets the texture that the effect marks as the animation texture, if it
	 * exists.
	 * @param  img The image to set the texture to, can be nullptr
	 * @return     Error code, see WError.h
	 */
	WError SetAnimationTexture(class WImage* img);

	/**
	 * Sets the texture that the effect marks as the instancing texture, if it
	 * exists.
	 * @param  img The image to set the texture to, can be nullptr
	 * @return     Error code, see WError.h
	 */
	WError SetInstancingTexture(class WImage* img);

	/**
	 * Retrieves the bound effect.
	 * @return The bound effect
	 */
	class WEffect* GetEffect() const;

	/**
	 * Checks the validity of the material. A material is valid if it has a
	 * valid effect assigned to it.
	 * @return true of the material is valid, false otherwise
	 */
	virtual bool Valid() const;

private:
	/** The Vulkan descriptor pool used for the descriptor set */
	VkDescriptorPool m_descriptorPool;
	/** The Vulkan descriptor set object */
	VkDescriptorSet m_descriptorSet;
	/** An array to hold write descriptor sets (filled/initialized on every
	    Bind() call) */
	vector<VkWriteDescriptorSet> m_writeDescriptorSets;

	struct UNIFORM_BUFFER_INFO {
		/** Uniform buffer handle */
		VkBuffer buffer;
		/** Memory backing buffer */
		VkDeviceMemory memory;
		/** Buffer descriptor information */
		VkDescriptorBufferInfo descriptor;
		/** Pointer to the ubo description in the effect */
		struct W_BOUND_RESOURCE* ubo_info;
	};
	/** List of the uniform buffers for the effect */
	vector<UNIFORM_BUFFER_INFO> m_uniformBuffers;

	struct SAMPLER_INFO {
		/** Descriptor information for the texture */
		VkDescriptorImageInfo descriptor;
		/** Image backing the resource */
		class WImage* img;
		/** Pointer to the texture description in the effect */
		struct W_BOUND_RESOURCE* sampler_info;
	};
	/** List of all textures (or samplers) for the effect */
	vector<SAMPLER_INFO> m_sampler_info;
	/** The bound effect */
	class WEffect* m_effect;

	/**
	 * Frees all resources allocated for the material.
	 */
	void _DestroyResources();
};

/**
 * @ingroup engineclass
 * Manager class for WMaterial.
 */
class WMaterialManager : public WManager<WMaterial> {
	friend class WMaterial;

	/**
	 * Returns "Material" string.
	 * @return Returns "Material" string
	 */
	virtual std::string GetTypeName() const;

public:
	WMaterialManager(class Wasabi* const app);
};
