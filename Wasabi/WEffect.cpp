#include "WEffect.h"

size_t W_SHADER_VARIABLE_INFO::GetSize() {
	return 4 * num_elems;
}

VkFormat W_SHADER_VARIABLE_INFO::GetFormat() {
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
	}
	return VK_FORMAT_UNDEFINED;
}

size_t W_BOUND_RESOURCE::GetSize() {
	size_t s = 0;
	for (int i = 0; i < variables.size(); i++)
		s += variables[i].GetSize();
	return s;
}

size_t W_INPUT_LAYOUT::GetSize() {
	size_t s = 0;
	for (int i = 0; i < attributes.size(); i++)
		s += attributes[i].GetSize();
	return s;
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
	m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	m_pipeline = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
	m_descriptorSetLayout = VK_NULL_HANDLE;

	m_blendState.colorWriteMask = 0xf;
	m_blendState.blendEnable = VK_FALSE;

	app->EffectManager->AddEntity(this);
}

WEffect::~WEffect() {
	for (int i = 0; i < m_shaders.size(); i++) {
		m_shaders[i]->RemoveReference();
	}
	m_shaders.clear();

	_DestroyPipeline();

	m_app->EffectManager->RemoveEntity(this);
}

std::string WEffect::GetTypeName() const {
	return "Effect";
}

bool WEffect::Valid() const {
	// valid when at least one shader has input layout (vertex shader)
	for (int i = 0; i < m_shaders.size(); i++)
		if (m_shaders[i]->m_desc.type == W_VERTEX_SHADER &&
			m_shaders[i]->m_desc.input_layout.GetSize() > 0 &&
			m_shaders[i]->Valid())
			return true;
	return false;
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

	return WError(W_SUCCEEDED);
}

WError WEffect::UnbindShader(W_SHADER_TYPE type) {
	for (int i = 0; i < m_shaders.size(); i++) {
		if (m_shaders[i]->m_desc.type == type) {
			m_shaders[i]->RemoveReference();
			m_shaders.erase(m_shaders.begin() + i);
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
	if (m_pipeline)
		vkDestroyPipeline(device, m_pipeline, nullptr);
	if (m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);

	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_pipeline = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
}

void WEffect::SetBlendingState(VkPipelineColorBlendAttachmentState state) {
	m_blendState = state;
}

WError WEffect::BuildPipeline() {
	VkDevice device = m_app->GetVulkanDevice();

	if (!Valid())
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
	// In a more complex scenario you would have different pipeline layouts for different
	// descriptor set layouts that could be reused
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

	W_INPUT_LAYOUT* IL = nullptr;
	for (int i = 0; i < m_shaders.size(); i++)
		if (m_shaders[i]->m_desc.type == W_VERTEX_SHADER)
			IL = &m_shaders[i]->m_desc.input_layout;

	vector<VkVertexInputBindingDescription> bindingDesc (1);
	// Binding description
	bindingDesc[0].binding = 0; // VERTEX_BUFFER_BIND_ID;
	bindingDesc[0].stride = IL->GetSize();
	bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> attribDesc (IL->attributes.size());
	// Attribute descriptions
	// Describes memory layout and shader attribute locations
	for (int i = 0; i < IL->attributes.size(); i++) {
		attribDesc[i].binding = 0;
		attribDesc[i].location = i;
		attribDesc[i].format = IL->attributes[i].GetFormat();
		attribDesc[i].offset = 0;
		if (i > 0)
			attribDesc[i].offset = attribDesc[i - 1].offset + IL->attributes[i-1].GetSize();
	}

	// Assign to vertex buffer
	VkPipelineVertexInputStateCreateInfo inputState;
	inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	inputState.pNext = NULL;
	inputState.flags = VK_FLAGS_NONE;
	inputState.vertexBindingDescriptionCount = bindingDesc.size();
	inputState.pVertexBindingDescriptions = bindingDesc.data();
	inputState.vertexAttributeDescriptionCount = attribDesc.size();
	inputState.pVertexAttributeDescriptions = attribDesc.data();


	//IA state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// Solid polygon mode
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	// No culling
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	// Color blend state
	// Describes blend modes and color masks
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// One blend attachment state
	// Blending is not used in this example
	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = { m_blendState };
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

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

	// Depth and stencil state
	// Describes depth and stenctil test and compare ops
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	// Basic depth compare setup with depth writes and depth test enabled
	// No stencil used 
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = depthStencilState.back;

	// Multi sampling state
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pSampleMask = NULL;
	// No multi sampling used in this example
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

	// The layout used for this pipeline
	pipelineCreateInfo.layout = m_pipelineLayout;

	pipelineCreateInfo.pVertexInputState = &inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = NULL; // viewport state is dynamic
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.renderPass = m_app->Renderer->GetRenderPass();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	// Create rendering pipeline
	err = vkCreateGraphicsPipelines(device, m_app->Renderer->GetPipelineCache(), 1, &pipelineCreateInfo, nullptr, &m_pipeline);
	if (err) {
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
		m_descriptorSetLayout = VK_NULL_HANDLE;
		return WError(W_FAILEDTOCREATEPIPELINE);
	}

	return WError(W_SUCCEEDED);
}

WError WEffect::Bind() {
	if (!Valid())
		return WError(W_NOTVALID);

	VkCommandBuffer renderCmdBuffer = m_app->Renderer->GetCommnadBuffer();

	vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	return WError(W_SUCCEEDED);
}

VkPipelineLayout* WEffect::GetPipelineLayout() {
	return &m_pipelineLayout;
}

VkDescriptorSetLayout* WEffect::GetDescriptorSetLayout() {
	return &m_descriptorSetLayout;
}

W_INPUT_LAYOUT WEffect::GetInputLayout() const {
	W_INPUT_LAYOUT l;
	for (int i = 0; i < m_shaders.size(); i++)
		if (m_shaders[i]->m_desc.type == W_VERTEX_SHADER)
			l = m_shaders[i]->m_desc.input_layout;
	return l;
}
