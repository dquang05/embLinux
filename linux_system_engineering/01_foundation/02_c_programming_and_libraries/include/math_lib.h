/**
 * @file math_lib.h
 * @brief Simple math library to demonstrate static and shared libraries.
 */

#ifndef MATH_LIB_H
#define MATH_LIB_H

/**
 * @brief Adds two integers.
 * 
 * @param a First integer.
 * @param b Second integer.
 * @return The sum of a and b.
 */
int math_add(int a, int b);

/**
 * @brief Subtracts the second integer from the first.
 * 
 * @param a First integer.
 * @param b Second integer.
 * @return The difference (a - b).
 */
int math_sub(int a, int b);

#endif /* MATH_LIB_H */
