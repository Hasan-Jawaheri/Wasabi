#include "WText.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

struct TextVertex {
	WVector2 pos, uv;
};

class TextGeometry : public WGeometry {
public:
	TextGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual W_VERTEX_DESCRIPTION GetVertexDescription() const {
		return W_VERTEX_DESCRIPTION({
			W_VERTEX_ATTRIBUTE("pos2", 2),
			W_ATTRIBUTE_UV,
		});
	}
};

class TextVS : public WShader {
public:
	TextVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.input_layout = W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 2), // UV
		});
		LoadCodeGLSL(
"#version 450\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
""
"layout(location = 0) in  vec2 inPos;\n"
"layout(location = 1) in  vec2 inUV;\n"
"layout(location = 0) out vec2 outUV;\n"
""
"void main() {\n"
"	outUV = inUV;\n"
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
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_FLOAT, 4, "color"),
				}),
			W_BOUND_RESOURCE(W_TYPE_SAMPLER, 1),
		};
		LoadCodeGLSL(
"#version 450\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
""
"layout(binding = 0) uniform UBO {\n"
"	vec4 color;\n"
"} ubo;\n"
"layout(binding = 1) uniform sampler2D sampler;\n"
"layout(location = 0) in vec2 inUV;\n"
"layout(location = 0) out vec4 outFragColor;\n"
""
"void main() {\n"
"	float c = texture(sampler, inUV).r;\n"
"	outFragColor = vec4(c) * ubo.color;\n"
"}\n"
		);
	}
};

WTextComponent::WTextComponent(Wasabi* app) : m_app(app) {
	m_curFont;
	m_curColor = WColor(0.1f, 0.6f, 0.2f, 1.0f);
}

WTextComponent::~WTextComponent() {
	for (std::map<unsigned int, W_FONT_OBJECT>::iterator i = m_fonts.begin(); i != m_fonts.end(); i++) {
		delete[] i->second.cdata;
		i->second.img->RemoveReference();
	}
	m_textGeometry->RemoveReference();
	m_textMaterial->RemoveReference();
}

void WTextComponent::Initialize() {
	CreateFont(0, "Arial");

	WShader* vs = new TextVS(m_app);
	vs->Load();
	WShader* ps = new TextPS(m_app);
	ps->Load();
	WEffect* textFX = new WEffect(m_app);
	textFX->SetName("textFX");
	textFX->BindShader(vs);
	textFX->BindShader(ps);

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
	textFX->SetBlendingState(bs);

	VkPipelineDepthStencilStateCreateInfo dss = {};
	dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	dss.depthTestEnable = VK_FALSE;
	dss.depthWriteEnable = VK_FALSE;
	dss.depthBoundsTestEnable = VK_FALSE;
	dss.stencilTestEnable = VK_FALSE;
	dss.front = dss.back;
	textFX->SetDepthStencilState(dss);

	textFX->BuildPipeline();
	vs->RemoveReference();
	ps->RemoveReference();
	m_textMaterial = new WMaterial(m_app);
	m_textMaterial->SetEffect(textFX);
	m_textMaterial->SetName("m_textMaterial");
	textFX->RemoveReference();

	unsigned int num_verts = (unsigned int)m_app->engineParams["textBatchSize"] * 4;
	unsigned int num_indices = (unsigned int)m_app->engineParams["textBatchSize"] * 6;
	TextVertex* vb = new TextVertex[num_verts];
	DWORD* ib = new DWORD[num_indices];

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

	m_textGeometry = new TextGeometry(m_app);
	m_textGeometry->CreateFromData(vb, num_verts, ib, num_indices, true);
	delete[] vb;
	delete[] ib;
}

void WTextComponent::AddFontDirectory(std::string dir) {
	if (dir.length() > 0) {
		if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
			dir += '/';
		m_directories.push_back(dir);
	}
}

WError WTextComponent::CreateFont(unsigned int ID, std::string fontName) {
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

	fpos_t fsize;
	fseek(fp, 0, SEEK_END);
	fgetpos(fp, &fsize);
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
		delete[] f.cdata;
		delete[] temp_bitmap;
		return WError(W_ERRORUNK);
	}

	f.img = new WImage(m_app);
	WError err = f.img->CretaeFromPixelsArray(temp_bitmap, bmp_size, bmp_size, 1, VK_FORMAT_R8_UNORM, 1);
	delete[] temp_bitmap;

	if (!err) {
		delete[] f.cdata;
		f.img->RemoveReference();
		return err;
	}

	m_fonts.insert(std::pair<unsigned int, W_FONT_OBJECT>(ID, f));

	return WError(W_SUCCEEDED);
}

