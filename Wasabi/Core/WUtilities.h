/** @file WUtilities.h
 *  @brief Utility functions
 *
 *  This file contains general-purpose utility functions for the engine
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "WMath.h"

class Wasabi;

namespace WUtil {
  /**
   * Convert a point in 3D space to a where it appears on the screen of an
   * application.
   * @param  app   The Wasabi application in which projection happens
   * @param  point The 3D point to convert
   * @param  x     A pointer to an integer to be filled with the projected
   *               x coordinate
   * @param  y     A pointer to an integer to be filled with the projected
   *               y coordinate
   * @return       true if the point appears in on the screen, false otherwise
   */
	bool Point3DToScreen2D(Wasabi* app, WVector3 point, int* x, int* y);

  /**
   * Checks if a given ray intersects a given cube in 3D space.
   * @param  cubeHalfSize Half the width of the cube
   * @param  rayPos       Origin of the ray
   * @param  rayDir       Direction of the ray
   * @param  cubePos      Position of the cube in 3D space
   * @return              true if the ray position at rayPos, pointing to
   *                      same direction as rayDir, intersects the cube whose
   *                      origin is at cubePos and whose dimensions are
   *                      2*cubeHalfSize, false otherwise
   */
	bool RayIntersectCube(float cubeHalfSize, WVector3 rayPos, WVector3 rayDir,
                        WVector3 cubePos = WVector3(0, 0, 0));

  /**
   * Checks if a given ray intersects a given box in 3D space.
   * @param  halfDimensions Half the dimensions of the box
   * @param  rayPos         Origin of the ray
   * @param  rayDir         Direction of the ray
   * @param  boxPos         Position of the cube in 3D space
   * @return                true if the ray position at rayPos, pointing to
   *                        same direction as rayDir, intersects the box whose
   *                        origin is at boxPos and whose dimensions are
   *                        2*halfDimensions, false otherwise
   */
	bool RayIntersectBox(WVector3 halfDimensions, WVector3 rayPos,
                       WVector3 rayDir, WVector3 boxPos = WVector3(0, 0, 0));

	/**
	 * Returns a random number between 0 and 1
	 */
	float frand_0_1();
	/**
	 * Returns a linear interpolation between x and y at factor f
	 */
	float flerp(float x, float y, float f);
};