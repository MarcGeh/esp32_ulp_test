/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files
 */
#include "soc/rtc_cntl_reg.h"
#include "soc/soc_ulp.h"

	/* ADC1 channel 6, GPIO34 */
	.set adc_channel, 6

	/* Configure the number of ADC samples to average on each measurement.
	For convenience, make it a power of 2. */
	.set adc_oversampling_factor_log, 2
	.set adc_oversampling_factor, (1 << adc_oversampling_factor_log)

	/* Define variables, which go into .bss section (zero-initialized data) */
	.bss

	/* The amount of ULP program starts until the main cpu is woken up  */
	.global wake_up_count
wake_up_count:
	.long 0

	/* ULP start counter. Counts the amounts of time the ULP program has been started */
	.global counter
counter:
	.long 0

	/* Last ADC measurement result */
	.global last_result
last_result:
	.long 0

	/* Code goes into .text section */
	.text
	.global entry
entry:
	/* increment counter */
	move r3, counter
	ld r2, r3, 0
	add r2, r2, 1
	st r2, r3, 0

	/* decide if the main CPU should be woken up */
    move r3, wake_up_count
    ld r3, r3, 0
    sub r3, r3, r2
    jump adc_start, ov

	/* value within range, end the program */
	.global exit
exit:
	halt


adc_start:
	/* do measurements using ADC */
	/* r0 will be used as accumulator */
	move r0, 0
	/* initialize the loop counter */
	stage_rst
measure:
	/* measure and add value to accumulator */
	adc r1, 0, adc_channel + 1
	add r0, r0, r1
	/* increment loop counter and check exit condition */
	stage_inc 1
	jumps measure, adc_oversampling_factor, lt

	/* divide accumulator by adc_oversampling_factor.
	   Since it is chosen as a power of two, use right shift */
	rsh r0, r0, adc_oversampling_factor_log
	/* averaged value is now in r0; store it into last_result */
	move r3, last_result
	st r0, r3, 0

	.global wake_up
wake_up:
	/* Check if the system can be woken up */
	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	and r0, r0, 1
	jump exit, eq

	/* Wake up the SoC, end program */
	wake
	WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
	halt
