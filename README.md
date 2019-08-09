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

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Particles

> <img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Soft Particles

> <img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Particle Lighting

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Basic Shadows

> <img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Soft Shadows

> <img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Cascaded Shadowmaps

> <img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Multi-directional Shadows

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Deferred Rendering

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Seamless Infinite Terrains (Geometry clipmaps)

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Skeletal Animations (GPU-based)

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Geometry Instancing

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Havok Physics (mostly replaced by bullet)

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> Bullet Physics

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/tick.png" width="16" height="16"> File format for game assets

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Map Editor

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Linux Support

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Mac Support

# Building
## Vulkan Requirements
- Install the Vulkan driver for your graphics card
    - nVidia graphics card: https://developer.nvidia.com/vulkan-driver
    - Intel graphics card: https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540
    - AMD graphics card: (???)
- Install LunarG Vulkan SDK (https://lunarg.com/vulkan-sdk/)

## Windows
- Install CMake (make sure to select the option to add it to `PATH`)
- Install Visual Studio (make sure to add the feature `Visual C++ tools for CMake`)
- Clone and build [BulletPhysics](https://github.com/bulletphysics/bullet3) (make sure to pass `-DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON` to the `cmake` command). After `cmake` a Visual Studio `.sln` file will be generated, which you should open and build using Visual Studio (both Debug and Release)
- Install [OpenAL SDK](https://www.openal.org/)
- (Optional) Install [FBX SDK](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2019-0)
- Run the following (*replace `Visual Studio 16 2019` with your installed version of visual studio, run `cmake --help` to see a list, replace `BULLET_ROOT` with the path where you built Bullet and replace OpenAL/FBXSDK paths if necessary*):
```bash
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -DBULLET_ROOT="C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build" -DBULLET_INCLUDE_DIR="C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/src" -DOPENAL_INCLUDE_DIR="c:/Program Files (x86)/OpenAL 1.1 SDK/include" -DOPENAL_LIBRARY="c:/Program Files (x86)/OpenAL 1.1 SDK/libs/Win64/OpenAL32.lib" -DFBXSDK_ROOT="C:/Program Files/Autodesk/FBX/FBX SDK/2019.0" ..
```
- Open `build/Wasabi.sln` in Visual Studio and build/run the code

## Linux
- Install dependencies
```bash
sudo apt-get install cmake g++-multilib libopenal-dev libx11-xcb-dev
```

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

## Gallery

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/particles.png" /> 
<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/ssao.png" /> 

## Contact

Please let me know about any issues, dead links, suggestions or any comments. My email is hasan.aljawaheri [at] gmail [dot] com

## Acknowledgments

* Thanks [nothings](https://github.com/nothings) for the [stb](https://github.com/nothings/stb) library which Wasabi uses for font rendering.
* Thanks [Abdulrahman Fayad](https://github.com/aboudfayad)

## License

GNU Public license, feel free to go wild with this.

