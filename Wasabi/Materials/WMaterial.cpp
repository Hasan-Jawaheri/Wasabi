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

	W_SAFE_REMOVEREF(m_effect);

	m_app->MaterialManager->RemoveEntity(this);
}

std::string WMaterial::GetTypeName() const {
	return "Material";
}

bool WMaterial::Valid() const {
	return m_effect && m_effect->Valid();
}

void WMaterial::_DestroyResources() {
	VkDevice device = m_app->GetVulkanDevice();

	W_SAFE_REMOVEREF(m_effect);

	for (int i = 0; i < m_uniformBuffers.size(); i++) {
		vkFreeMemory(device, m_uniformBuffers[i].memory, nullptr);
		vkDestroyBuffer(device, m_uniformBuffers[i].buffer, nullptr);
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
}

WError WMaterial::SetEffect(WEffect* const effect) {
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
				VkResult err = vkCreateBuffer(device, &bufferInfo, nullptr, &ubo.buffer);
				if (err) {
					_DestroyResources();
					return WError(W_UNABLETOCREATEBUFFER);
				}
				// Get memory requirements including size, alignment and memory type 
				VkMemoryRequirements memReqs;
				vkGetBufferMemoryRequirements(device, ubo.buffer, &memReqs);
				uballocInfo.allocationSize = memReqs.size;
				// Gets the appropriate memory type for this type of buffer allocation
				// Only memory types that are visible to the host
				m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &uballocInfo.memoryTypeIndex);
				// Allocate memory for the uniform buffer
				err = vkAllocateMemory(device, &uballocInfo, nullptr, &(ubo.memory));
				if (err) {
					vkDestroyBuffer(device, ubo.buffer, nullptr);
					_DestroyResources();
					return WError(W_OUTOFMEMORY);
				}
				// Bind memory to buffer
				err = vkBindBufferMemory(device, ubo.buffer, ubo.memory, 0);
				if (err) {
					vkDestroyBuffer(device, ubo.buffer, nullptr);
					vkFreeMemory(device, ubo.memory, nullptr);
					_DestroyResources();
					return WError(W_UNABLETOCREATEBUFFER);
				}

				// Store information in the uniform's descriptor
				ubo.descriptor.buffer = ubo.buffer;
				ubo.descriptor.offset = 0;
				ubo.descriptor.range = shader->m_desc.bound_resources[j].GetSize();
				ubo.ubo_info = &shader->m_desc.bound_resources[j];

				m_uniformBuffers.push_back(ubo);
			} else if (shader->m_desc.bound_resources[j].type == W_TYPE_SAMPLER) {
				bool already_added = false;
				for (int k = 0; k < m_sampler_info.size(); k++) {
					if (m_sampler_info[k].sampler_info->binding_index == shader->m_desc.bound_resources[j].binding_index) {
						// two shaders have the same sampler binding index, skip (it is the same sampler, the WEffect::CreatePipeline ensures that)
						already_added = true;
					}
				}
				if (already_added)
					continue;

				SAMPLER_INFO sampler;
				sampler.img = m_app->ImageManager->GetDefaultImage();
				m_app->ImageManager->GetDefaultImage()->AddReference();
				sampler.descriptor.sampler = m_app->Renderer->GetDefaultSampler();
				sampler.descriptor.imageView = m_app->ImageManager->GetDefaultImage()->GetView();
				sampler.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
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
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

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

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = effect->GetDescriptorSetLayout();

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

	m_effect = effect;
	effect->AddReference();

	return WError(W_SUCCEEDED);
}

WError WMaterial::Bind(WRenderTarget* rt, unsigned int num_vertex_buffers) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();
	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	int curDSIndex = 0;
	for (int i = 0; i < m_sampler_info.size(); i++) {
		W_BOUND_RESOURCE* info = m_sampler_info[i].sampler_info;
		if (m_sampler_info[i].img && m_sampler_info[i].img->Valid()) {
			m_sampler_info[i].descriptor.imageView = m_sampler_info[i].img->GetView();

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = m_descriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &m_sampler_info[i].descriptor;
			writeDescriptorSet.dstBinding = info->binding_index;

			m_writeDescriptorSets[curDSIndex++] = writeDescriptorSet;
		}
	}
	if (curDSIndex)
		vkUpdateDescriptorSets(device, curDSIndex, m_writeDescriptorSets.data(), 0, NULL);

	vkCmdBindDescriptorSets(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_effect->GetPipelineLayout(), 0, 1, &m_descriptorSet, 0, NULL);

	return m_effect->Bind(rt, num_vertex_buffers);
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
	for (int i = 0; i < m_uniformBuffers.size(); i++) {
		W_BOUND_RESOURCE* info = m_uniformBuffers[i].ubo_info;
		size_t cur_offset = 0;
		for (int j = 0; j < info->variables.size(); j++) {
			int varsize = info->variables[j].GetSize();
			// the data is 16-byte aligned, so if this variable doesn't fit in the current 16-byte-aligned-slot, move to the next 16 bytes block
			int ramining_bytes_in_alignment_slot = 16 - (cur_offset % 16);
			if (varsize > ramining_bytes_in_alignment_slot && ramining_bytes_in_alignment_slot < 16)
				cur_offset += ramining_bytes_in_alignment_slot;
			if (strcmp(info->variables[j].name.c_str(), varName) == 0) {
				if (info->variables[j].GetSize() < len)
					return WError(W_INVALIDPARAM);
				uint8_t *pData;
				VkResult vkRes = vkMapMemory(device, m_uniformBuffers[i].memory, cur_offset, len, 0, (void **)&pData);
				if (vkRes)
					return WError(W_UNABLETOMAPBUFFER);
				memcpy(pData, data, len);
				vkUnmapMemory(device, m_uniformBuffers[0].memory);

				isFound = true;
			}
			cur_offset += varsize;
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

WError WMaterial::SetAnimationTexture(WImage* img) {
	if (!Valid())
		return WError(W_NOTVALID);

	unsigned int binding_index = -1;
	for (int i = 0; i < m_effect->m_shaders.size() && binding_index == -1; i++) {
		if (m_effect->m_shaders[i]->m_desc.type == W_VERTEX_SHADER)
			binding_index = m_effect->m_shaders[i]->m_desc.animation_texture_index;
	}
	return SetTexture(binding_index, img);
}

WError WMaterial::SetInstancingTexture(WImage* img) {
	if (!Valid())
		return WError(W_NOTVALID);

	unsigned int binding_index = -1;
	for (int i = 0; i < m_effect->m_shaders.size() && binding_index == -1; i++) {
		if (m_effect->m_shaders[i]->m_desc.type == W_VERTEX_SHADER)
			binding_index = m_effect->m_shaders[i]->m_desc.instancing_texture_index;
	}
	return SetTexture(binding_index, img);
}

class WEffect* WMaterial::GetEffect() const {
	return m_effect;
}
