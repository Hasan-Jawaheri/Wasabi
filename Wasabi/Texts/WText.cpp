#include "WText.h"
#include "../Core/WCore.h"
#include "../Renderers/WRenderer.h"
#include "../Images/WImage.h"
#include "../Materials/WEffect.h"
#include "../Materials/WMaterial.h"
#include "../Geometries/WGeometry.h"
#include "../WindowAndInput/WWindowAndInputComponent.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h"

struct TextVertex {
	WVector2 pos, uv;
	WColor col;
};

class TextGeometry : public WGeometry {
public:
	TextGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int index) const {
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

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.input_layouts = {W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_2), // UV
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4), // color
		})};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(location = 0) in  vec2 inPos;\n"
			"layout(location = 1) in  vec2 inUV;\n"
			"layout(location = 2) in  vec4 inCol;\n"
			"layout(location = 0) out vec2 outUV;\n"
			"layout(location = 1) out vec4 outCol;\n"
			""
			"void main() {\n"
			"	outUV = inUV;\n"
			"	outCol = inCol;\n"
			"	gl_Position = vec4(inPos.xy, 0.0, 1.0);\n"
			"}\n"
		);
	}
};

class TextPS : public WShader {
public:
	TextPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 0),
		};
		LoadCodeGLSL(
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"#extension GL_ARB_shading_language_420pack : enable\n"
			""
			"layout(binding = 0) uniform sampler2D sampler;\n"
			"layout(location = 0) in vec2 inUV;\n"
			"layout(location = 1) in vec4 inCol;\n"
			"layout(location = 0) out vec4 outFragColor;\n"
			""
			"void main() {\n"
			"	float c = texture(sampler, inUV).r;\n"
			"	outFragColor = vec4(c) * inCol;\n"
			"}\n"
		);
	}
};

WTextComponent::WTextComponent(Wasabi* app) : m_app(app) {
	m_curFont = -1;
	m_curColor = WColor(0.1f, 0.6f, 0.2f, 1.0f);

	m_textEffect = nullptr;
}

