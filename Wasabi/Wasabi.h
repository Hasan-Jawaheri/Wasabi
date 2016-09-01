#pragma once

#include "Windows/WWindowComponent.h"
#include "Input/WInputComponent.h"
#include "Physics/WPhysicsComponent.h"
#include "Texts/WText.h"
#include "Renderers/WRenderer.h"
#include "Geometries/WGeometry.h"
#include "Objects/WObject.h"
#include "Materials/WEffect.h"
#include "Materials/WMaterial.h"
#include "Cameras/WCamera.h"
#include "Images/WImage.h"
#include "Images/WRenderTarget.h"
#include "Sprites/WSprite.h"
#include "Sounds/WSound.h"
#include "Lights/WLight.h"
#include "Animations/WAnimation.h"
#include "Animations/WSkeletalAnimation.h"
#ifdef _WIN32
#include "Windows/Windows/WWC_Win32.h"
#include "Input/Windows/WIC_Win32.h"
#elif defined(__linux__)
#include "Windows/Linux/WWC_Linux.h"
#include "Input/Linux/WIC_Linux.h"
#endif

#pragma comment(lib, "Wasabi.lib")

Wasabi* WInitialize();
