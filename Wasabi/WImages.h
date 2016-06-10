#pragma once

#include "Wasabi.h"
#include "WBase.h"
#include "WManager.h"

class WImage : public WBase {
public:
	WImage(Wasabi* const app);
	~WImage();

	virtual std::string GetTypeName() const;

	virtual bool	Valid() const;
};
