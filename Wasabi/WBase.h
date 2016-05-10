/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine entity base class
*********************************************************************/

#pragma once

#include "Common.h"
#include "WCore.h"

class WBase {
	virtual std::string GetTypeName() const = 0;

public:
	WBase(WCore* const core);
	virtual ~WBase();

	void				SetID(uint newID);
	uint				GetID() const;
	WCore*				GetCorePtr() const;
	void				SetName(std::string name);
	std::string			GetName() const;
	void				AddReference();
	void				RemoveReference();
	virtual bool		Valid() const = 0;

	void				SetManager(void* mgr);

private:
	int				m_refCount;
	uint			m_ID;
	std::string		m_name;
	WCore*			m_core;
	void*			m_mgr;

	int				m_iDbgChanges;
};
