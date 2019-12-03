# Contributing Guidelines
First of all, thanks for contributing!

These are the guidelines which contributors are meant to follow within reason. There may be exceptions to these rules, so use discretion.

## File Structure
C++ headers are all named with the `.hpp` extension to avoid any ambiguity with C headers. Any `.hpp` or `.cpp` that would look and behave the same way in both C and C++ may use the C extensions `.h` and `.c`.

One thing to consider when creating new files is that this project prefers to keep the number of object files low. This means that new `.cpp` files should be included into one of the main `.cpp` files from which object files are already being generated.
If a new feature is large-enough to merit a separate object file, one may be added, but must be reflected in the makefiles.
Also, any new `.hpp` or `.cpp` file that doesn't directly generate an object file must be added to the `AzCore/base/makefile _DEPS_*SOMETHING*` dependencies, as well as the `AzCore/projects/base_project_makefile _AZCORE_DEPS_*SOMETHING*` dependencies.
Any new `.cpp` that generates an object file must be reflected in the `AzCore/base/makefile _OBJ` variable, and `AzCore/projects/*project_name*/makefile _AZCORE_OBJ` variable if that project uses that functionality.

This is kind of a hassle, so I may be looking into better build systems or just better handling of makefiles, but for now this is how it has to be done.

## Projects
The best place to test new features is by implementing them in some of the projects. As such, features that have no associated project code will be less likely to be accepted into the codebase. In the very least, a unit test must be provided, but a more practical application of that feature is preferable. Sure, you're more likely to write buggy code by implementing it for a specific project rather than unit_tests, but you're also more likely to write code that's easier to use.

These projects tend to call
```C++
using namespace AzCore;
```
But keep in mind that the end-user might not want to do this.

## C Preprocessor Defines
`ALL_CAPS_WITH_WORDS_SEPARATED_BY_UNDERSCORES`
### Header Include guards
```C++
#ifndef AZCORE_FILE_NAME_HPP
#define AZCORE_FILE_NAME_HPP

namespace AzCore {

namespace SomeNamespace {

    // Define stuff here

} // namespace SomeNamespace

} // namespace AzCore

#endif // AZCORE_FILE_NAME_HPP
```
Note the comments on the `#endif` and closing brackets of namespaces.
### Macros
Avoid the use of macros in most cases, or just `#undef` them after you're done using them. For macros that you want to make available to the end-user, prepend the name with `AZCORE_` and implement it from a global scope, i.e. using the `AzCore` namespace explicitly.
```C++
#define AZCORE_FOO(expression) AzCore::Bar(expression)
```

## Namespaces
Generally, it's best to keep namespaces shallow. For most things, the AzCore namespace will be all you need. If you want to hide some details behind another namespace, you can do that as well, so long as the end-user won't need to use that namespace.

For namespaces that the end-user will use frequently, a short name with no capitals is preferred. It's advised to avoid situations where the end-user might have to reach into more than 2 layers of namespaces, including `AzCore`.

Examples of existing namespaces are:
- `AzCore::io`, which refers to human interface primitives, such as Window, RawInput, LogStream, etc.
- `AzCore::vk`, which refers to everything in the Vulkan Framework.
- `AzCore::font`, which refers to font file loading and atlas generation.

## Classes/Structs
Most classes and structs will use `UpperCamelCaseNaming`, with the exception of some that are relatively primitive (i.e. they are trivially-copyable and/or only implement operators). For example, `vec2`, an alias of `vec2_t<f32>`, would be an exception.

Data members of those classes and structs will be named with `lowerCamelCaseNaming`, and methods with `UpperCamelCaseNaming`.

AzCore tends to use `struct` for everything with few exceptions. The reason for this is that I feel that data privacy is more of an anti-feature than anything and hurts productivity significantly for approximately no gain.

For structs that have data that isn't meant to be accessed directly by most code, putting that data into an anonymous struct named "data" is my solution. This way the data can be accessed when needed without any of that `friend` nonsense. An example of this:
```C++
struct SomeThing {
    struct {
        AzCore::Array<i32> sensitiveMember;
    } data;

    i32 insensitiveMember;

    bool MethodToInteractWithSensitiveStuff();
};
```
## Functions
Simple functions (generally without side-effects) are usually named with `lowerCamelCaseNaming`. Examples include many of the math functions.

More complicated functions, or ones that DO have side-effects are usually named with `UpperCamelCaseNaming`.

