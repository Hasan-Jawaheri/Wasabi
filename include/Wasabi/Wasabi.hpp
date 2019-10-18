/** @file Wasabi.hpp
 *  @brief Main header file for the library
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Wasabi/WindowAndInput/WWindowAndInputComponent.hpp"
#include "Wasabi/Physics/WPhysicsComponent.hpp"
#include "Wasabi/Texts/WText.hpp"
#include "Wasabi/Cameras/WCamera.hpp"
#include "Wasabi/Renderers/WRenderer.hpp"
#include "Wasabi/Geometries/WGeometry.hpp"
#include "Wasabi/Objects/WObject.hpp"
#include "Wasabi/Materials/WEffect.hpp"
#include "Wasabi/Materials/WMaterial.hpp"
#include "Wasabi/Images/WImage.hpp"
#include "Wasabi/Images/WRenderTarget.hpp"
#include "Wasabi/Sprites/WSprite.hpp"
#include "Wasabi/Sounds/WSound.hpp"
#include "Wasabi/Lights/WLight.hpp"
#include "Wasabi/Animations/WAnimation.hpp"
#include "Wasabi/Animations/WSkeletalAnimation.hpp"
#include "Wasabi/Particles/WParticles.hpp"
#include "Wasabi/Terrains/WTerrain.hpp"

/**
 * This function is not defined by the library. It must be defined by the user.
 * This function will be called at the beginning of the program, and will
 * expect the user-defined code to create a newly allocated instance of an
 * application, which should be a child of a Wasabi class.
 * 
 * @return A newly allocated Wasabi child that will run the engine
 */
Wasabi* WInitialize();
