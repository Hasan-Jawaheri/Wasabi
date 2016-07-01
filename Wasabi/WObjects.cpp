#include "WObjects.h"

std::string WObjectManager::GetTypeName(void) const {
	return "Object";
}

WObjectManager::WObjectManager(class Wasabi* const app) : WManager<WObject>(app) {
}

void WObjectManager::Render() {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < _entities[i].size(); j++) {
			_entities[i][j]->Render();
		}
	}
}

WObject::WObject(Wasabi* const app, unsigned int ID) : WBase(app) {
	SetID(ID);

	m_geometry = nullptr;

	m_pipeline = VK_NULL_HANDLE;
	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
	m_descriptorSet = VK_NULL_HANDLE;
	m_descriptorPool = VK_NULL_HANDLE;

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	if (m_geometry)
		m_geometry->RemoveReference();
	m_geometry = nullptr;

	_DestroyPipeline();

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	return true; // TODO: put actual implementation
}

void WObject::Render() {
	if (m_geometry) {
		VkCommandBuffer renderCmdBuffer = m_app->Renderer->GetCommnadBuffer();

		vkCmdBindDescriptorSets(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, NULL);

		vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		WError err = m_geometry->Bind();
	}
}

WError WObject::SetGeometry(class WGeometry* geometry) {
	if (m_geometry)
		m_geometry->RemoveReference();

	m_geometry = geometry;
	if (geometry) {
		m_geometry->AddReference();
		return _CreatePipeline();
	}

	return WError(W_SUCCEEDED);
}

VkPipelineShaderStageCreateInfo loadShader(VkDevice device, std::string fileName, VkShaderStageFlagBits stage) {
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = vkTools::loadShader(fileName.c_str(), device, stage);
	shaderStage.pName = "main";
	assert(shaderStage.module != NULL);
	//shaderModules.push_back(shaderStage.module); TODO: FREE THIS
	return shaderStage;
}

void WObject::_DestroyPipeline() {
	VkDevice device = m_app->GetVulkanDevice();

	if (m_descriptorPool)
		vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
	if (m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
	if (m_pipelineLayout)
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	if (m_pipeline)
		vkDestroyPipeline(device, m_pipeline, nullptr);

	m_pipeline = VK_NULL_HANDLE;
	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
	m_descriptorSet = VK_NULL_HANDLE;
	m_descriptorPool = VK_NULL_HANDLE;
}

WError WObject::_CreatePipeline() {
	VkDevice device = m_app->GetVulkanDevice();

	if (!m_geometry)
		return WError(W_NOTVALID);
	if (!m_geometry->Valid())
		return WError(W_NOTVALID);

	// Assign states
	// Assign pipeline state create information
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	if (!m_geometry->PopulatePipelineInfo(&pipelineCreateInfo))
		return WError(W_NOTVALID);

	_DestroyPipeline();

	//
	// Create the uniform buffer
	//
	// Prepare and initialize uniform buffer containing shader uniforms
	// Vertex shader uniform buffer block
	VkBufferCreateInfo bufferInfo = {};
	VkMemoryAllocateInfo uballocInfo = {};
	uballocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	uballocInfo.pNext = NULL;
	uballocInfo.allocationSize = 0;
	uballocInfo.memoryTypeIndex = 0;

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(m_uboVS);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	// Create a new buffer
	VkResult err = vkCreateBuffer(device, &bufferInfo, nullptr, &m_uniformDataVS.buffer);
	assert(!err);
	// Get memory requirements including size, alignment and memory type 
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, m_uniformDataVS.buffer, &memReqs);
	uballocInfo.allocationSize = memReqs.size;
	// Gets the appropriate memory type for this type of buffer allocation
	// Only memory types that are visible to the host
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &uballocInfo.memoryTypeIndex);
	// Allocate memory for the uniform buffer
	err = vkAllocateMemory(device, &uballocInfo, nullptr, &(m_uniformDataVS.memory));
	assert(!err);
	// Bind memory to buffer
	err = vkBindBufferMemory(device, m_uniformDataVS.buffer, m_uniformDataVS.memory, 0);
	assert(!err);

	// Store information in the uniform's descriptor
	m_uniformDataVS.descriptor.buffer = m_uniformDataVS.buffer;
	m_uniformDataVS.descriptor.offset = 0;
	m_uniformDataVS.descriptor.range = sizeof(m_uboVS);

	//
	// Update the uniform buffer
	//
	// Update matrices
	m_uboVS.projectionMatrix = (WPerspectiveProjMatrixFOV(60.0f,
		(float)m_app->WindowComponent->GetWindowWidth() / (float)m_app->WindowComponent->GetWindowHeight(),
		0.1f, 256.0f));
	m_uboVS.viewMatrix = (WMatrixInverse(WTranslationMatrix(5, 0, -2.5f)));
	m_uboVS.modelMatrix = (WMatrix());

	// Map uniform buffer and update it
	uint8_t *pData;
	err = vkMapMemory(device, m_uniformDataVS.memory, 0, sizeof(m_uboVS), 0, (void **)&pData);
	assert(!err);
	memcpy(pData, &m_uboVS, sizeof(m_uboVS));
	vkUnmapMemory(device, m_uniformDataVS.memory);
	assert(!err);

	//
	// Create descriptor set layout
	//

	// Setup layout of descriptors used in this example
	// Basically connects the different shader stages to descriptors
	// for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout
	// binding

	// Binding 0 : Uniform buffer (Vertex shader)
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = 1;
	descriptorLayout.pBindings = &layoutBinding;

	err = vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, &m_descriptorSetLayout);
	assert(!err);

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
	assert(!err);

	//
	// Create descriptor pool
	//
	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[1];
	// This example only uses one descriptor type (uniform buffer) and only
	// requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1;
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	// Set the max. number of sets that can be requested
	// Requesting descriptors beyond maxSets will result in an error
	descriptorPoolInfo.maxSets = 1;

	VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
	assert(!vkRes);

	//
	// Create descriptor set
	//

	// Update descriptor sets determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point
	VkWriteDescriptorSet writeDescriptorSet = {};

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_descriptorSetLayout;

	vkRes = vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet);
	assert(!vkRes);

	// Binding 0 : Uniform buffer
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = m_descriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &m_uniformDataVS.descriptor;
	// Binds this uniform buffer to binding point 0
	writeDescriptorSet.dstBinding = 0;

	vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);

	// Create our rendering pipeline used in this example
	// Vulkan uses the concept of rendering pipelines to encapsulate
	// fixed states
	// This replaces OpenGL's huge (and cumbersome) state machine
	// A pipeline is then stored and hashed on the GPU making
	// pipeline changes much faster than having to set dozens of 
	// states
	// In a real world application you'd have dozens of pipelines
	// for every shader set used in a scene
	// Note that there are a few states that are not stored with
	// the pipeline. These are called dynamic states and the 
	// pipeline only stores that they are used with this pipeline,
	// but not their states

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
	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
	blendAttachmentState[0].colorWriteMask = 0xf;
	blendAttachmentState[0].blendEnable = VK_FALSE;
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

	// Load shaders
	// Shaders are loaded from the SPIR-V format, which can be generated from glsl
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	shaderStages[0] = loadShader(device, "shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(device, "shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// The layout used for this pipeline
	pipelineCreateInfo.layout = m_pipelineLayout;

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
	assert(!err);

	return WError(W_SUCCEEDED);
}

