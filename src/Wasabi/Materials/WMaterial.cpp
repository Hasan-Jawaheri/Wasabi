#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Images/WImage.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"

std::string WMaterialManager::GetTypeName() const {
	return "Material";
}

WMaterialManager::WMaterialManager(class Wasabi* const app) : WManager<WMaterial>(app) {
}

WMaterial::WMaterial(Wasabi* const app, uint32_t ID) : WFileAsset(app, ID) {
	m_descriptorPool = VK_NULL_HANDLE;
	m_effect = nullptr;

	app->MaterialManager->AddEntity(this);
}

WMaterial::~WMaterial() {
	_DestroyResources();

	m_app->MaterialManager->RemoveEntity(this);
}

std::string WMaterial::_GetTypeName() {
	return "Material";
}

std::string WMaterial::GetTypeName() const {
	return _GetTypeName();
}

void WMaterial::SetID(uint32_t newID) {
	m_app->MaterialManager->RemoveEntity(this);
	m_ID = newID;
	m_app->MaterialManager->AddEntity(this);
}

void WMaterial::SetName(std::string newName) {
	m_name = newName;
	m_app->MaterialManager->OnEntityNameChanged(this, newName);
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

	for (int i = 0; i < m_samplers.size(); i++) {
		for (uint32_t j = 0; j < m_samplers[i].images.size(); j++) {
			if (m_samplers[i].images[j])
				W_SAFE_REMOVEREF(m_samplers[i].images[j]);
		}
	}
	m_samplers.clear();

	for (int i = 0; i < m_pushConstants.size(); i++)
		W_SAFE_FREE(m_pushConstants[i].data);
	m_pushConstants.clear();

	for (auto it = m_descriptorSets.begin(); it != m_descriptorSets.end(); it++)
		m_app->MemoryManager->ReleaseDescriptorSet(*it, m_descriptorPool, m_app->GetCurrentBufferingIndex());
	m_app->MemoryManager->ReleaseDescriptorPool(m_descriptorPool, m_app->GetCurrentBufferingIndex());

	if (m_effect) {
		// if this material is being destroyed and is in the parent effect's per-frame materials, remove it
		// manually (the effect cannot hold a reference to this material because it would be a circular relationship)
		for (uint32_t i = 0; i < m_effect->m_perFrameMaterials.size(); i++) {
			if (m_effect->m_perFrameMaterials[i] == this) {
				m_effect->m_perFrameMaterials.erase(m_effect->m_perFrameMaterials.begin() + i);
				break;
			}
		}
	}
	W_SAFE_REMOVEREF(m_effect);
}

