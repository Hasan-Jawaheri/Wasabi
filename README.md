# Wasabi
Wasabi Vulkan Game Engine is currently a work-in-progress port for HasX11 Game Engine.

## Current Features
[tick]: 
[prog]: 

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> OpenAL Sounds

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Sprites

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Text

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Lights

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Effects & Materials

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Particles

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Deferred Rendering

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Seamless Infinite Terrains (Geometry clipmaps)

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Skeletal Animations

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Havok Physics

## Getting Started

To run Wasabi, you will need the following:

* Vulkan driver for your OS and graphics card
  * nVidia graphics card: https://developer.nvidia.com/vulkan-driver
  * Intel graphics card: https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540
  * AMD graphics card: (???)
* LunarG Vulkan SDK (https://lunarg.com/vulkan-sdk/)
* OpenAL SDK (https://www.openal.org/downloads/)

With the above installed, Wasabi should compile with Visual Studio. For other compilers, you need to add the include path and library path of OpenAL and LunarG SDK. Those can be usually found in C:/VulkanSDK/<version>/include, C:/VulkanSDK/<version>/bin32, C:\Program Files (x86)\OpenAL 1.1 SDK\include, C:\Program Files (x86)\OpenAL 1.1 SDK\libs\Win32\ on Windows.

## Usage
```C++
#include "Wasabi.h"

class MyApplication : public Wasabi {
public:
  WError Setup() {
    // start the engine
    WError err = StartEngine(640, 480);
    if (!err) {
      // an error occurred, report it and return it
      MessageBoxA(nullptr, "Ooops!", "Wasabi", MB_OK | MB_ICONERROR);
      return err;
    }
    return err;
  }
  bool Loop(float fDeltaTime) {
    return true; // return true to continue to next frame
  }
  void Cleanup() {
  }
};

Wasabi* WInitialize() {
  return new MyApplication();
}
```

To start the engine, you need to implement the function WInitialize and make it return a new instance of a class that implements the Wasabi abstract class (in this case MyApplication).

MyApplication must implement 3 functions from Wasabi: Setup, Loop and Cleanup:

* WError Setup(): All application setup code goes here. You will need to call StartEngine(width, height) here. StartEngine() creates the window and initializes the engine's resources. You shouldn't create any Wasabi objects (WObject, WGeometry, WImage, etc...) before calling StartEngine().
* bool Loop(float fDeltaTime): This function is called every frame. This is where you update your application.
* void Cleanup(): This is called before the engine exists so you can cleanup resources.

## Tutorials

WIP

## Documentation

WIP

## Contact

Please let me know about any issues, dead links, suggestions or any comments. My email is hasan.aljawaheri [at] gmail [dot] com
