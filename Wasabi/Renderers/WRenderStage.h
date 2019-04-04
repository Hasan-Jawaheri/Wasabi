#pragma once

#include "../Core/WCore.h"

enum W_RENDER_STAGE_TARGET {
	RENDER_STAGE_TARGET_BUFFER = 0,
	RENDER_STAGE_TARGET_BACK_BUFFER = 1,
	RENDER_STAGE_TARGET_PREVIOUS = 2,
};

enum W_RENDER_STAGE_FLAGS {
	RENDER_STAGE_FLAG_NONE = 0,
	RENDER_STAGE_FLAG_SPRITES_RENDER_STAGE = 1,
	RENDER_STAGE_FLAG_TEXTS_RENDER_STAGE = 2,
	RENDER_STAGE_FLAG_PARTICLES_RENDER_STAGE = 4,
	RENDER_STAGE_FLAG_PICKING_RENDER_STAGE = 8,
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
		WColor clearColor;

		OUTPUT_IMAGE();
		OUTPUT_IMAGE(std::string name, WColor clear = WColor(-1000000.0f, -1.0f, -1.0f, -1.0f));
		OUTPUT_IMAGE(std::string name, VkFormat format, WColor clear = WColor(-1000000.0f, -1.0f, -1.0f, -1.0f));
	};

	struct STAGE_DESCRIPTION {
		std::string name;
		W_RENDER_STAGE_TARGET target;
		std::vector<OUTPUT_IMAGE> colorOutputs;
		OUTPUT_IMAGE depthOutput;
		uint flags;
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
