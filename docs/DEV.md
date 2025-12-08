# Development on obZcene

Firstly, do note that the core architecture and flow of obZcene is yet to documented. The project itself is not big so feel free to understand on your own and document it.

> [!IMPORTANT]
> 
> The build system is currently just a big Makefile.
> 
> If need arises, I will switch over to `CMake` in the future
> to not complicate things.
> 

## Getting clangd to work

It is **HIGHLY** recommended you have a LSP (preferably clangd) to work on obZcene.

> [!NOTE]
> 
> To get clangd working, you will need to install `bear`:
> 
> ```bash 
> # on Arch (pacman)
> sudo pacman -S bear
> # then use it like so:
> cd obZcene
> bear -- make rebuild
> mv compile_commands.json build/
> ```
> 
> After that, reload your LSP it *should* work.
> 

Not sure on other distros, but you check out [bear](https://github.com/rizsotto/Bear) here for more details.

There arent too many rules just yet:

1. FORMAT THE CODE BEFORE COMMITING:

    Ensure you have `clang-format` installed and run:
    ```bash 
    make format
    ```

    I will also be adding that as a pre-commit hook but you will have to install `pre-commit` from pip.

2. Follow `.gitmessage` format of commit messages

    Please dont give useless commit messages that are not helpful or just plain wrong.

3. Naming convention of files/dirs:

    **Directories**: Capitalized name (except `obZcene`) [Ex: SDL/Math/Test/Dir/]
    **Files**: Lower case file name (add `_` between words if needed.) [Ex: file_name_is_long.c]

4. Header has prototype declarations and structs; C file has the implementation

5. Use header guards instead of `#pragma once`

6. Any general utility functions or containers (like dynamic array, 2d/3d vectors, matrices, etc.) go under `obZcene/Utils`.
    
    A question arises here then: Why does `obZcene/Math` even exist then?
    
    Simple answer: To make existing mathematical operations faster via **instrinsics** or **raw assembly**.
    What it is not: A directory to dump all mathematical data structures like `Vec` or `Mat`!

    Only after those operations are tested and benchmarked, they can be used inside `Utils/vec.h` for example.

7. Sensible commits
    
    Do not add too much content in one commit itself. It is cumbersome to read and understand all the changes made.
    Separate them into different logically equivalent chunks for better clarity

8. When to use `auto`

    Yes I have a typedef `auto` which just resolves to `__auto_type` for `gcc` and `clang` for compile time deduction of
    types (similar to cpp). But this is **NOT** meant to be spammed everywhere for convenience.

    Here is an example of using `auto` properly:

    ```c 
    auto x    = (OBZ_Channel)(i + j * k);               // When explicit casts are made [X]

    auto xptr = (float*)fun_which_has_voidptr(data, i); // When we are inferring a type from void pointer

    auto y    = some_func(data, double);                // When the return or conversion type is mentioned in the function / macro
    ```

    These are the only cases where `auto` is acceptable.

    Why did I make this choice? Idk thought why not.

9. Global variables

    It should have `g_` as its prefix with either static linkage or can extern it.
    
    ```c 
    static const int ok = 99;   // !!NOT ALLOWED!!

    static const int g_ok = 99; // ALLOWED
    ```

    For externing a variable, declare it in a header and call that in **ONE** `.c` file ONLY.

    ```c 
    // in A.h:
    extern float g_hello;

    // in some.c:
    int g_hello = 99;
    ```

10. Use `Utils` and obZcene helpers

    obZcene provides `obz_types.h`, `obz_log.h`, `obz_mem.h`, `obz_macros.h` and `include/obZcene/Utils` which provide a plethra of things that the core logic of the game engine uses extensively.

    For example, no actual `malloc` or `calloc` is meant to be made. You are expected to use `obz_malloc` and `obz_calloc` respectively.

    The `Utils` directory provides a dynamic array that is dtype irrelevant (works for any data type / structs), small arena allocator, null utilities and custom string operations (mainly required for parsing obj files)

There are more to come (hopefully) if I am not too busy.

> [!NOTE]
> 
> Refer to `API.md` to see how the API is structured.
> 
