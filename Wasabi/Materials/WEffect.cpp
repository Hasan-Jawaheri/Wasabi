#include "WEffect.h"
#include "WMaterial.h"
#include "../Images/WRenderTarget.h"
#include "../Renderers/WRenderer.h"

#include <iostream>
#include <unordered_map>
using std::unordered_map;

size_t W_SHADER_VARIABLE_TYPE_SIZES[] = { 4, 4, 4, 2, 0, 8, 12, 16, 64 };

static size_t RoundedUpToMultipleOf(size_t N, size_t multiple) {
	if (N % multiple == 0)
		return N;
	return N + multiple - (N % multiple);
}

W_SHADER_VARIABLE_INFO::W_SHADER_VARIABLE_INFO(
	W_SHADER_VARIABLE_TYPE _type,
	std::string _name
) : type(_type), num_elems(1), name(_name) {
	_size = W_SHADER_VARIABLE_TYPE_SIZES[(int)type];
}

W_SHADER_VARIABLE_INFO::W_SHADER_VARIABLE_INFO(
	W_SHADER_VARIABLE_TYPE _type,
	int _num_elems,
	std::string _name
) : type(_type), num_elems(_num_elems), name(_name) {
	_size = W_SHADER_VARIABLE_TYPE_SIZES[(int)type] * num_elems;
}

W_SHADER_VARIABLE_INFO::W_SHADER_VARIABLE_INFO(
	W_SHADER_VARIABLE_TYPE _type,
	int _num_elems,
	int _struct_size,
	int _struct_largest_base_alignment,
	std::string _name
) : type(_type), num_elems(_num_elems), struct_size(_struct_size), struct_largest_base_alignment(_struct_largest_base_alignment), name(_name) {
	_size = RoundedUpToMultipleOf(struct_size, GetAlignment()) * (num_elems - 1) + struct_size;
}

size_t W_SHADER_VARIABLE_INFO::GetSize() const {
	return _size;
}

size_t GetScalarAlignment(W_SHADER_VARIABLE_TYPE baseType, int numElements, int structSize, int structLargestBaseAlignment) {
	if (numElements > 1)
		return GetScalarAlignment(baseType, 1, structSize, structLargestBaseAlignment);
	else if (baseType == W_TYPE_STRUCT)
		return structLargestBaseAlignment;
	else if (baseType == W_TYPE_HALF)
		return 2;
	else
		return 4;
}

size_t GetBaseAlignment(W_SHADER_VARIABLE_TYPE baseType, int numElements, int structSize, int structLargestBaseAlignment) {
	if (numElements > 1)
		return GetBaseAlignment(baseType, 1, structSize, structLargestBaseAlignment);
	else if (baseType == W_TYPE_STRUCT)
		return structLargestBaseAlignment;
	else if (baseType == W_TYPE_MAT4X4)
		return GetBaseAlignment(W_TYPE_VEC_4, 1, structSize, structLargestBaseAlignment);
	else if (baseType == W_TYPE_VEC_2)
		return 2 * GetScalarAlignment(baseType, numElements, structSize, structLargestBaseAlignment);
	else if (baseType == W_TYPE_VEC_3 || baseType == W_TYPE_VEC_4)
		return 4 * GetScalarAlignment(baseType, numElements, structSize, structLargestBaseAlignment);
	else
		return GetScalarAlignment(baseType, numElements, structSize, structLargestBaseAlignment);
}

size_t GetExtendedAlignment(W_SHADER_VARIABLE_TYPE baseType, int numElements, int structSize, int structLargestBaseAlignment) {
	if (baseType == W_TYPE_STRUCT || numElements > 1) {
		return RoundedUpToMultipleOf(GetBaseAlignment(baseType, 1, structSize, structLargestBaseAlignment), 16);
	} else
		return GetBaseAlignment(baseType, numElements, structSize, structLargestBaseAlignment);
}

size_t W_SHADER_VARIABLE_INFO::GetAlignment() const {
	return GetExtendedAlignment(type, num_elems, struct_size, struct_largest_base_alignment);
}

