﻿# Basic C++ sample usages of Magma library and Vulkan graphics API

## Cloning

To clone this repository with external submodules:
```
git clone --recursive https://github.com/vcoda/basic-graphics-samples.git
``` 
or clone this repository only and update submodules manually:
```
git clone https://github.com/vcoda/basic-graphics-samples.git
cd basic-graphics-samples
git submodule update --init --recursive
``` 

## Build tools and SDK

## Windows

* [Microsoft Visual Studio Community 2017](https://www.visualstudio.com/downloads/)<br>
* [Git for Windows](https://git-scm.com/download)<br>
* [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)<br>

Check that VK_SDK_PATH environment variable is present after SDK installation:
```
echo %VK_SDK_PATH%
```
Shaders are automatically compiled using glslangValidator as Custom Build Tool.
If you use Visual Studio newer than 2017, change the SDK version in the project property pages or by right-clicking the solution and selecting "Retarget solution".
</br>

## Linux (Ubuntu)

Install GCC and Git (if not available):
```
sudo apt update
sudo apt install gcc
sudo apt install g++
sudo apt install make
sudo apt install git
```
Install XCB headers and libraries:
```
sudo apt install xcb
sudo apt install libxcb-icccm4-dev
```
For Xlib, install X11 headers and libraries (optional):
```
sudo apt install libx11-dev
sudo apt install libxrandr-dev
```

* [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)<br>

Go to the directory where .run file was saved:
```
chmod ugo+x vulkansdk-linux-x86_64-<version>.run
./vulkansdk-linux-x86_64-<version>.run
cd VulkanSDK/<version>/
source ./setup-env.sh
```
Check that Vulkan environment variables are present:
```
printenv | grep Vulkan
```

### Systems with AMD graphics hardware

Check whether AMDGPU-PRO stack is installed:
```
dpkg -l amdgpu-pro
```
If not, download and install it from an official web page:

* [AMD Radeon™ Software AMDGPU-PRO Driver for Linux](https://support.amd.com/en-us/kb-articles/Pages/AMDGPU-PRO-Install.aspx)<br>

NOTE: You have to make sure that graphics driver is compatible with current Linux kernel. Some of the driver's libraries are ***compiled*** against installed kernel headers, and if kernel API changed since driver release, compilation will fail and driver become malfunction after reboot. I used a combination of AMDGPU-PRO Driver Version 17.10 and Ubuntu 16.04.2 with kernel 4.8.0-36. Also disable system update as it may upgrade kernel to the version that is incompatible with installed graphics driver. 
Successfull AMDGPU-PRO installation should look like this:
```
Loading new amdgpu-pro-17.10-446706 DKMS files...
First Installation: checking all kernels...
Building only for 4.8.0-36-generic
Building for architecture x86_64
Building initial module for 4.8.0-36-generic
Done.
Forcing installation of amdgpu-pro
```
After reboot check that driver stack is installed:
```
dpkg -l amdgpu-pro
Desired=Unknown/Install/Remove/Purge/Hold
| Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/trig-aWait/Trig-pend
|/ Err?=(none)/Reinst-required (Status,Err: uppercase=bad)
||/ Name                            Version              Architecture         Description
+++-===============================-====================-====================-====================================================================
ii  amdgpu-pro                      17.10-446706         amd64                Meta package to install amdgpu Pro components.
```
### Systems with Nvidia graphics hardware

TODO

### Building

To build all samples, go to the repo root directory and run make script:
```
make -j<N>
```
where N is the number of threads to run with multi-threaded compilation. First, magma and quadric libraries will be built, then samples.
To build a particular sample, you can build dependencies separately, then go to the sample's directory and run make script:
```
cd 01-clear
make
./01-clear
```
There is debug build by default. For release build, set DEBUG variable to 0, e. g.:
```
make magma DEBUG=0 -j<N>
```

## Android

* [Android Studio](https://developer.android.com/studio/index.html)<br>
* [Android NDK](https://github.com/android-ndk/ndk/wiki)<br>
TODO

## macOS and iOS

* [XCode](https://developer.apple.com/xcode/)<br>
* [MoltenVK](https://github.com/KhronosGroup/MoltenVK)<br>
TODO

## Samples

### [01 - Clear framebuffer](01-clear/)
<img src="./screenshots/01.png" height="128px" align="left">
Shows how to setup command buffer for simple framebuffer clear.
<br><br><br><br><br>

### [02 - Triangle](02-triangle/)
<img src="./screenshots/02.png" height="128px" align="left">
Draws triangle primitive from the vertices generated in the vertex shader.
<br><br><br><br><br>

### [03 - Vertex buffer](03-vertex-buffer/)
<img src="./screenshots/03.png" height="128px" align="left">
Draws triangle primitive with per-vertex color using vertex buffer.
<br><br><br><br><br>

### [04 - Vertex transform](04-vertex-transform/)
<img src="./screenshots/04.png" height="128px" align="left">
Shows how to setup perspective transformation and apply it to the vertices in the vertex shader.
<br><br><br><br><br>

### [05 - Mesh](05-mesh/)
<img src="./screenshots/05.png" height="128px" align="left">
Generates famous Utah Teapot from the set of patches and draws wireframe mesh with perspective transformation.
Smoothness of the surface could be controlled using subdivision degree.
<br><br><br><br>

### [06 - Texture](06-texture/)
<img src="./screenshots/06.png" height="128px" align="left">
Shows how to load DXT texture data (using Gliml), create Vulkan images and combine them in the fragment shader using image samplers.
<br><br><br><br>

### [07 - Texture array](07-texture-array/)
<img src="./screenshots/07.png" height="128px" align="left">
Utilizes "texture array" hardware feature to apply multiple textures inside single draw call.
Different texture LODs could be viewed.
<br><br><br><br>

### [08 - Cubemap texture](08-texture-cube/)
<img src="./screenshots/08.png" height="128px" align="left">
Shows how to load cubemap .dds textures and perform environment mapping in the fragment shader.
<br><br><br><br>

### [09 - Volume texture](09-texture-volume/)
<img src="./screenshots/09.png" height="128px" align="left">
Loads volumetric data into 3D texture and performs ray casting with large number of samples to accomodate resulting color.
<br><br><br><br>

### [10.a - Render to texture](10.a-render-to-texture/)
<img src="./screenshots/10a.png" height="128px" align="left">
Performs render to texture using color and depth attachments. Framebuffer defines render pass, color/depth image views and dimensions.
Looks like AMD hardware uses compute queue for resolving, because it slows down my machine when ethereum miner is running.
<br><br><br>

### [10.b - Render to multisample texture](10.b-render-to-msaa-texture/)
<img src="./screenshots/10b.png" height="128px" align="left">
Performs render to multisample texture with resolve operation using RenderPass.
<br><br><br><br><br>

### [11 - Occlusion query](11-occlusion-query/)
<img src="./screenshots/11.png" height="128px" align="left">
Simple usage of hardware occlusion queries.
<br><br><br><br><br>

### [12 - Alpha blending](12-alpha-blend/)
<img src="./screenshots/12.png" height="128px" align="left">
Good old alpha blending.
<br><br><br><br><br>

### [13 - Specialization constants](13-specialization/)
<img src="./screenshots/13.png" height="128px" align="left">
Shows how to force shader compiler to perform static branching using specialization constants.
For each fragment shader branch, there is a separate pipeline instance.
<br><br><br><br>

### [14 - Push constants](14-pushconstants/)
<img src="./screenshots/14.png" height="128px" align="left">
Uses push constants - a limited storage inside command buffer. Push constants are updated when command buffer is recorded.
<br><br><br><br>

### [15 - Particles](15-particles/)
<img src="./screenshots/15.png" height="128px" align="left">
This sample shows how to use gl_PointSize built-in variable to draw particles that are properly scaled with distance.
As number of particles varies, vertex count put to indirect buffer to fetch from instead of specify it in vkCmdDraw() function with command buffer rebuild.
Particle engine initially implemented by Kevin Harris and adopted by me for rendering with Vulkan.
<br><br>

### [16 - Immediate mode](16-immediate-mode/)
<img src="./screenshots/16.png" height="128px" align="left">
Sometimes I miss immediate render mode from OpenGL 1.x era, apparently it was useful to (quickly) draw something on the screen.
This sample shows how to use Magma's ImmediateRender to draw different primitives without VB mapping, data copy, state management etc.
<br><br><br>

### [17 - ShaderToy](17-shadertoy/)
<img src="./screenshots/17.png" height="128px" align="left">
Unlike OpenGL, where shader compiler is provided by the graphics driver, Vulkan uses pre-compiled SPIR-V bytecode.
This sample shows how to use https://github.com/google/shaderc compiler from Google to compile GLSL shaders on fly 
and apply changes immediately, which may be useful for shader development. This sample only for Release x64 build configuration.
Example fragment shader taken from https://www.shadertoy.com/view/4tl3z4 (written by Reinder Nijhofftaken).
<br>

### [18 - Compute shader](18-compute/)
<img src="./screenshots/18.jpg" width="256px" align="left">
Compute shaders are core part of Vulkan. This sample performs arithmetic computations on two set of numbers using GPU compute shader.
<br><br>

## Credits
This framework uses a few third-party libraries:

* [Microsoft DirectXMath](https://github.com/Microsoft/DirectXMath)<br>
  Linear algebra library with fantastic CPU optimizations using SSE2/SSE3/SSE4/AVX/AVX2 intrinsics.<br>
  I wrote a simple [wrapper](https://github.com/magma-lib/rapid) over it to make its usage more OOP friendly.

* [Gliml](https://github.com/floooh/gliml)<br>
  Minimalistic image loader library by [Andre Weissflog](https://github.com/floooh).
