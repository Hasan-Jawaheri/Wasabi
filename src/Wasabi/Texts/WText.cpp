#include "Wasabi/Texts/WText.h"
#include "Wasabi/Core/WCore.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Geometries/WGeometry.h"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <filesystem>
#include <iostream>
namespace stdfs = std::filesystem;

struct TextVertex {
	WVector2 pos, uv;
	WColor col;
};

class TextGeometry : public WGeometry {
public:
	TextGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual uint32_t GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(uint32_t index) const {
		UNREFERENCED_PARAMETER(index);
		return W_VERTEX_DESCRIPTION({
			W_VERTEX_ATTRIBUTE("pos2", 2),
			W_ATTRIBUTE_UV,
			W_VERTEX_ATTRIBUTE("color", 4),
		});
	}
};

class TextVS : public WShader {
public:
	TextVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4), // color
		})};
		vector<uint8_t> code {
			#include "Shaders/text.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

class TextPS : public WShader {
public:
	TextPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_TEXTURE, 0, "textureFont"),
		};
		vector<uint8_t> code {
			#include "Shaders/text.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

WTextComponent::WTextComponent(Wasabi* app) : m_app(app) {
	m_curFont = (uint32_t)-1;
	m_curColor = WColor(0.1f, 0.6f, 0.2f, 1.0f);

	m_textEffect = nullptr;
}

WTextComponent::~WTextComponent() {
	for (std::map<uint32_t, W_FONT_OBJECT>::iterator i = m_fonts.begin(); i != m_fonts.end(); i++) {
		delete[] (stbtt_bakedchar*)i->second.cdata;
		i->second.img->RemoveReference();
		W_SAFE_REMOVEREF(i->second.textGeometry);
		W_SAFE_REMOVEREF(i->second.textMaterial);
	}
	W_SAFE_REMOVEREF(m_textEffect);
}

WError WTextComponent::Initialize() {
	WShader* vs = new TextVS(m_app);
	vs->Load();
	WShader* ps = new TextPS(m_app);
	ps->Load();
	m_textEffect = new WEffect(m_app);
	m_textEffect->SetName("textFX");
	m_app->FileManager->AddDefaultAsset(m_textEffect->GetName(), m_textEffect);
	WError ret = m_textEffect->BindShader(vs);
	if (!ret) {
		vs->RemoveReference();
		ps->RemoveReference();
		return ret;
	}
	ret = m_textEffect->BindShader(ps);
	if (!ret) {
		vs->RemoveReference();
		ps->RemoveReference();
		return ret;
	}

	VkPipelineColorBlendAttachmentState bs;
	bs.colorWriteMask = 0xf;
	bs.blendEnable = VK_TRUE;
	bs.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	bs.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	bs.colorBlendOp = VK_BLEND_OP_ADD;
	bs.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	bs.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	bs.alphaBlendOp = VK_BLEND_OP_ADD;
	bs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_textEffect->SetBlendingState(bs);

	VkPipelineDepthStencilStateCreateInfo dss = {};
	dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	dss.depthTestEnable = VK_FALSE;
	dss.depthWriteEnable = VK_FALSE;
	dss.depthBoundsTestEnable = VK_FALSE;
	dss.stencilTestEnable = VK_FALSE;
	dss.front = dss.back;
	m_textEffect->SetDepthStencilState(dss);

	ret = m_textEffect->BuildPipeline(m_app->Renderer->GetRenderTarget(m_app->Renderer->GetTextsRenderStageName()));
	vs->RemoveReference();
	ps->RemoveReference();

	if (ret) {
		#if defined(__linux__)
		ret = CreateTextFont(0, "Ubuntu-M");
		#else
		ret = CreateTextFont(0, "Arial");
		#endif
	}
	m_curFont = 0;

	return ret;
}

void WTextComponent::AddFontDirectory(std::string dir) {
	if (dir.length() > 0) {
		if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
			dir += '/';
		m_directories.push_back(dir);
	}
}

WError WTextComponent::CreateTextFont(uint32_t ID, std::string fontName) {
	std::map<uint32_t, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
	if (obj != m_fonts.end())
		return WError(W_INVALIDPARAM);


	if (fontName.find(".ttf") == std::string::npos)
		fontName += ".ttf";

	std::ifstream ttfFile;
	ttfFile.open(fontName.c_str(), ios::in | ios::binary);
	if (!ttfFile.is_open()) {
		std::for_each(fontName.begin(), fontName.end(), [](char& c) { c = (char)std::toupper(c); });
		ttfFile.open(fontName.c_str(), ios::in | ios::binary);
	}
	for (int i = 0; i < m_directories.size() && !ttfFile.is_open(); i++) {
		for (auto& dirPath : stdfs::recursive_directory_iterator(m_directories[i])) {
			stdfs::path p = dirPath;
			if (p.has_filename()) {
				std::string curFontName = p.filename().string();
				std::for_each(curFontName.begin(), curFontName.end(), [](char& c) { c = (char)std::toupper(c); });
				if (curFontName == fontName)
					ttfFile.open(p, ios::in | ios::binary);
			}
		}
	}
	if (!ttfFile.is_open())
		return WError(W_FILENOTFOUND);

	ttfFile.seekg(0, std::ios::end);
	size_t fsize = (size_t)ttfFile.tellg();
	ttfFile.seekg(0, std::ios::beg);

	unsigned char* buffer = new unsigned char[fsize+1];
	ttfFile.read(reinterpret_cast<char*>(buffer), fsize);
	ttfFile.close();

	uint32_t bmp_size = m_app->GetEngineParam<uint32_t>("fontBmpSize");
	uint32_t char_height = m_app->GetEngineParam<uint32_t>("fontBmpCharHeight");
	uint32_t num_chars = m_app->GetEngineParam<uint32_t>("fontBmpNumChars");
	W_FONT_OBJECT f;
	unsigned char* temp_bitmap = new unsigned char[bmp_size*bmp_size];
	f.cdata = new stbtt_bakedchar[num_chars];
	f.ID = ID;
	f.name = fontName;
	f.num_chars = num_chars;
	f.char_height = char_height;

	int res = stbtt_BakeFontBitmap(buffer, 0, (float)char_height, temp_bitmap, (int)bmp_size, (int)bmp_size, 32, (int)num_chars, (stbtt_bakedchar*)f.cdata);
	delete[] buffer;

	if (res <= 0) {
		delete[] (stbtt_bakedchar*)f.cdata;
		delete[] temp_bitmap;
		return WError(W_ERRORUNK);
	}

	f.img = m_app->ImageManager->CreateImage(temp_bitmap, bmp_size, bmp_size, VK_FORMAT_R8_UNORM);
	delete[] temp_bitmap;

	if (!f.img) {
		delete[] (stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		return WError(W_OUTOFMEMORY);
	}

	f.textMaterial = m_textEffect->CreateMaterial();
	if (!f.textMaterial) {
		delete[] (stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		W_SAFE_REMOVEREF(f.textMaterial);
		return WError(W_OUTOFMEMORY);
	}

	WError err = f.textMaterial->SetTexture(0, f.img);
	if (!err) {
		delete[](stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		W_SAFE_REMOVEREF(f.textMaterial);
		return err;
	}

	uint32_t num_verts = m_app->GetEngineParam<uint32_t>("textBatchSize") * 4;
	uint32_t num_indices = m_app->GetEngineParam<uint32_t>("textBatchSize") * 6;
	TextVertex* vb = new TextVertex[num_verts];
	uint* ib = new uint[num_indices];

	for (uint32_t i = 0; i < num_indices / 6; i++) {
		// tri 1
		ib[i * 6 + 0] = i * 4 + 0;
		ib[i * 6 + 1] = i * 4 + 1;
		ib[i * 6 + 2] = i * 4 + 2;
		// tri 2
		ib[i * 6 + 3] = i * 4 + 1;
		ib[i * 6 + 4] = i * 4 + 3;
		ib[i * 6 + 5] = i * 4 + 2;
	}

	f.textGeometry = new TextGeometry(m_app);
	err = f.textGeometry->CreateFromData(vb, num_verts, ib, num_indices, W_GEOMETRY_CREATE_VB_DYNAMIC | W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME);
	delete[] vb;
	delete[] ib;

	if (!err) {
		delete[] (stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		W_SAFE_REMOVEREF(f.textMaterial);
		W_SAFE_REMOVEREF(f.textGeometry);
		return err;
	}

	m_fonts.insert(std::pair<uint32_t, W_FONT_OBJECT>(ID, f));

	return WError(W_SUCCEEDED);
}

WError WTextComponent::DestroyFont(uint32_t ID) {
	std::map<uint32_t, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
	if (obj != m_fonts.end()) {
		delete[] (stbtt_bakedchar*)obj->second.cdata;
		obj->second.img->RemoveReference();
		obj->second.textGeometry->RemoveReference();
		obj->second.textMaterial->RemoveReference();
		m_fonts.erase(ID);
		return WError(W_SUCCEEDED);
	}

	return WError(W_INVALIDPARAM);
}

WError WTextComponent::SetTextColor(WColor col) {
	m_curColor = col;
	return WError(W_SUCCEEDED);
}

WError WTextComponent::SetFont(uint32_t ID) {
	std::map<uint32_t, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
	if (obj != m_fonts.end()) {
		m_curFont = ID;
		return WError(W_SUCCEEDED);
	}

	return WError(W_INVALIDPARAM);
}

WError WTextComponent::RenderText(std::string text, float x, float y, float fHeight) {
	return RenderText(text, x, y, fHeight, m_curFont, m_curColor);
}

WError WTextComponent::RenderText(std::string text, float x, float y, float fHeight, uint32_t fontID) {
	return RenderText(text, x, y, fHeight, fontID, m_curColor);
}

WError WTextComponent::RenderText(std::string text, float x, float y, float fHeight, uint32_t fontID, WColor col) {
	if (text.length() > m_app->GetEngineParam<uint32_t>("textBatchSize"))
		return WError(W_INVALIDPARAM);

	std::map<uint32_t, W_FONT_OBJECT>::iterator obj = m_fonts.find(fontID);
	if (obj != m_fonts.end()) {
		W_RENDERING_TEXT t;
		t.str = text;
		t.col = col;
		t.fHeight = fHeight;
		t.x = x;
		t.y = y;
		obj->second.texts.push_back(t);
		return WError(W_SUCCEEDED);
	}

	return WError(W_INVALIDPARAM);
}

void WTextComponent::Render(WRenderTarget* rt) {
	float scrWidth = (float)m_app->WindowAndInputComponent->GetWindowWidth();
	float scrHeight = (float)m_app->WindowAndInputComponent->GetWindowHeight();

	bool isEffectBound = false;
	for (uint32_t f = 0; f < m_fonts.size(); f++) {
		W_FONT_OBJECT* font = &m_fonts[f];
		TextVertex* vb = nullptr;
		int curvert = 0;
		if (font->texts.size()) {
			if (!isEffectBound)
				m_textEffect->Bind(rt);
			font->textMaterial->Bind(rt);
			isEffectBound = true;
			font->textGeometry->MapVertexBuffer((void**)&vb, W_MAP_WRITE);
		}
		for (auto text : font->texts) {
			float x = text.x * 2.0f / scrWidth - 1.0f;
			float y = text.y * 2.0f / scrHeight - 1.0f;
			float fScale = text.fHeight / (float)font->char_height;
			float fMapSize = (float)font->img->GetWidth();
			float maxCharHeight = 0.0f;
			float maxyoff = -FLT_MAX, minyoff = FLT_MAX;
			for (int i = 0; i < text.str.length(); i++) {
				if (text.str[i] == '\n')
					continue;
				stbtt_bakedchar *cd = &((stbtt_bakedchar*)font->cdata)[(uint)text.str[i] - 32];
				minyoff = fmin(minyoff, cd->yoff);
				maxyoff = fmax(maxyoff, cd->yoff);
			}
			maxCharHeight = (text.fHeight * fScale) * 2.0f / scrHeight;
			for (int i = 0; i < text.str.length(); i++) {
				char c = text.str[i];
				if (c == '\n') {
					x = text.x * 2.0f / scrWidth - 1.0f;
					y += maxCharHeight + 0.0f / scrHeight;
					continue;
				}
				stbtt_bakedchar *cd = &((stbtt_bakedchar*)font->cdata)[(uint)c - 32];

				float cw = ((cd->x1 - cd->x0) * fScale) * 2.0f / scrWidth;
				float ch = ((cd->y1 - cd->y0) * fScale) * 2.0f / scrHeight;
				float yo = ((cd->yoff - minyoff) * fScale) * 2.0f / scrHeight;

				vb[curvert + 0].pos = WVector2(x, y + yo);
				vb[curvert + 0].uv = WVector2(cd->x0, cd->y0) / fMapSize;
				vb[curvert + 0].col = text.col;
				vb[curvert + 1].pos = WVector2(x + cw, y + yo);
				vb[curvert + 1].uv = WVector2(cd->x1, cd->y0) / fMapSize;
				vb[curvert + 1].col = text.col;
				vb[curvert + 2].pos = WVector2(x, y + ch + yo);
				vb[curvert + 2].uv = WVector2(cd->x0, cd->y1) / fMapSize;
				vb[curvert + 2].col = text.col;
				vb[curvert + 3].pos = WVector2(x + cw, y + ch + yo);
				vb[curvert + 3].uv = WVector2(cd->x1, cd->y1) / fMapSize;
				vb[curvert + 3].col = text.col;
				curvert += 4;

				x += (cd->xadvance * fScale) * 2.0f / scrWidth;
			}
		}
		if (font->texts.size()) {
			font->textGeometry->UnmapVertexBuffer(false);
			font->textGeometry->Draw(rt, (curvert / 4) * 2 * 3);
			font->texts.clear();
		}
	}
}

float WTextComponent::GetTextWidth(std::string text, float fHeight, uint32_t fontID) {
	std::map<uint32_t, W_FONT_OBJECT>::iterator obj = m_fonts.find(fontID);
	if (obj != m_fonts.end()) {
		float fScale = fHeight / (float)obj->second.char_height;
		float curWidth = 0;
		float maxWidth = 0;
		for (auto letter : text) {
			if (letter == '\n') {
				curWidth = 0;
				continue;
			}
			stbtt_bakedchar *charData = &((stbtt_bakedchar*)obj->second.cdata)[(uint)letter - 32];
			curWidth += charData->xadvance * fScale;
			maxWidth = fmax(curWidth, maxWidth);
		}
		return maxWidth;
	}

	return 0;
}