WError WMaterial::CreateForEffect(WEffect* const effect, uint32_t bindingSet) {
	VkDevice device = m_app->GetVulkanDevice();

	if (effect && !effect->Valid())
		return WError(W_INVALIDPARAM);

	_DestroyResources();

	if (!effect)
		return WError(W_SUCCEEDED);

	//
	// Create the uniform buffers
	//
	uint32_t numBuffers = m_app->GetEngineParam<uint32_t>("bufferingCount");
	size_t writeDescriptorsSize = 0;
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
				for (uint32_t b = 0; b < numBuffers; b++) {
					ubo.dirty[b] = false;
					ubo.descriptorBufferInfos[b].buffer = ubo.buffer.GetBuffer(m_app, b);
					ubo.descriptorBufferInfos[b].offset = 0;
					ubo.descriptorBufferInfos[b].range = ubo.ubo_info->GetSize();
				}

				m_uniformBuffers.push_back(ubo);
				writeDescriptorsSize += ubo.descriptorBufferInfos.size();
			} else if (shader->m_desc.bound_resources[j].type == W_TYPE_TEXTURE) {
				bool already_added = false;
				for (int k = 0; k < m_samplers.size(); k++) {
					if (m_samplers[k].sampler_info->binding_index == shader->m_desc.bound_resources[j].binding_index) {
						// two shaders have the same sampler binding index, skip (it is the same sampler, the WEffect::CreatePipeline ensures that)
						already_added = true;
					}
				}
				if (already_added)
					continue;

				uint32_t textureArraySize = (uint32_t)shader->m_desc.bound_resources[j].GetSize();
				SAMPLER_INFO sampler = {};

				sampler.images.resize(textureArraySize);
				for (uint32_t k = 0; k < textureArraySize; k++) {
					sampler.images[k] = m_app->ImageManager->GetDefaultImage();
					sampler.images[k]->AddReference();
				}

				sampler.descriptors.resize(numBuffers);
				for (auto descriptors = sampler.descriptors.begin(); descriptors != sampler.descriptors.end(); descriptors++) {
					descriptors->resize(textureArraySize);
					for (auto descriptor = descriptors->begin(); descriptor != descriptors->end(); descriptor++) {
						descriptor->sampler = m_app->Renderer->GetTextureSampler();
						descriptor->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						descriptor->imageView = VK_NULL_HANDLE; // // will be assigned in the Bind() function
					}
				}
				sampler.sampler_info = &shader->m_desc.bound_resources[j];
				m_samplers.push_back(sampler);
				writeDescriptorsSize += sampler.descriptors.size() * sampler.descriptors[0].size();
			} else if (shader->m_desc.bound_resources[j].type == W_TYPE_PUSH_CONSTANT) {
				bool already_added = false;
				for (int k = 0; k < m_pushConstants.size(); k++) {
					if (m_pushConstants[k].pc_info->OffsetAtVariable(0) == shader->m_desc.bound_resources[j].OffsetAtVariable(0) &&
						m_pushConstants[k].pc_info->GetSize() == shader->m_desc.bound_resources[j].GetSize()) {
						// two shaders have the same push constant ranges, merge them
						m_pushConstants[k].shaderStages |= (VkShaderStageFlagBits)shader->m_desc.type;
						already_added = true;
					}
				}
				if (already_added)
					continue;

				PUSH_CONSTANT_INFO info = {};
				info.pc_info = &shader->m_desc.bound_resources[j];
				info.data = W_SAFE_ALLOC(info.pc_info->GetSize());
				info.shaderStages = (VkShaderStageFlagBits)shader->m_desc.type;
				m_pushConstants.push_back(info);
			}
		}
	}
	m_writeDescriptorSets.resize(writeDescriptorsSize);

	//
	// Create descriptor pool
	//
	// We need to tell the API the number of max. requested descriptors per type
	vector<VkDescriptorPoolSize> typeCounts;
	if (m_uniformBuffers.size() > 0) {
		VkDescriptorPoolSize s;
		s.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		s.descriptorCount = (uint32_t)m_uniformBuffers.size() * numBuffers;
		typeCounts.push_back(s);
	}
	if (m_samplers.size() > 0) {
		VkDescriptorPoolSize s;
		s.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		s.descriptorCount = 0;
		for (uint32_t i = 0; i < m_samplers.size(); i++)
			s.descriptorCount += (uint32_t)m_samplers[i].images.size() * numBuffers;
		typeCounts.push_back(s);
	}

	if (typeCounts.size() > 0) {
		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = NULL;
		descriptorPoolInfo.poolSizeCount = (uint32_t)typeCounts.size();
		descriptorPoolInfo.pPoolSizes = typeCounts.data();
		// Set the max. number of sets that can be requested
		// Requesting descriptors beyond maxSets will result in an error
		descriptorPoolInfo.maxSets = numBuffers;
		descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

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
		for (uint32_t i = 0; i < layouts.size(); i++)
			layouts[i] = effect->GetDescriptorSetLayout(bindingSet);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)m_descriptorSets.size();
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

