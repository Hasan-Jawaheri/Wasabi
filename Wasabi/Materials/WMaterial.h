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
 * (The RAM and the GPU memory). In Vulkan terms, a WMaterial is a descriptor
 * set that is a subset of all descriptors of a WEffect. Each material may
 * have a descriptor set containing all bound resources of a WEffect, or only
 * a subset of them.
 */
class WMaterial : public WBase, public WFileAsset {
	friend class WEffect;
	/**
	 * Returns "Material" string.
	 * @return Returns "Material" string
	 */
	virtual std::string GetTypeName() const;

	WMaterial(class Wasabi* const app, unsigned int ID = 0);
	~WMaterial();

	/**
	 * Builds the material's resources to be used for a certain effect. The
	 * material may be created for only a subset of the bound resources of
	 * the WEffect using the boundResources parameter.
	 * @param  effect      Effect to use
	 * @param  bindingSet  Set index to use from effect
	 * @return             Error code, see WError.h
	 */
	WError CreateForEffect(class WEffect* const effect, uint bindingSet = 0);

public:
	/**
	 * Binds the resources to the pipeline. In Vulkan terms, this binds the descriptor
	 * set associated with this material.
	 * @param  rt  Render target to bind to its command buffer
	 * @return     Error code, see WError.h
	 */
	virtual WError Bind(class WRenderTarget* rt);

	/**
	 * Retrieves the Vulkan descriptor set created by this material.
	 * @return Material's descriptor set
	 */
	VkDescriptorSet GetDescriptorSet() const;

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
	 * If the target variable is a structure, make sure to properly pad the members
	 * as specified by the Vulkan specs. If the target variable is an array of
	 * structures, you should also make sure that the structure is padded at the
	 * end of it. Refer to: https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#interfaces-resources-layout
	 * Wasabi will automatically do the padding for all the data, except when
	 * the user is explicitly setting structures data using this function.
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
	 * Sets a texture in the bound effect.
	 * @param  name  Name of the texture to bind to
	 * @param  img   The image to set the texture to, can be nullptr
	 * @return       Error code, see WError.h
	 */
	WError SetTexture(std::string name, class WImage* img);

	/**
	 * Checks the validity of the material. A material is valid if it has a
	 * valid effect assigned to it.
	 * @return true of the material is valid, false otherwise
	 */
	virtual bool Valid() const;

	virtual WError SaveToStream(class WFile* file, std::ostream& outputStream);
	virtual WError LoadFromStream(class WFile* file, std::istream& inputStream);

private:
	/** Effect that this material is bound to */
	class WEffect* m_effect;
	/** The Vulkan descriptor pool used for the descriptor set */
	VkDescriptorPool m_descriptorPool;
	/** The Vulkan descriptor set object */
	VkDescriptorSet m_descriptorSet;
	/** The set index of m_descriptorSet */
	uint m_setIndex;
	/** An array to hold write descriptor sets (filled/initialized on every
	    Bind() call) */
	std::vector<std::vector<VkWriteDescriptorSet>> m_writeDescriptorSets;

	struct UNIFORM_BUFFER_INFO {
		/** Uniform buffer resource */
		WBufferedBuffer buffer;
		/** Mapped buffer data (we map once, and unmap when material is destroyed) */
		void* mappedBufferData;
		/** Descriptor buffers, one for each buffer in the buffered buffer above */
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
		/** Current data in the material's buffer, before it is copied to GPU memory */
		void* data;
		/** Specifies whether or not data has changed and needs to be copied to GPU memory (one flag per buffer) */
		std::vector<bool> dirty;
		/** Pointer to the ubo description in the effect */
		struct W_BOUND_RESOURCE* ubo_info;
	};
	/** List of the uniform buffers for the effect */
	std::vector<UNIFORM_BUFFER_INFO> m_uniformBuffers;

	struct SAMPLER_INFO {
		/** Descriptor information for the texture */
		VkDescriptorImageInfo descriptor;
		/** Image backing the resource */
		class WImage* img;
		/** Pointer to the texture description in the effect */
		struct W_BOUND_RESOURCE* sampler_info;
	};
	/** List of all textures (or samplers) for the effect */
	std::vector<SAMPLER_INFO> m_sampler_info;

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
