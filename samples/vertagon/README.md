# Building & Running
Make sure to [build Wasabi](https://github.com/Hasan-Jawaheri/Wasabi#user-content-building) first as all the samples use the built Wasabi in this repo. Then run the following:
```bash
mkdir build && cd build
cmake ..
make
```
On Windows (MSVC) this should create a Visual Studio project and you can run the sample from there. Otherwise, there will be an executable `vertagon` created. You can run it with `(cd ..; ./build/vertagon)` (to set the working directory properly).

# External Use
You can pass a path to Wasabi to the cmake command to run it outside this repository (`cmake -DWASABI_PATH=<path to wasabi on your machine> ..`). Other than that, the samples were designed to be completely independent so they can be easily copied/used in your projects.
