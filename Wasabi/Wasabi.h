/** @file Wasabi.h
 *  @brief Main header file for the library
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Windows/WWindowComponent.h"
#include "Input/WInputComponent.h"
#include "Physics/WPhysicsComponent.h"
#include "Texts/WText.h"
#include "Cameras/WCamera.h"
#include "Renderers/WRenderer.h"
#include "Geometries/WGeometry.h"
#include "Objects/WObject.h"
#include "Materials/WEffect.h"
#include "Materials/WMaterial.h"
#include "Images/WImage.h"
#include "Images/WRenderTarget.h"
#include "Sprites/WSprite.h"
#include "Sounds/WSound.h"
#include "Lights/WLight.h"
#include "Animations/WAnimation.h"
#include "Animations/WSkeletalAnimation.h"
#include "Particles/WParticles.h"
#ifdef _WIN32
#include "Windows/Windows/WWC_Win32.h"
#include "Input/Windows/WIC_Win32.h"
#elif defined(__linux__)
#include "Windows/Linux/WWC_Linux.h"
#include "Input/Linux/WIC_Linux.h"
#endif

#pragma comment(lib, "Wasabi.lib")

/**
 * This function is not defined by the library. It must be defined by the user.
 * This function will be called at the beginning of the program, and will
 * expect the user-defined code to create a newly allocated instance of an
 * application, which should be a child of a Wasabi class.
 * 
 * @return A newly allocated Wasabi child that will run the engine
 */
Wasabi* WInitialize();
