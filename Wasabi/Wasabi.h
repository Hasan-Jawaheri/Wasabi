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
#include "Physics/Bullet/WBulletPhysics.h"
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
#include "Terrains/WTerrain.h"
#ifdef _WIN32
#include "Windows/Windows/WWC_Win32.h"
#include "Input/Windows/WIC_Win32.h"
#elif defined(__linux__)
#include "Windows/Linux/WWC_Linux.h"
#include "Input/Linux/WIC_Linux.h"
#endif

#pragma comment(lib, "Wasabi.lib")
#ifdef _DEBUG
#pragma comment(lib, "Bullet3Collision_vs2010_debug.lib")
#pragma comment(lib, "Bullet3Common_vs2010_debug.lib")
#pragma comment(lib, "Bullet3Dynamics_vs2010_debug.lib")
#pragma comment(lib, "Bullet3Geometry_vs2010_debug.lib")
#pragma comment(lib, "BulletCollision_vs2010_debug.lib")
#pragma comment(lib, "BulletDynamics_vs2010_debug.lib")
#pragma comment(lib, "LinearMath_vs2010_debug.lib")
/*
#pragma comment(lib, "Bullet2FileLoader_vs2010_debug.lib")
#pragma comment(lib, "BulletFileLoader_vs2010_debug.lib")
#pragma comment(lib, "BulletInverseDynamicsUtils_vs2010_debug.lib")
#pragma comment(lib, "BulletInverseDynamics_vs2010_debug.lib")
#pragma comment(lib, "BulletRobotics_vs2010_debug.lib")
#pragma comment(lib, "BulletSoftBody_vs2010_debug.lib")
#pragma comment(lib, "BulletWorldImporter_vs2010_debug.lib")
#pragma comment(lib, "BulletXmlWorldImporter_vs2010_debug.lib")
#pragma comment(lib, "BussIK_vs2010_debug.lib")
#pragma comment(lib, "clsocket_vs2010_debug.lib")
#pragma comment(lib, "ConvexDecomposition_vs2010_debug.lib")
*/
#else
#endif

/**
 * This function is not defined by the library. It must be defined by the user.
 * This function will be called at the beginning of the program, and will
 * expect the user-defined code to create a newly allocated instance of an
 * application, which should be a child of a Wasabi class.
 * 
 * @return A newly allocated Wasabi child that will run the engine
 */
Wasabi* WInitialize();
