/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine common include
*********************************************************************/

#pragma once

#include "WError.h"
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>

using namespace std;
using std::ios;
using std::basic_filebuf;
using std::vector;

#define W_SAFE_RELEASE(x) { if ( x ) { x->Release ( ); x = NULL; } }
#define W_SAFE_REMOVEREF(x) { if ( x ) { x->RemoveReference ( ); x = NULL; } }
#define W_SAFE_DELETE(x) { if ( x ) { delete x; x = NULL; } }
#define W_SAFE_DELETE_ARRAY(x) { if ( x ) { delete[] x; x = NULL; } }
#define W_SAFE_ALLOC(x) GlobalAlloc ( GPTR, x )
#define W_SAFE_FREE(x) { if ( x ) { GlobalFree ( x ); } }

#ifndef max
#define max(a,b) (a>b?a:b)
#endif
#ifndef min
#define min(a,b) (a<b?a:b)
#endif

typedef unsigned int uint;
