#include "WRenderStage.h"
#include "WRenderer.h"
#include "../Images/WImage.h"
#include "../Images/WRenderTarget.h"

WRenderStage::OUTPUT_IMAGE::OUTPUT_IMAGE() {
	name = "";
}

WRenderStage::OUTPUT_IMAGE::OUTPUT_IMAGE(std::string n, WColor clear) {
	name = n;
	isFromPreviousStage = true;
	clearColor = clear;
}

WRenderStage::OUTPUT_IMAGE::OUTPUT_IMAGE(std::string n, VkFormat f, uint c, size_t s, WColor clear) {
	name = n;
	isFromPreviousStage = false;
	format = f;
	numComponents = c;
	componentSize = s;
	clearColor = clear;
}

WRenderStage::WRenderStage(class Wasabi* const app) {
	m_app = app;
	m_renderTarget = nullptr;
	m_depthOutput = nullptr;
	m_stageDescription = {};
}

WRenderTarget* WRenderStage::GetRenderTarget() const {
	return m_renderTarget;
}

WImage* WRenderStage::GetOutputImage(std::string name) const {
	if (name == m_stageDescription.depthOutput.name)
		return m_depthOutput;

	for (uint i = 0; i < m_stageDescription.colorOutputs.size(); i++) {
		if (name == m_stageDescription.colorOutputs[i].name)
			return m_colorOutputs[i];
	}

	return nullptr;
}

WError WRenderStage::Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height) {
	Cleanup();

	if (m_stageDescription.target == RENDER_STAGE_TARGET_BUFFER) {
		if (m_stageDescription.colorOutputs.size() == 0 && m_stageDescription.depthOutput.name == "")
			return WError(W_NOTVALID);

		m_renderTarget = m_app->RenderTargetManager->CreateRenderTarget();

		for (uint i = 0; i < m_stageDescription.colorOutputs.size(); i++) {
			if (m_stageDescription.colorOutputs[i].name == "")
				return WError(W_NOTVALID);

			if (m_stageDescription.colorOutputs[i].isFromPreviousStage) {
				WImage* previousImg = nullptr;
				for (int i = previousStages.size() - 2; i >= 0 && !previousImg; i--)
					previousImg = previousStages[i]->GetOutputImage(m_stageDescription.colorOutputs[i].name);
				if (!previousImg)
					return WError(W_NOTVALID);
				previousImg->AddReference();
				m_colorOutputs.push_back(previousImg);
			} else {
				m_colorOutputs.push_back(new WImage(m_app));
			}
		}

		if (m_stageDescription.depthOutput.name != "") {
			if (m_stageDescription.depthOutput.isFromPreviousStage) {
				WImage* previousImg = nullptr;
				for (int i = previousStages.size() - 2; i >= 0 && !previousImg; i--)
					previousImg = previousStages[i]->GetOutputImage(m_stageDescription.depthOutput.name);
				if (!previousImg)
					return WError(W_NOTVALID);
				previousImg->AddReference();
				m_depthOutput = previousImg;
			} else {
				m_depthOutput = new WImage(m_app);
			}
		}
	} else if (m_stageDescription.target == RENDER_STAGE_TARGET_BACK_BUFFER) {
		m_renderTarget = m_app->RenderTargetManager->CreateRenderTarget();
	} else if (m_stageDescription.target == RENDER_STAGE_TARGET_PREVIOUS) {
		m_renderTarget = previousStages[previousStages.size() - 2]->m_renderTarget;
	}

	return Resize(width, height);
}

void WRenderStage::Cleanup() {
	if (m_stageDescription.target != RENDER_STAGE_TARGET_PREVIOUS)
		W_SAFE_REMOVEREF(m_renderTarget);
	for (auto it = m_colorOutputs.begin(); it != m_colorOutputs.end(); it++)
		W_SAFE_REMOVEREF((*it));
	m_colorOutputs.clear();
	W_SAFE_REMOVEREF(m_depthOutput);
}

WError WRenderStage::Resize(uint width, uint height) {
	if (m_stageDescription.target == RENDER_STAGE_TARGET_BUFFER) {
		OUTPUT_IMAGE desc;
		for (uint i = 0; i < m_stageDescription.colorOutputs.size(); i++) {
			desc = m_stageDescription.colorOutputs[i];
			WImage* output = m_colorOutputs[i];
			if (output) {
				WError status = output->CreateFromPixelsArray(nullptr, width, height, desc.format, true);
				if (!status)
					return status;
			}
		}
		desc = m_stageDescription.depthOutput;
		if (m_depthOutput) {
			WError status = m_depthOutput->CreateFromPixelsArray(nullptr, width, height, desc.format, true);
			if (!status)
				return status;
		}

		WError status = m_renderTarget->Create(width, height, m_colorOutputs, m_depthOutput);
		if (!status)
			return status;
	} else if (m_stageDescription.target == RENDER_STAGE_TARGET_BACK_BUFFER) {
		VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		VkFormat depthFormat; // Find a suitable depth format
		VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(m_app->GetVulkanPhysicalDevice(), &depthFormat);
		if (!validDepthFormat)
			return WError(W_HARDWARENOTSUPPORTED);

		vector<VkImageView> swapchainViews;
		for (int i = 0; i < m_app->Renderer->GetSwapchain()->imageCount; i++)
			swapchainViews.push_back(m_app->Renderer->GetSwapchain()->buffers[i].view);
		WError status = m_renderTarget->Create(width, height, swapchainViews.data(), swapchainViews.size(), colorFormat, depthFormat);
		if (!status)
			return status;
	}

	for (uint i = 0; i < m_stageDescription.colorOutputs.size(); i++) {
		if (abs(-1000000.0f - m_stageDescription.colorOutputs[i].clearColor.r) > W_EPSILON)
			m_renderTarget->SetClearColor(m_stageDescription.colorOutputs[i].clearColor, i);
	}
	if (abs(-1000000.0f - m_stageDescription.depthOutput.clearColor.r) > W_EPSILON)
		m_renderTarget->SetClearColor(m_stageDescription.depthOutput.clearColor, m_stageDescription.colorOutputs.size());

	return WError(W_SUCCEEDED);
}
