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

2. APIs by definition have very stable return types.

    So this means that the onus is on the **user** who has to assert and check if the API is returning an expected value. The API will not perform too many static assertions or any bound checking for you.

    Note that in the future, I will be adding a better API model to better check the functionality of the source code (currently since so many api calls are under `void` it is hard to diagnose any minor problems, you will have to rely on sheer logging)

3. Always use `obz_macros.h` and `obz_log.h`

    These are core utils of the obZcene that are meant to be **very reliable and stable** (minimal changes will occur to these headers).

4. This doc is super primitive!
    
    I will be adding basic doxygen docs **SOON** (idk how soon; hopefully after the first beta release?)
