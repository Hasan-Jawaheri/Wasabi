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
	return m_descriptorSets.size() > 0;
}

void WMaterial::_DestroyResources() {
	for (int i = 0; i < m_uniformBuffers.size(); i++) {
		m_uniformBuffers[i].buffer.Destroy(m_app);
		W_SAFE_FREE(m_uniformBuffers[i].data);
	}
	m_uniformBuffers.clear();

	for (int i = 0; i < m_sampler_info.size(); i++) {
		if (m_sampler_info[i].img)
			m_sampler_info[i].img->RemoveReference();
	}
	m_sampler_info.clear();

	for (auto it = m_descriptorSets.begin(); it != m_descriptorSets.end(); it++)
		m_app->MemoryManager->ReleaseDescriptorSet(*it, m_descriptorPool, m_app->GetCurrentBufferingIndex());
	m_app->MemoryManager->ReleaseDescriptorPool(m_descriptorPool, m_app->GetCurrentBufferingIndex());

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
	uint numBuffers = (uint)m_app->engineParams["bufferingCount"];
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

				UNIFORM_BUFFER_INFO ubo = {};
				ubo.ubo_info = &shader->m_desc.bound_resources[j];
				VkResult result = ubo.buffer.Create(m_app, numBuffers, ubo.ubo_info->GetSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, nullptr, W_MEMORY_HOST_VISIBLE);
				if (result != VK_SUCCESS) {
					_DestroyResources();
					return WError(W_OUTOFMEMORY);
				}

				ubo.data = W_SAFE_ALLOC(ubo.ubo_info->GetSize());
				ubo.dirty.resize(numBuffers);
				ubo.descriptorBufferInfos.resize(numBuffers);
				for (uint b = 0; b < numBuffers; b++) {
					ubo.dirty[b] = false;
					ubo.descriptorBufferInfos[b].buffer = ubo.buffer.GetBuffer(m_app, b);
					ubo.descriptorBufferInfos[b].offset = 0;
					ubo.descriptorBufferInfos[b].range = ubo.ubo_info->GetSize();
				}

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
				sampler.img->AddReference();
				sampler.descriptors.resize(numBuffers);
				for (auto descriptor = sampler.descriptors.begin(); descriptor != sampler.descriptors.end(); descriptor++) {
					descriptor->sampler = m_app->Renderer->GetTextureSampler();
					descriptor->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					descriptor->imageView = VK_NULL_HANDLE; // // will be assigned in the Bind() function
				}
				sampler.sampler_info = &shader->m_desc.bound_resources[j];
				m_sampler_info.push_back(sampler);
			}
		}
	}
	m_writeDescriptorSets.resize((m_sampler_info.size() + m_uniformBuffers.size()) * numBuffers);

	//
	// Create descriptor pool
	//
	// We need to tell the API the number of max. requested descriptors per type
	vector<VkDescriptorPoolSize> typeCounts;
	if (m_uniformBuffers.size() > 0) {
		VkDescriptorPoolSize s;
		s.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		s.descriptorCount = m_uniformBuffers.size() * numBuffers;
		typeCounts.push_back(s);
	}
	if (m_sampler_info.size() > 0) {
		VkDescriptorPoolSize s;
		s.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		s.descriptorCount = m_sampler_info.size() * numBuffers;
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
		descriptorPoolInfo.maxSets = numBuffers;

		VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
		if (vkRes) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}

		//
		// Create descriptor set
		//

		m_descriptorSets.resize(descriptorPoolInfo.maxSets);
		std::vector<VkDescriptorSetLayout> layouts(m_descriptorSets.size());
		for (uint i = 0; i < layouts.size(); i++)
			layouts[i] = effect->GetDescriptorSetLayout(bindingSet);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = m_descriptorSets.size();
		allocInfo.pSetLayouts = layouts.data();

		vkRes = vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data());
		if (vkRes) {
			_DestroyResources();
			return WError(W_OUTOFMEMORY);
		}
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

	int numUpdateDescriptors = 0;
	uint bufferIndex = m_app->GetCurrentBufferingIndex();

	// update UBOs that changed
	for (auto ubo = m_uniformBuffers.begin(); ubo != m_uniformBuffers.end(); ubo++) {
		if (ubo->dirty[bufferIndex]) {
			void* pBufferData;
			ubo->buffer.Map(m_app, bufferIndex, &pBufferData, W_MAP_WRITE);
			memcpy((char*)pBufferData + ubo->descriptorBufferInfos[bufferIndex].offset, ubo->data, ubo->descriptorBufferInfos[bufferIndex].range);
			ubo->buffer.Unmap(m_app, bufferIndex);
			ubo->dirty[bufferIndex] = false;
		}

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = m_descriptorSets[bufferIndex];
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.pBufferInfo = &ubo->descriptorBufferInfos[bufferIndex];
		writeDescriptorSet.dstBinding = ubo->ubo_info->binding_index;

		m_writeDescriptorSets[numUpdateDescriptors++] = writeDescriptorSet;
	}

	// update textures that changed
	for (auto sampler = m_sampler_info.begin(); sampler != m_sampler_info.end(); sampler++) {
		W_BOUND_RESOURCE* info = sampler->sampler_info;
		if (sampler->img && sampler->img->Valid()) {
			if (sampler->descriptors[bufferIndex].imageView != sampler->img->GetView()) {
				sampler->descriptors[bufferIndex].imageView = sampler->img->GetView();
				sampler->descriptors[bufferIndex].imageLayout = sampler->img->GetViewLayout();

				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = m_descriptorSets[bufferIndex];
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSet.pImageInfo = &sampler->descriptors[bufferIndex];
				writeDescriptorSet.dstBinding = info->binding_index;

				m_writeDescriptorSets[numUpdateDescriptors++] = writeDescriptorSet;
			}
		}
	}
	if (numUpdateDescriptors > 0)
		vkUpdateDescriptorSets(device, numUpdateDescriptors, m_writeDescriptorSets.data(), 0, NULL);

	vkCmdBindDescriptorSets(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_effect->GetPipelineLayout(), m_setIndex, 1, &m_descriptorSets[bufferIndex], 0, nullptr);

	return WError(W_SUCCEEDED);
}

