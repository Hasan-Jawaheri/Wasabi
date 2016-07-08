#include "WBase.h"
#include "WManager.h"

/********************************************************************************
***********************************WBase class**********************************
********************************************************************************/
WBase::WBase(Wasabi* const app, unsigned int ID) {
	//default values
	m_ID = ID;
	m_name = "";
	m_app = app;
	m_iDbgChanges = 0;
	m_mgr = nullptr;
	m_refCount = 1;
}
WBase::~WBase() {
	if (m_refCount > 0)
		std::cout << "WARNING: something with ID " << m_ID <<
		" was destroyed with reference count " << m_refCount << "\n";
}
void WBase::SetManager(void* mgr) {
	m_mgr = mgr;
}
void WBase::SetID(unsigned int newID) {
	if (m_iDbgChanges > 1)
		std::cout << GetTypeName() << " with ID " << m_ID << " changed it's ID to " << newID << "\n";

	m_iDbgChanges++;

	//set the ID of the object
	if (m_mgr)
		((WManager<WBase>*)m_mgr)->RemoveEntity(this);
	m_ID = newID;
	if (m_mgr)
		((WManager<WBase>*)m_mgr)->AddEntity(this);
}
unsigned int WBase::GetID() const {
	return m_ID;
}
Wasabi* WBase::GetAppPtr() const {
	//get a pointer to the main core of the object
	return m_app;
}
void WBase::SetName(std::string name) {
	if (m_iDbgChanges > 1)
		std::cout << GetTypeName() << " with name " << m_name << " changed it's name to " << name << "\n";

	m_iDbgChanges++;

	//set the name of the object
	m_name = name;
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