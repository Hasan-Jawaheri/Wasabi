#include "WError.h"

WError::WError(W_ERROR err) {
	m_error = err;
}

WError::operator bool() {
	return m_error == W_SUCCEEDED;
}