#pragma once

#include "../Core/WCore.h"

enum W_RENDER_STAGE_TARGET {
	RENDER_STAGE_TARGET_BUFFER = 0,
	RENDER_STAGE_TARGET_BACK_BUFFER = 1,
	RENDER_STAGE_TARGET_PREVIOUS = 2,
};

class WRenderStage {
	friend class WRenderer;

protected:
	class Wasabi* m_app;
	class WRenderTarget* m_renderTarget;
	std::vector<class WImage*> m_colorOutputs;
	class WImage* m_depthOutput;

	struct OUTPUT_IMAGE {
		std::string name;
		bool isFromPreviousStage;
		VkFormat format;
		uint numComponents;
		size_t componentSize;
		WColor clearColor;

		OUTPUT_IMAGE();
		OUTPUT_IMAGE(std::string name, WColor clear = WColor(-1000000.0f, -1.0f, -1.0f, -1.0f));
		OUTPUT_IMAGE(std::string name, VkFormat format, uint numComponents, size_t componentSize, WColor clear = WColor(-1000000.0f, -1.0f, -1.0f, -1.0f));
	};

	struct STAGE_DESCRIPTION {
		std::string name;
		W_RENDER_STAGE_TARGET target;
		std::vector<OUTPUT_IMAGE> colorOutputs;
		OUTPUT_IMAGE depthOutput;
	} m_stageDescription;

public:
	WRenderStage(class Wasabi* const app);

	class WRenderTarget* GetRenderTarget() const;
	class WImage* GetOutputImage(std::string name) const;

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint width, uint height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint filter) = 0;
	virtual void Cleanup();
	virtual WError Resize(uint width, uint height);
};
