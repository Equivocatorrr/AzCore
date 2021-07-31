# AzCore
A collection of tools written by *Philip Haynes* designed to lower the barrier of entry to cross-platform game development
using C++. Most of these tools were created to fill a particular need in the designing of a game from the ground up. The
design of the interfaces were made with cross-platform development in mind, so use of these tools is uniform between the
platforms it currently supports.

### Note
This toolset is still deep in development. While it may be possible to do many things using it in its current state, many
other features are still on the TODO list. Also, these tools aren't necessarily designed to be used completely on their own,
but rather to provide a pretty good starting point for writing your own feature-full programs.

## Goals
To provide flexible tools and frameworks that help in writing cross-platform applications in an elegant way. This is, of course,
subjective, and *Philip's* definition involves a code style that tries to approach brevity and precision at the same time.
It's a style that *Philip* has developed over time for himself which he believes suits high- and low-level programming
equally, and which makes more guarantees than the C++ standard does.

## Methodology
The guiding principles behind development of this toolset are cyclical and non-feature-exhaustive.

In short, the desired features are determined by work on the various [projects](https://github.com/SingularityAzure/AzCore/tree/master/projects), and implemented alongside those projects.

Features are implemented on an as-needed basis such that any feature has a known use-case and a way to test it. This prevents
the developer from writing code they think *might* be useful, and instead focus on code that is definitely useful.
This also helps the developer in making logical and helpful decisions to reduce pain points in the end-user code, because the
developer is also writing end-user code at the same time.

This, of course, also has downsides, namely the need to occasionally touch up the AzCore/base while writing end-user code.
For some projects this is not desirable, and so these tools are still too early in development for such projects.

## Contributing
**_Be sure to read the [Contribution Guidelines](https://github.com/SingularityAzure/AzCore/blob/master/CONTRIBUTING.md)_**

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

`logger.Print("This ", integer, " is ", float, " less ", string, " cumbersome!");`

*DEPRECATED*: io::LogStream is a std::cout-like structure that logs information to both the console and a log file simultaneously.
#### Math
- Vector, Matrix, Complex, and Quaternion data types.
- BigInt, a fast, 128-byte integer data type that uses up to 15 u64's to represent values approximately spanning 10^289 to 10^289.
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
- `BucketArray<typename T, i32 count>`, a stack-allocated constant-storage-sized array with a variable effective size.
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
