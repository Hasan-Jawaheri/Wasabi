#include "WMaterial.h"
#include "WEffect.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"
#include "../Renderers/WRenderer.h"

std::string WMaterialManager::GetTypeName() const {
	return "Material";
}

WMaterialManager::WMaterialManager(class Wasabi* const app) : WManager<WMaterial>(app) {
}

WMaterial::WMaterial(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_descriptorSet = VK_NULL_HANDLE;
	m_descriptorPool = VK_NULL_HANDLE;
	m_effect = nullptr;

	app->MaterialManager->AddEntity(this);
}

WMaterial::~WMaterial() {
	_DestroyResources();

	m_app->MaterialManager->RemoveEntity(this);
}

std::string WMaterial::GetTypeName() const {
	return "Material";
}

bool WMaterial::Valid() const {
	return m_descriptorSet != VK_NULL_HANDLE;
}

void WMaterial::_DestroyResources() {
	VkDevice device = m_app->GetVulkanDevice();

	for (int i = 0; i < m_uniformBuffers.size(); i++) {
		vkFreeMemory(device, m_uniformBuffers[i].memory, nullptr);
		vkDestroyBuffer(device, m_uniformBuffers[i].descriptor.buffer, nullptr);
		W_SAFE_FREE(m_uniformBuffers[i].data);
	}
	m_uniformBuffers.clear();

	for (int i = 0; i < m_sampler_info.size(); i++) {
		if (m_sampler_info[i].img)
			m_sampler_info[i].img->RemoveReference();
	}
	m_sampler_info.clear();

	if (m_descriptorPool)
		vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);

	m_descriptorSet = VK_NULL_HANDLE;
	m_descriptorPool = VK_NULL_HANDLE;

	W_SAFE_REMOVEREF(m_effect);
}

