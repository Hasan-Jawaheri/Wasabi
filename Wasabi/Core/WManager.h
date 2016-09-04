/** @file WImage.h
 *  @brief Manager class implementation
 *
 *  A WManager is a template for a class that manages classes which inherit
 *  from a WBase. A manager has a list of all instances of the class that it
 *  manages, as long as the class makes sure it registers itself (usually in
 *  its constructor) and removes itself upon destruction (usually in its
 *  destructor).
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

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

/**
 * @ingroup engineclass
 *
 * A WManager is a template for a class that manages classes which inherit
 * from a WBase. A manager has a list of all instances of the class that it
 * manages, as long as the class makes sure it registers itself (usually in
 * its constructor) and removes itself upon destruction (usually in its
 * destructor).
 */
template<typename T>
class WManager {
protected:
	/** a hash table of all entities registered */
	std::vector<T*> m_entities[W_HASHTABLESIZE];

	/**
	 * This function must be implemented by a child class. It should return the
	 * name of the object that it is managing, which can be used for debugging
	 * purposes.
	 * @return  [description]
	 */
	virtual std::string GetTypeName() const = 0;

public:
	/** Pointer to the Wasabi instance that made this manager */
	class Wasabi* const m_app;

	WManager(class Wasabi* const a) : m_app(a) { __bDbgDestructing = false; }
	~WManager() {
		__bDbgDestructing = true;
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
			for (unsigned int i = 0; i < m_entities[j].size(); i)
				m_entities[j][i]->RemoveReference();
	}

	/**
	 * Registers an entity in the manager.
	 * @param entity Pointer to the entity to register
	 */
	void AddEntity(T* entity) {
		if (!entity)
			return;
		m_entities[W_HASH(entity->GetID())].push_back(entity);
		entity->SetManager((void*)this);
		std::cout << "[" << GetTypeName() << " " << entity->GetID() << "] Added to the manager.\n";
	}

	/**
	 * De-registers an entity from the manager.
	 * @param  entity Entity to de-register
	 * @return        true if the entity was found (and de-registered), false
	 *                otherwise
	 */
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

	/**
	 * Initializes the manager.
	 */
	virtual void Init() {
	}

	/**
	 * Retrieves a registered entity using its ID.
	 * @param  ID ID of the registered object
	 * @return    The registered object, nullptr if its not found
	 */
	T* GetEntity(unsigned int ID) const {
		unsigned int tableIndex = W_HASH(ID);
		if (tableIndex) //ID 0 is not searched for
			for (unsigned int i = 0; i < m_entities[tableIndex].size(); i++)
				if (m_entities[tableIndex][i]->GetID() == ID)
					return m_entities[tableIndex][i];
		return nullptr;
	}

	/**
	 * Retrieves a registered entity using its name.
	 * @param  name Name of the object to retrieve
	 * @return      The registered object, nullptr if its not found
	 */
	T* GetEntity(std::string name) const {
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
			for (unsigned int i = 0; i < m_entities[j].size(); i++)
				if (m_entities[j][i]->GetName() == name)
					return m_entities[j][i];
		return nullptr;
	}

	/**
	 * Retrieves a registered entity using its index into the hashtable.
	 * @param  index Index of the object to retrieve
	 * @return       The registered object, nullptr if its not found
	 */
	T* GetEntityByIndex(unsigned int index) const {
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
		{
			if (index < m_entities[j].size())
				return m_entities[j][index];
			index -= m_entities[j].size();
		}
		return nullptr;
	}

	/**
	 * Retrieves the number of registered entities.
	 * @return  Number of registered entities
	 */
	unsigned int GetEntitiesCount(void) const {
		unsigned int size = 0;
		for (unsigned int j = 0; j < W_HASHTABLESIZE; j++)
			size += m_entities[j].size();
		return size;
	}

private:
	/** A debug value, used to block out printing during manager destruction */
	bool __bDbgDestructing;
};
