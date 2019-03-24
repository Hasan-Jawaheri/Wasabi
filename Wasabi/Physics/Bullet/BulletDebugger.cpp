#if 0

#include "BulletDebugger.h"
#include "../../Cameras/WCamera.h"
#include "../../WindowAndInput/WWindowAndInputComponent.h"
#include "../../Texts/WText.h"
#include "../../Renderers/ForwardRenderer/WForwardRenderer.h"
#include "../../Objects/WObject.h"
#include "../../Geometries/WGeometry.h"
#include "../../Materials/WMaterial.h"
#include "../../Materials/WEffect.h"

struct LineVertex {
	WVector3 pos;
	WColor col;
};

class LinesVS : public WShader {
public:
	LinesVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4), // color
		}) };
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "gProjection"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "gView"), // view
			}),
		};
		LoadCodeGLSL("\
			#version 450\n\
			#extension GL_ARB_separate_shader_objects : enable\n\
			#extension GL_ARB_shading_language_420pack : enable\n\
			layout(location = 0) in vec3 inPos;\n\
			layout(location = 1) in vec4 inCol;\n\
			layout(location = 0) out vec4 outCol;\n\
			layout(binding = 0) uniform UBO {\n\
				mat4x4 gProjection;\n\
				mat4x4 gView;\n\
			} ubo;\n\
			void main() {\n\
				outCol = inCol;\n\
				gl_Position = ubo.gProjection * ubo.gView * vec4(inPos.xyz, 1.0);\n\
			}"
		);
	}
};

class LinesPS : public WShader {
public:
	LinesPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load() {
		m_desc.type = W_FRAGMENT_SHADER;
		LoadCodeGLSL("\
			#version 450\n\
			#extension GL_ARB_separate_shader_objects : enable\n\
			#extension GL_ARB_shading_language_420pack : enable\n\
			layout(location = 0) in vec4 inCol;\n\
			layout(location = 0) out vec4 outFragColor;\n\
			void main() {\n\
				outFragColor = inCol;\n\
			}"
		);
	}
};

class WLinesGeometry : public WGeometry {
public:
	WLinesGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual unsigned int GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(unsigned int index) const {
		return W_VERTEX_DESCRIPTION({
			W_ATTRIBUTE_POSITION,
			W_VERTEX_ATTRIBUTE("color", 4),
			});
	}
	virtual size_t GetVertexDescriptionSize(unsigned int layout_index = 0) const {
		return sizeof(LineVertex);
	}
};

BulletDebugger::BulletDebugger(WBulletPhysics* physics, uint maxLines) : Wasabi(), btIDebugDraw() {
	m_physics = physics;
	m_keep_running = true;
	m_debugMode = 1;
	m_maxLines = maxLines;
	fYaw = 0;
	fPitch = 30;
	fDist = -15;
	m_curLines = 0;
}

void BulletDebugger::ApplyMousePivot() {
	WCamera* cam = CameraManager->GetDefaultCamera();
	static bool bMouseHidden = false;
	static int lx, ly;
	if (WindowAndInputComponent->MouseClick(MOUSE_LEFT)) {
		if (!bMouseHidden) {
			WindowAndInputComponent->ShowCursor(false);
			bMouseHidden = true;

			lx = WindowAndInputComponent->MouseX(MOUSEPOS_DESKTOP, 0);
			ly = WindowAndInputComponent->MouseY(MOUSEPOS_DESKTOP, 0);

			WindowAndInputComponent->SetMousePosition(800 / 2, 600 / 2, MOUSEPOS_VIEWPORT);
		}

		int mx = WindowAndInputComponent->MouseX(MOUSEPOS_VIEWPORT, 0);
		int my = WindowAndInputComponent->MouseY(MOUSEPOS_VIEWPORT, 0);

		int dx = mx - 800 / 2;
		int dy = my - 600 / 2;

		if (abs(dx) < 2)
			dx = 0;
		if (abs(dy) < 2)
			dy = 0;

		fYaw += (float)dx / 2.0f;
		fPitch += (float)dy / 2.0f;

		if (dx || dy)
			WindowAndInputComponent->SetMousePosition(800 / 2, 600 / 2);
	} else {
		if (bMouseHidden) {
			WindowAndInputComponent->ShowCursor(true);
			bMouseHidden = false;

			WindowAndInputComponent->SetMousePosition(lx, ly, MOUSEPOS_DESKTOP);
		}
	}

	float fMouseZ = (float)WindowAndInputComponent->MouseZ();
	fDist += (fMouseZ / 120.0f) * (abs(fDist) / 10.0f);
	WindowAndInputComponent->SetMouseZ(0);
	fDist = min(-1, fDist);

	cam->SetPosition(vPos);
	cam->SetAngle(WQuaternion());
	cam->Yaw(fYaw);
	cam->Pitch(fPitch);
	cam->Move(fDist);
}

