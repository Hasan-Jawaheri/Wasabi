#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#elif (defined __linux__)
#ifndef VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif
#endif

#include <vulkan/vulkan.h>
#pragma comment (lib, "vulkan-1.lib")
#include "VkTools/vulkanswapchain.hpp"
#include "VkTools/vulkantools.h"

#include <math.h>
#include <float.h>
#include <climits>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>
#include <chrono>

#include "WError.h"
#include "WTimer.h"
#include "WMath.h"