VkFormat W_SHADER_VARIABLE_INFO::GetFormat() const {
	switch (type) {
	case W_TYPE_FLOAT:
		switch (num_elems) {
		case 1: return VK_FORMAT_R32_SFLOAT;
		case 2: return VK_FORMAT_R32G32_SFLOAT;
		case 3: return VK_FORMAT_R32G32B32_SFLOAT;
		case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		default: return VK_FORMAT_UNDEFINED;
		}
		break;
	case W_TYPE_INT:
		switch (num_elems) {
		case 1: return VK_FORMAT_R32_SINT;
		case 2: return VK_FORMAT_R32G32_SINT;
		case 3: return VK_FORMAT_R32G32B32_SINT;
		case 4: return VK_FORMAT_R32G32B32A32_SINT;
		default: return VK_FORMAT_UNDEFINED;
		}
		break;
	case W_TYPE_UINT:
		switch (num_elems) {
		case 1: return VK_FORMAT_R32_UINT;
		case 2: return VK_FORMAT_R32G32_UINT;
		case 3: return VK_FORMAT_R32G32B32_UINT;
		case 4: return VK_FORMAT_R32G32B32A32_UINT;
		default: return VK_FORMAT_UNDEFINED;
		}
		break;
	case W_TYPE_HALF:
		switch (num_elems) {
		case 1: return VK_FORMAT_R16_SFLOAT;
		case 2: return VK_FORMAT_R16G16_SFLOAT;
		case 3: return VK_FORMAT_R16G16B16_SFLOAT;
		case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		default: return VK_FORMAT_UNDEFINED;
		}
		break;
	case W_TYPE_VEC_2:
		return VK_FORMAT_R32G32_SFLOAT;
	case W_TYPE_VEC_3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case W_TYPE_VEC_4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	return VK_FORMAT_UNDEFINED;
}

W_BOUND_RESOURCE::W_BOUND_RESOURCE(
	W_SHADER_BOUND_RESOURCE_TYPE t,
	uint index,
	std::string _name,
	std::vector<W_SHADER_VARIABLE_INFO> v
) : W_BOUND_RESOURCE(t, index, 0, _name, v) {}

W_BOUND_RESOURCE::W_BOUND_RESOURCE(
	W_SHADER_BOUND_RESOURCE_TYPE t,
	uint index,
	uint set,
	std::string _name,
	std::vector<W_SHADER_VARIABLE_INFO> v
) : variables(v), type(t), binding_index(index), binding_set(set), name(_name) {
	size_t curOffset = 0;
	_offsets.resize(variables.size());
	for (int i = 0; i < variables.size(); i++) {
		int varSize = variables[i].GetSize();
		int varAlignment = variables[i].GetAlignment();
		if (curOffset % varAlignment > 0)
			curOffset += varAlignment - (curOffset % varAlignment); // apply alignment
		_offsets[i] = curOffset;
		curOffset += varSize;
	}
	_size = curOffset;
}

size_t W_BOUND_RESOURCE::GetSize() const {
	return _size;
}

size_t W_BOUND_RESOURCE::OffsetAtVariable(unsigned int variable_index) const {
	return _offsets[variable_index];
}

bool W_BOUND_RESOURCE::IsSimilarTo(W_BOUND_RESOURCE resource) {
	if (type != resource.type || binding_index != resource.binding_index || variables.size() != resource.variables.size())
		return false;
	for (int i = 0; i < variables.size(); i++) {
		if (variables[i].type != resource.variables[i].type || variables[i].GetSize() != resource.variables[i].GetSize() || variables[i].num_elems != resource.variables[i].num_elems)
			return false;
	}
	return true;
}

size_t W_INPUT_LAYOUT::GetSize() const {
	if (_size == -1) {
		_size = 0;
		for (int i = 0; i < attributes.size(); i++)
			_size += attributes[i].GetSize();
	}
	return _size;
}

std::string WShaderManager::GetTypeName(void) const {
	return "Shader";
}

WShaderManager::WShaderManager(class Wasabi* const app) : WManager<WShader>(app) {
}

std::string WShader::GetTypeName() const {
	return "Shader";
}

WShader::WShader(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_module = VK_NULL_HANDLE;
	m_code = nullptr;
	m_codeLen = 0;
	m_isSPIRV = false;
	app->ShaderManager->AddEntity(this);
}

WShader::~WShader() {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);
	m_module = VK_NULL_HANDLE;
	W_SAFE_FREE(m_code);

	m_app->ShaderManager->RemoveEntity(this);
}

void WShader::LoadCodeSPIRV(const char* const code, int len, bool bSaveData) {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);

	int roundedLen = (len + 3) & ((uint)-1 << 2);
	m_code = (char*)W_SAFE_ALLOC(roundedLen);
	memcpy(m_code, code, roundedLen);
	if (len < roundedLen)
		memset(m_code + len, 0, roundedLen - len);
	m_codeLen = roundedLen;
	m_isSPIRV = true;

	m_module = vkTools::loadShaderFromCode(m_code, m_codeLen, m_app->GetVulkanDevice(), (VkShaderStageFlagBits)m_desc.type);
	if (!bSaveData || !m_module) {
		W_SAFE_FREE(m_code);
		m_codeLen = 0;
	}
}

