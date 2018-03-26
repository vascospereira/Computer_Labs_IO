#include <minix/syslib.h>
#include <minix/drivers.h>
#include <math.h>
//#include <limits.h>
//#include "timer.h"
#include "i8254.h"

static unsigned int interrupt_counter = 0;
static int hook_id = 0;

int timer_set_square(unsigned long timer, unsigned long freq) {

	unsigned long min_freq = (unsigned long) ceil(
			(double) TIMER_FREQ / USHRT_MAX);
	unsigned long max_freq = TIMER_FREQ / 2;

	if (freq < min_freq)
		return 1;
	else if (freq > max_freq)
		return 2;

	/**
	 * divisor can't be a value over (2^16?1)=65535 -> (0xFFFF)
	 *
	 * frequency (freq) must be at least 19. If lower, overflows 16 bit registers because is above 65535
	 */
	unsigned int divisor = TIMER_FREQ / freq; /* Calculates divisor which results in the given frequency */

	unsigned char control_word, timer_addr, config;

	if (timer == 0) {
		control_word = TIMER_SEL0;
		timer_addr = TIMER_0;
	} else if (timer == 1) {
		control_word = TIMER_SEL1;
		timer_addr = TIMER_1;
	} else if (timer == 2) {
		control_word = TIMER_SEL2;
		timer_addr = TIMER_2;
	} else
		return 3;

	control_word |= TIMER_LSB_MSB; // Register type selector
	if (timer_get_conf(timer, &config) != OK)
		return 3;

	//control_word |= (BIT(3) | BIT(2) | BIT(1) | BIT(0)) & config;

	control_word |= TIMER_SQR_WAVE; // Mode 3
	printf("CTRL_WORD is: 0x%02X\n", control_word);
	/**
	 * SYS_DEVIO kernel call for doing I/O
	 * Write to the control register before accessing any of the timers
	 */
	unsigned char lsb = (unsigned char) (divisor & BYTE_MASK); /* Least Significant Byte of divisor */
	unsigned char msb = (unsigned char) ((divisor >> 8) & BYTE_MASK); /* Most Significant Byte of divisor */
	if (sys_outb(TIMER_CTRL, control_word) == OK)
		if (sys_outb(timer_addr, lsb) == OK) /* Writes LSB to timer */
			if (sys_outb(timer_addr, msb) == OK)
				return 0;
			else
				return 4;
		else
			return 5;
	else
		return 6;
}

int timer_subscribe_int(void) {
	int num = hook_id;
	if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id) != OK)
		return -1;
	if (sys_irqenable(&hook_id) != OK)
		return -1;
	return BIT(num);
}

int timer_unsubscribe_int() {
	if (sys_irqdisable(&hook_id) != OK)
		return 1;
	if (sys_irqrmpolicy(&hook_id) != OK) /* Attempts to unsubscribe interrupt notifications on IRQ line 0 */
		return 1;
	return 0;
}

void timer_int_handler() {
	interrupt_counter++;
}

int timer_get_conf(unsigned long timer, unsigned char *st) {

	unsigned char rb_command = TIMER_RB_CMD;
	rb_command |= TIMER_RB_COUNT_;

	if (timer < 0 || timer > 2)
		return 1;
	rb_command |= TIMER_RB_SEL(timer);

	if (sys_outb(TIMER_CTRL, rb_command) != OK)
		return 2;
	unsigned char timer_addr;
	switch (timer) {
	case 0:
		timer_addr = TIMER_0;
		break;
	case 1:
		timer_addr = TIMER_1;
		break;
	case 2:
		timer_addr = TIMER_2;
		break;
	default:
		break;
	}

	unsigned long timer_reader;
	if (sys_inb(timer_addr, &timer_reader) != OK)
		return 3;
	*st = (unsigned char) timer_reader;
	return 0;
}

