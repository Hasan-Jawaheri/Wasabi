#pragma once

#include "../Wasabi.h"

class WMaterial : public WBase {
	virtual std::string GetTypeName() const;

	VkDescriptorPool					m_descriptorPool;
	VkDescriptorSet						m_descriptorSet;

	struct UNIFORM_BUFFER_INFO {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
		struct W_BOUND_RESOURCE* ubo_info;
	};
	vector<UNIFORM_BUFFER_INFO> m_uniformBuffers;

	struct SAMPLER_INFO {
		VkDescriptorImageInfo descriptor;
		class WImage* img;
		struct W_BOUND_RESOURCE* sampler_info;
	};
	vector<SAMPLER_INFO> m_sampler_info;

	class WEffect* m_effect;

	void _DestroyResources();

public:
	WMaterial(Wasabi* const app, unsigned int ID = 0);
	~WMaterial();

	WError			SetEffect(class WEffect* const effect);
	virtual WError	Bind(class WRenderTarget* rt, unsigned int num_vertex_buffers = -1);

	WError			SetVariableFloat(std::string varName, float fVal);
	WError			SetVariableFloatArray(std::string varName, float* fArr, int num_elements);
	WError			SetVariableInt(std::string varName, int iVal);
	WError			SetVariableIntArray(std::string varName, int* iArr, int num_elements);
	WError			SetVariableMatrix(std::string varName, WMatrix mtx);
	WError			SetVariableVector2(std::string varName, WVector2 vec);
	WError			SetVariableVector3(std::string varName, WVector3 vec);
	WError			SetVariableVector4(std::string varName, WVector4 vec);
	WError			SetVariableColor(std::string varName, WColor col);
	WError			SetVariableData(std::string varName, void* data, int len);

	WError			SetTexture(int binding_index, class WImage* img);
	WError			SetAnimationTexture(class WImage* img);
	WError			SetInstancingTexture(class WImage* img);

	class WEffect*	GetEffect() const;

	virtual bool	Valid() const;
};

class WMaterialManager : public WManager<WMaterial> {
	friend class WMaterial;

	virtual std::string GetTypeName() const;

public:
	WMaterialManager(class Wasabi* const app);
};
