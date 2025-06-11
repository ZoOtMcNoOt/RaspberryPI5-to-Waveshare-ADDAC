/**
 * @file Debug.h
 * @brief Provides a conditional debug printing macro.
 *
 * This header defines a macro `Debug(...)` that prints debug messages
 * to stdout only when the `DEBUG` preprocessor flag is defined and non-zero.
 * If `DEBUG` is not defined or is zero, the `Debug(...)` macro compiles to nothing,
 * incurring no runtime overhead.
 */

#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>

#if DEBUG
    /**
     * @brief Prints a formatted debug message if DEBUG is enabled.
     * @param __info Format string for the debug message.
     * @param ... Variable arguments for the format string.
     *
     * Example usage: Debug("Value is %d\n", myValue);
     */
	#define Debug(__info,...) printf("Debug: " __info,##__VA_ARGS__)
#else
    /**
     * @brief A no-operation macro if DEBUG is disabled.
     */
	#define Debug(__info,...)
#endif

#endif // __DEBUG_H