#include "Wasabi/Renderers/WRenderStage.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/Images/WRenderTarget.h"

WRenderStage::OUTPUT_IMAGE::OUTPUT_IMAGE() {
	name = "";
	isFromPreviousStage = true;
	clearColor = WColor(-1000000.0f, -1.0f, -1.0f, -1.0f);
}

WRenderStage::OUTPUT_IMAGE::OUTPUT_IMAGE(std::string n, WColor clear) {
	name = n;
	isFromPreviousStage = true;
	clearColor = clear;
}

WRenderStage::OUTPUT_IMAGE::OUTPUT_IMAGE(std::string n, VkFormat f, WColor clear) {
	name = n;
	isFromPreviousStage = false;
	format = f;
	clearColor = clear;
}

WRenderStage::WRenderStage(class Wasabi* const app) : m_stageDescription({}) {
	m_app = app;
	m_renderTarget = nullptr;
	m_depthOutput = nullptr;
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
		m_renderTarget->SetName("RenderTarget-" + m_stageDescription.name);

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
				m_colorOutputs.push_back(nullptr); // will be created in Resize
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
				m_depthOutput = nullptr; // will be created in Resize
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
			if (!desc.isFromPreviousStage) {
				WImage* output = m_colorOutputs[i];
				if (output) {
					WError status = output->CreateFromPixelsArray(nullptr, width, height, desc.format, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
					if (!status)
						return status;
				} else {
					m_colorOutputs[i] = m_app->ImageManager->CreateImage(nullptr, width, height, desc.format, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
					if (!m_colorOutputs[i])
						return WError(W_OUTOFMEMORY);
				}
			}
		}
		desc = m_stageDescription.depthOutput;
		if (desc.name != "" && !desc.isFromPreviousStage) {
			if (m_depthOutput) {
				WError status = m_depthOutput->CreateFromPixelsArray(nullptr, width, height, desc.format, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
				if (!status)
					return status;
			} else {
				m_depthOutput = m_app->ImageManager->CreateImage(nullptr, width, height, desc.format, W_IMAGE_CREATE_TEXTURE | W_IMAGE_CREATE_RENDER_TARGET_ATTACHMENT);
				if (!m_depthOutput)
					return WError(W_OUTOFMEMORY);
			}
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
		if (abs(-1000000.0f - m_stageDescription.colorOutputs[i].clearColor.r) > W_EPSILON && !m_stageDescription.colorOutputs[i].isFromPreviousStage)
			m_renderTarget->SetClearColor(m_stageDescription.colorOutputs[i].clearColor, i);
	}
	if (abs(-1000000.0f - m_stageDescription.depthOutput.clearColor.r) > W_EPSILON && !m_stageDescription.depthOutput.isFromPreviousStage)
		m_renderTarget->SetClearColor(m_stageDescription.depthOutput.clearColor, m_renderTarget->GetNumColorOutputs());

	return WError(W_SUCCEEDED);
}