WError WMaterial::CreateForEffect(WEffect* const effect, uint bindingSet) {
	VkDevice device = m_app->GetVulkanDevice();

	if (effect && !effect->Valid())
		return WError(W_INVALIDPARAM);

	_DestroyResources();

	if (!effect)
		return WError(W_SUCCEEDED);

	//
	// Create the uniform buffers
	//
	for (int i = 0; i < effect->m_shaders.size(); i++) {
		WShader* shader = effect->m_shaders[i];
		for (int j = 0; j < shader->m_desc.bound_resources.size(); j++) {
			if (shader->m_desc.bound_resources[j].binding_set != bindingSet)
				continue;

			if (shader->m_desc.bound_resources[j].type == W_TYPE_UBO) {
				bool already_added = false;
				for (int k = 0; k < m_uniformBuffers.size(); k++) {
					if (m_uniformBuffers[k].ubo_info->binding_index == shader->m_desc.bound_resources[j].binding_index) {
						// two shaders have the same UBO binding index, skip (it is the same UBO, the WEffect::CreatePipeline ensures that)
						already_added = true;
					}
				}
				if (already_added)
					continue;

				UNIFORM_BUFFER_INFO ubo;

				VkBufferCreateInfo bufferInfo = {};
				VkMemoryAllocateInfo uballocInfo = {};
				uballocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				uballocInfo.pNext = NULL;
				uballocInfo.allocationSize = 0;
				uballocInfo.memoryTypeIndex = 0;

				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = shader->m_desc.bound_resources[j].GetSize();
				bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

				// Create a new buffer
				VkResult err = vkCreateBuffer(device, &bufferInfo, nullptr, &ubo.descriptor.buffer);
				if (err) {
					_DestroyResources();
					return WError(W_UNABLETOCREATEBUFFER);
				}
				// Get memory requirements including size, alignment and memory type 
				VkMemoryRequirements memReqs;
				vkGetBufferMemoryRequirements(device, ubo.descriptor.buffer, &memReqs);
				uballocInfo.allocationSize = memReqs.size;
				// Gets the appropriate memory type for this type of buffer allocation
				// Only memory types that are visible to the host
				m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &uballocInfo.memoryTypeIndex);
				// Allocate memory for the uniform buffer
				err = vkAllocateMemory(device, &uballocInfo, nullptr, &(ubo.memory));
				if (err) {
					vkDestroyBuffer(device, ubo.descriptor.buffer, nullptr);
					_DestroyResources();
					return WError(W_OUTOFMEMORY);
				}
				// Bind memory to buffer
				err = vkBindBufferMemory(device, ubo.descriptor.buffer, ubo.memory, 0);
				if (err) {
					vkDestroyBuffer(device, ubo.descriptor.buffer, nullptr);
					vkFreeMemory(device, ubo.memory, nullptr);
					_DestroyResources();
					return WError(W_UNABLETOCREATEBUFFER);
				}

				// Store information in the uniform's descriptor
				ubo.descriptor.offset = 0;
				ubo.descriptor.range = shader->m_desc.bound_resources[j].GetSize();
				ubo.ubo_info = &shader->m_desc.bound_resources[j];

				ubo.data = W_SAFE_ALLOC(ubo.descriptor.range);
				ubo.dirty = false;

				m_uniformBuffers.push_back(ubo);
			} else if (shader->m_desc.bound_resources[j].type == W_TYPE_TEXTURE) {
				bool already_added = false;
				for (int k = 0; k < m_sampler_info.size(); k++) {
					if (m_sampler_info[k].sampler_info->binding_index == shader->m_desc.bound_resources[j].binding_index) {
						// two shaders have the same sampler binding index, skip (it is the same sampler, the WEffect::CreatePipeline ensures that)
						already_added = true;
					}
				}
				if (already_added)
					continue;

				SAMPLER_INFO sampler = {};
				sampler.img = m_app->ImageManager->GetDefaultImage();
				m_app->ImageManager->GetDefaultImage()->AddReference();
				sampler.descriptor.sampler = m_app->Renderer->GetTextureSampler();
				sampler.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				sampler.descriptor.imageView = sampler.img->GetView();
				sampler.sampler_info = &shader->m_desc.bound_resources[j];
				m_sampler_info.push_back(sampler);
			}
		}
	}
	m_writeDescriptorSets = vector<VkWriteDescriptorSet>(m_sampler_info.size());

	//
	// Create descriptor pool
	//
	// We need to tell the API the number of max. requested descriptors per type
	vector<VkDescriptorPoolSize> typeCounts;
	if (m_uniformBuffers.size() > 0) {
		VkDescriptorPoolSize s;
		s.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		s.descriptorCount = m_uniformBuffers.size();
		typeCounts.push_back(s);
	}
	if (m_sampler_info.size() > 0) {
		VkDescriptorPoolSize s;
		s.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		s.descriptorCount = m_sampler_info.size();
		typeCounts.push_back(s);
	}

	if (typeCounts.size() > 0) {
		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = NULL;
		descriptorPoolInfo.poolSizeCount = typeCounts.size();
		descriptorPoolInfo.pPoolSizes = typeCounts.data();
		// Set the max. number of sets that can be requested
		// Requesting descriptors beyond maxSets will result in an error
		descriptorPoolInfo.maxSets = descriptorPoolInfo.poolSizeCount;

		VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
		if (vkRes) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}

		//
		// Create descriptor set
		//

		VkDescriptorSetLayout layout = effect->GetDescriptorSetLayout(bindingSet);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		vkRes = vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet);
		if (vkRes) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}

		// Update descriptor sets determining the shader binding points
		// For every binding point used in a shader there needs to be one
		// descriptor set matching that binding point
		vector<VkWriteDescriptorSet> writes;
		for (int i = 0; i < m_uniformBuffers.size(); i++) {
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = m_descriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &m_uniformBuffers[i].descriptor;
			writeDescriptorSet.dstBinding = m_uniformBuffers[i].ubo_info->binding_index;

			writes.push_back(writeDescriptorSet);
		}
		for (int i = 0; i < m_sampler_info.size(); i++) {
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = m_descriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &m_sampler_info[i].descriptor;
			writeDescriptorSet.dstBinding = m_sampler_info[i].sampler_info->binding_index;

			writes.push_back(writeDescriptorSet);
		}

		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, NULL);
	}

	m_effect = effect;
	m_effect->AddReference();
	m_setIndex = bindingSet;

	return WError(W_SUCCEEDED);
}

WError WMaterial::Bind(WRenderTarget* rt) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();

	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	// update textures that changed
	int curDescriptorIndex = 0;
	for (int i = 0; i < m_sampler_info.size(); i++) {
		W_BOUND_RESOURCE* info = m_sampler_info[i].sampler_info;
		if (m_sampler_info[i].img && m_sampler_info[i].img->Valid()) {
			if (m_sampler_info[i].descriptor.imageView != m_sampler_info[i].img->GetView()) {
				m_sampler_info[i].descriptor.imageView = m_sampler_info[i].img->GetView();

				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = m_descriptorSet;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSet.pImageInfo = &m_sampler_info[i].descriptor;
				writeDescriptorSet.dstBinding = info->binding_index;

				m_writeDescriptorSets[curDescriptorIndex++] = writeDescriptorSet;
			}
		}
	}
	if (curDescriptorIndex)
		vkUpdateDescriptorSets(device, curDescriptorIndex, m_writeDescriptorSets.data(), 0, NULL);

	// update dirty UBOs
	for (auto ubo = m_uniformBuffers.begin(); ubo != m_uniformBuffers.end(); ubo++) {
		if (ubo->dirty) {
			void *pData;
			VkResult vkRes = vkMapMemory(device, ubo->memory, ubo->descriptor.offset, ubo->descriptor.range, 0, (void **)&pData);
			if (vkRes)
				return WError(W_UNABLETOMAPBUFFER);
			memcpy(pData, ubo->data, ubo->descriptor.range);
			vkUnmapMemory(device, ubo->memory);
			ubo->dirty = true;
		}
	}

	vkCmdBindDescriptorSets(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_effect->GetPipelineLayout(), m_setIndex, 1, &m_descriptorSet, 0, nullptr);

	return WError(W_SUCCEEDED);
}

