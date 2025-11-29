# Development on obZcene

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

There are more to come (hopefully) if I am not too busy.

> [!NOTE]
> 
> Refer to `API.md` to see how the API is structured.
> 
