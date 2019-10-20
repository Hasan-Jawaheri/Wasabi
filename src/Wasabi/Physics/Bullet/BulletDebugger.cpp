#include "Wasabi/Physics/Bullet/BulletDebugger.hpp"
#include "Wasabi/Cameras/WCamera.hpp"
#include "Wasabi/WindowAndInput/WWindowAndInputComponent.hpp"
#include "Wasabi/Texts/WText.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Renderers/Common/WSpritesRenderStage.hpp"
#include "Wasabi/Objects/WObject.hpp"
#include "Wasabi/Geometries/WGeometry.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"

struct LineVertex {
	WVector3 pos;
	WColor col;
};

class LinesVS : public WShader {
public:
	LinesVS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_VERTEX_SHADER;
		m_desc.input_layouts = { W_INPUT_LAYOUT({
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_3), // position
			W_SHADER_VARIABLE_INFO(W_TYPE_VEC_4), // color
		}) };
		m_desc.bound_resources = {
			W_BOUND_RESOURCE(W_TYPE_UBO, 0, "uboPerFrame", {
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "proj"), // projection
				W_SHADER_VARIABLE_INFO(W_TYPE_MAT4X4, "view"), // view
			}),
		};
		vector<uint8_t> code {
			#include "Shaders/lines.vert.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

class LinesPS : public WShader {
public:
	LinesPS(class Wasabi* const app) : WShader(app) {}

	virtual void Load(bool bSaveData = false) {
		m_desc.type = W_FRAGMENT_SHADER;
		vector<uint8_t> code {
			#include "Shaders/lines.frag.glsl.spv"
		};
		LoadCodeSPIRV((char*)code.data(), (int)code.size(), bSaveData);
	}
};

class WLinesGeometry : public WGeometry {
public:
	WLinesGeometry(Wasabi* const app) : WGeometry(app) {}

	virtual uint32_t GetVertexBufferCount() const {
		return 1;
	}
	virtual W_VERTEX_DESCRIPTION GetVertexDescription(uint32_t index) const {
		UNREFERENCED_PARAMETER(index);
		return W_VERTEX_DESCRIPTION({
			W_ATTRIBUTE_POSITION,
			W_VERTEX_ATTRIBUTE("color", 4),
		});
	}
	virtual size_t GetVertexDescriptionSize(uint32_t layout_index = 0) const {
		UNREFERENCED_PARAMETER(layout_index);
		return sizeof(LineVertex);
	}
};

class LinesRenderStage : public WRenderStage {
	class WEffect* m_linesFX;

public:
	LinesRenderStage(class Wasabi* const app) : WRenderStage(app) {
		m_stageDescription.name = __func__;
		m_stageDescription.target = RENDER_STAGE_TARGET_BACK_BUFFER;
		m_stageDescription.flags = RENDER_STAGE_FLAG_PICKING_RENDER_STAGE;
		m_linesFX = nullptr;
	}

	virtual WError Initialize(std::vector<WRenderStage*>& previousStages, uint32_t width, uint32_t height) {
		WError err = WRenderStage::Initialize(previousStages, width, height);
		if (!err)
			return err;

		m_linesFX = new WEffect(m_app);
		LinesVS* vs = new LinesVS(m_app);
		vs->Load();
		LinesPS* ps = new LinesPS(m_app);
		ps->Load();
		m_linesFX->BindShader(vs);
		m_linesFX->BindShader(ps);
		m_linesFX->SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		m_linesFX->BuildPipeline(m_renderTarget);
		W_SAFE_REMOVEREF(ps);
		W_SAFE_REMOVEREF(vs);

		return WError(W_SUCCEEDED);
	}

	virtual WError Render(class WRenderer* renderer, class WRenderTarget* rt, uint32_t filter) {
		UNREFERENCED_PARAMETER(renderer);
		UNREFERENCED_PARAMETER(filter);

		WCamera* cam = rt->GetCamera();

		m_linesFX->Bind(rt);

		uint32_t numObjects = m_app->ObjectManager->GetEntitiesCount();
		for (uint32_t i = 0; i < numObjects; i++) {
			WObject* obj = m_app->ObjectManager->GetEntityByIndex(i);
			WMaterial* material = obj->GetMaterial(m_linesFX);
			if (!material) {
				obj->AddEffect(m_linesFX);
				material = obj->GetMaterial(m_linesFX);
			}
			material->SetVariable<WMatrix>("proj", cam->GetProjectionMatrix());
			material->SetVariable<WMatrix>("view", cam->GetViewMatrix());
			obj->Render(rt, material, false);
		}
		return WError(W_SUCCEEDED);
	}

	virtual void Cleanup() {
		W_SAFE_REMOVEREF(m_linesFX);
	}

	virtual WError Resize(uint32_t width, uint32_t height) {
		return WRenderStage::Resize(width, height);
	}
};

BulletDebugger::BulletDebugger(WBulletPhysics* physics, uint32_t maxLines, std::string appName) : Wasabi(), btIDebugDraw() {
	m_appName = appName + "-bullet-debugger";
	m_physics = physics;
	m_keepRunning = true;
	m_debugMode = 1;
	m_maxLines = maxLines;
	fYaw = 0;
	fPitch = 30;
	fDist = -15;
	m_curLines = 0;
}

void BulletDebugger::ApplyMousePivot() {
	static bool bMouseHidden = false;
	static double lx, ly;
	WCamera* cam = CameraManager->GetDefaultCamera();
	WWindowAndInputComponent* WIC = WindowAndInputComponent;
	if (WIC->MouseClick(MOUSE_LEFT)) {
		if (!bMouseHidden) {
			bMouseHidden = true;
			lx = WIC->MouseX(MOUSEPOS_DESKTOP, 0);
			ly = WIC->MouseY(MOUSEPOS_DESKTOP, 0);
			WIC->SetCursorMotionMode(true);
		}

		double mx = WIC->MouseX(MOUSEPOS_DESKTOP, 0);
		double my = WIC->MouseY(MOUSEPOS_DESKTOP, 0);

		double dx = mx - lx;
		double dy = my - ly;

		fYaw += (float)dx / 2.0f;
		fPitch += (float)dy / 2.0f;

		WIC->SetMousePosition(lx, ly, MOUSEPOS_DESKTOP);
	} else {
		if (bMouseHidden) {
			bMouseHidden = false;
			WIC->SetCursorMotionMode(false);
		}
	}

	float fMouseZ = (float)WIC->MouseZ();
	fDist += (fMouseZ) * (std::abs(fDist) / 10.0f);
	WIC->SetMouseZ(0);
	fDist = std::min(-1.0f, fDist);

	cam->SetPosition(vPos);
	cam->SetAngle(WQuaternion());
	cam->Yaw(fYaw);
	cam->Pitch(fPitch);
	cam->Move(fDist);
}

WError BulletDebugger::Setup() {
	SetEngineParam("appName", m_appName.c_str());

	WError err = StartEngine(800, 600);
	if (!err)
		WindowAndInputComponent->ShowErrorMessage(err.AsString());
	else {
		Renderer->SetRenderingStages({
			new LinesRenderStage(this),
		});

		m_linesDrawer = ObjectManager->CreateObject();

		WGeometry* geometry = new WLinesGeometry(this);
		void* vb = calloc(m_maxLines * 2, geometry->GetVertexDescriptionSize());
		geometry->CreateFromData(vb, m_maxLines * 2, nullptr, 0, W_GEOMETRY_CREATE_VB_DYNAMIC | W_GEOMETRY_CREATE_VB_REWRITE_EVERY_FRAME);
		free(vb);
		m_linesDrawer->SetGeometry(geometry);
		W_SAFE_REMOVEREF(geometry);
	}
	return err;
}

bool BulletDebugger::Loop(float fDeltaTime) {
	UNREFERENCED_PARAMETER(fDeltaTime);

	ApplyMousePivot();

	char title[128];
	sprintf_s(title, 128, "FPS: %.2f (Elapsed %.2fs)", FPS, Timer.GetElapsedTime());
	TextComponent->RenderText(title, 5, 5, 32);

	std::vector<LINE> curLines;
	m_linesLock.lock();
	curLines = m_lines[0];
	m_linesLock.unlock();

	LineVertex* vb;
	m_linesDrawer->GetGeometry()->MapVertexBuffer((void**)&vb, W_MAP_WRITE);
	for (uint32_t i = 0; i < m_maxLines*2; i += 2) {
		uint32_t lineIndex = i / 2;
		if (lineIndex < curLines.size()) {
			vb[i+0] = LineVertex({ curLines[lineIndex].from, curLines[lineIndex].color });
			vb[i+1] = LineVertex({ curLines[lineIndex].to,   curLines[lineIndex].color });
		} else
			memset(static_cast<void*>(&vb[i]), 0, sizeof(LineVertex) * 2);
	}
	m_linesDrawer->GetGeometry()->UnmapVertexBuffer(false);

	return true;
}

void BulletDebugger::Cleanup() {
	W_SAFE_REMOVEREF(m_linesDrawer);
}

void BulletDebugger::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
	LINE line;
	line.from = BTWConvertVec3(from);
	line.to = BTWConvertVec3(to);
	line.color = WColor(color.x(), color.y(), color.z());
	m_lines[1].push_back(line);
}

void BulletDebugger::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {
	UNREFERENCED_PARAMETER(PointOnB);
	UNREFERENCED_PARAMETER(normalOnB);
	UNREFERENCED_PARAMETER(distance);
	UNREFERENCED_PARAMETER(lifeTime);
	UNREFERENCED_PARAMETER(color);
}

void BulletDebugger::reportErrorWarning(const char *warningString) {
	UNREFERENCED_PARAMETER(warningString);
}

void BulletDebugger::draw3dText(const btVector3 &location, const char *textString) {
	UNREFERENCED_PARAMETER(location);
	UNREFERENCED_PARAMETER(textString);
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

WError BulletDebugger::SetupRenderer() {
	return Renderer->SetRenderingStages({
		new LinesRenderStage(this),
	});
}

WPhysicsComponent* BulletDebugger::CreatePhysicsComponent() {
	return nullptr;
}

void BulletDebugger::Thread(void* debugger_ptr) {
	BulletDebugger* debugger = (BulletDebugger*)debugger_ptr;
	RunWasabi(debugger);
}