VkDescriptorSet WMaterial::GetDescriptorSet() const {
	return m_descriptorSet;
}

WError WMaterial::SetVariableFloat(const char* varName, float fVal) {
	return SetVariableData(varName, &fVal, sizeof(float));
}

WError WMaterial::SetVariableFloatArray(const char* varName, float* fArr, int num_elements) {
	return SetVariableData(varName, fArr, sizeof(float) * num_elements);
}

WError WMaterial::SetVariableInt(const char* varName, int iVal) {
	return SetVariableData(varName, &iVal, sizeof(int));
}

WError WMaterial::SetVariableIntArray(const char* varName, int* iArr, int num_elements) {
	return SetVariableData(varName, iArr, sizeof(int) * num_elements);
}

WError WMaterial::SetVariableMatrix(const char* varName, WMatrix mtx) {
	return SetVariableData(varName, &mtx, sizeof(float) * 4 * 4);
}

WError WMaterial::SetVariableVector2(const char* varName, WVector2 vec) {
	return SetVariableData(varName, &vec, sizeof(WVector2));
}

WError WMaterial::SetVariableVector3(const char* varName, WVector3 vec) {
	return SetVariableData(varName, &vec, sizeof(WVector3));
}

WError WMaterial::SetVariableVector4(const char* varName, WVector4 vec) {
	return SetVariableData(varName, &vec, sizeof(WVector4));
}

WError WMaterial::SetVariableColor(const char* varName, WColor col) {
	return SetVariableData(varName, &col, sizeof(WColor));
}

WError WMaterial::SetVariableData(const char* varName, void* data, int len) {
	VkDevice device = m_app->GetVulkanDevice();
	bool isFound = false;
	for (auto ubo = m_uniformBuffers.begin(); ubo != m_uniformBuffers.end(); ubo++) {
		W_BOUND_RESOURCE* info = ubo->ubo_info;
		for (int j = 0; j < info->variables.size(); j++) {
			if (strcmp(info->variables[j].name.c_str(), varName) == 0) {
				size_t varsize = info->variables[j].GetSize();
				size_t offset = info->OffsetAtVariable(j);
				if (varsize < len)
					return WError(W_INVALIDPARAM);
				memcpy((char*)ubo->data + offset, data, len);
				ubo->dirty = true;
				isFound = true;
			}
		}
	}
	return WError(isFound ? W_SUCCEEDED : W_INVALIDPARAM);
}

WError WMaterial::SetTexture(int binding_index, WImage* img) {
	VkDevice device = m_app->GetVulkanDevice();
	bool isFound = false;
	for (int i = 0; i < m_sampler_info.size(); i++) {
		W_BOUND_RESOURCE* info = m_sampler_info[i].sampler_info;
		if (info->binding_index == binding_index) {
			if (m_sampler_info[i].img)
				W_SAFE_REMOVEREF(m_sampler_info[i].img);
			if (img) {
				m_sampler_info[i].img = img;
				img->AddReference();
			} else {
				m_sampler_info[i].img = m_app->ImageManager->GetDefaultImage();
				m_app->ImageManager->GetDefaultImage()->AddReference();
			}

			isFound = true;
		}
	}
	return WError(isFound ? W_SUCCEEDED : W_INVALIDPARAM);
}

WError WMaterial::SetTexture(std::string name, WImage* img) {
	VkDevice device = m_app->GetVulkanDevice();
	bool isFound = false;
	for (int i = 0; i < m_sampler_info.size(); i++) {
		W_BOUND_RESOURCE* info = m_sampler_info[i].sampler_info;
		if (info->name == name) {
			if (m_sampler_info[i].img)
				W_SAFE_REMOVEREF(m_sampler_info[i].img);
			if (img) {
				m_sampler_info[i].img = img;
				img->AddReference();
			} else {
				m_sampler_info[i].img = m_app->ImageManager->GetDefaultImage();
				m_app->ImageManager->GetDefaultImage()->AddReference();
			}

			isFound = true;
		}
	}
	return WError(isFound ? W_SUCCEEDED : W_INVALIDPARAM);
}

