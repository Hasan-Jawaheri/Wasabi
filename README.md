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

<img src="https://github.com/Hasan-Jawaheri/Wasabi/raw/master/gitstuff/wip.ico" width="16" height="16"> Assimp integration (and getting rid of FBX SDK!)

# Building
### Requirements
- Install LunarG Vulkan SDK (https://lunarg.com/vulkan-sdk/). You may also need to install vendor/gpu specific driver for Vulkan (or maybe that's not a thing anymore...)
- Install `cmake` 3.15.
- (Windows) Visual Studio (make sure to add the feature `Visual C++ tools for CMake` during installation)
- (MacOS) XCode

### Building Wasabi
```bash
mkdir build && cd build
cmake ..
```

This will automatically initialize and update submodule dependencies. On Windows, this will generate a solution `build/Wasabi.sln`, which you can open to use Visual Studio on the source code to edit/compile.

# Usage

To link an application to Wasabi, you will need to link to Vulkan and to the wasabi library `libwasabi.a` (or `wasabi.lib` on Windows). On mac, you will need to add the following frameworks as well: Cocoa, CoreAudio, IOKit, CoreFoundation, CoreVideo, AudioUnit. You may use the CMake helper in `CMake/LinkToWasabi.cmake` (e.g. `link_target_to_wasabi(wasabi_test "${CMAKE_BINARY_DIR}/dist")` in your cmake file) to automatically link to the engine.

```C++
#include "Wasabi/Wasabi.h"

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

* [stb](https://github.com/nothings/stb) library which Wasabi uses for font rendering.
* [Abdulrahman Fayad](https://github.com/aboudfayad)
* [GLFW](https://github.com/glfw/glfw)

## License

GNU Public license, feel free to go wild with this.
