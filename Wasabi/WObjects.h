#pragma once

#include "Wasabi.h"

class WObject : public WBase {
	virtual std::string GetTypeName() const;

	VkPipeline							m_pipeline;
	VkDescriptorSetLayout				m_descriptorSetLayout;
	VkDescriptorPool					m_descriptorPool;
	VkDescriptorSet						m_descriptorSet;
	VkPipelineLayout					m_pipelineLayout;

	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
	}  m_uniformDataVS;

	struct {
		WMatrix projectionMatrix;
		WMatrix modelMatrix;
		WMatrix viewMatrix;
	} m_uboVS;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} m_vertices;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} m_indices;

	VkResult _CreateTriangle();
	VkResult _CreatePipeline();

public:
	WObject(Wasabi* const app, unsigned int ID = 0);
	~WObject();

	void Render();

	virtual bool	Valid() const;
};

class WObjectManager : public WManager<WObject> {
	friend class WObject;

	virtual std::string GetTypeName(void) const;

public:
	WObjectManager(class Wasabi* const app);

	void Render();
};
