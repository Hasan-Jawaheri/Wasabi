#include "WEffect.h"
#include "../Images/WRenderTarget.h"

size_t W_SHADER_VARIABLE_INFO::GetSize() const {
	if (_size == -1)
		_size = 4 * num_elems;
	return _size;
}

VkFormat W_SHADER_VARIABLE_INFO::GetFormat() const {
	if (type == W_TYPE_FLOAT) {
		switch (num_elems) {
		case 1: return VK_FORMAT_R32_SFLOAT;
		case 2: return VK_FORMAT_R32G32_SFLOAT;
		case 3: return VK_FORMAT_R32G32B32_SFLOAT;
		case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		default: return VK_FORMAT_UNDEFINED;
		}
	} else if (type == W_TYPE_INT) {
		switch (num_elems) {
		case 1: return VK_FORMAT_R32_SINT;
		case 2: return VK_FORMAT_R32G32_SINT;
		case 3: return VK_FORMAT_R32G32B32_SINT;
		case 4: return VK_FORMAT_R32G32B32A32_SINT;
		default: return VK_FORMAT_UNDEFINED;
		}
	} else if (type == W_TYPE_UINT) {
		switch (num_elems) {
		case 1: return VK_FORMAT_R32_UINT;
		case 2: return VK_FORMAT_R32G32_UINT;
		case 3: return VK_FORMAT_R32G32B32_UINT;
		case 4: return VK_FORMAT_R32G32B32A32_UINT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}
	return VK_FORMAT_UNDEFINED;
}

size_t W_BOUND_RESOURCE::GetSize() const {
	if (_size == -1) {
		_size = 0;
		for (int i = 0; i < variables.size(); i++) {
			int varsize = variables[i].GetSize();
			int ramining_bytes_in_alignment_slot = 16 - (_size % 16);
			if (varsize > ramining_bytes_in_alignment_slot && ramining_bytes_in_alignment_slot < 16)
				_size += ramining_bytes_in_alignment_slot;
			_size += varsize;
		}
	}
	return _size;
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
	app->ShaderManager->AddEntity(this);
}

WShader::~WShader() {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);
	m_module = VK_NULL_HANDLE;

	m_app->ShaderManager->RemoveEntity(this);
}

void WShader::LoadCodeSPIRV(const char* const code, int len) {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);
	m_module = vkTools::loadShaderFromCode(code, len, m_app->GetVulkanDevice(), (VkShaderStageFlagBits)m_desc.type);
}

void WShader::LoadCodeGLSL(std::string code) {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);
	m_module = vkTools::loadShaderGLSLFromCode(code.c_str(), code.length(), m_app->GetVulkanDevice(), (VkShaderStageFlagBits)m_desc.type);
}
void WShader::LoadCodeSPIRVFromFile(std::string filename) {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);
	m_module = vkTools::loadShader(filename.c_str(), m_app->GetVulkanDevice(), (VkShaderStageFlagBits)m_desc.type);
}

void WShader::LoadCodeGLSLFromFile(std::string filename) {
	if (m_module)
		vkDestroyShaderModule(m_app->GetVulkanDevice(), m_module, nullptr);
	m_module = vkTools::loadShaderGLSL(filename.c_str(), m_app->GetVulkanDevice(), (VkShaderStageFlagBits)m_desc.type);
}

bool WShader::Valid() const {
	// valid when shader module exists
	return m_module != VK_NULL_HANDLE;
}

std::string WEffectManager::GetTypeName(void) const {
	return "Effect";
}

WEffectManager::WEffectManager(class Wasabi* const app) : WManager<WEffect>(app) {
}

WEffect::WEffect(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_vertexShaderIndex = -1;

	m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	m_pipelineLayout = VK_NULL_HANDLE;
	m_descriptorSetLayout = VK_NULL_HANDLE;

	VkPipelineColorBlendAttachmentState blendState;
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
	if (m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
	for (int i = 0; i < m_pipelines.size(); i++)
		vkDestroyPipeline(device, m_pipelines[i], nullptr);
	m_pipelines.clear();

	m_descriptorSetLayout = VK_NULL_HANDLE;
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
	vector<VkDescriptorSetLayoutBinding> layoutBindings;
	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i]->m_desc.bound_resources.size()) {
			for (int j = 0; j < m_shaders[i]->m_desc.bound_resources.size(); j++) {
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.stageFlags = (VkShaderStageFlagBits)m_shaders[i]->m_desc.type;
				layoutBinding.pImmutableSamplers = NULL;
				if (m_shaders[i]->m_desc.bound_resources[j].type == W_TYPE_UBO) {
					layoutBinding.binding = m_shaders[i]->m_desc.bound_resources[j].binding_index;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					layoutBinding.descriptorCount = 1;
					layoutBindings.push_back(layoutBinding);
				} else if (m_shaders[i]->m_desc.bound_resources[j].type == W_TYPE_SAMPLER) {
					layoutBinding.binding = m_shaders[i]->m_desc.bound_resources[j].binding_index;
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					layoutBinding.descriptorCount = 1;
					layoutBindings.push_back(layoutBinding);
				}
			}
		}
	}

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = layoutBindings.size();
	descriptorLayout.pBindings = layoutBindings.data();

	VkResult err = vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, &m_descriptorSetLayout);
	if (err) {
		return WError(W_FAILEDTOCREATEDESCRIPTORSETLAYOUT);
	}

	// Create the pipeline layout that is used to generate the rendering pipelines that
	// are based on this descriptor set layout
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

	err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
	if (err) {
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
		m_descriptorSetLayout = VK_NULL_HANDLE;
		return WError(W_FAILEDTOCREATEPIPELINELAYOUT);
	}

	//IA state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
	if (err) {
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
		m_descriptorSetLayout = VK_NULL_HANDLE;
		for (int i = 0; i < m_pipelines.size(); i++) {
			if (m_pipelines[i] != VK_NULL_HANDLE)
				vkDestroyPipeline(device, m_pipelines[i], nullptr);
		}
		m_pipelines.clear();
		return WError(W_FAILEDTOCREATEPIPELINE);
	}

	return WError(W_SUCCEEDED);
}

WError WEffect::Bind(WRenderTarget* rt, unsigned int num_vertex_buffers) {
	if (!Valid())
		return WError(W_NOTVALID);

	VkCommandBuffer renderCmdBuffer = rt->GetCommnadBuffer();
	if (!renderCmdBuffer)
		return WError(W_NORENDERTARGET);

	unsigned int pipeline = fmin(num_vertex_buffers == 0 ? 0 : num_vertex_buffers - 1, m_pipelines.size()-1);
	vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[pipeline]);

	return WError(W_SUCCEEDED);
}

VkPipelineLayout* WEffect::GetPipelineLayout() {
	return &m_pipelineLayout;
}

VkDescriptorSetLayout* WEffect::GetDescriptorSetLayout() {
	return &m_descriptorSetLayout;
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
