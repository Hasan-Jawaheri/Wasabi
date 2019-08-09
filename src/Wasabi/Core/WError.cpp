#include "Core/WError.h"

WError::WError() {
	m_error = W_ERRORUNK;
}

WError::WError(W_ERROR err) {
	m_error = err;
}

std::string WError::AsString() const {
	switch (m_error) {
	case W_ERRORUNK: return "Unkown error occurred";
	case W_SUCCEEDED: return "Succeeded";
	case W_WINDOWNOTCREATED: return "Failed to create the main window";
	case W_CLASSREGISTERFAILED: return "Failed to register the windows window class";
	case W_UNABLETOCREATEDEVICE: return "Failed to create a Vulkan device";
	case W_UNABLETOCREATEIMAGE: return "Failed to create the Vulkan image view";
	case W_UNABLETOCREATEBUFFER: return "Failed to crate a buffer";
	case W_NOTVALID: return "The object being used is not valid";
	case W_FILENOTFOUND: return "The specified file cannot be found on the system";
	case W_UNABLETOCREATESWAPCHAIN: return "Failed to create the swap chain";
	case W_INVALIDPARAM: return "At least one invalid parameter was passed to the function";
	case W_UNABLETOMAPBUFFER: return "Failed to map the memory buffer";
	case W_PHYSICSNOTINITIALIZED: return "The physics component hasn't been initialize";
	case W_INVALIDFILEFORMAT: return "The file format is not valid";
	case W_FAILEDTOCREATEINSTANCE: return "Failed to create the Vulkan instance";
	case W_FAILEDTOLISTDEVICES: return "Failed to enumerate Vulkan devices";
	case W_HARDWARENOTSUPPORTED: return "The hardware doesn't have the minimum requirements to run the engine";
	case W_OUTOFMEMORY: return "System is out of memory";
	case W_FAILEDTOCREATEPIPELINE: return "Failed to create the Vulkan graphics or compute pipeline";
	case W_FAILEDTOCREATEDESCRIPTORSETLAYOUT: return "Failed to create the Vulkan descriptor set layout";
	case W_FAILEDTOCREATEPIPELINELAYOUT: return "Failed to create the Vulkan pipeline layout";
	case W_NORENDERTARGET: return "The render target is invalid for this use";
	case W_INVALIDREPEATEDBINDINGINDEX: return "The effect contains two shaders that use the same binding index for two different resources";
	case W_ALREADYLOADED: return "Asset with the given name is already loaded, use GetAsset instead";
	case W_NAMECONFLICT: return "Another asset with the same name is already saved";
	default: return "Invalid error code";
	}
}

WError::operator bool() {
	return m_error == W_SUCCEEDED;
}