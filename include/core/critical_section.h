#ifndef INC_CORE_CRITICAL_SECTION_H_
#define INC_CORE_CRITICAL_SECTION_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Determine the current interrupts enabled state
 *
 * This function can be called to determine whether or not interrupts are currently enabled.
 * 
 * @note
 * NOTE:
 * This function works for both cortex-A and cortex-M, although the underlying implementation
 * differs.
 * 
 * @return true if interrupts are enabled, false otherwise
 */
bool are_interrupts_enabled(void);

/** 
 * Determine if this code is executing from an interrupt
 *
 * This function can be called to determine if the code is running on interrupt context.
 * 
 * @note
 * NOTE:
 * This function works for both cortex-A and cortex-M, although the underlying implementation
 * differs.
 * 
 * @return true if in an isr, false otherwise
 */
bool is_isr_active(void);

/** 
 * Mark the start of a critical section
 *
 * This function should be called to mark the start of a critical section of code.
 * 
 * @note
 * NOTES:
 * 1) The use of this style of critical section is targetted at C based implementations.
 * 2) These critical sections can be nested.
 * 3) The interrupt enable state on entry to the first critical section (of a nested set, or single
 *    section) will be preserved on exit from the section.
 * 4) This implementation will currently only work on code running in privileged mode.
 */
void critical_section_enter(void);

/** 
 * Mark the end of a critical section
 *
 * This function should be called to mark the end of a critical section of code.
 * 
 * @note
 * NOTES:
 * 1) The use of this style of critical section is targetted at C based implementations.
 * 2) These critical sections can be nested.
 * 3) The interrupt enable state on entry to the first critical section (of a nested set, or single
 *    section) will be preserved on exit from the section.
 * 4) This implementation will currently only work on code running in privileged mode.
 */
void critical_section_exit(void);

/**
 * Determine if we are currently in a critical section
 *
 * @return true if in a critical section, false otherwise.
 */
bool in_critical_section(void);

#ifdef __cplusplus
}
#endif

#endif /* !INC_CORE_CRITICAL_SECTION_H_ */