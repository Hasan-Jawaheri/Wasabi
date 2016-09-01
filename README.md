# Wasabi
Wasabi Vulkan Game Engine is currently a work-in-progress port for HasX11 Game Engine. Wasabi is designed to allow C++ programmers to write games and graphics applications easily without having to worry about the details of the tedious graphics APIs (Vulkan, Direct3D, OpenGL, etc...).

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

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Skeletal Animations

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Geometry Instancing

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Havok Physics

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Map Editor

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Linux Support

## Getting Started

To run Wasabi, you will need the following:

* Vulkan driver for your OS and graphics card
  * nVidia graphics card: https://developer.nvidia.com/vulkan-driver
  * Intel graphics card: https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540
  * AMD graphics card: (???)
* LunarG Vulkan SDK (https://lunarg.com/vulkan-sdk/)
* OpenAL SDK (https://www.openal.org/downloads/)

With the above installed, Wasabi should compile with Visual Studio using **x86 configuration (NOT x64)**. For other compilers, you need to add the include path and library path of OpenAL and LunarG SDK. Those can be usually found in C:/VulkanSDK/<version>/include, C:/VulkanSDK/<version>/bin32, C:\Program Files (x86)\OpenAL 1.1 SDK\include, C:\Program Files (x86)\OpenAL 1.1 SDK\libs\Win32\ on Windows.

## Usage
```C++
#include "Wasabi.h"

class MyApplication : public Wasabi {
public:
  WError Setup() {
    // start the engine
    WError status = StartEngine(640, 480);
    if (!status) {
      // Failed to start the engine...
      return status;
    }
    return status;
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

* Setup(): All application setup code goes here. You will need to call StartEngine(width, height) here. StartEngine() creates the window and initializes the engine's resources. You shouldn't create any Wasabi objects (WObject, WGeometry, WImage, etc...) before calling StartEngine().
* Loop(): This function is called every frame. This is where you update your application.
* Cleanup(): This is called before the engine exits so you can cleanup resources.

## Tutorials

WIP

## Documentation

WIP

## Contribution

WIP

## Contact

Please let me know about any issues, dead links, suggestions or any comments. My email is hasan.aljawaheri [at] gmail [dot] com

## Acknowledgments

Thanks [nothings](https://github.com/nothings) for the [stb](https://github.com/nothings/stb) library which Wasabi uses for font rendering.

## License

GNU Public license, feel free to go wild with this.
