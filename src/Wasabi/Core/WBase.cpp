#include "Wasabi/Core/WBase.h"
#include "Wasabi/Core/WManager.h"

/********************************************************************************
***********************************WBase class**********************************
********************************************************************************/
WBase::WBase(Wasabi* const app, uint32_t ID) {
	static uint32_t generatedId = 1;
	m_ID = ID;
	m_name = "object" + std::to_string(generatedId++);
	m_app = app;
	m_mgr = nullptr;
	m_refCount = 1;
}

WBase::~WBase() {
}

void WBase::SetManager(void* mgr) {
	m_mgr = mgr;
}

void WBase::SetID(uint32_t newID) {
	//set the ID of the object
	if (m_mgr)
		((WManager<WBase>*)m_mgr)->RemoveEntity(this);
	m_ID = newID;
	if (m_mgr)
		((WManager<WBase>*)m_mgr)->AddEntity(this);
}

uint32_t WBase::GetID() const {
	return m_ID;
}

Wasabi* WBase::GetAppPtr() const {
	//get a pointer to the main core of the object
	return m_app;
}

void WBase::SetName(std::string name) {
	//set the name of the object
	m_name = name;
	((WManager<WBase>*)m_mgr)->OnEntityNameChanged(this, name);
}

std::string WBase::GetName() const {
	//return the name of the object via name variable
	return m_name;
}

void WBase::AddReference() {
	m_refCount++;
}

void WBase::RemoveReference() {
	m_refCount--;
	if (m_refCount < 1)
		delete this;
}