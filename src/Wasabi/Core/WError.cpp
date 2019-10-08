#include "Wasabi/Core/WError.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
#elif defined(__APPLE__)
#endif

static std::string getBacktraceString();

WError::WError() {
	m_error = W_ERRORUNK;
}

WError::WError(W_ERROR err) {
	m_error = err;
}

std::string WError::AsString(bool getBacktrace) const {
	const char* error;
	switch (m_error) {
	case W_ERRORUNK: error = "Unkown error occurred";
		break;
	case W_SUCCEEDED: error = "Succeeded";
		break;
	case W_WINDOWNOTCREATED: error = "Failed to create the main window";
		break;
	case W_CLASSREGISTERFAILED: error = "Failed to register the windows window class";
		break;
	case W_UNABLETOCREATEDEVICE: error = "Failed to create a Vulkan device";
		break;
	case W_UNABLETOCREATEIMAGE: error = "Failed to create the Vulkan image view";
		break;
	case W_UNABLETOCREATEBUFFER: error = "Failed to crate a buffer";
		break;
	case W_NOTVALID: error = "The object being used is not valid";
		break;
	case W_FILENOTFOUND: error = "The specified file cannot be found on the system";
		break;
	case W_UNABLETOCREATESWAPCHAIN: error = "Failed to create the swap chain";
		break;
	case W_INVALIDPARAM: error = "At least one invalid parameter was passed to the function";
		break;
	case W_UNABLETOMAPBUFFER: error = "Failed to map the memory buffer";
		break;
	case W_PHYSICSNOTINITIALIZED: error = "The physics component hasn't been initialize";
		break;
	case W_INVALIDFILEFORMAT: error = "The file format is not valid";
		break;
	case W_FAILEDTOCREATEINSTANCE: error = "Failed to create the Vulkan instance";
		break;
	case W_FAILEDTOLISTDEVICES: error = "Failed to enumerate Vulkan devices";
		break;
	case W_HARDWARENOTSUPPORTED: error = "The hardware doesn't have the minimum requirements to run the engine";
		break;
	case W_OUTOFMEMORY: error = "System is out of memory";
		break;
	case W_FAILEDTOCREATEPIPELINE: error = "Failed to create the Vulkan graphics or compute pipeline";
		break;
	case W_FAILEDTOCREATEDESCRIPTORSETLAYOUT: error = "Failed to create the Vulkan descriptor set layout";
		break;
	case W_FAILEDTOCREATEPIPELINELAYOUT: error = "Failed to create the Vulkan pipeline layout";
		break;
	case W_NORENDERTARGET: error = "The render target is invalid for this use";
		break;
	case W_INVALIDREPEATEDBINDINGINDEX: error = "The effect contains two shaders that use the same binding index for two different resources";
		break;
	case W_ALREADYLOADED: error = "Asset with the given name is already loaded, use GetAsset instead";
		break;
	case W_NAMECONFLICT: error = "Another asset with the same name is already saved";
		break;
	default: error = "Invalid error code";
	}

	std::string errorString = error;
	if (getBacktrace)
		errorString = getBacktraceString() + "\n" + errorString;
	return errorString;
}

WError::operator bool() {
	return m_error == W_SUCCEEDED;
}

bool WError::operator == (const WError& other) const {
	return m_error == other.m_error;
}

bool WError::operator == (const W_ERROR& other) const {
	return m_error == other;
}

bool WError::operator != (const WError& other) const {
	return m_error != other.m_error;
}

bool WError::operator != (const W_ERROR& other) const {
	return m_error != other;
}

#if defined(_WIN32)
static std::string getBacktraceString() {
	return "";
}
#elif defined(__linux__)
static std::string getBacktraceString() {
	return "";
}
#elif defined(__APPLE__)
static std::string getBacktraceString() {
	return "";
}
#endif
