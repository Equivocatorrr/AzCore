# AzCore
A collection of tools written by *Philip Haynes* designed to lower the barrier of entry to cross-platform game development
using C++. Most of these tools were created to fill a particular need in the designing of a game from the ground up. The
design of the interfaces were made with cross-platform development in mind, so use of these tools is uniform between the
platforms it currently supports.

### Note
This toolset is still deep in development. While it may be possible to do many things using it in its current state, many
other features are still on the TODO list. Also, these tools aren't necessarily designed to be used completely on their own,
but rather to provide a pretty good starting point for writing your own feature-full programs.

## Features
"Why should I use this?" you might ask. Well, that depends a lot on what you really want to do, but if you're anything like
me, you want to understand what's really going on below the surface while also being able to iterate concepts fairly quickly.
Those goals may seem to be contradictory in nature, but I don't think they have to be. So provided here are many of the tools
I've developed over several years to approach that goal. My problem with so many other tools out there is that they simply use
too many lines of code to do relatively simple things. The tools here are all designed to reduce the lines of code necessary
to make feature-full programs. Whether that appeals to you or not, there it is.
#### Window System I/O
Window configuration, creation, and cleanup that includes a keyboard-layout-agnostic input model, ideal for games. Provides an
intermediate representation for key codes that translate the same way for every platform.
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
Using a std::cout-like structure to log information to both the console and a log file simultaneously.
#### Math
Provides several commonly useful functions as well as Vector, Matrix, Complex, and Quaternion data types.
Separately defines an arbitrary-precision integer data type.
#### Memory Primitives
Provides alternatives to the C++ STL vectors, lists and strings. Also provides a unique combination of the two, ideal for
sparse indices, such as for a Unicode mapping.
Includes a UTF-8 to Unicode converter.
#### Font Loading & On-The-Fly SDF Atlas Generator
Parses font files and very quickly packs atlases of Signed-Distance-Field glyphs without any external libraries.
Signed distance fields are generated by rigorous distance algorithms rather than searching through super-sampled bitmaps, allowing for lightning-quick rendering of glyphs with 100% accuracy.
Atlases can be added to on-the-fly, allowing for exhaustive support of font files to work for all languages without packing every glyph available at the beginning of the program.
Composite glyphs are deconstructed, and only their components are packed. This should allow better use of space at a minimal runtime cost.
## Platforms
- GNU/Linux
- Windows
## License
This work is licensed under the Creative Commons Attribution-ShareAlike 4.0 International License. To view a copy of this
license, visit http://creativecommons.org/licenses/by-sa/4.0/ or send a letter to Creative Commons, PO Box 1866, Mountain
View, CA 94042, USA.
