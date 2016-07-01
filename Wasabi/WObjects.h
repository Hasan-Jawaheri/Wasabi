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

	void _DestroyPipeline();
	WError _CreatePipeline();

	class WGeometry* m_geometry;

public:
	WObject(Wasabi* const app, unsigned int ID = 0);
	~WObject();

	void Render();

	WError SetGeometry(class WGeometry* geometry);

	virtual bool	Valid() const;
};

class WObjectManager : public WManager<WObject> {
	friend class WObject;

	virtual std::string GetTypeName(void) const;

public:
	WObjectManager(class Wasabi* const app);

	void Render();
};