void WShader::LoadCodeGLSL(std::string code, bool bSaveData) {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);

	int roundedLen = ((code.size() + 3) & ((uint)-1 << 2));
	m_code = (char*)W_SAFE_ALLOC(roundedLen);
	memcpy(m_code, code.c_str(), roundedLen);
	if (code.size() < roundedLen)
		memset(m_code + code.size(), '\n', roundedLen - code.size());
	m_codeLen = roundedLen;
	m_isSPIRV = false;

	m_module = vkTools::loadShaderGLSLFromCode(m_code, m_codeLen, m_app->GetVulkanDevice(), (VkShaderStageFlagBits)m_desc.type);
	if (!bSaveData || !m_module) {
		W_SAFE_FREE(m_code);
		m_codeLen = 0;
	}
}

void WShader::LoadCodeSPIRVFromFile(std::string filename, bool bSaveData) {
	std::ifstream file;
	file.open(filename, ios::in | ios::binary | ios::ate);
	if (file.is_open()) {
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		vector<char> buf;
		if (file.read(buf.data(), size))
			LoadCodeSPIRV(buf.data(), buf.size(), bSaveData);
		file.close();
	}
}

void WShader::LoadCodeGLSLFromFile(std::string filename, bool bSaveData) {
	std::ifstream file;
	file.open(filename, ios::in | ios::ate);
	if (file.is_open()) {
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		vector<char> buf;
		if (file.read(buf.data(), size))
			LoadCodeGLSL(std::string(buf.data(), buf.size()), bSaveData);
		file.close();
	}
}

bool WShader::Valid() const {
	// valid when shader module exists
	return m_module != VK_NULL_HANDLE;
}

WError WShader::SaveToStream(class WFile* file, std::ostream& outputStream) {
	if (!Valid() || !m_code)
		return WError(W_NOTVALID);

	outputStream.write((char*)&m_isSPIRV, sizeof(m_isSPIRV));
	outputStream.write((char*)&m_codeLen, sizeof(m_codeLen));
	outputStream.write((char*)m_code, m_codeLen);

	outputStream.write((char*)&m_desc.type, sizeof(m_desc.type));

	// save the input layout
	uint tmp;
	tmp = m_desc.input_layouts.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	for (uint i = 0; i < m_desc.input_layouts.size(); i++) {
		W_INPUT_LAYOUT* IL = &m_desc.input_layouts[i];
		outputStream.write((char*)&IL->input_rate, sizeof(IL->input_rate));
		tmp = IL->attributes.size();
		outputStream.write((char*)&tmp, sizeof(tmp));
		for (uint j = 0; j < IL->attributes.size(); j++) {
			W_SHADER_VARIABLE_INFO* attr = &IL->attributes[j];
			outputStream.write((char*)&attr->type, sizeof(attr->type));
			outputStream.write((char*)&attr->num_elems, sizeof(attr->num_elems));
			outputStream.write((char*)&attr->struct_size, sizeof(attr->struct_size));
			outputStream.write((char*)&attr->struct_largest_base_alignment, sizeof(attr->struct_largest_base_alignment));
			tmp = attr->name.length();
			outputStream.write((char*)&tmp, sizeof(tmp));
			if (tmp > 0)
				outputStream.write((char*)attr->name.c_str(), attr->name.length());
		}
	}

	// save the bound resources
	tmp = m_desc.bound_resources.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	for (uint i = 0; i < m_desc.bound_resources.size(); i++) {
		W_BOUND_RESOURCE* resource = &m_desc.bound_resources[i];
		outputStream.write((char*)&resource->type, sizeof(resource->type));
		outputStream.write((char*)&resource->binding_index, sizeof(resource->binding_index));
		tmp = resource->name.size();
		outputStream.write((char*)&tmp, sizeof(tmp));
		outputStream.write(resource->name.c_str(), tmp);
		tmp = resource->variables.size();
		outputStream.write((char*)&tmp, sizeof(tmp));
		for (uint j = 0; j < resource->variables.size(); j++) {
			W_SHADER_VARIABLE_INFO* attr = &resource->variables[j];
			outputStream.write((char*)&attr->type, sizeof(attr->type));
			outputStream.write((char*)&attr->num_elems, sizeof(attr->num_elems));
			outputStream.write((char*)&attr->struct_size, sizeof(attr->struct_size));
			outputStream.write((char*)&attr->struct_largest_base_alignment, sizeof(attr->struct_largest_base_alignment));
			tmp = attr->name.length();
			outputStream.write((char*)&tmp, sizeof(tmp));
			if (tmp > 0)
				outputStream.write((char*)attr->name.c_str(), attr->name.length());
		}
	}

	return WError(W_SUCCEEDED);
}

