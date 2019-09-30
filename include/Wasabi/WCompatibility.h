/**
 * @file WCompatibility.h
 * Compatibility macros
 **/

#pragma once

#ifndef _WIN32
#define fopen_s(a,b,c) *a = fopen(b, c)
#define strcpy_s(a,b,c) strcpy(a,c)
#define sprintf_s(a,b,c,...) sprintf(a,c, __VA_ARGS__)
#define ZeroMemory(x,y) memset(x, 0, y)
#endif

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(p) (p)
#endif
