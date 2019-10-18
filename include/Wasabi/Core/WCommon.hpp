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

#include "Wasabi/WCompatibility.hpp"

#include <vulkan/vulkan.h>
#include "Wasabi/Core/VkTools/vulkanswapchain.hpp"
#include "Wasabi/Core/VkTools/vulkantools.hpp"

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

#include "Wasabi/Core/WError.hpp"
#include "Wasabi/Core/WTimer.hpp"
#include "Wasabi/Core/WMath.hpp"
