#pragma once

#include "Wasabi/Renderers/WRenderStage.hpp"
#include "Wasabi/Renderers/Common/WRenderFragment.hpp"
#include "Wasabi/Materials/WEffect.hpp"

class WGBufferVS : public WShader {
public:
	WGBufferVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc();
};

class WGBufferAnimatedVS : public WShader {
public:
	WGBufferAnimatedVS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc();
};

class WGBufferPS : public WShader {
public:
	WGBufferPS(class Wasabi* const app);
	virtual void Load(bool bSaveData = false);
	static W_SHADER_DESC GetDesc();
};

/**
 * GBuffer layout:
 * Depth attachment: D16 - depth
 * Color attachment 0: R8G8B8A8 - albedo (diffuse color)
 * Color attachment 1: R16G16B16A16 - rg is packed normals, b is specular power, a is specular intensity
 * Code for packing and unpacking of normals can be found in `src/Wasabi/Renderers/Common/Shaders/utils.glsl`
 * (WasabiPackNormalSpheremapTransform and WasabiUnpackNormalSpheremapTransform)
 */
class WGBufferRenderStage : public WRenderStage {
	WObjectsRenderFragment* m_objectsFragment;
	class WMaterial* m_perFrameMaterial;

	WObjectsRenderFragment* m_animatedObjectsFragment;
	class WMaterial* m_perFrameAnimatedMaterial;

	WGBufferVS* m_defaultVS;
	WGBufferAnimatedVS* m_defaultAnimatedVS;
	WGBufferPS* m_defaultPS;
	WEffect* m_defaultFX;
	WEffect* m_defaultAnimatedFX;

public:
	WGBufferRenderStage(class Wasabi* const app);

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height);
	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter);
	virtual void Cleanup();
	virtual WError Resize(uint32_t width, uint32_t height);

	WGBufferVS* GetDefaultVertexShader() const;
	WGBufferAnimatedVS* GetDefaultAnimatedVertexShader() const;
	WGBufferPS* GetDefaultPixelShader() const;
	WEffect* GetDefaultEffect() const;
	WEffect* GetDefaultAnimatedEffect() const;
};
