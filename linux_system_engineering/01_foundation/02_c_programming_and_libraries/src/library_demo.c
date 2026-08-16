/**
 * @file library_demo.c
 * @brief Demonstrates using the math library. 
 * Can be compiled as a statically or dynamically linked executable.
 */

#include <stdio.h>
#include <stdlib.h>
#include "math_lib.h"

/**
 * @brief Main execution function.
 * 
 * @return EXIT_SUCCESS.
 */
int main(void) {
    int a = 10;
    int b = 5;

    printf("Running Library Demo...\n");
    printf("%d + %d = %d\n", a, b, math_add(a, b));
    printf("%d - %d = %d\n", a, b, math_sub(a, b));

    return EXIT_SUCCESS;
}
