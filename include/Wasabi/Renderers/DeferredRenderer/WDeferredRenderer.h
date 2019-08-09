/** @file WDeferredRenderer.h
 *  @brief Utility to setup a deferred renderer
 *
 *  Deferred rendering is a rendering technique in which objects are not
 *  rendered directly to the intended render target, but are rendered (in
 *  several stages) to off-screen buffers and then finally composed onto the
 *  final destination (render target).
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include "Core/WCore.h"

WError WInitializeDeferredRenderer(Wasabi* app);
