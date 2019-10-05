/** @file WError.h
 *  @brief Error codes implementation
 *
 *  A WError is a convenient way of reporting errors, providing tools for
 *  debugging.
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include <string>
using std::string;

/**
 * List of all Wasabi error codes.
 */
enum W_ERROR: uint8_t {
	/** Unkown error occurred */
	W_ERRORUNK = 0,
	/** Succeeded */
	W_SUCCEEDED = 1,
	/** Failed to create the main window */
	W_WINDOWNOTCREATED = 2,
	/** Failed to register the windows window class */
	W_CLASSREGISTERFAILED = 3,
	/** Failed to create a Vulkan device */
	W_UNABLETOCREATEDEVICE = 4,
	/** Failed to create the Vulkan image view */
	W_UNABLETOCREATEIMAGE = 5,
	/** Failed to crate a buffer */
	W_UNABLETOCREATEBUFFER = 6,
	/** The object being used is not valid */
	W_NOTVALID = 7,
	/** The specified file cannot be found on the system */
	W_FILENOTFOUND = 8,
	/** Failed to create the swap chain */
	W_UNABLETOCREATESWAPCHAIN = 9,
	/** At least one invalid parameter was passed to the function */
	W_INVALIDPARAM = 10,
	/** Failed to map the memory buffer */
	W_UNABLETOMAPBUFFER = 11,
	/** The physics component hasn't been initialized */
	W_PHYSICSNOTINITIALIZED = 12,
	/** The file format is not valid */
	W_INVALIDFILEFORMAT = 13,
	/** Failed to create the Vulkan instance */
	W_FAILEDTOCREATEINSTANCE = 14,
	/** Failed to enumerate Vulkan devices */
	W_FAILEDTOLISTDEVICES = 15,
	/** The hardware doesn't have the minimum requirements to run the engine */
	W_HARDWARENOTSUPPORTED = 16,
	/** System is out of memory */
	W_OUTOFMEMORY = 17,
	/** Failed to create the Vulkan graphics or compute pipeline */
	W_FAILEDTOCREATEPIPELINE = 18,
	/** Failed to create the Vulkan descriptor set layout */
	W_FAILEDTOCREATEDESCRIPTORSETLAYOUT = 19,
	/** Failed to create the Vulkan pipeline layout */
	W_FAILEDTOCREATEPIPELINELAYOUT = 20,
	/** The render target is invalid for this use */
	W_NORENDERTARGET = 21,
	/** Two shaders in the given effect use the same binding index for two different resources */
	W_INVALIDREPEATEDBINDINGINDEX = 22,
	/** File asset already loaded */
	W_ALREADYLOADED = 23,
	/** Name conflicts with another asset */
	W_NAMECONFLICT = 24,
};

/**
 * @ingroup engineclass
 *
 * This class represents a general purpose error code.
 */
class WError {
public:
	/** The carried error code */
	W_ERROR m_error;

	WError();
	WError(W_ERROR err);

	/**
	 * Retrieves the string corresponding to the carried error.
	 * @return A string corresponding to m_error
	 */
	std::string AsString() const;

	/**
	 * Evaluates this error code to a boolean.
	 * @return return true if m_error == W_SUCCEEDED, false otherwise
	 */
	operator bool();

	/** Performs == on the underlying m_error */
	bool operator == (const WError& other) const;
	/** Performs == on the underlying m_error */
	bool operator == (const W_ERROR& other) const;
	/** Performs != on the underlying m_error */
	bool operator != (const WError& other) const;
	/** Performs != on the underlying m_error */
	bool operator != (const W_ERROR& other) const;
};
