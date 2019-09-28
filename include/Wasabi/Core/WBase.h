/** @file WBase.h
 *  @brief WBase implementation
 *
 *  WBase class is used by the engine to create a unified way of managing
 *  objects (such as WImage, WObject, WGeometry, etc...) such that they all
 *  have IDs, names, managers, and can be freed using reference counting.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;

/**
 * @ingroup engineclass
 *
 * This is the base class, used as a parent for most engine classes (such as
 * WObject, WImage, etc...). It provides a unified way of managing objects
 * such that they all have IDs, names, managers, and can be freed using
 * reference counting. Initially, a WBase has a reference count of 1. If the
 * count ever reaches 0, the object will delete itself. So an application
 * should \b NEVER delete or free an object that inherits from WBase, but
 * should simply call its RemoveReference() instead.
 */
class WBase {
public:
	WBase(class Wasabi* const app, uint32_t ID = 0);
	virtual ~WBase();

	/**
	 * This function must be implemented by a child class. This is used for
	 * debugging, in which a class should return its name.
	 * @return The name of the class
	 */
	virtual std::string GetTypeName() const = 0;

	/**
	 * Sets the ID of this object and notifies its manager.
	 * @param newID New ID
	 */
	void SetID(uint32_t newID);

	/**
	 * Retrieves the ID of this object.
	 * @return The ID of this object
	 */
	uint32_t GetID() const;

	/**
	 * Retrieves a pointer to the Wasabi class used to create this object.
	 * @return A pointer to the Wasabi class
	 */
	class Wasabi* GetAppPtr() const;

	/**
	 * Sets the name of this object.
	 * @param name New name for the object
	 */
	void SetName(std::string name);

	/**
	 * Retrieves the name of this object.
	 * @return The name of the object
	 */
	std::string GetName() const;

	/**
	 * Add to the reference count.
	 */
	void AddReference();

	/**
	 * Remove from the reference count. If the reference count reaches 0, this
	 * object destroys itself
	 */
	void RemoveReference();

	/**
	 * Must be implemented by the child. This function should report the validity
	 * status of the object, in whatever sense of "validity" it chooses.
	 * @return true if the object is "valid", false otherwise
	 */
	virtual bool Valid() const = 0;

	/**
	 * Sets the manager of this object. The manager should be of a type
	 * WManager<X> such that X is a child of WBase.
	 * @param mgr The manager
	 */
	void SetManager(void* mgr);

protected:
	/** The Wasabi application under which this object is created */
	class Wasabi*	m_app;

private:
	/** The reference count, starting at 1 */
	int m_refCount;
	/** ID of this object */
	uint32_t m_ID;
	/** Name of this object */
	std::string m_name;
	/** The manager of the object */
	void* m_mgr;
};
