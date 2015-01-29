/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>
#include <tal.h>

void tal_rx_frame_cb(frame_info_t *rx_frame) {
	/**/
}

void tal_tx_frame_done_cb(retval_t status, frame_info_t *frame) {
	/**/
}

int main (void)
{
	board_init();

	while(true){
		tal_task();
		sleepmgr_enter_sleep();
	}
}
