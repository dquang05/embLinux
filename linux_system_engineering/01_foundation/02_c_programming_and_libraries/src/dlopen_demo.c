/**
 * @file dlopen_demo.c
 * @brief Demonstrates dynamic loading of a shared library at run-time using dlopen API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h> // Required for dlopen API

/**
 * @brief Main execution function.
 * 
 * Dynamically loads libmath.so, resolves the math_add symbol, and executes it.
 * 
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int main(void) {
    void *handle;
    int (*add_func)(int, int);
    char *error;

    printf("Attempting to load libmath.so dynamically...\n");

    // 1. Open the shared library
    // RTLD_LAZY: resolve undefined symbols as code from the dynamic library is executed.
    handle = dlopen("./build/libmath.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    // Clear any existing error
    dlerror();

    // 2. Resolve the symbol (get the function pointer)
    // We must cast the void* returned by dlsym to the correct function pointer type.
    // In C, technically casting void* to function pointer is undefined behavior according to strict C standard,
    // but POSIX explicitly supports it via this workaround:
    *(void **) (&add_func) = dlsym(handle, "math_add");

    // Check for errors from dlsym
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "dlsym failed: %s\n", error);
        dlclose(handle);
        return EXIT_FAILURE;
    }

    // 3. Execute the function
    int a = 20;
    int b = 15;
    printf("Successfully loaded function. Executing math_add(%d, %d)...\n", a, b);
    printf("Result: %d\n", add_func(a, b));

    // 4. Close the library (Resource Management)
    if (dlclose(handle) != 0) {
        fprintf(stderr, "dlclose failed: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
