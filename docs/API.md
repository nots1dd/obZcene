# API docs for obZcene

Couple of things:

1. All APIs are under the naming convention:

    ```c 
    obz_api_call(...);
    ```

    If you go through the `include/obZcene` directory you will come across even more blatant cases of API decl:

    ```c 
    // in A.h let's say:

    OBZ_API void obz_api_call(...);
    ```

    Note that all the `core` functions that the APIs use, the naming convention for those are:

    ```c 
    int __OBZ_func_name(...);
    ```

    Some macros to take note of that classify the function type in obZcene:

    ```h
    // in a header file...
    OBZ_API int obz_api_call(...);
    ```

    The above is a regular api decl of an obzcene function. It is meant to be used as a public API.

    ```h
    // in a header file...
    OBZ_API_INLINE float obz_api_call(...) { ... }
    ```
    
    The above is an **INLINED** API call of an obZcene function. It is meant to be used as an inlined public API.

    Note that the different between `OBZ_API` and `OBZ_API_INLINE` is just that `OBZ_API` is split into header and C file (decl goes in header and implementation goes in corresponding C file)

    Meanwhile `OBZ_API_INLINE` is completely inlined and resides solely in the header.

    ```c 
    // in a c source file...
    OBZ_INTERNAL double __OBZ_func_name(...) { ... }

    // it could also be a prototype decl in a header:

    // in A.h 
    OBZ_INTERNAL double __OBZ_func_name(...);

    // then in A.c 
    double __OBZ_func_name(...) { ... }
    ```

    This is a macro to NOT inline the function (makes it easy to search the repo for internal functions) and explicitly states that this is a **private** and **internal** function call that is NOT meant to be used directly by the user.

    ```h 
    // in a header file...
    OBZ_INTERNAL_INLINE uint8_t __OBZ_f_name(...) { ... }
    ```

    `OBZ_INTERNAL_INLINE` is just `OBZ_FORCE_INLINE` but the name gives some clarity regarding which functions are part of the API and which are just helpers.

    The `__OBZ_` prefix also helps to identify internal helpers for the engine.


2. APIs by definition have very stable return types.

    So this means that the onus is on the **user** who has to assert and check if the API is returning an expected value. The API will not perform too many static assertions or any bound checking for you.

    Note that in the future, I will be adding a better API model to better check the functionality of the source code (currently since so many api calls are under `void` it is hard to diagnose any minor problems, you will have to rely on sheer logging)

3. Always use `obz_macros.h` and `obz_log.h`

    These are core utils of the obZcene that are meant to be **very reliable and stable** (minimal changes will occur to these headers).

4. This doc is super primitive!
    
    I will be adding basic doxygen docs **SOON** (idk how soon; hopefully after the first beta release?)
