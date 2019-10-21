/**
 * @file WCompatibility.hpp
 * Compatibility macros
 **/

#pragma once

/**
 * MSVC _s functions for non-MSVC
 */

#ifndef _MSC_VER

#define fopen_s(a,b,c) *a = fopen(b, c)
#define strcpy_s(a,b,c) strcpy(a,c)
#define sprintf_s(a,b,c,...) sprintf(a,c, __VA_ARGS__)
#define ZeroMemory(x,y) memset(x, 0, y)

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(p) ((void)p)
#endif

#else // !defined(_MSC_VER)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif

/**
 * Vulkan extensions
 */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#elif (defined __linux__)
#ifndef VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif
#endif
