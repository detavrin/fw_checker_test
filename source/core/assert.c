#include <stdio.h>
#include <stdarg.h>

#include "core/assert.h"

/**
 * @brief Assert Action Handler
 *
 * This routine implements the action to be taken when an assertion fails.
 *
 * System designers may wish to substitute this implementation to take other
 * actions, such as logging program counter, line number, debug information
 * to a persistent repository and/or rebooting the system.
 */
__weak void assert_post_action(const char *file, unsigned int line)
{
	(void)(file);
	(void)(line);

	while(true);
}

void assert_print(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	vprintf(fmt, ap);

	va_end(ap);
}
