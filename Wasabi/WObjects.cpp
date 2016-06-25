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

	m_pipeline = VK_NULL_HANDLE;
	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
	m_descriptorSet = VK_NULL_HANDLE;

	_CreateTriangle();
	_CreatePipeline();

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	VkDevice device = m_app->GetVulkanDevice();

	if (m_descriptorSetLayout)
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
	if (m_pipelineLayout)
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	if (m_pipeline)
		vkDestroyPipeline(device, m_pipeline, nullptr);

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	return true; // TODO: put actual implementation
}

void WObject::Render() {
	VkCommandBuffer renderCmdBuffer = m_app->Renderer->GetCommnadBuffer();

	vkCmdBindDescriptorSets(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, NULL);

	vkCmdBindPipeline(renderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	/* DRAW HERE */
	// Bind triangle vertices
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(renderCmdBuffer, 0/*VERTEX_BUFFER_BIND_ID*/, 1, &m_vertices.buf, offsets);

	// Bind triangle indices
	vkCmdBindIndexBuffer(renderCmdBuffer, m_indices.buf, 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	vkCmdDrawIndexed(renderCmdBuffer, m_indices.count, 1, 0, 0, 1);
	/* ********* */
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

VkResult WObject::_CreateTriangle() {
	VkDevice device = m_app->GetVulkanDevice();

	struct Vertex {
		float pos[3];
		float col[3];
	};

	// Setup vertices
	std::vector<Vertex> vertexBuffer = {
		{ { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
		{ { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
	};
	int vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

	// Setup indices
	std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
	uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	m_indices.count = indexBuffer.size();

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	void *data;

	// Static data like vertex and index buffer should be stored on the device memory 
	// for optimal (and fastest) access by the GPU
	//
	// To achieve this we use so-called "staging buffers" :
	// - Create a buffer that's visible to the host (and can be mapped)
	// - Copy the data to this buffer
	// - Create another buffer that's local on the device (VRAM) with the same size
	// - Copy the data from the host to the device using a command buffer

	struct StagingBuffer {
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	struct {
		StagingBuffer vertices;
		StagingBuffer indices;
	} stagingBuffers;

	// Buffer copies are done on the queue, so we need a command buffer for them
	VkCommandBufferAllocateInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = m_app->Renderer->GetCommandPool();
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;

	VkCommandBuffer copyCommandBuffer;
	vkTools::checkResult(vkAllocateCommandBuffers(device, &cmdBufInfo, &copyCommandBuffer));

	// Vertex buffer
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = vertexBufferSize;
	// Buffer is used as the copy source
	vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// Create a host-visible buffer to copy the vertex data to (staging buffer)
	vkTools::checkResult(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
	vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
	// Map and copy
	vkTools::checkResult(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
	memcpy(data, vertexBuffer.data(), vertexBufferSize);
	vkUnmapMemory(device, stagingBuffers.vertices.memory);
	vkTools::checkResult(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

	// Create the destination buffer with device only visibility
	// Buffer will be used as a vertex buffer and is the copy destination
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkTools::checkResult(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &m_vertices.buf));
	vkGetBufferMemoryRequirements(device, m_vertices.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &m_vertices.mem));
	vkTools::checkResult(vkBindBufferMemory(device, m_vertices.buf, m_vertices.mem, 0));

	// Index buffer
	// todo : comment
	VkBufferCreateInfo indexbufferInfo = {};
	indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexbufferInfo.size = indexBufferSize;
	indexbufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	// Copy index data to a buffer visible to the host (staging buffer)
	vkTools::checkResult(vkCreateBuffer(device, &indexbufferInfo, nullptr, &stagingBuffers.indices.buffer));
	vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
	vkTools::checkResult(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
	memcpy(data, indexBuffer.data(), indexBufferSize);
	vkUnmapMemory(device, stagingBuffers.indices.memory);
	vkTools::checkResult(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

	// Create destination buffer with device only visibility
	indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkTools::checkResult(vkCreateBuffer(device, &indexbufferInfo, nullptr, &m_indices.buf));
	vkGetBufferMemoryRequirements(device, m_indices.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	m_app->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
	vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &m_indices.mem));
	vkTools::checkResult(vkBindBufferMemory(device, m_indices.buf, m_indices.mem, 0));
	m_indices.count = indexBuffer.size();

	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.pNext = NULL;

	VkBufferCopy copyRegion = {};

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies 
	// have been submitted and executed
	vkTools::checkResult(vkBeginCommandBuffer(copyCommandBuffer, &cmdBufferBeginInfo));

	// Vertex buffer
	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(
		copyCommandBuffer,
		stagingBuffers.vertices.buffer,
		m_vertices.buf,
		1,
		&copyRegion);
	// Index buffer
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(
		copyCommandBuffer,
		stagingBuffers.indices.buffer,
		m_indices.buf,
		1,
		&copyRegion);

	vkTools::checkResult(vkEndCommandBuffer(copyCommandBuffer));

	// Submit copies to the queue
	VkSubmitInfo copySubmitInfo = {};
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

	vkTools::checkResult(vkQueueSubmit(m_app->Renderer->GetQueue(), 1, &copySubmitInfo, VK_NULL_HANDLE));
	vkTools::checkResult(vkQueueWaitIdle(m_app->Renderer->GetQueue()));

	vkFreeCommandBuffers(device, m_app->Renderer->GetCommandPool(), 1, &copyCommandBuffer);

	// Destroy staging buffers
	vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
	vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
	vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
	vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);

	// Binding description
	m_vertices.bindingDescriptions.resize(1);
	m_vertices.bindingDescriptions[0].binding = 0; // VERTEX_BUFFER_BIND_ID;
	m_vertices.bindingDescriptions[0].stride = sizeof(Vertex);
	m_vertices.bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Attribute descriptions
	// Describes memory layout and shader attribute locations
	m_vertices.attributeDescriptions.resize(2);
	// Location 0 : Position
	m_vertices.attributeDescriptions[0].binding = 0; // VERTEX_BUFFER_BIND_ID;
	m_vertices.attributeDescriptions[0].location = 0;
	m_vertices.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertices.attributeDescriptions[0].offset = 0;
	// Location 1 : Color
	m_vertices.attributeDescriptions[1].binding = 0; // VERTEX_BUFFER_BIND_ID;
	m_vertices.attributeDescriptions[1].location = 1;
	m_vertices.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertices.attributeDescriptions[1].offset = sizeof(float) * 3;

	// Assign to vertex buffer
	m_vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertices.inputState.pNext = NULL;
	m_vertices.inputState.flags = VK_FLAGS_NONE;
	m_vertices.inputState.vertexBindingDescriptionCount = m_vertices.bindingDescriptions.size();
	m_vertices.inputState.pVertexBindingDescriptions = m_vertices.bindingDescriptions.data();
	m_vertices.inputState.vertexAttributeDescriptionCount = m_vertices.attributeDescriptions.size();
	m_vertices.inputState.pVertexAttributeDescriptions = m_vertices.attributeDescriptions.data();

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
	// Create descriptor set
	//

	// Update descriptor sets determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point
	VkWriteDescriptorSet writeDescriptorSet = {};

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_app->Renderer->GetDescriptorPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_descriptorSetLayout;

	VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet);
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

	return VK_SUCCESS;
}

VkResult WObject::_CreatePipeline() {
	VkDevice device = m_app->GetVulkanDevice();

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

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};

	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// The layout used for this pipeline
	pipelineCreateInfo.layout = m_pipelineLayout;
	// Renderpass this pipeline is attached to
	pipelineCreateInfo.renderPass = m_app->Renderer->GetRenderPass();

	// Vertex input state
	// Describes the topoloy used with this pipeline
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// This pipeline renders vertex data as triangle lists
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
	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
	blendAttachmentState[0].colorWriteMask = 0xf;
	blendAttachmentState[0].blendEnable = VK_FALSE;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	// One viewport
	viewportState.viewportCount = 1;
	// One scissor rectangle
	viewportState.scissorCount = 1;

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

	// Assign states
	// Assign pipeline state create information
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &m_vertices.inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.renderPass = m_app->Renderer->GetRenderPass();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	// Create rendering pipeline
	VkResult err = vkCreateGraphicsPipelines(device, m_app->Renderer->GetPipelineCache(), 1, &pipelineCreateInfo, nullptr, &m_pipeline);
	assert(!err);

	return VK_SUCCESS;
}

