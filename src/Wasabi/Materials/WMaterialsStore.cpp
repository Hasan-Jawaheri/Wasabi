#include "Wasabi/Materials/WMaterialsStore.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"

WMaterialsStore::WMaterialsStore() {
	m_materialsCollection = new WMaterialCollection();
}

WMaterialsStore::~WMaterialsStore() {
	ClearEffects();
	W_SAFE_DELETE(m_materialsCollection);
}

void WMaterialsStore::_AddMaterial(class WMaterial* material) {
	material->GetEffect()->AddReference();
	m_materialMap.insert(std::make_pair(material->GetEffect(), material));
	m_materialsCollection->m_materials.insert(std::make_pair(material, true));
	OnMaterialAdded(material);
}

void WMaterialsStore::AddEffect(WEffect* effect, uint32_t bindingSet) {
	effect->AddReference();
	RemoveEffect(effect);
	WMaterial* material = effect->CreateMaterial(bindingSet);
	material->SetName(effect->GetName() + "-" + material->GetName());
	if (material) {
		m_materialMap.insert(std::pair<WEffect*, WMaterial*>(effect, material));
		m_materialsCollection->m_materials.insert(std::make_pair(material, true));
		OnMaterialAdded(material);
	} else
		effect->RemoveReference();
}

void WMaterialsStore::RemoveEffect(WEffect* effect) {
	auto it = m_materialMap.find(effect);
	if (it != m_materialMap.end()) {
		m_materialsCollection->m_materials.erase(it->second);
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
		(*it)->RemoveReference();
		m_materialMap.erase(*it);
	}
	m_materialsCollection->m_materials.erase(material);
}

void WMaterialsStore::ClearEffects() {
	for (auto it = m_materialMap.begin(); it != m_materialMap.end(); it++) {
		it->first->RemoveReference();
		it->second->RemoveReference();
	}
	m_materialMap.clear();
	m_materialsCollection->m_materials.clear();
}

WMaterial* WMaterialsStore::GetMaterial(WEffect* effect) {
	auto it = m_materialMap.find(effect);
	if (it != m_materialMap.end())
		return it->second;
	return nullptr;
}

WMaterialCollection& WMaterialsStore::GetMaterials() {
	return *m_materialsCollection;
}
