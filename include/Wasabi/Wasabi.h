/** @file Wasabi.h
 *  @brief Main header file for the library
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/WindowAndInput/WWindowAndInputComponent.h"
#include "Wasabi/Physics/WPhysicsComponent.h"
#include "Wasabi/Texts/WText.h"
#include "Wasabi/Cameras/WCamera.h"
#include "Wasabi/Renderers/WRenderer.h"
#include "Wasabi/Geometries/WGeometry.h"
#include "Wasabi/Objects/WObject.h"
#include "Wasabi/Materials/WEffect.h"
#include "Wasabi/Materials/WMaterial.h"
#include "Wasabi/Images/WImage.h"
#include "Wasabi/Images/WRenderTarget.h"
#include "Wasabi/Sprites/WSprite.h"
#include "Wasabi/Sounds/WSound.h"
#include "Wasabi/Lights/WLight.h"
#include "Wasabi/Animations/WAnimation.h"
#include "Wasabi/Animations/WSkeletalAnimation.h"
#include "Wasabi/Particles/WParticles.h"
#include "Wasabi/Terrains/WTerrain.h"

/**
 * This function is not defined by the library. It must be defined by the user.
 * This function will be called at the beginning of the program, and will
 * expect the user-defined code to create a newly allocated instance of an
 * application, which should be a child of a Wasabi class.
 * 
 * @return A newly allocated Wasabi child that will run the engine
 */
Wasabi* WInitialize();
