#include "WSprite.h"

std::string WSpriteManager::GetTypeName() const {
	return "Sprite";
}

WSpriteManager::WSpriteManager(class Wasabi* const app) : WManager<WSprite>(app) {

}

void WSpriteManager::Render() {
	for (int i = 0; i < W_HASHTABLESIZE; i++) {
		for (int j = 0; j < _entities[i].size(); j++) {
			_entities[i][j]->Render();
		}
	}
}

std::string WSprite::GetTypeName() const {
	return "Sprite";
}

WSprite::WSprite(Wasabi* const app, unsigned int ID) : WBase(app, ID) {
	m_hidden = false;

	app->SpriteManager->AddEntity(this);
}

WSprite::~WSprite() {
	m_app->SpriteManager->RemoveEntity(this);
}

void WSprite::Render() {
	if (!m_hidden) {

	}
}

void WSprite::Show() {
	m_hidden = false;
}

void WSprite::Hide() {
	m_hidden = true;
}

bool WSprite::Hidden() const {
	return m_hidden;
}

bool WSprite::Valid() const {
	return false; // TODO: do this
}