WError WTextComponent::DestroyFont(unsigned int ID) {
	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(ID);
	if (obj != m_fonts.end()) {
		delete[] obj->second.cdata;
		obj->second.img->RemoveReference();
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

WError WTextComponent::RenderText(std::string text, int x, int y, float fHeight) {
	return RenderText(text, x, y, fHeight, m_curFont, m_curColor);
}

WError WTextComponent::RenderText(std::string text, int x, int y, float fHeight, unsigned int fontID) {
	return RenderText(text, x, y, fHeight, fontID, m_curColor);
}

WError WTextComponent::RenderText(std::string text, int x, int y, float fHeight, unsigned int fontID, WColor col) {
	if (text.length() > (unsigned int)m_app->engineParams["textBatchSize"])
		return WError(W_INVALIDPARAM);

	std::map<unsigned int, W_FONT_OBJECT>::iterator obj = m_fonts.find(fontID);
	if (obj != m_fonts.end()) {
		W_RENDERING_TEXT t;
		t.str = text;
		t.col = col;
		t.fHeight = fHeight;
		t.font = obj->second;
		t.x = x;
		t.y = y;
		m_texts.push_back(t);
		return WError(W_SUCCEEDED);
	}

	return WError(W_INVALIDPARAM);
}

void WTextComponent::Render() {
	float scrWidth = m_app->WindowComponent->GetWindowWidth();
	float scrHeight = m_app->WindowComponent->GetWindowHeight();
	for (auto text : m_texts) {
		TextVertex* vb;
		int curvert = 0;
		m_textGeometry->MapVertexBuffer((void**)&vb);
		float x = text.x * 2.0f / scrWidth - 1.0f;
		float y = text.y * 2.0f / scrHeight - 1.0f;
		float fScale = text.fHeight / (float)text.font.char_height;
		float fMapSize = text.font.img->GetWidth();
		float maxCharHeight = 0.0f;
		for (int i = 0; i < text.str.length(); i++) {
			if (text.str[i] == '\n')
				continue;
			stbtt_bakedchar *cd = &((stbtt_bakedchar*)text.font.cdata)[(uint32_t)text.str[i] - 32];
			maxCharHeight = max(maxCharHeight, cd->y1 - cd->y0);
		}
		maxCharHeight = (maxCharHeight * fScale) * 2.0f / scrHeight;
		for (int i = 0; i < text.str.length(); i++) {
			char c = text.str[i];
			if (c == '\n') {
				x = text.x * 2.0f / scrWidth - 1.0f;
				y += maxCharHeight + 10 / scrHeight;
				continue;
			}
			stbtt_bakedchar *cd = &((stbtt_bakedchar*)text.font.cdata)[(uint32_t)c - 32];

			float cw = ((cd->x1 - cd->x0) * fScale) * 2.0f / scrWidth;
			float ch = ((cd->y1 - cd->y0) * fScale) * 2.0f / scrHeight;

			vb[curvert + 0].pos = WVector2(x, y + maxCharHeight - ch);
			vb[curvert + 0].uv = WVector2(cd->x0, cd->y0) / fMapSize;
			vb[curvert + 1].pos = WVector2(x + cw, y + maxCharHeight - ch);
			vb[curvert + 1].uv = WVector2(cd->x1, cd->y0) / fMapSize;
			vb[curvert + 2].pos = WVector2(x, y + maxCharHeight);
			vb[curvert + 2].uv = WVector2(cd->x0, cd->y1) / fMapSize;
			vb[curvert + 3].pos = WVector2(x + cw, y + maxCharHeight);
			vb[curvert + 3].uv = WVector2(cd->x1, cd->y1) / fMapSize;
			curvert += 4;

			x += (cd->xadvance * fScale) * 2.0f / scrWidth;
		}
		m_textGeometry->UnmapVertexBuffer();
		m_textMaterial->SetVariableFloatArray("color", (float*)&text.col, 4);
		m_textMaterial->SetTexture(1, text.font.img);
		m_textMaterial->Bind();
		m_textGeometry->Draw((curvert / 4) * 2);
	}
	m_texts.clear();
}

unsigned int WTextComponent::GetTextWidth(std::string text, float fHeight, unsigned int fontID) {
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
			stbtt_bakedchar *charData = &((stbtt_bakedchar*)obj->second.cdata)[(uint32_t)letter - 32];
			curWidth += charData->xadvance * fScale;
			maxWidth = max(curWidth, maxWidth);
		}
		return maxWidth;
	}

	return 0;
}