VkDescriptorSet WMaterial::GetDescriptorSet() const {
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	return m_descriptorSets[bufferIndex];
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
	uint bufferIndex = m_app->GetCurrentBufferingIndex();
	bool isFound = false;
	for (auto ubo = m_uniformBuffers.begin(); ubo != m_uniformBuffers.end(); ubo++) {
		W_BOUND_RESOURCE* info = ubo->ubo_info;
		for (int j = 0; j < info->variables.size(); j++) {
			if (strcmp(info->variables[j].name.c_str(), varName) == 0) {
				size_t varsize = info->variables[j].GetSize();
				size_t offset = info->OffsetAtVariable(j);
				if (varsize < len || offset + len > ubo->descriptorBufferInfos[bufferIndex].range)
					return WError(W_INVALIDPARAM);
				memcpy((char*)ubo->data + offset, data, len);
				for (uint d = 0; d < ubo->dirty.size(); d++)
					ubo->dirty[d] = true;
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
		tmp = UBO->ubo_info->GetSize();
		outputStream.write((char*)&tmp, sizeof(tmp));
		outputStream.write((char*)UBO->data, tmp);
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
				if (uboData[i].first != UBO->ubo_info->GetSize())
					err = WError(W_INVALIDFILEFORMAT);
				else
					memcpy(UBO->data, uboData[i].second, uboData[i].first);
			}
		}
	}

	for (uint i = 0; i < uboData.size(); i++)
		W_SAFE_FREE(uboData[i].second);

	if (!err)
		_DestroyResources();

	return err;
}

