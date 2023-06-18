# AzCore
A collection of tools written by *Philip Haynes* designed to lower the barrier of entry to cross-platform game development
using C++. Most of these tools were created to fill a particular need in the designing of a game from the ground up. The
design of the interfaces were made with cross-platform development in mind, so use of these tools is uniform between the
platforms it currently supports.

### Note
This toolset is still deep in development. While it may be possible to do many things using it in its current state, many
other features are still on the TODO list.

## Goals
To provide flexible tools and frameworks that help in writing cross-platform applications in an elegant way. This is, of course,
subjective, and *Philip's* definition involves a code style that tries to approach brevity and precision at the same time.
It's a style that *Philip* has developed over time for himself which he believes suits high- and low-level programming
equally, and which makes as many guarantees as possible.

## Methodology
The guiding principles behind development of this toolset are cyclical and non-feature-exhaustive.

In short, the desired features are determined by work on the various [projects](projects), and implemented alongside those projects.

Features are implemented on an as-needed basis such that any feature has a known use-case and a way to test it. This prevents
the developer from writing code they think *might* be useful, and instead focus on code that is definitely useful.
This also helps the developer in making logical and helpful decisions to reduce pain points in the end-user code, because the
developer is also writing end-user code at the same time.

This, of course, also has downsides, namely the need to occasionally touch up the AzCore/base while writing end-user code.
Such occurrences are becoming rarer as the tools age, but note that any new features are subject to change rapidly.

## Contributing
**_Be sure to read the [Contribution Guidelines](CONTRIBUTING.md)_**

