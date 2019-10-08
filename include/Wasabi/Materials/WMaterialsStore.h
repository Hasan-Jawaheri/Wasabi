#pragma once

#include <unordered_map>
#include <string>
#include <vector>

/**
 * Represents a collection of materials that can be used by render-able entities.
 * Materials that are added to a collection usually represent per-entity resources
 * such as per-object UBOs/textures for a WObject. This collection will hold a
 * reference to every material it stores.
 */
class WMaterialsStore {
protected:
	/** Collection of materials */
	std::unordered_map<class WEffect*, class WMaterial*> m_materialMap;
	class WMaterialCollection* m_materialsCollection;

	void _AddMaterial(class WMaterial* material);

public:
	WMaterialsStore();
	virtual ~WMaterialsStore();

	void AddEffect(class WEffect* effect, uint32_t bindingSet = 0);

	void RemoveEffect(class WEffect* effect);

	void RemoveEffect(class WMaterial* material);

	void ClearEffects();

	class WMaterial* GetMaterial(class WEffect* effect);

	class WMaterialCollection& GetMaterials();
};