WTextComponent::~WTextComponent() {
	for (std::map<unsigned int, W_FONT_OBJECT>::iterator i = m_fonts.begin(); i != m_fonts.end(); i++) {
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

	ret = m_textEffect->BuildPipeline(m_app->Renderer->GetDefaultRenderTarget());
	vs->RemoveReference();
	ps->RemoveReference();

	if (ret)
		ret = CreateTextFont(0, "Arial");
	m_curFont = 0;

	return WError(W_SUCCEEDED);
}

void WTextComponent::AddFontDirectory(std::string dir) {
	if (dir.length() > 0) {
		if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
			dir += '/';
		m_directories.push_back(dir);
	}
}

WError WTextComponent::CreateTextFont(unsigned int ID, std::string fontName) {
	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
	if (obj != m_fonts.end())
		return WError(W_INVALIDPARAM);

	if (fontName.find(".ttf") == std::string::npos)
		fontName += ".ttf";

	FILE* fp = nullptr;
	fopen_s(&fp, fontName.c_str(), "rb");
	for (int i = 0; i < m_directories.size() && !fp; i++) {
		std::string n = m_directories[i] + fontName;
		fopen_s(&fp, n.c_str(), "rb");
	}
	if (!fp)
		return WError(W_FILENOTFOUND);

	fseek(fp, 0, SEEK_END);
	uint fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char* buffer = new unsigned char[fsize+1];
	fread(buffer, 1, fsize+1, fp);
	fclose(fp);

	unsigned int bmp_size = (unsigned int)m_app->engineParams["fontBmpSize"];
	unsigned int char_height = (unsigned int)m_app->engineParams["fontBmpCharHeight"];
	unsigned int num_chars = (unsigned int)m_app->engineParams["fontBmpNumChars"];
	W_FONT_OBJECT f;
	unsigned char* temp_bitmap = new unsigned char[bmp_size*bmp_size];
	f.cdata = new stbtt_bakedchar[num_chars];
	f.ID = ID;
	f.name = fontName;
	f.num_chars = num_chars;
	f.char_height = char_height;

	int res = stbtt_BakeFontBitmap(buffer, 0, char_height, temp_bitmap, bmp_size, bmp_size,
		32, num_chars, (stbtt_bakedchar*)f.cdata);
	delete[] buffer;

	if (res <= 0) {
		delete[] (stbtt_bakedchar*)f.cdata;
		delete[] temp_bitmap;
		return WError(W_ERRORUNK);
	}

	f.img = new WImage(m_app);
	WError err = f.img->CreateFromPixelsArray(temp_bitmap, bmp_size, bmp_size, false, 1, VK_FORMAT_R8_UNORM, 1);
	delete[] temp_bitmap;

	if (!err) {
		delete[] (stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		return err;
	}

	f.textMaterial = new WMaterial(m_app);
	err = f.textMaterial->SetEffect(m_textEffect);
	if (!err) {
		delete[] (stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		W_SAFE_REMOVEREF(f.textMaterial);
		return err;
	}

	unsigned int num_verts = (unsigned int)m_app->engineParams["textBatchSize"] * 4;
	unsigned int num_indices = (unsigned int)m_app->engineParams["textBatchSize"] * 6;
	TextVertex* vb = new TextVertex[num_verts];
	uint* ib = new uint[num_indices];

	for (int i = 0; i < num_indices / 6; i++) {
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
	err = f.textGeometry->CreateFromData(vb, num_verts, ib, num_indices, true);
	delete[] vb;
	delete[] ib;

	if (!err) {
		delete[] (stbtt_bakedchar*)f.cdata;
		f.img->RemoveReference();
		W_SAFE_REMOVEREF(f.textMaterial);
		W_SAFE_REMOVEREF(f.textGeometry);
		return err;
	}

	m_fonts.insert(std::pair<unsigned int, W_FONT_OBJECT>(ID, f));

	return WError(W_SUCCEEDED);
}

WError WTextComponent::DestroyFont(unsigned int ID) {
	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
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

WError WTextComponent::SetFont(unsigned int ID) {
	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
	if (obj != m_fonts.end()) {
		m_curFont = ID;
		return WError(W_SUCCEEDED);
	}

	return WError(W_INVALIDPARAM);
}

WError WTextComponent::RenderText(std::string text, float x, float y, float fHeight) {
	return RenderText(text, x, y, fHeight, m_curFont, m_curColor);
}

WError WTextComponent::RenderText(std::string text, float x, float y, float fHeight, unsigned int fontID) {
	return RenderText(text, x, y, fHeight, fontID, m_curColor);
}

WError WTextComponent::RenderText(std::string text, float x, float y, float fHeight, unsigned int fontID, WColor col) {
	if (text.length() > (unsigned int)m_app->engineParams["textBatchSize"])
		return WError(W_INVALIDPARAM);

	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(fontID);
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
	float scrWidth = m_app->WindowAndInputComponent->GetWindowWidth();
	float scrHeight = m_app->WindowAndInputComponent->GetWindowHeight();

	for (int f = 0; f < m_fonts.size(); f++) {
		W_FONT_OBJECT* font = &m_fonts[f];
		TextVertex* vb = nullptr;
		int curvert = 0;
		if (font->texts.size()) {
			font->textMaterial->SetTexture(0, font->img);
			font->textMaterial->Bind(rt);
			font->textGeometry->MapVertexBuffer((void**)&vb);
		}
		for (auto text : font->texts) {
			float x = text.x * 2.0f / scrWidth - 1.0f;
			float y = text.y * 2.0f / scrHeight - 1.0f;
			float fScale = text.fHeight / (float)font->char_height;
			float fMapSize = font->img->GetWidth();
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
			font->textGeometry->UnmapVertexBuffer();
			font->textGeometry->Draw(rt, (curvert / 4) * 2 * 3);
			font->texts.clear();
		}
	}
}

float WTextComponent::GetTextWidth(std::string text, float fHeight, unsigned int fontID) {
	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(fontID);
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
