#pragma once

#include "../../Core/WManager.h"

#include <unordered_map>
#include <string>

template<typename TKey, typename TValue>
class ResourceCollectionBase {
	std::unordered_map<TKey*, TValue*> m_collection;
	std::string m_name;
	WManager<TKey>* m_manager;

	void OnEntityChange(TKey* entity, bool bAdded) {
		if (bAdded) {
			m_collection.insert(std::pair<TKey*, TValue*>(entity, CreateResource(entity)));
		} else {
			auto it = m_collection.find(entity);
			if (it != m_collection.end())
				m_collection.erase(it);
		}
	}

public:
	ResourceCollectionBase() {
		m_manager = nullptr;
	}

	virtual void Initialize(std::string name, WManager<TKey>* manager) {
		Cleanup();
		m_manager = manager;
		m_name = name;

		for (unsigned int i = 0; i < m_manager->GetEntitiesCount(); i++)
			OnEntityChange(m_manager->GetEntityByIndex(i), true);
		m_manager->RegisterChangeCallback(m_name, [this](TKey* e, bool a) {this->OnEntityChange(e, a); });
	}

	virtual void Cleanup() {
		for (auto it = m_collection.begin(); it != m_collection.end(); it++)
			(*it).second->RemoveReference();
		m_collection.clear();
		if (m_manager)
			m_manager->RemoveChangeCallback(m_name);
		m_manager = nullptr;
	}

	TValue* GetResourceOf(TKey* entity) {
		auto it = m_collection.find(entity);
		if (it == m_collection.end())
			return nullptr;
		return it->second;
	}

	virtual TValue* CreateResource(TKey* entity) = 0;
};

template<typename TKey, typename TValue>
class ResourceCollection : public ResourceCollectionBase<TKey, TValue> {
	std::function<TValue*(TKey*)> m_createResource;

public:
	virtual void Initialize(std::string name, WManager<TKey>* manager, std::function<TValue*(TKey*)> createResource) {
		m_createResource = createResource;
		ResourceCollectionBase<TKey, TValue>::Initialize(name, manager);
	}

	virtual TValue* CreateResource(TKey* entity) {
		return m_createResource(entity);
	}
};