WError WShader::LoadFromStream(class WFile* file, std::istream& inputStream) {
	bool bSaveData = false;

	inputStream.read((char*)&m_isSPIRV, sizeof(m_isSPIRV));
	inputStream.read((char*)&m_codeLen, sizeof(m_codeLen));

	char* code = (char*)W_SAFE_ALLOC(m_codeLen);
	inputStream.read(code, m_codeLen);

	if (m_isSPIRV)
		LoadCodeSPIRV(m_code, m_codeLen, bSaveData);
	else
		LoadCodeGLSL(std::string(code, m_codeLen), bSaveData);

	W_SAFE_FREE(code);

	if (!m_module)
		return WError(W_INVALIDFILEFORMAT);

	inputStream.read((char*)&m_desc.type, sizeof(m_desc.type));

	// load the input layout
	uint tmp;
	inputStream.read((char*)&tmp, sizeof(tmp));
	m_desc.input_layouts.resize(tmp);
	for (uint i = 0; i < m_desc.input_layouts.size(); i++) {
		W_INPUT_LAYOUT* IL = &m_desc.input_layouts[i];
		inputStream.read((char*)&IL->input_rate, sizeof(IL->input_rate));
		inputStream.read((char*)&tmp, sizeof(tmp));
		IL->attributes.resize(tmp);
		for (uint j = 0; j < IL->attributes.size(); j++) {
			W_SHADER_VARIABLE_INFO* attr = &IL->attributes[j];
			inputStream.read((char*)&attr->type, sizeof(attr->type));
			inputStream.read((char*)&attr->num_elems, sizeof(attr->num_elems));
			inputStream.read((char*)&attr->struct_size, sizeof(attr->struct_size));
			inputStream.read((char*)&attr->struct_largest_base_alignment, sizeof(attr->struct_largest_base_alignment));
			tmp = attr->name.length();
			inputStream.read((char*)&tmp, sizeof(tmp));
			if (tmp > 0) {
				char* name = new char[tmp];
				inputStream.read(name, tmp);
				attr->name = std::string(name, tmp);
				delete[] name;
			}
			if (attr->type == W_TYPE_STRUCT)
				*attr = W_SHADER_VARIABLE_INFO(attr->type, attr->num_elems, attr->struct_size, attr->struct_largest_base_alignment, attr->name);
			else
				*attr = W_SHADER_VARIABLE_INFO(attr->type, attr->num_elems, attr->name);
		}
		*IL = W_INPUT_LAYOUT(IL->attributes, IL->input_rate);
	}

	// load the bound resources
	tmp = m_desc.bound_resources.size();
	inputStream.read((char*)&tmp, sizeof(tmp));
	m_desc.bound_resources.resize(tmp);
	for (uint i = 0; i < m_desc.bound_resources.size(); i++) {
		W_BOUND_RESOURCE* resource = &m_desc.bound_resources[i];
		inputStream.read((char*)&resource->type, sizeof(resource->type));
		inputStream.read((char*)&resource->binding_index, sizeof(resource->binding_index));
		inputStream.read((char*)&tmp, sizeof(tmp));
		if (tmp > 0) {
			char* name = new char[tmp];
			inputStream.read(name, tmp);
			resource->name = std::string(name, tmp);
			delete[] name;
		}
		inputStream.read((char*)&tmp, sizeof(tmp));
		resource->variables.resize(tmp);
		for (uint j = 0; j < resource->variables.size(); j++) {
			W_SHADER_VARIABLE_INFO* attr = &resource->variables[j];
			inputStream.read((char*)&attr->type, sizeof(attr->type));
			inputStream.read((char*)&attr->num_elems, sizeof(attr->num_elems));
			inputStream.read((char*)&attr->struct_size, sizeof(attr->struct_size));
			inputStream.read((char*)&attr->struct_largest_base_alignment, sizeof(attr->struct_largest_base_alignment));
			tmp = attr->name.length();
			inputStream.read((char*)&tmp, sizeof(tmp));
			if (tmp > 0) {
				char* name = new char[tmp];
				inputStream.read(name, tmp);
				attr->name = std::string(name, tmp);
				delete[] name;
			}
			if (attr->type == W_TYPE_STRUCT)
				*attr = W_SHADER_VARIABLE_INFO(attr->type, attr->num_elems, attr->struct_size, attr->struct_largest_base_alignment, attr->name);
			else
				*attr = W_SHADER_VARIABLE_INFO(attr->type, attr->num_elems, attr->name);
		}
		*resource = W_BOUND_RESOURCE(resource->type, resource->binding_index, resource->name, resource->variables);
	}

	return WError(W_SUCCEEDED);
}