WError WMaterial::Bind(WRenderTarget* rt, bool bindDescSet, bool bindPushConsts) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkDevice device = m_app->GetVulkanDevice();

	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	if (bindDescSet) {
		int numUpdateDescriptors = 0;
		uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();

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
		for (auto sampler = m_samplers.begin(); sampler != m_samplers.end(); sampler++) {
			W_BOUND_RESOURCE* info = sampler->sampler_info;
			bool bChanged = false;
			for (uint32_t textureArrayIndex = 0; textureArrayIndex < (uint32_t)sampler->images.size(); textureArrayIndex++) {
				if (sampler->images[textureArrayIndex] && sampler->images[textureArrayIndex]->Valid()) {
					if (sampler->descriptors[bufferIndex][textureArrayIndex].imageView != sampler->images[textureArrayIndex]->GetView()) {
						sampler->descriptors[bufferIndex][textureArrayIndex].imageView = sampler->images[textureArrayIndex]->GetView();
						sampler->descriptors[bufferIndex][textureArrayIndex].imageLayout = sampler->images[textureArrayIndex]->GetViewLayout();
						bChanged = true;
					}
				}
			}
			if (bChanged) {
				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = m_descriptorSets[bufferIndex];
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSet.descriptorCount = (uint32_t)sampler->descriptors[bufferIndex].size();
				writeDescriptorSet.pImageInfo = sampler->descriptors[bufferIndex].data();
				writeDescriptorSet.dstBinding = info->binding_index;

				m_writeDescriptorSets[numUpdateDescriptors++] = writeDescriptorSet;
			}
		}
		if (numUpdateDescriptors > 0)
			vkUpdateDescriptorSets(device, numUpdateDescriptors, m_writeDescriptorSets.data(), 0, NULL);

		vkCmdBindDescriptorSets(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_effect->GetPipelineLayout(), m_setIndex, 1, &m_descriptorSets[bufferIndex], 0, nullptr);
	}

	if (bindPushConsts) {
		for (auto pc = m_pushConstants.begin(); pc != m_pushConstants.end(); pc++)
			vkCmdPushConstants(renderCmdBuffer, m_effect->GetPipelineLayout(), pc->shaderStages, (uint32_t)pc->pc_info->OffsetAtVariable(0), (uint32_t)pc->pc_info->GetSize(), pc->data);
	}

	return WError(W_SUCCEEDED);
}

VkDescriptorSet WMaterial::GetDescriptorSet() const {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	return m_descriptorSets[bufferIndex];
}

WEffect* WMaterial::GetEffect() const {
	return m_effect;
}

WError WMaterial::SetVariableData(const char* varName, void* data, size_t len) {
	uint32_t bufferIndex = m_app->GetCurrentBufferingIndex();
	bool isFound = false;
	for (auto ubo = m_uniformBuffers.begin(); ubo != m_uniformBuffers.end(); ubo++) {
		W_BOUND_RESOURCE* info = ubo->ubo_info;
		for (int j = 0; j < info->variables.size(); j++) {
			if (strcmp(info->variables[j].name.c_str(), varName) == 0) {
				size_t varsize = info->variables[j].GetSize();
				size_t offset = info->OffsetAtVariable(j);
				if (varsize < len || offset + len > ubo->descriptorBufferInfos[bufferIndex].range)
					return WError(W_INVALIDPARAM);
				if (memcmp((char*)ubo->data + offset, data, len) != 0) {
					memcpy((char*)ubo->data + offset, data, len);
					for (uint32_t d = 0; d < ubo->dirty.size(); d++)
						ubo->dirty[d] = true;
				}
				isFound = true;
			}
		}
	}
	for (auto pc = m_pushConstants.begin(); pc != m_pushConstants.end(); pc++) {
		W_BOUND_RESOURCE* info = pc->pc_info;
		for (int j = 0; j < info->variables.size(); j++) {
			if (strcmp(info->variables[j].name.c_str(), varName) == 0) {
				size_t varsize = info->variables[j].GetSize();
				size_t offset = info->OffsetAtVariable(j);
				if (varsize < len || offset + len > pc->pc_info->GetSize())
					return WError(W_INVALIDPARAM);
				memcpy((char*)pc->data + offset, data, len);
			}
		}
	}
	return WError(isFound ? W_SUCCEEDED : W_INVALIDPARAM);
}

