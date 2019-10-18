#include "Wasabi/Core/WBase.hpp"
#include "Wasabi/Core/WManager.hpp"

/********************************************************************************
***********************************WBase class**********************************
********************************************************************************/
WBase::WBase(Wasabi* const app, uint32_t ID) {
	static uint32_t generatedId = 1;
	m_ID = ID;
	m_name = "object" + std::to_string(generatedId++);
	m_app = app;
	m_refCount = 1;
}

WBase::~WBase() {
}

uint32_t WBase::GetID() const {
	return m_ID;
}

Wasabi* WBase::GetAppPtr() const {
	//get a pointer to the main core of the object
	return m_app;
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