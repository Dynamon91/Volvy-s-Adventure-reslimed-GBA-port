#ifndef VOLVY_TEST_ARM_STRING_H
#define VOLVY_TEST_ARM_STRING_H
/* Freestanding ARM target code-generation check only.
 * Never use this test declaration shim in the actual devkitARM build. */
#include <stddef.h>
void *memset(void *destination, int value, size_t bytes);
void *memcpy(void *destination, const void *source, size_t bytes);
#endif
