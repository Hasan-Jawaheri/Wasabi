#pragma once

#include "Wasabi.h"

enum W_SHADER_TYPE {
	W_VERTEX_SHADER = VK_SHADER_STAGE_VERTEX_BIT,
	W_FRAGMENT_SHADER = VK_SHADER_STAGE_FRAGMENT_BIT,
	W_PIXEL_SHADER = VK_SHADER_STAGE_FRAGMENT_BIT,
};

enum W_SHADER_VARIABLE_TYPE {
	W_TYPE_FLOAT = 0,
	W_TYPE_INT = 1,
};

typedef struct W_SHADER_VARIABLE_INFO {
	W_SHADER_VARIABLE_INFO(W_SHADER_VARIABLE_TYPE _type, int _num_elems, std::string _name = "")
		: type(_type), num_elems(_num_elems), name(_name) {}

	W_SHADER_VARIABLE_TYPE type;
	int num_elems;
	std::string name;

	size_t GetSize();
	VkFormat GetFormat();
} W_SHADER_VARIABLE_INFO;

typedef struct W_UBO_INFO {
	W_UBO_INFO(std::vector<W_SHADER_VARIABLE_INFO> v) : variables(v) {}

	std::vector<W_SHADER_VARIABLE_INFO> variables;

	size_t GetSize();
} W_UBO_INFO;

typedef struct W_SHADER_DESC {
	W_SHADER_TYPE type;
	std::vector<W_UBO_INFO> ubo_info;
	std::vector<W_SHADER_VARIABLE_INFO> input_layout;
} W_SHADER_DESC;

class WShader : public WBase {
	friend class WEffect;
	friend class WMaterial;
	virtual std::string GetTypeName() const;

protected:
	W_SHADER_DESC m_desc;
	VkShaderModule m_module;

	void LoadCodeSPIRV(const char* const code, int len);
	void LoadCodeGLSL(std::string code);
	void LoadCodeSPIRVFromFile(std::string filename);
	void LoadCodeGLSLFromFile(std::string filename);

public:
	WShader(class Wasabi* const app, unsigned int ID = 0);
	~WShader();

	virtual void Load() = 0;

	virtual bool		Valid() const;
};

class WShaderManager : public WManager<WShader> {
	friend class WShader;

	virtual std::string GetTypeName() const;

public:
	WShaderManager(class Wasabi* const app);
};

class WEffect : public WBase {
	friend class WMaterial;
	virtual std::string GetTypeName() const;

	std::vector<WShader*>				m_shaders;
	VkPrimitiveTopology					m_topology;
	VkPipeline							m_pipeline;
	VkPipelineLayout					m_pipelineLayout;
	VkDescriptorSetLayout				m_descriptorSetLayout;

	void _DestroyPipeline();

public:
	WEffect(class Wasabi* const app, unsigned int ID = 0);
	~WEffect();

	WError					BindShader(WShader* shader);
	WError					UnbindShader(W_SHADER_TYPE type);
	void					SetPrimitiveTopology(VkPrimitiveTopology topology);
	WError					BuildPipeline();
	WError					Bind();
	
	VkPipelineLayout*		GetPipelineLayout();
	VkDescriptorSetLayout*	GetDescriptorSetLayout();

	virtual bool			Valid() const;
};

class WEffectManager : public WManager<WEffect> {
	friend class WEffect;

	virtual std::string GetTypeName() const;

public:
	WEffectManager(class Wasabi* const app);
};

