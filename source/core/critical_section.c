#include "core/assert.h"
#include "core/critical_section.h"

#include "cmsis_gcc.h"

static uint32_t critical_section_reentrancy_counter = 0;
static bool     critical_interrupts_enabled = false;
static bool     state_saved = false;

bool are_interrupts_enabled(void)
{
    return ((__get_PRIMASK() & 0x1) == 0);
}

bool is_isr_active(void)
{
    return (__get_IPSR() != 0U);
}

bool in_critical_section(void)
{
    return (state_saved == true);
}

void critical_section_enter(void)
{
	const bool interrupt_state = are_interrupts_enabled();

	__disable_irq();

	if (state_saved == false) {
		critical_interrupts_enabled = interrupt_state;
		state_saved = true;
	}

    // If the reentrancy counter overflows something has gone badly wrong.
    ASSERT_NO_MSG(critical_section_reentrancy_counter < UINT32_MAX);

    ++critical_section_reentrancy_counter;
}

void critical_section_exit(void)
{
    // If critical_section_enter has not previously been called, do nothing
    if (critical_section_reentrancy_counter == 0) {
        return;
    }

    --critical_section_reentrancy_counter;

    if (critical_section_reentrancy_counter == 0) {
    	// Interrupts must be disabled on invoking an exit from a critical section
    	ASSERT_NO_MSG(!are_interrupts_enabled());
		state_saved = false;

		// Restore the IRQs to their state prior to entering the critical section
		if (critical_interrupts_enabled == true) {
			__enable_irq();
		}
    }
}
