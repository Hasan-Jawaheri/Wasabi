#include "WObject.h"

std::string WObjectManager::GetTypeName(void) const {
	return "Object";
}

WObjectManager::WObjectManager(class Wasabi* const app) : WManager<WObject>(app) {
}

void WObjectManager::Render() {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < _entities[i].size(); j++) {
			_entities[i][j]->Render();
		}
	}
}

WObject::WObject(Wasabi* const app, unsigned int ID) : WBase(app) {
	SetID(ID);

	m_geometry = nullptr;
	m_material = app->Renderer->CreateDefaultMaterial();

	app->ObjectManager->AddEntity(this);
}

WObject::~WObject() {
	if (m_geometry)
		m_geometry->RemoveReference();
	m_geometry = nullptr;
	if (m_material)
		m_material->RemoveReference();
	m_material = nullptr;

	m_app->ObjectManager->RemoveEntity(this);
}

std::string WObject::GetTypeName() const {
	return "Object";
}

bool WObject::Valid() const {
	return true; // TODO: put actual implementation
}

void WObject::Render() {
	if (m_geometry && m_material && m_geometry->Valid() && m_material->Valid()) {
		m_material->SetVariableMatrix("gWorld", WMatrix());
		m_material->SetVariableMatrix("gProjection", WPerspectiveProjMatrixFOV(60.0f,
			(float)m_app->WindowComponent->GetWindowWidth() / (float)m_app->WindowComponent->GetWindowHeight(),
			0.1f, 256.0f));
		m_material->SetVariableMatrix("gView", WMatrixInverse(WTranslationMatrix(-10 + 5 * (int)GetID(), 0, -2.5f)));

		WError err;
		err = m_material->Bind();
		err = m_geometry->Draw();
	}
}

WError WObject::SetGeometry(class WGeometry* geometry) {
	if (m_geometry)
		m_geometry->RemoveReference();

	m_geometry = geometry;
	if (geometry) {
		m_geometry->AddReference();
	}

	return WError(W_SUCCEEDED);
}

WError WObject::SetMaterial(class WMaterial* material) {
	if (m_material)
		m_material->RemoveReference();

	m_material = material;
	if (material) {
		m_material->AddReference();
	}

	return WError(W_SUCCEEDED);
}

