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

#include "Wasabi/Core/WCore.hpp"

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
class WMaterial : public WFileAsset {
	friend class WEffect;
	friend class WFile;

protected:
	virtual ~WMaterial();
	WMaterial(class Wasabi* const app, uint32_t ID = 0);

	/**
	 * Builds the material's resources to be used for a certain effect. The
	 * material may be created for only a subset of the bound resources of
	 * the WEffect using the boundResources parameter.
	 * @param  effect      Effect to use
	 * @param  bindingSet  Set index to use from effect
	 * @return             Error code, see WError.h
	 */
	WError CreateForEffect(class WEffect* const effect, uint32_t bindingSet = 0);

public:
	/**
	 * Returns "Material" string.
	 * @return Returns "Material" string
	 */
	static std::string _GetTypeName();
	virtual std::string GetTypeName() const override;
	virtual void SetID(uint32_t newID) override;
	virtual void SetName(std::string newName) override;

	/**
	 * Binds the resources to the pipeline. In Vulkan terms, this binds the descriptor
	 * set associated with this material as well as the push constants.
	 * @param  rt            Render target to bind to its command buffer
	 * @param bindDescSet    Whether or not to bind the descriptor set
	 * @param bindPushConsts Whether or not to bind push constants
	 * @return     Error code, see WError.h
	 */
	virtual WError Bind(class WRenderTarget* rt, bool bindDescSet = true, bool bindPushConsts = true);

	/**
	 * Retrieves the Vulkan descriptor set created by this material.
	 * @return Material's descriptor set
	 */
	VkDescriptorSet GetDescriptorSet() const;

	/**
	 * @return The effect of this material.
	 */
	class WEffect* GetEffect() const;

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is T. If multiple variables have the same name, they
	 * will all be set.
	 * @param  varName Name of the variable to set
	 * @param  value   Value to set
	 * @return         Error code, see WError.h
	 */
	template<typename T>
	WError SetVariable(const char* varName, T value) {
		return SetVariableData(varName, &value, sizeof(T));
	}

	/**
	 * Sets a variable in one of the bound effect's shaders whose name is varName
	 * and whose type is an array of T. If multiple variables have the
	 * same name, they will all be set.
	 * @param  varName      Name of the variable to set
	 * @param  arr          Address of the array to set
	 * @param  numElements  Number of elements in arr
	 * @return              Error code, see WError.h
	 */
	template<typename T>
	WError SetVariableArray(const char* varName, T* arr, int numElements) {
		return SetVariableData(varName, arr, sizeof(T) * numElements);
	}

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
	WError SetVariableData(const char* varName, void* data, size_t len);

	/**
	 * Sets a texture in the bound effect.
	 * @param  bindingIndex  The binding index of the texture
	 * @param  img           The image to set the texture to, can be nullptr
	 * @param  arrayIndex    Index into the texture array (if its an array)
	 * @return               Error code, see WError.h
	 */
	WError SetTexture(uint32_t bindingIndex, class WImage* img, uint32_t arrayIndex = 0);

	/**
	 * Sets a texture in the bound effect.
	 * @param  name        Name of the texture to bind to
	 * @param  img         The image to set the texture to, can be nullptr
	 * @param  arrayIndex  Index into the texture array (if its an array)
	 * @return             Error code, see WError.h
	 */
	WError SetTexture(std::string name, class WImage* img, uint32_t arrayIndex = 0);

	/**
	 * Checks the validity of the material. A material is valid if it has a
	 * valid effect assigned to it.
	 * @return true of the material is valid, false otherwise
	 */
	virtual bool Valid() const override;

	static std::vector<void*> LoadArgs();
	virtual WError SaveToStream(WFile* file, std::ostream& outputStream) override;
	virtual WError LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) override;

private:
	/** Effect that this material is bound to */
	class WEffect* m_effect;
	/** The Vulkan descriptor pool used for the descriptor set */
	VkDescriptorPool m_descriptorPool;
	/** The Vulkan descriptor set objects, one per buffered frame */
	std::vector<VkDescriptorSet> m_descriptorSets;
	/** The set index of m_descriptorSet */
	uint32_t m_setIndex;
	/** An array to hold write descriptor sets (filled/initialized on every
	    Bind() call) */
	std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;

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
		/** Descriptor information for the texture, one array per buffered buffer */
		std::vector<std::vector<VkDescriptorImageInfo>> descriptors;
		/** Array of image backing the texture array (size == 1 if its not an array) */
		std::vector<class WImage*> images;
		/** Pointer to the texture description in the effect */
		struct W_BOUND_RESOURCE* sampler_info;
	};
	/** List of all textures (or samplers) for the effect */
	std::vector<SAMPLER_INFO> m_samplers;

	struct PUSH_CONSTANT_INFO {
		/** Data in the push constant buffer */
		void* data;
		/** Shader stages for this push constant */
		VkShaderStageFlags shaderStages;
		/** Pointer to the push constant description in the effect */
		struct W_BOUND_RESOURCE* pc_info;
	};
	std::vector<PUSH_CONSTANT_INFO> m_pushConstants;

	/**
	 * Frees all resources allocated for the material.
	 */
	void _DestroyResources();
};

/**
 * Helper class to deal with a collection of materials using the same interface as
 * a single one.
 */
class WMaterialCollection {
public:
	std::unordered_map<WMaterial*, bool> m_materials;

	template<typename T>
	WError SetVariable(const char* varName, T value) {
		return SetVariableData(varName, &value, sizeof(T));
	}
	template<typename T>
	WError SetVariableArray(const char* varName, T* arr, int numElements) {
		return SetVariableData(varName, arr, sizeof(T) * numElements);
	}
	WError SetVariableData(const char* varName, void* data, size_t len);
	WError SetTexture(uint32_t bindingIndex, class WImage* img, uint32_t arrayIndex = 0);
	WError SetTexture(std::string name, class WImage* img, uint32_t arrayIndex = 0);
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
