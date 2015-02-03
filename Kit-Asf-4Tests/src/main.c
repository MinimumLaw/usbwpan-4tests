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
#include <avr2025_mac.h>
#include <tal.h>
#include <string.h> /* memcpy */

COMPILER_WORD_ALIGNED
uint8_t	udc_to_prcm_buff[128];

COMPILER_WORD_ALIGNED
uint8_t prcm_to_udc_buff[256];

queue_t host_to_radio_queue, radio_to_host_queue;

/*
 * TAL (RF) callbacks
 */
void tal_rx_frame_cb(frame_info_t *rx_frame) {
	/* simple move buffer from RF recive to USB transmitt queue */
	qmm_queue_append(&radio_to_host_queue, rx_frame->buffer_header);
	/* then restart rx procedure */
	tal_rx_enable(PHY_RX_ON);
}

void tal_tx_frame_done_cb(retval_t status, frame_info_t *frame) {
	/**/
}

/*
 * USB callbacks
 */
void udc_data_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep) {
	buffer_t *buff;

	if(status == UDD_EP_TRANSFER_OK) {
		buff = bmm_buffer_alloc(nb_transfered);
		if(buff == NULL) {
			/* No buffer allocated (No memory???) - simple drop
				received data and abort later receive */
			udd_ep_abort(ep);
			return;
		};
		memcpy(buff->body, udc_to_prcm_buff, nb_transfered);
		qmm_queue_append(&host_to_radio_queue, buff);
		/* restart transfer */
		udd_ep_run(UDI_VENDOR_EP_BULK_IN, false, udc_to_prcm_buff,
			128, udc_data_received);

	} else {
		/* Transfer aborted , try to restart transfer */
		udd_ep_run(UDI_VENDOR_EP_BULK_IN, false, udc_to_prcm_buff,
			128, udc_data_received);
	}
}

void udc_data_transmitted(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep) {
	/* set NACK on endpoint, until next transfer */
	udd_ep_abort(ep);
}

void udc_suspend(void) {
	/**/
}

void udc_resume(void) {
	/**/
}

void udc_remote_wakeup(bool state) {
	/**/
}

bool udc_enable_dev(void) {
	/**/
	udd_ep_run(UDI_VENDOR_EP_BULK_OUT, false, udc_to_prcm_buff,
		128, udc_data_received);
/*
	udd_ep_run(UDI_VENDOR_EP_BULK_IN, false, prcm_to_udc_buff,
		2568, udc_data_transmitted);
*/
	tal_reset(true);
	return true;
}

void udc_disable_dev(void) {
	/**/
}

enum { /* Setup requests wValue for device configurations */
	REQ_WPAN_START,
	REQ_WPAN_STOP,
	REQ_WPAN_SET_CHANNEL,
	REQ_WPAN_SET_HWADDR_FILT,
	REQ_WAPN_SET_HWADDR,
	REQ_WPAN_SET_TXPOWER,
	REQ_WAPN_SET_LBT,
	REQ_WPAN_SET_CCA_MODE,
	REQ_WPAN_SET_CCA_ED_LEVEL,
	REQ_WPAN_SET_CSMA_PARAMS,
	REQ_WPAN_SET_FRAME_RETRIES,
	REQ_WPAN_GET_ED,
	REQ_WAPN_GET_FEATURES,
	REQ_WPAN_GET_CHANNEL_LIST,
};

bool udc_prcm_setup_req(void) {
	/**/
	bool ret = false;
	switch(udd_g_ctrlreq.req.wValue) {
		case REQ_WPAN_START:
		case REQ_WPAN_STOP:
		case REQ_WPAN_SET_CHANNEL:
		case REQ_WPAN_SET_HWADDR_FILT:
		case REQ_WAPN_SET_HWADDR:
		case REQ_WPAN_SET_TXPOWER:
		case REQ_WAPN_SET_LBT:
		case REQ_WPAN_SET_CCA_MODE:
		case REQ_WPAN_SET_CCA_ED_LEVEL:
		case REQ_WPAN_SET_CSMA_PARAMS:
		case REQ_WPAN_SET_FRAME_RETRIES:
			ret = true;
			break;
	}
	return ret;
}

bool udc_prcm_setup_ack(void) {
	/**/
	bool ret = false;
	switch (udd_g_ctrlreq.req.wValue) {
		case REQ_WPAN_GET_ED:
		case REQ_WAPN_GET_FEATURES:
		case REQ_WPAN_GET_CHANNEL_LIST:
			ret = true;
			break;
	}
	return ret;
}

/*
 * Personal Radio Communication Module task
 */
void prcm_task_init(void) {
	qmm_queue_init(&radio_to_host_queue);
	qmm_queue_init(&host_to_radio_queue);
}

void prcm_task(void) {
	/**/
	buffer_t *to_host, *to_radio;

	to_host = qmm_queue_remove(&radio_to_host_queue, NULL);
	if(to_host != NULL) {
		/* send frame via USB */
		udd_ep_run(UDI_VENDOR_EP_BULK_IN, false, to_host->body,
			to_host->body[0] + 3, udc_data_transmitted);
		bmm_buffer_free(to_host);
	};

	to_radio = qmm_queue_remove(&host_to_radio_queue, NULL);
	if(to_radio != NULL) {
		/* send frame via radio */
		tal_tx_frame((struct frame_info_t *)to_radio->body, 0,false); 
			/*dev_cfg.csma_mode, dev_cfg.max_frame_retries > 0 ? true : false); */
		bmm_buffer_free(to_radio);
	};
}

int main (void)
{
	board_init();

	while(true){
		pal_task();
		tal_task();
		prcm_task();
		sleepmgr_enter_sleep();
	}
}