WError BulletDebugger::Setup() {
	WError err = StartEngine(800, 600);
	if (!err)
		WindowAndInputComponent->ShowErrorMessage(err.AsString());
	else {
		m_linesDrawer = new WObject(this);
		WEffect* fx = new WEffect(this);
		LinesVS* vs = new LinesVS(this);
		vs->Load();
		LinesPS* ps = new LinesPS(this);
		ps->Load();
		fx->BindShader(vs);
		fx->BindShader(ps);
		fx->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		fx->BuildPipeline(Renderer->GetDefaultRenderTarget());
		WMaterial* mat = new WMaterial(this);
		mat->SetEffect(fx);
		m_linesDrawer->SetMaterial(mat);
		W_SAFE_REMOVEREF(ps);
		W_SAFE_REMOVEREF(vs);
		W_SAFE_REMOVEREF(fx);
		W_SAFE_REMOVEREF(mat);

		WGeometry* geometry = new WLinesGeometry(this);
		void* vb = calloc(m_maxLines * 2, geometry->GetVertexDescriptionSize());
		geometry->CreateFromData(vb, m_maxLines * 2, nullptr, 0, true);
		free(vb);
		m_linesDrawer->SetGeometry(geometry);
		W_SAFE_REMOVEREF(geometry);
	}
	return err;
}

bool BulletDebugger::Loop(float fDeltaTime) {
	ApplyMousePivot();

	char title[128];
	sprintf_s(title, 128, "FPS: %.2f (Elapsed %.2fs)", FPS, Timer.GetElapsedTime());
	TextComponent->RenderText(title, 5, 5, 32);

	std::vector<LINE> curLines;
	m_linesLock.lock();
	curLines = m_lines[0];
	m_linesLock.unlock();

	LineVertex* vb;
	m_linesDrawer->GetGeometry()->MapVertexBuffer((void**)&vb);
	for (unsigned int i = 0; i < m_maxLines*2; i += 2) {
		unsigned int lineIndex = i / 2;
		if (lineIndex < curLines.size()) {
			vb[i+0] = LineVertex({ curLines[lineIndex].from, curLines[lineIndex].color });
			vb[i+1] = LineVertex({ curLines[lineIndex].to,   curLines[lineIndex].color });
		} else
			memset(&vb[i], 0, sizeof(LineVertex) * 2);
	}
	m_linesDrawer->GetGeometry()->UnmapVertexBuffer();

	return true;
}

void BulletDebugger::Cleanup() {
}

void BulletDebugger::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
	LINE line;
	line.from = BTWConvertVec3(from);
	line.to = BTWConvertVec3(to);
	line.color = WColor(color.x(), color.y(), color.z());
	m_lines[1].push_back(line);
}

void BulletDebugger::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {

}

void BulletDebugger::reportErrorWarning(const char *warningString) {

}

void BulletDebugger::draw3dText(const btVector3 &location, const char *textString) {

}

void BulletDebugger::setDebugMode(int debugMode) {
	m_debugMode = debugMode;
}

int BulletDebugger::getDebugMode() const {
	return m_debugMode;
}

void BulletDebugger::clearLines() {
	m_linesLock.lock();
	m_lines[0] = m_lines[1];
	m_linesLock.unlock();
	m_lines[1].clear();
}

WRenderer* BulletDebugger::CreateRenderer() {
	return new WForwardRenderer(this);
}

WPhysicsComponent* BulletDebugger::CreatePhysicsComponent() {
	return nullptr;
}

void BulletDebugger::Thread(void* debugger_ptr) {
	BulletDebugger* debugger = (BulletDebugger*)debugger_ptr;
	RunWasabi(debugger);
}

#endif