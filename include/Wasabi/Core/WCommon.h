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
#include "Wasabi/Core/VkTools/vulkanswapchain.hpp"
#include "Wasabi/Core/VkTools/vulkantools.h"

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

#include "Wasabi/Core/WError.h"
#include "Wasabi/Core/WTimer.h"
#include "Wasabi/Core/WMath.h"
