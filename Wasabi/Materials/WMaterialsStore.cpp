#include "WMaterialsStore.h"
#include "WMaterial.h"
#include "WEffect.h"
#include "../Renderers/WRenderer.h"

WMaterialsStore::WMaterialsStore() {
	m_defaultEffect = nullptr;
}

void WMaterialsStore::AddEffect(WEffect* effect, unsigned int bindingSet) {
	effect->AddReference();
	RemoveEffect(effect);
	WMaterial* material = effect->CreateMaterial(bindingSet);
	if (material) {
		m_materialMap.insert(std::pair<WEffect*, WMaterial*>(effect, material));
		if (!m_defaultEffect)
			m_defaultEffect = effect;
	} else
		effect->RemoveReference();
}

void WMaterialsStore::RemoveEffect(WEffect* effect) {
	auto it = m_materialMap.find(effect);
	if (it != m_materialMap.end()) {
		if (effect == m_defaultEffect)
			m_defaultEffect = nullptr;
		effect->RemoveReference();
		it->second->RemoveReference();
		m_materialMap.erase(effect);
	}
}

void WMaterialsStore::RemoveEffect(WMaterial* material) {
	std::vector<WEffect*> effectsToRemove;
	for (auto it = m_materialMap.begin(); it != m_materialMap.end(); it++) {
		if (it->second == material) {
			it->second->RemoveReference();
			effectsToRemove.push_back(it->first);
		}
	}
	for (auto it = effectsToRemove.begin(); it != effectsToRemove.end(); it++) {
		if (*it == m_defaultEffect)
			m_defaultEffect = nullptr;
		(*it)->RemoveReference();
		m_materialMap.erase(*it);
	}
}

void WMaterialsStore::ClearEffects() {
	for (auto it = m_materialMap.begin(); it != m_materialMap.end(); it++) {
		it->first->RemoveReference();
		it->second->RemoveReference();
	}
	m_materialMap.clear();
	m_defaultEffect = nullptr;
}

WEffect* WMaterialsStore::GetDefaultEffect() const {
	return m_defaultEffect;
}

WMaterial* WMaterialsStore::GetMaterial(WEffect* effect) {
	if (!effect)
		effect = m_defaultEffect;

	auto it = m_materialMap.find(effect);
	if (it != m_materialMap.end())
		return it->second;
	return nullptr;
}
