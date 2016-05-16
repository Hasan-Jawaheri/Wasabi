/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine entity manager
*********************************************************************/

#pragma once

#include "Wasabi.h"

#define W_HASHTABLESIZE 512
#define W_HASH(id) id == 0 ? 0 : max ( id % W_HASHTABLESIZE, 1 )

/*********************************************************************
Base class for all classes managers
*********************************************************************/
template<typename T>
class WManager {
	bool __bDbgDestructing;
protected:
	vector<T*> _entities[W_HASHTABLESIZE];

	virtual std::string GetTypeName(void) const = 0;

public:
	Wasabi* const app;
	WManager(Wasabi* const _app) : app(_app) { __bDbgDestructing = false; }
	~WManager(void) {
		__bDbgDestructing = true;
		for (uint j = 0; j < W_HASHTABLESIZE; j++)
			for (uint i = 0; i < _entities[j].size(); i)
				_entities[j][i]->RemoveReference();
	}
	void AddEntity(T* entity) {
		if (!entity)
			return;
		_entities[W_HASH(entity->GetID())].push_back(entity);
		entity->SetManager((void*)this);
		std::cout << "[" << GetTypeName() << " " << entity->GetID() << "] Added to the manager.\n";
	}
	bool RemoveEntity(T* entity) {
		uint tableIndex = W_HASH(entity->GetID());
		for (uint i = 0; i < _entities[tableIndex].size(); i++)
			if (_entities[tableIndex][i] == entity)
			{
				if (!__bDbgDestructing)
					std::cout << "[" << GetTypeName() << " " << entity->GetID() << "] Removed from manager.\n";

				_entities[tableIndex].erase(_entities[tableIndex].begin() + i);
				return true;
			}
		return false;
	}

	virtual void Init(void) {
	}

	T* GetEntity(uint ID) const {
		uint tableIndex = W_HASH(ID);
		if (tableIndex) //ID 0 is not searched for
			for (uint i = 0; i < _entities[tableIndex].size(); i++)
				if (_entities[tableIndex][i]->GetID() == ID)
					return _entities[tableIndex][i];
		return nullptr;
	}
	T* GetEntity(std::string name) const {
		for (uint j = 0; j < W_HASHTABLESIZE; j++)
			for (uint i = 0; i < _entities[j].size(); i++)
				if (_entities[j][i]->GetName() == name)
					return _entities[j][i];
		return nullptr;
	}
	T* GetEntityByIndex(uint index) const {
		for (uint j = 0; j < W_HASHTABLESIZE; j++)
		{
			if (index < _entities[j].size())
				return _entities[j][index];
			index -= _entities[j].size();
		}
		return nullptr;
	}
	uint GetEntitiesCount(void) const {
		uint size = 0;
		for (uint j = 0; j < W_HASHTABLESIZE; j++)
			size += _entities[j].size();
		return size;
	}
};