int timer_display_conf(unsigned char conf) {
	printf("CONFIGURATION: 0x%02X\n", conf);

	if (conf & TIMER_STATUS_OUTPUT_)
		printf("OUTPUT Line Is High\n");
	else
		printf("OUTPUT Line Is Low\n");

	unsigned char access_type = (conf & TIMER_ACCESS_) >> ACCESS_TYPE_OFFSET;

	switch (access_type) {
	case LSB:
		printf("ACCESS TYPE: LSB\n");
		break;
	case MSB:
		printf("ACCESS TYPE: MSB\n");
		break;
	case LSB_MSB:
		printf("ACCESS TYPE: LSB Followed by MSB\n");
		break;
	default:
		return 1;
	}

	unsigned char op_mode = (conf & TIMER_MODE_) >> OP_MODE_OFFSET;

	switch ((unsigned int) op_mode) {
	case 0:
		printf("OPERATING MODE 0: INTERRUPT ON TERMINAL COUNT\n");
		break;
	case 1:
		printf("OPERATING MODE 1: HARDWARE RETRIGGERABLE ONE-SHOT\n");
		break;
	case 2:
	case 6:
		printf("OPERATING MODE 2: RATE GENERATOR\n");
		break;
	case 3:
	case 7:
		printf("OPERATING MODE 3: SQUARE WAVE MODE\n");
		break;
	case 4:
		printf("OPERATING MODE 4: SOFTWARE TRIGGERED STROBE\n");
		break;
	case 5:
		printf("OPERATING MODE 5: HARDWARE TRIGGERED STROBE (RETRIGGERABLE)\n");
		break;
	default:
		break;
	}

	if (conf & TIMER_BCD_)
		printf("Binary-Coded Decimal (BCD) Counting Mode\n");
	else
		printf("BINARY Counting Mode\n");
	return 0;
}

int timer_test_square(unsigned long freq) {
	int code = timer_set_square(TIMER_ZERO_NUM, freq);
	switch (code) {
	case 1:
		printf("timer_set_square: Frequency Too Low\n");
		break;
	case 2:
		printf("timer_set_square: Frequency Too High\n");
		break;
	case 3:
		printf("timer_set_square: Invalid Timer Specifier\n");
		break;
	case 4:
		printf("timer_set_square: Failed to Write MSB\n");
		break;
	case 5:
		printf("timer_set_square: Failed to Write LSB\n");
		break;
	case 6:
		printf("timer_set_square: Failed to Write Control Word\n");
		break;
	default:
		return 0;
	}
	return 1;
}

int timer_test_int(unsigned long time) {

	if (time < 1) {
		printf("timer_test_int::Time Must Be Bigger Than 0.\n");
		return 1;
	}

	unsigned long irq_bit_mask;

	if ((irq_bit_mask = timer_subscribe_int()) == -1) {
		printf("timer_test_int::Failed To Subscribe The Timer.\n");
		return 2;
	}

	unsigned long seconds = 0;
	int error;
	int ipc_status;
	message msg;

	/* Interrupt loop to be executed while isn't reached the number of seconds in time. */
	while (seconds < time) {
		/*Receiving messages from the DD*/
		if ((error = driver_receive(ANY, &msg, &ipc_status)) != 0) {
			printf("timer_test_int::driver_receive() failed with: %d", error);
			continue;
		}

		if (is_ipc_notify(ipc_status)) { /* Tests if the message received is a notification from the kernel */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /*In the case that the notification is sent by the hardware*/
				if (msg.NOTIFY_ARG & irq_bit_mask) { /*Test if the interrupt received is the timer interrupt IRQ Line 0*/
					timer_int_handler();
					if (interrupt_counter >= STANDARD_FREQ) { /* 60 interrupts -> one second */
						seconds++;
						interrupt_counter = 0;
						printf("A Second Has Passed... Total Seconds: %d\n", seconds);
					}
				}
				break;
			default:
				break; /* No other notifications expected */
			}
		}
	}

	if (timer_unsubscribe_int() != 0) {
		printf("timer_test_int::Failed to disable and unsubscribe timer interrupt!\n");
		return 3;
	}
	return 0;
}

int timer_test_config(unsigned long timer) {
	unsigned char config;
	int code;
	if ((code = timer_get_conf(timer, &config)) == 0)
		if (timer_display_conf(config) != 0) {
			printf("timer_display_conf() failure: invalid type access\n");
			return 1;
		} else
			return 0;
	else if (code == 1)
		printf("invalid timer <0, 1 or 2>\n", code);
	else if (code == 2)
		printf("sys_outb() failure\n");
	else
		printf("sys_inb() failure\n");
	return 1;
}