std::string WEffectManager::GetTypeName(void) const {
	return "Effect";
}

WEffectManager::WEffectManager(class Wasabi* const app) : WManager<WEffect>(app) {
}

WEffect::WEffect(Wasabi* const app, unsigned int ID) : WBase(app, ID), m_depthStencilState({}) {
	m_vertexShaderIndex = -1;

	m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	m_pipelineLayout = VK_NULL_HANDLE;

	VkPipelineColorBlendAttachmentState blendState = {};
	blendState.colorWriteMask = 0xf;
	blendState.blendEnable = VK_FALSE;
	m_blendStates.push_back(blendState);

	m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilState.depthTestEnable = VK_TRUE;
	m_depthStencilState.depthWriteEnable = VK_TRUE;
	m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
	m_depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	m_depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	m_depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	m_depthStencilState.stencilTestEnable = VK_FALSE;
	m_depthStencilState.front = m_depthStencilState.back;

	// Rasterization state
	m_rasterizationState = {};
	m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL; // Solid polygon mode
	m_rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT; // Backface culling
	m_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizationState.depthClampEnable = VK_FALSE;
	m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizationState.depthBiasEnable = VK_FALSE;
	m_rasterizationState.lineWidth = 1.0f;

	app->EffectManager->AddEntity(this);
}

WEffect::~WEffect() {
	for (int i = 0; i < m_shaders.size(); i++) {
		m_shaders[i]->RemoveReference();
	}
	m_shaders.clear();
	m_vertexShaderIndex = -1;

	_DestroyPipeline();

	m_app->EffectManager->RemoveEntity(this);
}

std::string WEffect::GetTypeName() const {
	return "Effect";
}

bool WEffect::_ValidShaders() const {
	// valid when at least one shader has input layout (vertex shader)
	for (int i = 0; i < m_shaders.size(); i++)
		if (m_shaders[i]->m_desc.type == W_VERTEX_SHADER &&
			m_shaders[i]->m_desc.input_layouts.size() > 0 &&
			m_shaders[i]->m_desc.input_layouts[0].GetSize() > 0 &&
			m_shaders[i]->Valid())
			return true;
	return false;
}

bool WEffect::Valid() const {
	return _ValidShaders() && m_pipelines.size() > 0;
}

WError WEffect::BindShader(WShader* shader) {
	if (!shader)
		return WError(W_INVALIDPARAM);

	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i]->m_desc.type == shader->m_desc.type) {
			m_shaders[i]->RemoveReference();
			m_shaders.erase(m_shaders.begin() + i);
			break;
		}
	}

	m_shaders.push_back(shader);
	shader->AddReference();

	if (shader->m_desc.type == W_VERTEX_SHADER)
		m_vertexShaderIndex = m_shaders.size() - 1;

	return WError(W_SUCCEEDED);
}

WError WEffect::UnbindShader(W_SHADER_TYPE type) {
	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i]->m_desc.type == type) {
			m_shaders[i]->RemoveReference();
			m_shaders.erase(m_shaders.begin() + i);

			// if vertex shader was removed, make sure to remove its cached index
			if (i == m_vertexShaderIndex)
				m_vertexShaderIndex = -1;

			return WError(W_SUCCEEDED);
		}
	}

	return WError(W_INVALIDPARAM);
}

void WEffect::SetPrimitiveTopology(VkPrimitiveTopology topology) {
	m_topology = topology;
}

void WEffect::_DestroyPipeline() {
	VkDevice device = m_app->GetVulkanDevice();

	if (m_pipelineLayout)
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	for (auto it = m_descriptorSetLayouts.begin(); it != m_descriptorSetLayouts.end(); it++)
		vkDestroyDescriptorSetLayout(device, it->second, nullptr);
	m_descriptorSetLayouts.clear();
	for (int i = 0; i < m_pipelines.size(); i++)
		vkDestroyPipeline(device, m_pipelines[i], nullptr);
	m_pipelines.clear();
	
	m_pipelineLayout = VK_NULL_HANDLE;
}

void WEffect::SetBlendingState(VkPipelineColorBlendAttachmentState state) {
	SetBlendingStates(vector<VkPipelineColorBlendAttachmentState>({ state }));
}

void WEffect::SetBlendingStates(vector<VkPipelineColorBlendAttachmentState> states) {
	m_blendStates = states;
}

void WEffect::SetDepthStencilState(VkPipelineDepthStencilStateCreateInfo state) {
	m_depthStencilState = state;
}

void WEffect::SetRasterizationState(VkPipelineRasterizationStateCreateInfo state) {
	m_rasterizationState = state;
}