WError WMaterial::SetTexture(uint32_t binding_index, WImage* img, uint32_t arrayIndex) {
	bool isFound = false;
	for (int i = 0; i < m_samplers.size(); i++) {
		W_BOUND_RESOURCE* info = m_samplers[i].sampler_info;
		if (info->binding_index == binding_index) {
			if (arrayIndex < m_samplers[i].images.size()) {
				if (m_samplers[i].images[arrayIndex] != img) {
					if (m_samplers[i].images[arrayIndex]) {
						W_SAFE_REMOVEREF(m_samplers[i].images[arrayIndex]);
					}
					if (img) {
						m_samplers[i].images[arrayIndex] = img;
						img->AddReference();
					} else {
						m_samplers[i].images[arrayIndex] = m_app->ImageManager->GetDefaultImage();
						m_app->ImageManager->GetDefaultImage()->AddReference();
					}
				}
				isFound = true;
			}
		}
	}
	return WError(isFound ? W_SUCCEEDED : W_INVALIDPARAM);
}

WError WMaterial::SetTexture(std::string name, WImage* img, uint32_t arrayIndex) {
	bool isFound = false;
	for (int i = 0; i < m_samplers.size(); i++) {
		W_BOUND_RESOURCE* info = m_samplers[i].sampler_info;
		if (info->name == name) {
			if (arrayIndex < m_samplers[i].images.size()) {
				if (m_samplers[i].images[arrayIndex] != img) {
					if (m_samplers[i].images[arrayIndex]) {
						W_SAFE_REMOVEREF(m_samplers[i].images[arrayIndex]);
					}
					if (img) {
						m_samplers[i].images[arrayIndex] = img;
						img->AddReference();
					} else {
						m_samplers[i].images[arrayIndex] = m_app->ImageManager->GetDefaultImage();
						m_app->ImageManager->GetDefaultImage()->AddReference();
					}
				}
				isFound = true;
			}
		}
	}
	return WError(isFound ? W_SUCCEEDED : W_INVALIDPARAM);
}

WError WMaterial::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	// write the UBO data
	uint32_t tmp = (uint32_t)m_uniformBuffers.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	for (uint32_t i = 0; i < m_uniformBuffers.size(); i++) {
		UNIFORM_BUFFER_INFO* UBO = &m_uniformBuffers[i];
		VkDeviceSize size = UBO->ubo_info->GetSize();
		outputStream.write((char*)& size, sizeof(size));
		outputStream.write((char*)UBO->data, size);
	}

	// write the texture data
	char tmpName[W_MAX_ASSET_NAME_SIZE];
	tmp = (uint32_t)m_samplers.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	for (uint32_t i = 0; i < m_samplers.size(); i++) {
		SAMPLER_INFO* SI = &m_samplers[i];
		outputStream.write((char*)& SI->sampler_info->binding_index, sizeof(SI->sampler_info->binding_index));
		tmp = (uint32_t)SI->images.size();
		outputStream.write((char*)&tmp, sizeof(tmp));
		for (uint32_t j = 0; j < SI->images.size(); j++) {
			strcpy_s(tmpName, W_MAX_ASSET_NAME_SIZE, SI->images[j]->GetName().c_str());
			outputStream.write(tmpName, W_MAX_ASSET_NAME_SIZE);
		}
	}
	outputStream.write((char*)&m_setIndex, sizeof(m_setIndex));

	strcpy_s(tmpName, W_MAX_ASSET_NAME_SIZE, m_effect->GetName().c_str());
	outputStream.write(tmpName, W_MAX_ASSET_NAME_SIZE);

	// save dependencies
	for (uint32_t i = 0; i < m_samplers.size(); i++) {
		SAMPLER_INFO* SI = &m_samplers[i];
		for (uint32_t j = 0; j < SI->images.size(); j++) {
			WError err = file->SaveAsset(SI->images[j]);
			if (!err)
				return err;
		}
	}
	WError err = file->SaveAsset(m_effect);
	if (!err)
		return err;

	return WError(W_SUCCEEDED);
}

std::vector<void*> WMaterial::LoadArgs() {
	return std::vector<void*>({});
}