Any and all contributions are appreciated so long as they're in line with the **Goals** of this project.
If you have any questions regarding a specific feature, you may ask me a question via email at singularity.haynes@gmail.com.
#### Feedback
Simply providing feedback is probably the easiest way to contribute. Be sure to check out the *AzCore* Project under the
[Projects](https://github.com/SingularityAzure/AzCore/projects) tab and the already-open
[Issues](https://github.com/SingularityAzure/AzCore/issues) before opening a new Issue, as your suggestion may be on the
*To-Do* list already. If it is, but you'd like me to treat it with a higher-priority, you may open an Issue explaining why.
#### Implementing
If you find something lacking and know how to fix it, you may do so and open a
[Pull Request](https://github.com/SingularityAzure/AzCore/pulls).

## Building
First clone the repository:
```console
$ git clone --recurse-submodules https://github.com/SingularityAzure/AzCore.git
$ cd AzCore
```
### Linux
#### Dependencies
AzCore uses Vulkan as its primary graphics API, and xcb and Wayland for window IO, so make sure to set up a build environment for Vulkan, xcb, wayland, xkbcommon, and openal for the example projects.
You can use your distribution's Vulkan dev package, or you can grab the Vulkan SDK from [LunarG.](https://www.lunarg.com/vulkan-sdk/)
*Note that using the Vulkan SDK requires setting the environment variable `LINUX_VULKAN_SDK` to the path of the SDK's root folder, or specifying the path when calling `build.sh`. Likewise, the environment variable `WIN32_VULKAN_SDK` can be set for cross-compiling to Windows.*

Also note that you only need the development package from your distribution *OR* the SDK and **not both.** *When using the development packages, you don't need to set any environment variables.*

#### Building
This project is built using CMake (3.25 or newer), but to keep things simple and easy, I've written a script to handle everything cleanly. Just calling: `$ ./build.sh` should tell you what the script can do for you.

To build the Release configuration and install it in one go, it's as simple as calling
```console
$ ./build.sh ReleaseL install
```
Or if you need to specify the path to the Vulkan SDK:
```console
$ ./build.sh ReleaseL install LINUX_VULKAN_SDK /path/to/sdk
```

**Note that installing on Linux requires root privilege. While I can vouch that the script won't do anything malicious, I implore you not to take me at my word and look through the script yourself first. Alternatively, if you're familiar with CMake, you can run the install without the script.**

Assuming everything built correctly, you should now have AzCore installed on your system, ready to be used in your projects.

#### Cross-Compiling to Windows
Using `mingw-w64` you can cross-compile for Windows, and even install this library into the cross-compilation toolchain.
```console
$ ./build.sh ReleaseW install
```
Or if you need to specify the path to the Vulkan SDK:
```console
$ ./build.sh ReleaseW install WIN32_VULKAN_SDK /path/to/win32_sdk
```

### Windows
#### Dependencies
You'll need a development environment set up with MSVC, [CMake 3.25 or newer](https://cmake.org/download/), and Vulkan. Since AzCore uses Vulkan as its primary graphics API, you'll need to grab the Vulkan SDK from [LunarG.](https://www.lunarg.com/vulkan-sdk/) Once you have that you can set the environment variable `VULKAN_SDK` to the path you installed the SDK into (`C:\VulkanSDK\<version>` by default).

#### Building
This project is built using CMake, but to keep things simple and easy, I've written a script to handle everything cleanly. Just calling: `> build.bat` should tell you what the script can do for you.

To build the Release configuration and install it in one go, it's as simple as calling
```console
> build.bat Release install
```
Or if you need to specify the path to the Vulkan SDK:
```console
> build.bat Release install VULKAN_SDK C:\path\to\sdk
```

### Running the Sample Projects
Included are several projects that demonstrate the usage of AzCore. In lieu of proper documentation, these are the next best thing to look at for getting started.
- **window_test** Simply opens a window and prints keyboard and mouse button inputs. Uses *Window IO.*
- **vulkan_quad** A simple example that renders a single textured quad using *Vulkan* and *Window IO*
- **vulkan_tesseract** Renders a rotating tesseract (a 4D hypercube) that you can explore. Uses *Vulkan,* *Window IO* and *Raw Input* for gamepads.
- **LD46** Mine and Flubz' entry into Ludum Dare 46. A simple platformer with a simple story. Uses *Vulkan* and *Window IO*
- **tower_defense** A simple tower defense that uses *Vulkan* and *Window IO*
- **fractal_explorer** An example of using *SIMD,* the *Software Renderer,* and *Window IO* to explore the mandelbrot set.
- **font_test** An old way of testing the generation of *font* atlases.
- **persistence** Uses *BigInt* to determine how many iterations it takes for any number to resolve to zero by repeatedly multiplying its base-10 digits together.
- **unit_tests** A small suite of tests to verify the correctness of the library. More tests are always needed.
- **Az2D** A 2D game engine in development which is used by LD46 and tower_defense.
- **Az2D_Example** Example project for an Az2D game.

#### Linux
The build script allows you to run these from the root of the project like so:
```console
$ ./build.sh run vulkan_tesseract
```
Or you can navigate to their folders and call them like so:
```console
$ cd projects/vulkan_tesseract
$ bin/vulkan_tesseract
```

#### Windows
The build script allows you to run these from the root of the project like so:
```console
> build.bat run vulkan_tesseract
```
Or you can navigate to their folders and call them like so:
```console
> cd projects\vulkan_tesseract
> bin\vulkan_tesseract.exe
```

## Using in Your Projects
If you happen to use CMake, that makes it very easy to use AzCore. All you need to do is add the following to your root `CMakeLists.txt` file:
```cmake
find_package(AzCore REQUIRED)
```
For linking, the following interfaces are available:
```cmake
# Basics
target_link_libraries(your_target PUBLIC AzCore)
# For using web sockets
target_link_libraries(your_target PUBLIC AzCore_Net)
# For using Window IO
target_link_libraries(your_target PUBLIC AzCore_IO)
# For using Vulkan with Window IO
target_link_libraries(your_target PUBLIC AzCore_Vulkan)
# For using the Software Renderer with Window IO
target_link_libraries(your_target PUBLIC AzCore_SoftwareRenderer)
```
Since AzCore is exported a static library, if you don't use CMake, then you'll probably have to link all of the libraries yourself. This allows for a single library that doesn't require you to link to features you don't use. To keep maintenance simpler, I'll refrain from listing the library dependencies here, and instead refer you to [base/CMakeLists.txt](base/CMakeLists.txt)

## Features
#### Window System I/O
- Window configuration, creation, and cleanup.
- A keyboard-layout-agnostic input model, ideal for games.
- A keyboard-layout-regarding input model for anything else.
- Provides an intermediate representation for key codes that translate the same way for every platform.
- Raw input for gamepads.

#### Vulkan Framework
Provides a more intuitive approach to setting up a program using Vulkan by using a tree-like structure. By inheriting loads of
information, many concepts which are difficult to understand in Vulkan become much less cumbersome. This is enhanced by
providing many informative sanity checks that should help you design your program, as well as supporting any validation layers
you want in a way that provides you with contextual information.
While this may help significantly in designing a Vulkan program, it won't replace an understanding of the Vulkan API, and only
shallowly abstracts some concepts. The vast majority of the structures mirror those provided by the Vulkan API almost exactly,
but does so in a style that describes the relationship between structures.

In my estimations thus far, the framework divides the necessary lines of code by about ten when compared to writing raw Vulkan
code.

#### Logging
io::Log is an intuitive and efficient way to log to both a log file and console at the same time. It completely usurps the C++ stream-like logger style for a somewhat more traditional printf-style logger. Using template varargs, you don't need a format string, and instead just put the values to print in order.
```C++
i32 integer = 3;
f32 float_num = 1.0f;
AzCore::String string = "(much less)";
AzCore::io::Log logger = io::Log(
   "filename.log",
   /*optional bool useConsole=  */ true   /*default true*/,
   /*optional bool useFile=     */ true   /*default false*/,
   /*optional FILE *consoleFile=*/ stdout /*default stdout*/
);
logger.PrintLn("This ", integer, " is ", float_num, " less ", string, " cumbersome!").Flush();

AzCore::io::cout.Print("There's also io::cout available, ").Flush();
AzCore::io::cerr.PrintLn("as well as io::cerr.").Flush();
// We're only using Flush because we have 3 different Logs and actual order is only guaranteed for individual ones.
// Just using PrintLn is almost always good enough without Flush.
```
Result in console:
```console
[filename.log]  This 3 is 1.0 less (much less) cumbersome!
There's also io::cout available, as well as io::cerr.
```
Result in `filename.log`:
```
This 3 is 1.0 less (much less) cumbersome!
```

#### Math
- Vector, Matrix, Complex, and Quaternion data types.
- Angle types with implicit modular arithmetic.
- Polynomial solvers (root finding) orders 1 through 5.
- BigInt, a fast, 128-byte integer data type that uses up to 15 u64's to represent values approximately spanning -10^289 to 10^289.

#### Memory Primitives
- `Array<typename T, i32 allocTail>`, a heap-allocated array of type T with an optional string terminator guaranteed to be on
the end.
- `ArrayWithBucket<typename T, i32 noAllocCount, i32 allocTail>`, like Array above, except for sizes between 0 and noAllocCount, it uses a buffer within the struct rather than heap-allocating one. For larger sizes, it behaves the same as Array.
- `String` and `WString` are `ArrayWithBucket<char, 16, 1>` and `Array<char32, 4, 1>` respectively and come with some special operators to work
nicely together with `char*` and `char32*` strings.
- UTF-8 to Unicode converter, used to translate a `String` or a `char*` to `WString`.
- Converters from most basic data types to `String`, including 128-bit wide integers and floats.
- Converters from `String` and `WString` to `f32`.
- `String Stringify(args...)` returns a String that concatenates all the args together, converting any types into characters that have `void AppendToString(String &string, T value)` defined for them. This is done without allocating more than one String.
- `StaticArray<typename T, i32 count>`, a constant-storage-sized array with a variable effective size.
- Tools to identify and convert endianness.
- `List<typename T>`, a singly-linked list.
- `Ptr<typename T>`, polymorphs into either a raw `T *pointer` or an index of an `Array<T, 0>`. Helps in the case of Array data
  moving because of a resize.
- `Range<typename T>`, like `Ptr<T>` except it has an associated size. This can represent a range inside a List<T>, an Array<T>, or a c-style array.
- `SimpleRange<typename T>`, points to a c-style array with an associated size. Less versatile than Range, but much simpler and without much (or any?) overhead.
- `HashMap<typename Key_t, typename Value_t, u16 arraySize>` and `HashSet<typename Key_t, u16 arraySize>` use `constexpr i32 IndexHash<u16 bounds>(T in)` functions to index into an Array of Nodes, which is O(1) for sets of values smaller than arraySize.
- `BinaryMap<typename Key_t, typename Value_t>` and `BinarySet<typename Key_t>` use `bool operator<(...)` from `Key_t` to sort its Nodes and look them up.
- `UniquePtr<typename T>` manages a single heap-allocated object in a way that's less restrictive than `std::unique_ptr`

#### Font Files and Atlases
- Dependency-free font parser
- Automatic atlas packing
- Signed distance fields are generated by rigorous distance algorithms rather than searching through super-sampled bitmaps,
allowing for lightning-quick rendering of glyphs with 100% accuracy.
- Atlases can be added to on-the-fly, allowing for exhaustive support of font files to work for all languages without packing
every glyph available at the beginning of the program.
- Composite glyphs are deconstructed, and only their components are packed. This should allow better use of space at a
minimal runtime cost.

## Platforms
- GNU/Linux
- Windows

## License
This work is licensed under the Creative Commons Attribution-ShareAlike 4.0 International License. To view a copy of this
license, visit http://creativecommons.org/licenses/by-sa/4.0/ or send a letter to Creative Commons, PO Box 1866, Mountain
View, CA 94042, USA.