WError WEffect::BuildPipeline(WRenderTarget* rt) {
	VkDevice device = m_app->GetVulkanDevice();

	if (!_ValidShaders())
		return WError(W_NOTVALID);

	_DestroyPipeline();

	//
	// Create descriptor set layout
	//
	unordered_map<int, W_BOUND_RESOURCE> used_bindings;
	unordered_map<uint, vector<VkDescriptorSetLayoutBinding>> layoutBindingsMap;
	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i]->m_desc.bound_resources.size()) {
			for (int j = 0; j < m_shaders[i]->m_desc.bound_resources.size(); j++) {
				W_BOUND_RESOURCE* boundResource = &m_shaders[i]->m_desc.bound_resources[j];
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = (VkShaderStageFlagBits)m_shaders[i]->m_desc.type;
				layoutBinding.pImmutableSamplers = NULL;

				auto used_bindings_iter = used_bindings.find(boundResource->binding_index);
				if (used_bindings_iter != used_bindings.end()) {
					// repeated binding index, don't add it to the layoutBindings again and make sure it's the same UBO if it's a UBO
					if (boundResource->type == W_TYPE_UBO && used_bindings_iter->second.type == W_TYPE_UBO) {
						if (!used_bindings_iter->second.IsSimilarTo(m_shaders[i]->m_desc.bound_resources[j]))
							return WError(W_INVALIDREPEATEDBINDINGINDEX);
					}
					// add this shader stage to the possible stages for that binding index
					for (auto mapIter = layoutBindingsMap.begin(); mapIter != layoutBindingsMap.end(); mapIter++)
						for (auto vecIter = mapIter->second.begin(); vecIter != mapIter->second.end(); vecIter++)
							if (vecIter->binding == boundResource->binding_index)
								vecIter->stageFlags |= m_shaders[i]->m_desc.type;
					continue;
				}

				if (boundResource->type == W_TYPE_UBO) {
					layoutBinding.binding = boundResource->binding_index;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					layoutBinding.descriptorCount = 1;
					used_bindings.insert(std::pair<int, W_BOUND_RESOURCE>(boundResource->binding_index, m_shaders[i]->m_desc.bound_resources[j]));
				} else if (boundResource->type == W_TYPE_TEXTURE) {
					layoutBinding.binding = boundResource->binding_index;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					layoutBinding.descriptorCount = 1;
				}
				auto iter = layoutBindingsMap.find(boundResource->binding_set);
				if (iter == layoutBindingsMap.end()) {
					layoutBindingsMap.insert(std::pair<uint, vector<VkDescriptorSetLayoutBinding>>(boundResource->binding_set, { layoutBinding }));
				} else
					iter->second.push_back(layoutBinding);
			}
		}
	}

	VkResult err;

	vector<VkDescriptorSetLayout> descriptorSetLayoutVector;
	for (auto it = layoutBindingsMap.begin(); it != layoutBindingsMap.end(); it++) {
		VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = NULL;
		descriptorLayout.bindingCount = it->second.size();
		descriptorLayout.pBindings = it->second.data();

		VkDescriptorSetLayout descriptorSetLayout;
		err = vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, &descriptorSetLayout);
		if (err)
			return WError(W_FAILEDTOCREATEDESCRIPTORSETLAYOUT);
		m_descriptorSetLayouts.insert(std::pair<uint, VkDescriptorSetLayout>(it->first, descriptorSetLayout));
		descriptorSetLayoutVector.push_back(descriptorSetLayout);
	}

	// Create the pipeline layout that is used to generate the rendering pipelines that
	// are based on this descriptor set layout
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayoutVector.size();
	pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayoutVector.data();

	err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
	if (err)
		return WError(W_FAILEDTOCREATEPIPELINELAYOUT);

	//IA state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = m_topology;

	// Color blend state
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// One blend attachment state
	vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
	if (m_blendStates.size()) {
		for (int i = 0; i < rt->GetNumColorOutputs(); i++)
			blendAttachmentStates.push_back(i < m_blendStates.size() ? m_blendStates[i] : m_blendStates[0]);
		colorBlendState.attachmentCount = blendAttachmentStates.size();
		colorBlendState.pAttachments = blendAttachmentStates.data();
	}

	// Enable dynamic states
	// Describes the dynamic states to be used with this pipeline
	// Dynamic states can be set even after the pipeline has been created
	// So there is no need to create new pipelines just for changing
	// a viewport's dimensions or a scissor box
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	// The dynamic state properties themselves are stored in the command buffer
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = dynamicStateEnables.size();

	// Multi sampling state
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pSampleMask = NULL;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // No multi sampling

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	// One viewport
	viewportState.viewportCount = 1;
	// One scissor rectangle
	viewportState.scissorCount = 1;

	// Assign states
	// Assign pipeline state create information
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	// Load shaders
	// Shaders are loaded from the SPIR-V format, which can be generated from glsl
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (int i = 0; i < m_shaders.size(); i++) {
		VkPipelineShaderStageCreateInfo stage = {};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.pName = "main";
		stage.module = m_shaders[i]->m_module;
		stage.stage = (VkShaderStageFlagBits)m_shaders[i]->m_desc.type;
		shaderStages.push_back(stage);
	}

	pipelineCreateInfo.layout = m_pipelineLayout;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pRasterizationState = &m_rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &m_depthStencilState;
	pipelineCreateInfo.renderPass = rt->GetRenderPass();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	// Create rendering pipelines, one for each VB count (starting from 0)
	std::vector<VkGraphicsPipelineCreateInfo> pipelineCreateInfos;

	vector<W_INPUT_LAYOUT*> ILs; // all ILs for this effect
	unsigned int num_attributes = 0;
	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i]->m_desc.type == W_VERTEX_SHADER) {
			for (int j = 0; j < m_shaders[i]->m_desc.input_layouts.size(); j++) {
				ILs.push_back(&m_shaders[i]->m_desc.input_layouts[j]);
				num_attributes += m_shaders[i]->m_desc.input_layouts[j].attributes.size();
			}
		}
	}

	std::vector<VkVertexInputBindingDescription> bindingDesc(ILs.size());
	std::vector<VkVertexInputAttributeDescription> attribDesc(num_attributes);

	unsigned int cur_attrib = 0;
	// Binding description
	for (int i = 0; i < ILs.size(); i++) {
		bindingDesc[i].binding = i; // VERTEX_BUFFER_BIND_ID;
		bindingDesc[i].stride = ILs[i]->GetSize();
		if (ILs[i]->input_rate == W_INPUT_RATE_PER_VERTEX)
			bindingDesc[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		else if (ILs[i]->input_rate == W_INPUT_RATE_PER_INSTANCE)
			bindingDesc[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		// Attribute descriptions
		// Describes memory layout and shader attribute locations
		unsigned int prev_size = 0;
		for (int j = 0; j < ILs[i]->attributes.size(); j++) {
			attribDesc[cur_attrib].binding = i;
			attribDesc[cur_attrib].location = cur_attrib;
			attribDesc[cur_attrib].format = ILs[i]->attributes[j].GetFormat();
			attribDesc[cur_attrib].offset = 0;
			if (j > 0)
				attribDesc[cur_attrib].offset = attribDesc[cur_attrib - 1].offset + prev_size;
			prev_size = ILs[i]->attributes[j].GetSize();
			cur_attrib++;
		}
	}

	// Assign to vertex buffer
	std::vector<VkPipelineVertexInputStateCreateInfo> inputStates(ILs.size() + 1);
	VkPipelineVertexInputStateCreateInfo inputState;
	inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	inputState.pNext = NULL;
	inputState.flags = VK_FLAGS_NONE;
	inputState.vertexBindingDescriptionCount = 0;
	inputState.pVertexBindingDescriptions = bindingDesc.data();
	inputState.vertexAttributeDescriptionCount = 0;
	inputState.pVertexAttributeDescriptions = attribDesc.data();
	inputStates[0] = inputState;
	for (int i = 1; i < ILs.size() + 1; i++) {
		VkPipelineVertexInputStateCreateInfo newstate = inputStates[i - 1];
		newstate.vertexBindingDescriptionCount++;
		newstate.vertexAttributeDescriptionCount += ILs[i-1]->attributes.size();
		inputStates[i] = newstate;
	}

	if (inputStates.size() == 1) { // 0 ILs, make one pipeline without buffers
		pipelineCreateInfo.pVertexInputState = &inputStates[0];
		pipelineCreateInfos.push_back(pipelineCreateInfo);
	} else { // 1 or more ILs available, make a pipeline for each and dont make a 0-buffer pipeline
		for (int i = 1; i < inputStates.size(); i++) {
			pipelineCreateInfo.pVertexInputState = &inputStates[i];
			if (i > 1) {
				pipelineCreateInfo.basePipelineIndex = 0; // 0th index in pipelineCreateInfos
				pipelineCreateInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
			} else
				pipelineCreateInfo.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
			pipelineCreateInfos.push_back(pipelineCreateInfo);
		}
	}

	m_pipelines.resize(pipelineCreateInfos.size()); // one with 0 VBs, 1 VB, 2 VBs, ..., ILs.size() VBs
	err = vkCreateGraphicsPipelines(device, rt->GetPipelineCache(), m_pipelines.size(),
									pipelineCreateInfos.data(), nullptr, m_pipelines.data());
	if (err)
		return WError(W_FAILEDTOCREATEPIPELINE);

	return WError(W_SUCCEEDED);
}

WError WEffect::Bind(WRenderTarget* rt, uint num_vertex_buffers) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	unsigned int pipeline = fmin(num_vertex_buffers == 0 ? 0 : num_vertex_buffers - 1, m_pipelines.size() - 1);
	vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[pipeline]);

	return WError(W_SUCCEEDED);
}

