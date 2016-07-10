#include "WLight.h"

WLightManager::WLightManager(class Wasabi* const app) : WManager<WLight>(app) {

}

WLightManager::~WLightManager() {

}

std::string WLightManager::GetTypeName() const {
	return "Light";
}

WError WLightManager::Load() {

}

WLight::WLight(class Wasabi* const app, unsigned int ID) : WBase(app, ID) {

	m_app->LightManager->AddEntity(this);
}

WLight::~WLight() {
	m_app->LightManager->RemoveEntity(this);
}

std::string WLight::GetTypeName() const {
	return "Light";
}

bool WLight::Valid() const {
	return true; // TODO: fill
}
