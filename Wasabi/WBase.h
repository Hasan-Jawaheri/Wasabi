/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine entity base class
*********************************************************************/

#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;

class WBase {
public:
	WBase(class Wasabi* const app);
	virtual ~WBase();

	virtual std::string GetTypeName() const = 0;

	void			SetID(unsigned int newID);
	unsigned int	GetID() const;
	class Wasabi*	GetAppPtr() const;
	void			SetName(std::string name);
	std::string		GetName() const;
	void			AddReference();
	void			RemoveReference();
	virtual bool	Valid() const = 0;

	void			SetManager(void* mgr);

protected:
	class Wasabi*	m_app;
private:
	int				m_refCount;
	unsigned int	m_ID;
	std::string		m_name;
	void*			m_mgr;

	int				m_iDbgChanges;
};