WMaterial* WEffect::CreateMaterial(uint bindingSet) {
	WMaterial* material = new WMaterial(m_app);
	if (!material->CreateForEffect(this, bindingSet))
		W_SAFE_REMOVEREF(material);
	return material;
}

VkPipelineLayout WEffect::GetPipelineLayout() const {
	return m_pipelineLayout;
}

VkDescriptorSetLayout WEffect::GetDescriptorSetLayout(uint setIndex) const {
	auto it = m_descriptorSetLayouts.find(setIndex);
	if (it == m_descriptorSetLayouts.end())
		return VK_NULL_HANDLE;
	return it->second;
}

W_INPUT_LAYOUT WEffect::GetInputLayout(unsigned int layout_index) const {
	if (m_vertexShaderIndex >= 0 && layout_index < m_shaders[m_vertexShaderIndex]->m_desc.input_layouts.size())
		return m_shaders[m_vertexShaderIndex]->m_desc.input_layouts[layout_index];
	return W_INPUT_LAYOUT();
}

size_t WEffect::GetInputLayoutSize(unsigned int layout_index) const {
	if (m_vertexShaderIndex >= 0 && layout_index < m_shaders[m_vertexShaderIndex]->m_desc.input_layouts.size())
		return m_shaders[m_vertexShaderIndex]->m_desc.input_layouts[layout_index].GetSize();
	return 0;
}

