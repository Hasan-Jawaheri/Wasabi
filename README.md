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
