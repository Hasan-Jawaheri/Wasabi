/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine entity base class
*********************************************************************/

#pragma once

#include "Wasabi.h"

class WBase {
public:
	WBase(Wasabi* const app);
	virtual ~WBase();

	virtual std::string GetTypeName() const = 0;

	void			SetID(uint newID);
	uint			GetID() const;
	Wasabi*			GetAppPtr() const;
	void			SetName(std::string name);
	std::string		GetName() const;
	void			AddReference();
	void			RemoveReference();
	virtual bool	Valid() const = 0;

	void			SetManager(void* mgr);

private:
	int				m_refCount;
	uint			m_ID;
	std::string		m_name;
	Wasabi*			m_app;
	void*			m_mgr;

	int				m_iDbgChanges;
};