WError WEffect::SaveToStream(WFile* file, std::ostream& outputStream) {
	if (!Valid())
		return WError(W_NOTVALID);

	uint tmp;

	outputStream.write((char*)&m_topology, sizeof(m_topology));
	outputStream.write((char*)&m_depthStencilState, sizeof(m_depthStencilState));
	outputStream.write((char*)&m_rasterizationState, sizeof(m_rasterizationState));
	tmp = m_blendStates.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	outputStream.write((char*)m_blendStates.data(), m_blendStates.size() * sizeof(VkPipelineColorBlendAttachmentState));
	tmp = m_shaders.size();
	outputStream.write((char*)&tmp, sizeof(tmp));
	std::streampos shaderIdsBegin = outputStream.tellp();
	for (uint i = 0; i < m_shaders.size(); i++) {
		tmp = 0;
		outputStream.write((char*)&tmp, sizeof(tmp));
	}
	_MarkFileEnd(file, outputStream.tellp());
	for (uint i = 0; i < m_shaders.size(); i++) {
		WError err = file->SaveAsset(m_shaders[i], &tmp);
		if (!err)
			return err;
		outputStream.seekp(shaderIdsBegin + std::streamoff(i * sizeof(uint)));
		outputStream.write((char*)&tmp, sizeof(tmp));
	}

	return WError(W_SUCCEEDED);
}

WError WEffect::LoadFromStream(WFile* file, std::istream& inputStream) {
	WRenderTarget* rt = m_app->Renderer->GetRenderTarget();
	_DestroyPipeline();

	uint tmp;
	inputStream.read((char*)&m_topology, sizeof(m_topology));
	inputStream.read((char*)&m_depthStencilState, sizeof(m_depthStencilState));
	inputStream.read((char*)&m_rasterizationState, sizeof(m_rasterizationState));
	inputStream.read((char*)&tmp, sizeof(tmp));
	m_blendStates.resize(tmp);
	inputStream.read((char*)m_blendStates.data(), m_blendStates.size() * sizeof(VkPipelineColorBlendAttachmentState));
	inputStream.read((char*)&tmp, sizeof(tmp));

	vector<uint> dependencyIds(tmp);
	for (uint i = 0; i < tmp; i++)
		inputStream.read((char*)&dependencyIds[i], sizeof(uint));

	WError err;
	for (uint i = 0; i < dependencyIds.size(); i++) {
		WShader* shader;
		err = file->LoadAsset<WShader>(dependencyIds[i], &shader);
		if (!err)
			break;
		err = BindShader(shader);
		shader->RemoveReference(); // reference is now owned by us in BindShader, no need for this one
		if (!err)
			break;
	}

	if (err)
		BuildPipeline(rt);
	else
		_DestroyPipeline();
	return err;
}