WError WMaterial::LoadFromStream(WFile* file, std::istream& inputStream, std::vector<void*>& args, std::string nameSuffix) {
	UNREFERENCED_PARAMETER(args);

	_DestroyResources();

	// read the UBO data
	uint32_t numUBOs;
	std::vector<std::pair<VkDeviceSize, void*>> uboData;
	inputStream.read((char*)&numUBOs, sizeof(numUBOs));
	for (uint32_t i = 0; i < numUBOs; i++) {
		VkDeviceSize size;
		inputStream.read((char*)&size, sizeof(size));
		uboData.push_back(std::pair<VkDeviceSize, void*>(size, W_SAFE_ALLOC(size)));
		inputStream.read((char*)uboData[uboData.size() - 1].second, size);
	}

	// read the texture data
	uint32_t numTextures;
	std::vector<std::pair<uint, std::vector<std::string>>> textureData;
	inputStream.read((char*)&numTextures, sizeof(numTextures));
	for (uint32_t i = 0; i < numTextures; i++) {
		uint32_t arraySize, index;
		inputStream.read((char*)&index, sizeof(index));
		inputStream.read((char*)&arraySize, sizeof(arraySize));
		std::vector<std::string> names(arraySize);
		for (uint32_t j = 0; j < arraySize; j++) {
			char tmpName[W_MAX_ASSET_NAME_SIZE];
			inputStream.read(tmpName, W_MAX_ASSET_NAME_SIZE);
			names[j] = std::string(tmpName);
		}
		textureData.push_back(std::make_pair(index, names));
	}
	inputStream.read((char*)&m_setIndex, sizeof(m_setIndex));
	char effectName[W_MAX_ASSET_NAME_SIZE];
	inputStream.read(effectName, W_MAX_ASSET_NAME_SIZE);

	// load dependencies
	WEffect* fx;
	WError err = file->LoadAsset<WEffect>(effectName, &fx, WEffect::LoadArgs(m_app->Renderer->GetRenderTarget()), ""); // never copy the effect, always share
	if (err) {
		err = CreateForEffect(fx, m_setIndex);
		fx->RemoveReference();
		if (err) {
			for (uint32_t i = 0; i < textureData.size() && err; i++) {
				uint32_t bindingIndex = textureData[i].first;
				std::vector<std::string>& textureNames = textureData[i].second;
				for (uint32_t j = 0; j < textureNames.size(); j++) {
					WImage* tex;
					err = file->LoadAsset<WImage>(textureNames[j], &tex, WImage::LoadArgs(), ""); // never copy the image, always share
					if (err) {
						err = SetTexture(bindingIndex, tex, j);
						tex->RemoveReference();
					}
				}
			}
		}
	}

	if (err) {
		if (m_uniformBuffers.size() != uboData.size())
			err = WError(W_INVALIDFILEFORMAT);
		else {
			for (uint32_t i = 0; i < m_uniformBuffers.size() && err; i++) {
				UNIFORM_BUFFER_INFO* UBO = &m_uniformBuffers[i];
				if (uboData[i].first != UBO->ubo_info->GetSize())
					err = WError(W_INVALIDFILEFORMAT);
				else
					memcpy(UBO->data, uboData[i].second, uboData[i].first);
			}
		}
	}

	for (uint32_t i = 0; i < uboData.size(); i++)
		W_SAFE_FREE(uboData[i].second);

	if (!err)
		_DestroyResources();

	return err;
}

WError WMaterialCollection::SetVariableData(const char* varName, void* data, size_t len) {
	WError ret = WError(W_NOTVALID);
	for (auto it : m_materials) {
		WError err = it.first->SetVariableData(varName, data, len);
		if (ret != W_SUCCEEDED)
			ret = err;
	}
	return ret;
}

WError WMaterialCollection::SetTexture(uint32_t bindingIndex, class WImage* img, uint32_t arrayIndex) {
	WError ret = WError(W_NOTVALID);
	for (auto it : m_materials) {
		WError err = it.first->SetTexture(bindingIndex, img, arrayIndex);
		if (ret != W_SUCCEEDED)
			ret = err;
	}
	return ret;
}

WError WMaterialCollection::SetTexture(std::string name, class WImage* img, uint32_t arrayIndex) {
	WError ret = WError(W_NOTVALID);
	for (auto it : m_materials) {
		WError err = it.first->SetTexture(name, img, arrayIndex);
		if (ret != W_SUCCEEDED)
			ret = err;
	}
	return ret;
}
