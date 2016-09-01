#pragma once

#include "../Core/Core.h"

enum W_SHADER_TYPE {
	W_VERTEX_SHADER = VK_SHADER_STAGE_VERTEX_BIT,
	W_FRAGMENT_SHADER = VK_SHADER_STAGE_FRAGMENT_BIT,
	W_PIXEL_SHADER = VK_SHADER_STAGE_FRAGMENT_BIT,
};

enum W_SHADER_VARIABLE_TYPE {
	W_TYPE_FLOAT = 0,
	W_TYPE_INT = 1,
	W_TYPE_UINT = 2,
};

enum W_SHADER_BOUND_RESOURCE_TYPE {
	W_TYPE_UBO = 0,
	W_TYPE_SAMPLER = 1,
};

enum W_VERTEX_INPUT_RATE {
	W_INPUT_RATE_PER_VERTEX = 0,
	W_INPUT_RATE_PER_INSTANCE = 1,
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

typedef struct W_BOUND_RESOURCE {
	W_BOUND_RESOURCE(W_SHADER_BOUND_RESOURCE_TYPE t,
					 unsigned int index,
					 std::vector<W_SHADER_VARIABLE_INFO> v = std::vector<W_SHADER_VARIABLE_INFO>())
		: variables(v), type(t), binding_index(index) {}

	W_SHADER_BOUND_RESOURCE_TYPE type;
	unsigned int binding_index;
	std::vector<W_SHADER_VARIABLE_INFO> variables;

	size_t GetSize();
} W_BOUND_RESOURCE;

typedef struct W_INPUT_LAYOUT {
	W_INPUT_LAYOUT(std::vector<W_SHADER_VARIABLE_INFO> a, W_VERTEX_INPUT_RATE r = W_INPUT_RATE_PER_VERTEX)
		: attributes(a), input_rate(r) {
	}
	W_INPUT_LAYOUT() : attributes({}) {}

	std::vector<W_SHADER_VARIABLE_INFO> attributes;
	W_VERTEX_INPUT_RATE input_rate;

	size_t GetSize();
} W_INPUT_LAYOUT;

typedef struct W_SHADER_DESC {
	W_SHADER_DESC() : animation_texture_index(-1), instancing_texture_index(-1) {}
	W_SHADER_TYPE type;
	std::vector<W_BOUND_RESOURCE> bound_resources; // UBOs and samplers
	std::vector<W_INPUT_LAYOUT> input_layouts; // one layout for each vertex buffer that can be bound
	unsigned int animation_texture_index, instancing_texture_index;
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

	virtual void		Load() = 0;

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

	std::vector<VkPipeline>				m_pipelines;
	std::vector<WShader*>				m_shaders;
	VkPrimitiveTopology					m_topology;
	VkPipelineLayout					m_pipelineLayout;
	VkDescriptorSetLayout				m_descriptorSetLayout;

	VkPipelineColorBlendAttachmentState m_blendState;
	VkPipelineDepthStencilStateCreateInfo m_depthStencilState;
	VkPipelineRasterizationStateCreateInfo m_rasterizationState;

	void _DestroyPipeline();
	bool _ValidShaders() const;

public:
	WEffect(class Wasabi* const app, unsigned int ID = 0);
	~WEffect();

	WError					BindShader(WShader* shader);
	WError					UnbindShader(W_SHADER_TYPE type);
	void					SetPrimitiveTopology(VkPrimitiveTopology topology);
	void					SetBlendingState(VkPipelineColorBlendAttachmentState state);
	void					SetDepthStencilState(VkPipelineDepthStencilStateCreateInfo state);
	void					SetRasterizationState(VkPipelineRasterizationStateCreateInfo state);
	WError					BuildPipeline(class WRenderTarget* rt);
	WError					Bind(class WRenderTarget* rt, unsigned int num_vertex_buffers = -1);
	
	VkPipelineLayout*		GetPipelineLayout();
	VkDescriptorSetLayout*	GetDescriptorSetLayout();
	W_INPUT_LAYOUT			GetInputLayout(unsigned int layout_index = 0) const;

	virtual bool			Valid() const;
};

class WEffectManager : public WManager<WEffect> {
	friend class WEffect;

	virtual std::string GetTypeName() const;

public:
	WEffectManager(class Wasabi* const app);
};

