/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine entity manager
*********************************************************************/

#pragma once

#include <vector>
#include <string>
#include <iostream>
using std::ios;
using std::vector;
using std::string;

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define W_HASHTABLESIZE 512
#define W_HASH(id) id == 0 ? 0 : max ( id % W_HASHTABLESIZE, 1 )

/*********************************************************************
Base class for all classes managers
*********************************************************************/
template<typename T>
class WManager {
	bool __bDbgDestructing;
protected:
	std::vector<T*> m_entities[W_HASHTABLESIZE];

	virtual std::string GetTypeName(void) const = 0;

public:
	class Wasabi* const m_app;
	WManager(class Wasabi* const _app) : m_app(_app) { __bDbgDestructing = false; }
	~WManager(void) {
		__bDbgDestructing = true;
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
			for (unsigned int i = 0; i < m_entities[j].size(); i)
				m_entities[j][i]->RemoveReference();
	}
	void AddEntity(T* entity) {
		if (!entity)
			return;
		m_entities[W_HASH(entity->GetID())].push_back(entity);
		entity->SetManager((void*)this);
		std::cout << "[" << GetTypeName() << " " << entity->GetID() << "] Added to the manager.\n";
	}
	bool RemoveEntity(T* entity) {
		unsigned int tableIndex = W_HASH(entity->GetID());
		for (unsigned int i = 0; i < m_entities[tableIndex].size(); i++)
			if (m_entities[tableIndex][i] == entity)
			{
				if (!__bDbgDestructing)
					std::cout << "[" << GetTypeName() << " " << entity->GetID() << "] Removed from manager.\n";

				m_entities[tableIndex].erase(m_entities[tableIndex].begin() + i);
				return true;
			}
		return false;
	}

	virtual void Init(void) {
	}

	T* GetEntity(unsigned int ID) const {
		unsigned int tableIndex = W_HASH(ID);
		if (tableIndex) //ID 0 is not searched for
			for (unsigned int i = 0; i < m_entities[tableIndex].size(); i++)
				if (m_entities[tableIndex][i]->GetID() == ID)
					return m_entities[tableIndex][i];
		return nullptr;
	}
	T* GetEntity(std::string name) const {
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
			for (unsigned int i = 0; i < m_entities[j].size(); i++)
				if (m_entities[j][i]->GetName() == name)
					return m_entities[j][i];
		return nullptr;
	}
	T* GetEntityByIndex(unsigned int index) const {
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
		{
			if (index < m_entities[j].size())
				return m_entities[j][index];
			index -= m_entities[j].size();
		}
		return nullptr;
	}
	unsigned int GetEntitiesCount(void) const {
		unsigned int size = 0;
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
			size += m_entities[j].size();
		return size;
	}
};
