#include <asf.h>
#include <avr2025_mac.h>
#include <tal.h>
#include <string.h> /* memcpy */

#include "wpan.h"

uint32_t		channel_page_support[WPAN_NUM_PAGES];
wpan_dev_cfg	dev_cfg;

/* START and STOP use dummy write */
static struct wpan_dummy_write dummy_write;
void start_callback(void)
{
	if(RX_ON == tal_rx_enable(PHY_TX_ON))
		dev_cfg.wpan_active = true;
}

void stop_callback(void)
{
	if(TRX_OFF == tal_rx_enable(PHY_TRX_OFF))
		dev_cfg.wpan_active = false;
}

/* set channel */
static struct wpan_channel_data channel_data;
void set_channel_callback(void)
{
	if(MAC_SUCCESS == tal_pib_set(phyCurrentPage,(pib_value_t *)&channel_data.page)) {
		dev_cfg.page = channel_data.page;	
	};
	
	if(MAC_SUCCESS == tal_pib_set(phyCurrentChannel,(pib_value_t*)&channel_data.channel)) {
		dev_cfg.channel = channel_data.channel;
	};
}

static struct wpan_hw_addr_filt hw_filter_recv;
void hw_filter_recv_callback(void)
{
	if(hw_filter_recv.changed & IEEE802515_AFILT_IEEEADDR_CHANGED) {
		if(MAC_SUCCESS != tal_pib_set(macIeeeAddress,(pib_value_t*)&hw_filter_recv.ieee_addr)) {
			dev_cfg.ieee_addr = hw_filter_recv.ieee_addr;	
		};
	};
	
	if(hw_filter_recv.changed & IEEE802515_AFILT_SADDR_CHANGED) {
		if(MAC_SUCCESS != tal_pib_set(macShortAddress,(pib_value_t*)&hw_filter_recv.short_addr)) {
			dev_cfg.short_addr = hw_filter_recv.short_addr;	
		};
	};
	
	if(hw_filter_recv.changed & IEEE802515_AFILT_PANID_CHANGED) {
		if(MAC_SUCCESS != tal_pib_set(macPANId,(pib_value_t*)&hw_filter_recv.pan_id)) {
			dev_cfg.pan_id = hw_filter_recv.pan_id;
		};
	};
	
	if(hw_filter_recv.changed & IEEE802515_AFILT_PANC_CHANGED) {
		if(MAC_SUCCESS != tal_pib_set(mac_i_pan_coordinator,(pib_value_t*)&hw_filter_recv.pan_coordinator)) {
			dev_cfg.pan_coordinator = hw_filter_recv.pan_coordinator;
		};
	};
	
	/* clear all known flags */
	hw_filter_recv.changed &= ~(IEEE802515_AFILT_IEEEADDR_CHANGED |
		IEEE802515_AFILT_SADDR_CHANGED |
		IEEE802515_AFILT_PANID_CHANGED |
		IEEE802515_AFILT_PANC_CHANGED);
}

/* hw_addr */
uint64_t set_hwaddr;

void set_hwaddr_callback(void)
{
	if(MAC_SUCCESS == tal_pib_set(macIeeeAddress, (pib_value_t*)&set_hwaddr)) {
		dev_cfg.ieee_addr = set_hwaddr;
		/* ToDo: write to EEPROM */
	};
}

/* tx_power */
static struct wpan_tx_power tx_power;
void tx_power_callback(void)
{
	if(MAC_SUCCESS == tal_pib_set(phyTransmitPower, (pib_value_t*)&tx_power.tx_power)) {
		dev_cfg.tx_power = tx_power.tx_power;
	};
}

/* listen before talk */
static struct wpan_lbt new_lbt;
void lbt_callback(void)
{
	trx_bit_write(SR_CSMA_LBT_MODE, new_lbt.mode ? 1 : 0);
	dev_cfg.lbt_mode = new_lbt.mode;
}

/* cca mode */
static struct wpan_cca new_cca;
void cca_calback(void)
{
	if(MAC_SUCCESS == tal_pib_set(phyCCAMode, (pib_value_t*)&new_cca.mode)) {
		dev_cfg.cca_mode = new_cca.mode;
	};
}

/* cca ed level (threshold) */
static struct wpan_cca_threshold ed_level;
void ed_level_callback(void)
{
	trx_bit_write(SR_CCA_ED_THRES,ed_level.level);
	dev_cfg.cca_ed_level = ed_level.level;
}

/* csma */
static struct wpan_csma_params csma_param;
void csma_param_callback(void)
{
	if (MAC_SUCCESS == tal_pib_set(macMinBE,(pib_value_t *)&csma_param.min_be)) {
		dev_cfg.csma_min_be = csma_param.min_be;
	};

	if (MAC_SUCCESS == tal_pib_set(macMaxBE,(pib_value_t *)&csma_param.max_be)) {
		dev_cfg.csma_max_be = csma_param.max_be;
	};
	
	if (MAC_SUCCESS == tal_pib_set(macMaxFrameRetries,(pib_value_t *)&csma_param.retries)) {
		dev_cfg.max_frame_retries = csma_param.retries;
	};
}

/* frame retries */
static struct wpan_frame_retries frame_retries;
void frame_retries_callback(void)
{
	if(MAC_SUCCESS == tal_pib_set(macMaxFrameRetries,(pib_value_t*)&frame_retries.count)) {
		dev_cfg.max_frame_retries = frame_retries.count;
	};
}

#define SETUP_REQ_CALLBACK(structure, function) \
{ \
	udd_g_ctrlreq.payload = &structure; \
	udd_g_ctrlreq.payload_size = udd_g_ctrlreq.req.wLength; \
	udd_g_ctrlreq.callback = &function; \
	ret = true; \
}

bool prepare_setup_request(uint16_t req_type)
{
	bool ret = true;
	switch(req_type) {
		case REQ_WPAN_START:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_dummy_write)) {
				SETUP_REQ_CALLBACK(dummy_write, start_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_STOP:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_dummy_write)) {
				SETUP_REQ_CALLBACK(dummy_write, stop_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_CHANNEL:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_channel_data)) {
				SETUP_REQ_CALLBACK(channel_data, set_channel_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_HWADDR_FILT:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_hw_addr_filt)) {
				SETUP_REQ_CALLBACK(hw_filter_recv, hw_filter_recv_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_HWADDR:
			if(udd_g_ctrlreq.req.wLength == sizeof(uint64_t)) {
				SETUP_REQ_CALLBACK(set_hwaddr, set_hwaddr_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_TXPOWER:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_tx_power)) {
				SETUP_REQ_CALLBACK(tx_power, tx_power_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_LBT:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_lbt)) {
				SETUP_REQ_CALLBACK(new_lbt, lbt_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_CCA_MODE:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_cca)) {
				SETUP_REQ_CALLBACK(new_cca, cca_calback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_CCA_ED_LEVEL:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_cca_threshold)) {
				SETUP_REQ_CALLBACK(ed_level, ed_level_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_CSMA_PARAMS:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_csma_params)) {
				SETUP_REQ_CALLBACK(csma_param, csma_param_callback);
			} else
				ret = false;
			break;
		case REQ_WPAN_SET_FRAME_RETRIES:
			if(udd_g_ctrlreq.req.wLength == sizeof(struct wpan_frame_retries)) {
				SETUP_REQ_CALLBACK(frame_retries, frame_retries_callback);
			} else
				ret = false;
			break;
		default:
			ret = false;
	};
	return ret;
}