WError WMaterial::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();

	// write the UBO data
	uint tmp = m_uniformBuffers.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	for (uint i = 0; i < m_uniformBuffers.size(); i++) {
		UNIFORM_BUFFER_INFO* UBO = &m_uniformBuffers[i];
		outputStream.write((char*)&UBO->descriptor.range, sizeof(UBO->descriptor.range));
		outputStream.write((char*)UBO->data, UBO->descriptor.range);
	}

	// write the texture data
	tmp = m_sampler_info.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	std::streampos texturesOffset = outputStream.tellp();
	for (uint i = 0; i < m_sampler_info.size(); i++) {
		SAMPLER_INFO* SI = &m_sampler_info[i];
		tmp = 0;
		outputStream.write((char*)&tmp, sizeof(tmp));
		outputStream.write((char*)&SI->sampler_info->binding_index, sizeof(SI->sampler_info->binding_index));
	}
	outputStream.write((char*)&m_setIndex, sizeof(m_setIndex));
	outputStream.write((char*)&tmp, sizeof(tmp)); // effect id
	_MarkFileEnd(file, outputStream.tellp());

	// save dependencies
	for (uint i = 0; i < m_sampler_info.size(); i++) {
		SAMPLER_INFO* SI = &m_sampler_info[i];
		if (SI->img) {
			WError err = file->SaveAsset(SI->img, &tmp);
			if (!err)
				return err;
			outputStream.seekp(texturesOffset + std::streamoff(i * (2 * sizeof(uint))));
			outputStream.write((char*)&tmp, sizeof(tmp));
		}
	}
	WError err = file->SaveAsset(m_effect, &tmp);
	if (!err)
		return err;
	outputStream.seekp(texturesOffset + std::streamoff(m_sampler_info.size() * (2 * sizeof(uint))));
	outputStream.write((char*)&tmp, sizeof(tmp));

	return WError(W_SUCCEEDED);
}

WError WMaterial::LoadFromStream(WFile* file, std::istream& inputStream) {
	_DestroyResources();

	VkDevice device = m_app->GetVulkanDevice();

	// read the UBO data
	uint numUBOs;
	vector<std::pair<VkDeviceSize, void*>> uboData;
	inputStream.read((char*)&numUBOs, sizeof(numUBOs));
	for (uint i = 0; i < numUBOs; i++) {
		VkDeviceSize size;
		inputStream.read((char*)&size, sizeof(size));
		uboData.push_back(std::pair<VkDeviceSize, void*>(size, W_SAFE_ALLOC(size)));
		inputStream.read((char*)uboData[uboData.size() - 1].second, size);
	}

	// read the texture data
	uint numTextures;
	vector<std::pair<uint, uint>> textureData;
	inputStream.read((char*)&numTextures, sizeof(numTextures));
	for (uint i = 0; i < numTextures; i++) {
		uint tid, index;
		inputStream.read((char*)&tid, sizeof(tid));
		inputStream.read((char*)&index, sizeof(index));
		textureData.push_back(std::pair<uint, uint>(tid, index));
	}
	inputStream.read((char*)&m_setIndex, sizeof(m_setIndex));
	uint fxId;
	inputStream.read((char*)&fxId, sizeof(fxId));

	// load dependencies
	WEffect* fx;
	WError err = file->LoadAsset<WEffect>(fxId, &fx);
	if (err) {
		err = CreateForEffect(fx, m_setIndex);
		fx->RemoveReference();
		if (err) {
			for (uint i = 0; i < textureData.size() && err; i++) {
				WImage* tex;
				err = file->LoadAsset<WImage>(textureData[i].first, &tex);
				if (err) {
					err = SetTexture(textureData[i].second, tex);
					tex->RemoveReference();
				}
			}
		}
	}

	if (err) {
		if (m_uniformBuffers.size() != uboData.size())
			err = WError(W_INVALIDFILEFORMAT);
		else {
			for (uint i = 0; i < m_uniformBuffers.size() && err; i++) {
				UNIFORM_BUFFER_INFO* UBO = &m_uniformBuffers[i];
				if (uboData[i].first != UBO->descriptor.range)
					err = WError(W_INVALIDFILEFORMAT);
				else
					memcpy(UBO->data, uboData[i].second, UBO->descriptor.range);
			}
		}
	}

	for (uint i = 0; i < uboData.size(); i++)
		W_SAFE_FREE(uboData[i].second);

	if (!err)
		_DestroyResources();

	return err;
}

