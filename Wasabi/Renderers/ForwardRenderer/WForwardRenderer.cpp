#include "WForwardRenderer.h"
#include "../WRenderer.h"
#include "../WRenderStage.h"
#include "../../Lights/WLight.h"
#include "../../Materials/WMaterial.h"
#include "../../Materials/WEffect.h"
#include "../../Images/WRenderTarget.h"
#include "WForwardRenderStage.h"
#include "../Common/WSpritesRenderStage.h"
#include "../Common/CommonRenderStages.h"

WForwardRenderer::WForwardRenderer(Wasabi* const app) : WRenderer(app) {
	m_sampler = VK_NULL_HANDLE;
	m_lights = nullptr;
	m_app->engineParams.insert(std::pair<std::string, void*>("maxLights", (void*)8));
}

WError WForwardRenderer::Initiailize() {
	//
	// Create the texture sampler
	//
	VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Max level-of-detail should match mip level count
	sampler.maxLod = 1;
	// Enable anisotropic filtering
	sampler.maxAnisotropy = 8;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkResult err = vkCreateSampler(m_device, &sampler, nullptr, &m_sampler);
	if (err != VK_SUCCESS)
		return WError(W_OUTOFMEMORY);

	return WError(W_SUCCEEDED);
}

WError WForwardRenderer::LoadDependantResources() {
	return SetRenderingStages({
		new WForwardRenderStage(m_app),
		new WSpritesRenderStage(m_app),
		new WTextsRenderStage(m_app),
	});
}

WError WForwardRenderer::Resize(uint width, uint height) {
	return WRenderer::Resize(width, height);
}

void WForwardRenderer::Cleanup() {
	if (m_sampler)
		vkDestroySampler(m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;

	W_SAFE_DELETE_ARRAY(m_lights);
}

VkSampler WForwardRenderer::GetTextureSampler(W_TEXTURE_SAMPLER_TYPE type) const {
	return m_sampler;
}
