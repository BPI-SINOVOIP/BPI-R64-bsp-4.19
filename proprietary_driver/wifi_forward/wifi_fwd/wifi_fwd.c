/****************************************************************************
 * Mediatek Inc.
 * 5F., No.5, Taiyuan 1st St., Zhubei City,
 * Hsinchu County 302, Taiwan, R.O.C.
 * (c) Copyright 2014, Mediatek, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
    wifi_fwd.c

    Abstract:

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
     Annie Lu  2014-06-30	  Initial version
 */

#include "wifi_fwd.h"

struct net_device *ap_2g, *apcli_2g, *ap_5g, *apcli_5g, *br0;
struct net_device *ap1_2g, *apcli1_2g, *ap1_5g, *apcli1_5g;
struct FwdPair *WiFiFwdBase;
struct DevInfo *FwdDevice;
struct PacketSource *pkt_src;
struct TxSourceEntry *tx_src_tbl;
unsigned int tx_src_count;	/* count the value of tx_src_tbl */

/* currently supports up to 3 */
void *adapter_tmp_1;
void *adapter_tmp_2;
void *adapter_tmp_3;
struct wifi_fwd_func_table wf_fp_tbl;
unsigned char wf_debug_level;
static unsigned long wifi_fwd_op_flag;
static unsigned char rep_net_dev;

static unsigned char next_2g_band;
static unsigned char fwd_counter;
static signed char main_5g_h_link_cnt;
static signed char main_5g_link_cnt;
static signed char main_2g_link_cnt;

static unsigned int eth_rep5g_h_wrg_uni_cnt;
static unsigned int eth_rep5g_h_wrg_bct_cnt;
static unsigned int eth_rep5g_wrg_uni_cnt;
static unsigned int eth_rep5g_wrg_bct_cnt;
static unsigned int eth_rep2g_wrg_uni_cnt;
static unsigned int eth_rep2g_wrg_bct_cnt;

static unsigned int band_cb_offset;
static unsigned int recv_from_cb_offset;

static unsigned int easy_setup_intf_count;

REPEATER_ADAPTER_DATA_TABLE global_map_tbl_1, global_map_tbl_2, global_map_tbl_3;

struct APCLI_BRIDGE_LEARNING_MAPPING_MAP global_map_tbl;
spinlock_t global_map_tbl_lock;
unsigned long global_map_tbl_irq_flags;
spinlock_t global_band_tbl_lock;
unsigned long global_band_tbl_irq_flags;
spinlock_t priority_tbl_lock;
unsigned long priority_tbl_irq_flags;

extern void wifi_fwd_register(struct wifi_fwd_func_table *table);

#define ARP_ETH_TYPE	0x0806
#define LLTD_ETH_TYPE	0x88D9

#ifndef TCP
#define TCP				0x06
#endif				/* !TCP */

#ifndef UDP
#define UDP				0x11
#endif				/* !UDP */

#ifndef ETH_P_IPV6
#define ETH_P_IPV6		0x86DD
#endif				/* !ETH_P_IPV6 */

#ifndef ETH_P_IP
#define ETH_P_IP		0x0800	/* Internet Protocol packet */
#endif				/* ETH_P_IP */

#ifndef LENGTH_802_3
#define LENGTH_802_3	14
#endif				/* !LENGTH_802_3 */

#define IPV6_NEXT_HEADER_UDP	0x11
#define IPV6_HDR_LEN  40

struct wifi_fwd_func_table wf_fp_tbl;

static REPEATER_ADAPTER_DATA_TABLE *rep_table[] = {

	&global_map_tbl_1,
	&global_map_tbl_2,
	&global_map_tbl_3,
};

static void **adapter_table[] = {

	&adapter_tmp_1,
	&adapter_tmp_2,
	&adapter_tmp_3,
};

unsigned char num_rep_tbl = (sizeof(rep_table) / sizeof(REPEATER_ADAPTER_DATA_TABLE *));
unsigned char num_adapter_tbl = (sizeof(adapter_table) / sizeof(void **));

const unsigned char bpdu_group_address[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

struct net_device *ra_dev_get_by_name(const char *name)
{
	struct net_device *dev;

	dev = dev_get_by_name(&init_net, name);

	if (dev)
		dev_put(dev);

	return dev;
}

static unsigned char is_fastlane_mode(struct net_device *dev)
{
	int link_cnt = 0;

	if (main_2g_link_cnt != 0)
		link_cnt++;
	if (main_5g_link_cnt != 0)
		link_cnt++;
	if (main_5g_h_link_cnt != 0)
		link_cnt++;

	if (link_cnt == 1)
		return 1;
	else
		return 0;
}

static unsigned char is_concurrent_mode(struct net_device *dev)
{
	int link_cnt = 0;

	if (main_2g_link_cnt != 0)
		link_cnt++;
	if (main_5g_link_cnt != 0)
		link_cnt++;
	if (main_5g_h_link_cnt != 0)
		link_cnt++;

	if (link_cnt > 1)
		return 1;
	else
		return 0;
}

static unsigned char is_mbss_mode(unsigned int band_from)
{
	if (((main_2g_link_cnt >= 2) && IS_PACKET_FROM_2G(band_from))
	    || ((main_5g_link_cnt >= 2) && IS_PACKET_FROM_5G(band_from))
	    || ((main_5g_h_link_cnt >= 2) && IS_PACKET_FROM_5G_H(band_from)))
		return 1;
	else
		return 0;
}

/*
*	return value:
*	>=0:	array index of FwdDevice
*	-1:		search failed
*/
static int wifi_fwd_find_device(struct net_device *dev)
{
	int idx = 0;

	for (idx = 0; idx < WIFI_FWD_DEVICE_SIZE; idx++) {
		if ((FwdDevice != NULL) && (FwdDevice[idx].valid == TRUE) && (FwdDevice[idx].dev == dev))
			return idx;
	}

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] no entry found\n", __func__));
	return -1;
}

/*
*	return value:
*	>=0:	array index of FwdDevice
*	-1:		search failed
*/
static int wifi_fwd_find_device_by_info(BAND_INDEX band, unsigned int type)
{
	int idx = 0;

	for (idx = 0; idx < WIFI_FWD_DEVICE_SIZE; idx++) {
		if ((FwdDevice != NULL) && (FwdDevice[idx].valid == TRUE) && (FwdDevice[idx].band == band) && (FwdDevice[idx].type == type))
			return idx;
	}

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] no entry found\n", __func__));
	return -1;
}

static void cal_link_cnt_by_idx(unsigned char index, unsigned char policy)
{
	struct DevInfo *dev_info = NULL;

	if (index > WIFI_FWD_DEVICE_SIZE)
		return;

	dev_info = FwdDevice + index;

	if (dev_info == NULL)
		return;

	if (IS_APCLI_DEV(dev_info->type) && IS_VALID(dev_info->valid)) {
		switch (dev_info->band) {
		case band_2g:
			if (policy > 0)
				main_2g_link_cnt++;
			else
				main_2g_link_cnt--;

				if (main_2g_link_cnt < 0)
					main_2g_link_cnt = 0;

			break;

		case band_5g:
			if (policy > 0)
				main_5g_link_cnt++;
			else
				main_5g_link_cnt--;

			if (main_5g_link_cnt < 0)
				main_5g_link_cnt = 0;

			break;

		case band_5g_h:
			if (policy > 0)
				main_5g_h_link_cnt++;
			else
				main_5g_h_link_cnt--;

			if (main_5g_h_link_cnt < 0)
				main_5g_h_link_cnt = 0;

			break;

		default:
			WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s][%d] unknown band(%x)\n", __func__, __LINE__, dev_info->band));
			break;
		}
	}

	if (policy > 0)
		fwd_counter++;
	else
		fwd_counter--;
}

static void cal_link_count_by_net_dev(struct net_device *dev, unsigned char policy)
{
	int link_idx = 0;

	if (dev == NULL)
		return;

	link_idx = wifi_fwd_find_device(dev);

	if (link_idx >= 0)
		cal_link_cnt_by_idx(link_idx, policy);
}

static void dump_bridge_by_name(void)
{
#ifndef CONFIG_SUPPORT_OPENWRT
	br0 = ra_dev_get_by_name("br0");
#else
	br0 = ra_dev_get_by_name("br-lan");
#endif
}

static void wifi_fwd_reset_link_count(void)
{
	fwd_counter = 0;
	main_5g_h_link_cnt = 0;
	main_5g_link_cnt = 0;
	main_2g_link_cnt = 0;
}

static bool wifi_fwd_needed(void)
{
	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACTIVE)
	    || !WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED)) {
		return FALSE;
	} else if (easy_setup_intf_count != 0) {
		return TRUE;
	} else if (fwd_counter == 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}

static void wifi_fwd_show_entry(void)
{
	int idx = 0;

	WF_FWD_PRINT(WF_DEBUG_OFF, ("driver version: %s  .\n", FWD_VERSION));

	WF_FWD_PRINT(WF_DEBUG_OFF, ("\n[%s]fwd_link=%d,[5g-h main]=%d [5g main]=%d, [2g main]=%d\n", __func__, fwd_counter, main_5g_h_link_cnt, main_5g_link_cnt, main_2g_link_cnt));

	for (idx = 0; idx < WIFI_FWD_TBL_SIZE; idx++) {
		if (WiFiFwdBase[idx].valid)
			WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]index=%d, valid=%d, apcli-dev=0x%p, ap-dev=0x%p\n", __func__, idx, WiFiFwdBase[idx].valid, WiFiFwdBase[idx].src, WiFiFwdBase[idx].dest));
	}

	WF_FWD_PRINT(WF_DEBUG_OFF, ("---------------------------------------------------------------------------------------------\n"));

	for (idx = 0; idx < WIFI_FWD_DEVICE_SIZE; idx++) {
		if (FwdDevice[idx].valid) {
			WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]index=%d, valid=%d, ", __func__, idx, FwdDevice[idx].valid));

			if (FwdDevice[idx].type == INT_MAIN)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("type=MAIN,   "));
			else if (FwdDevice[idx].type == INT_MBSSID)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("type=MBSSID, "));
			else if (FwdDevice[idx].type == INT_APCLI)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("type=APCLI,  "));

			WF_FWD_PRINT(WF_DEBUG_OFF, ("mbss=0x%02x, ", FwdDevice[idx].mbss_idx));

			if (FwdDevice[idx].band == band_2g)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("band=2G,   src=0x%p, ", FwdDevice[idx].dev));
			else if (FwdDevice[idx].band == band_5g)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("band=5G,   src=0x%p, ", FwdDevice[idx].dev));
			else if (FwdDevice[idx].band == band_5g_h)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("band=5G-h, src=0x%p, ", FwdDevice[idx].dev));

			if (FwdDevice[idx].partner != NULL)
				WF_FWD_PRINT(WF_DEBUG_OFF, ("partner=0x%p\n", FwdDevice[idx].partner->dev));
			else
				WF_FWD_PRINT(WF_DEBUG_OFF, ("no partner\n"));
		} else
			continue;
	}

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]eth_rep5g_h_wrg_uni_cnt=%u, eth_rep5g_wrg_uni_cnt=%u, eth_rep2g_wrg_uni_cnt=%u\n",
				    __func__, eth_rep5g_h_wrg_uni_cnt, eth_rep5g_wrg_uni_cnt, eth_rep2g_wrg_uni_cnt));

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]eth_rep5g_h_wrg_bct_cnt=%u, eth_rep5g_wrg_bct_cnt=%u, eth_rep2g_wrg_bct_cnt=%u\n",
				    __func__, eth_rep5g_h_wrg_bct_cnt, eth_rep5g_wrg_bct_cnt, eth_rep2g_wrg_bct_cnt));

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]rep=%d\n", __func__, rep_net_dev));
}

static void wifi_fwd_delete_entry_by_idx(unsigned char idx)
{
	int i = 0;

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("\n[%s]--------------------------------------------\n", __func__));

	if (idx < WIFI_FWD_TBL_SIZE) {
		if (WiFiFwdBase[idx].valid) {
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s]index=%d, valid=%d, src dev=0x%p, dest dev=0x%p\n", __func__, idx, WiFiFwdBase[idx].valid, WiFiFwdBase[idx].src, WiFiFwdBase[idx].dest));

			cal_link_count_by_net_dev(WiFiFwdBase[idx].src, 0);
			memset(&WiFiFwdBase[idx], 0, sizeof(struct FwdPair));

		} else
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s]index=%d is void originally\n", __func__, idx));
	} else {
		for (i = 0; i < WIFI_FWD_TBL_SIZE; i++)
			memset(&WiFiFwdBase[i], 0, sizeof(struct FwdPair));

		wifi_fwd_reset_link_count();
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] flush all entries\n", __func__));
	}
}

static void packet_source_show_entry(void)
{
	int idx = 0;

	WF_FWD_PRINT(WF_DEBUG_OFF, ("\n[%s]--------------------------------------------\n", __func__));

	for (idx = 0; idx < WIFI_FWD_TBL_SIZE; idx++) {
		if (pkt_src[idx].valid)
			WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]index=%d, valid=%d, src=0x%p, peer=0x%p, h_source=%02X:%02X:%02X:%02X:%02X:%02X, h_dest=%02X:%02X:%02X:%02X:%02X:%02X\n",
						    __func__, idx, pkt_src[idx].valid, pkt_src[idx].src, pkt_src[idx].peer,
						    pkt_src[idx].h_source[0], pkt_src[idx].h_source[1], pkt_src[idx].h_source[2],
						    pkt_src[idx].h_source[3], pkt_src[idx].h_source[4], pkt_src[idx].h_source[5],
						    pkt_src[idx].h_dest[0], pkt_src[idx].h_dest[1], pkt_src[idx].h_dest[2], pkt_src[idx].h_dest[3], pkt_src[idx].h_dest[4], pkt_src[idx].h_dest[5]));
	}
}

static void packet_source_delete_entry(unsigned char idx)
{
	int i = 0;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("\n[%s]--------------------------------------------\n", __func__));

	if (idx < WIFI_FWD_TBL_SIZE) {
		if (pkt_src[idx].valid) {
			memset(&pkt_src[idx], 0, sizeof(struct PacketSource));

			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s]index=%d, valid=%d, src=0x%p, peer=0x%p, h_source=%02X:%02X:%02X:%02X:%02X:%02X, h_dest=%02X:%02X:%02X:%02X:%02X:%02X\n",
						      __func__, idx, pkt_src[idx].valid, pkt_src[idx].src, pkt_src[idx].peer,
						      pkt_src[idx].h_source[0], pkt_src[idx].h_source[1], pkt_src[idx].h_source[2],
						      pkt_src[idx].h_source[3], pkt_src[idx].h_source[4], pkt_src[idx].h_source[5],
						      pkt_src[idx].h_dest[0], pkt_src[idx].h_dest[1], pkt_src[idx].h_dest[2], pkt_src[idx].h_dest[3], pkt_src[idx].h_dest[4], pkt_src[idx].h_dest[5]));
		} else
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s]index=%d is void originally\n", __func__, idx));
	} else if (idx == WIFI_FWD_TBL_SIZE) {
		eth_rep5g_h_wrg_uni_cnt = 0;
		eth_rep5g_h_wrg_bct_cnt = 0;
		eth_rep5g_wrg_uni_cnt = 0;
		eth_rep5g_wrg_bct_cnt = 0;
		eth_rep2g_wrg_uni_cnt = 0;
		eth_rep2g_wrg_bct_cnt = 0;

		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] reset counters\n", __func__));
	} else {
		for (i = 0; i < WIFI_FWD_TBL_SIZE; i++)
			memset(&pkt_src[i], 0, sizeof(struct PacketSource));

		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] flush all entries\n", __func__));
	}
}

/*
*	return value:
*	>=0:	array index of WiFiFwdBase
*	-1:		search failed
*/
static int wifi_fwd_find_empty_entry(void)
{
	int idx = 0;

	for (idx = 0; idx < WIFI_FWD_TBL_SIZE; idx++) {
		if (WiFiFwdBase[idx].valid == FALSE)
			return idx;
	}

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] table full\n", __func__));
	return -1;
}

/*
*	return value:
*	>=0:	array index of WiFiFwdBase
*	-1:		search failed
*/
static int wifi_fwd_find_entry(struct net_device *src, struct net_device *dest)
{
	int idx = 0;

	for (idx = 0; idx < WIFI_FWD_TBL_SIZE; idx++) {
		if ((WiFiFwdBase[idx].valid == TRUE) && (WiFiFwdBase[idx].src == src) && (WiFiFwdBase[idx].dest == dest))
			return idx;
	}

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] no entry found\n", __func__));
	return -1;
}

static unsigned char wifi_fwd_establish_entry(struct net_device *src, struct net_device *dest)
{
	struct FwdPair *entry = NULL;
	int idx = 0;

	if (!src || !dest)
		return 0;

	/* check if it is an existed entry */
	idx = wifi_fwd_find_entry(src, dest);
	if (idx >= 0)
		return 0;

	/* to establish the path between src and dest */
	idx = wifi_fwd_find_empty_entry();
	if (idx == -1)
		return 0;

	entry = WiFiFwdBase + idx;
	entry->valid = TRUE;
	entry->src = src;
	entry->dest = dest;
	cal_link_count_by_net_dev(entry->src, 1);

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s %d] valid=%d, src=0x%p, dest=0x%p\n", __func__, idx, entry->valid, entry->src, entry->dest));

	return 1;
}

/*
*	return value:
*	1:	clear OK
*	0:	clear failed (wrong input index)
*/
static int wifi_fwd_clear_entry(int index)
{
	struct FwdPair *entry = NULL;

	entry = WiFiFwdBase + index;

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[wifi_fwd_clear_entry] original: index=%d, valid=%d, src_dev=0x%p, dest_dev=0x%p\n", index, entry->valid, entry->src, entry->dest));

	if (entry->valid) {
		memset(entry, 0, sizeof(struct FwdPair));
		return 1;
	}

	return 0;
}

/*
*	return value:
*	1:	clear OK
*	0:	clear failed (wrong input index)
*/
static int wifi_fwd_clear_device(int index)
{
	struct DevInfo *entry = NULL;

	entry = FwdDevice + index;

	if (entry->valid) {
		memset(entry, 0, sizeof(struct DevInfo));
		return 1;
	}
	return 0;
}

static void wifi_fwd_set_cb_num(unsigned int band_cb_num, unsigned int receive_cb_num)
{
	band_cb_offset = band_cb_num;
	recv_from_cb_offset = receive_cb_num;

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] band_cb_offset=%d, recv_from_cb_offset=%d\n", __func__, band_cb_offset, recv_from_cb_offset));
}

static bool wifi_fwd_check_active(void)
{
	if (WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACTIVE)
	    && WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return TRUE;
	else
		return FALSE;
}

static unsigned char wifi_fwd_check_rep(unsigned char rep)
{
	switch (rep) {
	case eth_traffic_band_2g:
		if (main_2g_link_cnt == 0) {
			if (next_2g_band == band_5g) {
				if (main_5g_link_cnt != 0)
					return eth_traffic_band_5g;
				else if (main_5g_h_link_cnt != 0)
					return eth_traffic_band_5g_H;
			} else {
				if (main_5g_h_link_cnt != 0)
					return eth_traffic_band_5g_H;
				else if (main_5g_link_cnt != 0)
					return eth_traffic_band_5g;
			}
		} else
			break;
	case eth_traffic_band_5g:
		if (main_5g_link_cnt == 0) {
			if (main_5g_h_link_cnt != 0)
				return eth_traffic_band_5g_H;
			else if (main_2g_link_cnt != 0)
				return eth_traffic_band_2g;
		} else
			break;

	case eth_traffic_band_5g_H:
		if (main_5g_h_link_cnt == 0) {
			if (main_5g_link_cnt != 0)
				return eth_traffic_band_5g;
			else if (main_2g_link_cnt != 0)
				return eth_traffic_band_2g;
		} else
			break;
	default:
		WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] rep=%d by default\n", __func__, eth_traffic_band_5g));
		return eth_traffic_band_5g;
	}

	return rep;
}

static void wifi_fwd_get_rep(unsigned char rep)
{
	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	if (rep == 0)
		rep_net_dev = eth_traffic_band_2g;
	else if (rep == eth_traffic_band_5g_H)
		rep_net_dev = eth_traffic_band_5g_H;
	else
		rep_net_dev = eth_traffic_band_5g;

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] rep = %d\n", __func__, rep_net_dev));
}

static void wifi_fwd_set_debug_level(unsigned char level)
{
	if (level == 0)
		wf_debug_level = WF_DEBUG_OFF;
	else
		wf_debug_level = WF_DEBUG_ON;

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] level=%d\n", __func__, wf_debug_level));
}

static void wifi_fwd_pro_active(void)
{
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_pro_halt(void)
{
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_pro_enabled(void)
{
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_ENABLED);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_pro_disabled(void)
{
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_ENABLED);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_access_schedule_active(void)
{
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_access_schedule_halt(void)
{
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_hijack_active(void)
{
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_HIJACK_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_hijack_halt(void)
{
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_HIJACK_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_bpdu_active(void)
{
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_BPDU_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void wifi_fwd_bpdu_halt(void)
{
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_BPDU_ACTIVE);
	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s]\n", __func__));
}

static void packet_tx_source_flush_all(void)
{
	struct TxSourceEntry *entry = NULL;
	int i = 0;

	if (!tx_src_tbl || (tx_src_count == 0))
		return;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = tx_src_tbl + i;

		/* keep the records of wireless stations */
		if ((entry->valid == 1) && (entry->bfrom_wireless == 0)) {
			tx_src_count--;
			entry->valid = 0;
		}

		if (tx_src_count == 0)
			break;
	}
}

/* delete the records of wireless stations */
static void wf_fwd_delete_entry_inform(unsigned char *addr)
{
	struct TxSourceEntry *entry = NULL;
	int i = 0, count = 0;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	if (!addr || (tx_src_count == 0))
		return;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = tx_src_tbl + i;

		if (entry->valid == TRUE) {
			count++;
			if (MAC_ADDR_EQUAL(addr, entry->h_source)) {
				entry->valid = FALSE;
				entry->bfrom_wireless = 0;
				entry->eth_tx_band = 0;
				tx_src_count--;

				if (tx_src_count < 0)
					tx_src_count = 0;

				return;
			}

			if (count >= tx_src_count)
				return;
		}
	}
}

/* add the records of wireless stations */
static void wf_fwd_add_entry_inform(unsigned char *addr)
{
	struct TxSourceEntry *entry = NULL;
	int i = 0;

	if (!addr)
		return;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = tx_src_tbl + i;

		if (entry->valid == FALSE) {
			tx_src_count++;
			entry->valid = TRUE;
			entry->bfrom_wireless = 1;
			COPY_MAC_ADDR(entry->h_source, addr);
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s %d] valid=%d, src_mac=%02X:%02X:%02X:%02X:%02X:%02X\n",
			__func__, i, entry->valid, entry->h_source[0], entry->h_source[1], entry->h_source[2], entry->h_source[3], entry->h_source[4], entry->h_source[5]));
			break;
		}
	}
}

/*
*	return value:
*	0:	insert failed
*	1:	insert success
*	2:	do nothing
*/
static int wifi_fwd_insert_tx_source_entry(struct sk_buff *skb, struct DevInfo *tx_dev_info)
{
	struct ethhdr *mh = eth_hdr(skb);
	struct TxSourceEntry *entry = NULL;
	int i = 0;
	unsigned char bInsert = 0, eth_traffic_band;
	unsigned int recv_from = 0;

	if ((IS_VALID(tx_dev_info->valid) && !is_concurrent_mode(tx_dev_info->dev)) || !mh)
		return 2;

	recv_from = WIFI_FWD_GET_PACKET_RECV_FROM(skb, recv_from_cb_offset);

	if (IS_VALID(tx_dev_info->valid) && IS_APCLI_DEV(tx_dev_info->type)) {
		if ((easy_setup_intf_count != 0) && (IS_PACKET_FROM_APCLI(recv_from))) {
			/*printk("FWD_MODULE: CLI(band idx: 0x%x) won't add src entry from pkt from other CLI\n",tx_dev_info->band); */
			return 2;
		}

		/*
		*	For Wireless STA, it will be created by hook(wf_fwd_add_entry_inform_hook).
		*	So we just create for those targets from ethernet.
		*/
		if (!IS_PACKET_FROM_ETHER(recv_from))
			return 2;

		eth_traffic_band = wifi_fwd_check_rep(rep_net_dev);
		for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
			entry = tx_src_tbl + i;
			if ((entry->valid == 1) && MAC_ADDR_EQUAL(mh->h_source, entry->h_source)) {
				entry->eth_tx_band = eth_traffic_band;
				return 2;
			}
		}

		bInsert = 1;
	}

	if (!bInsert)
		return 2;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = tx_src_tbl + i;
		if (entry->valid == 0) {
			tx_src_count++;
			entry->valid = 1;
			entry->bfrom_wireless = 0;
			entry->eth_tx_band = eth_traffic_band;
			COPY_MAC_ADDR(entry->h_source, mh->h_source);
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s %d] valid=%d, src_mac=%02X:%02X:%02X:%02X:%02X:%02X\n",
				__func__, i, entry->valid, entry->h_source[0], entry->h_source[1], entry->h_source[2], entry->h_source[3], entry->h_source[4], entry->h_source[5]));
			return 1;
		}
	}

	WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] No free space for insert tx source entry.\n", __func__));
	return 0;
}

/*
*	return value:
*	0:	no looping
*	1:	looping found
*	2:	forward to bridge
*/
static unsigned char wifi_fwd_check_looping(struct sk_buff *skb, struct DevInfo *dev_info)
{
	struct ethhdr *mh = eth_hdr(skb);
	struct TxSourceEntry *entry = NULL;
	unsigned int recv_from = WIFI_FWD_GET_PACKET_RECV_FROM(skb, recv_from_cb_offset);
	unsigned int band_from = WIFI_FWD_GET_PACKET_BAND(skb, band_cb_offset);
	int i = 0, count = 0;
	unsigned char bMulticast = 0;

	/*
	*	no need to check while:
	*	1. this skb has no ethernet header
	*	2. this packet is not received from ap-client
	*	3. tx_src_count is 0 -> tx_src_tbl has no entries
	*	4. dev_info is valid and not in concurrent mode
	*/
	if ((IS_VALID(dev_info->valid) && !is_concurrent_mode(dev_info->dev)) || !IS_PACKET_FROM_APCLI(recv_from) || (tx_src_count == 0) || !mh)
		return 0;

	if (IS_VALID(dev_info->valid) && IS_APCLI_DEV(dev_info->type)) {
		if ((mh->h_dest[0] & 0x1) == 0x1)
			bMulticast = 1;

		for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
			entry = tx_src_tbl + i;
			if (entry->valid == 1) {
				count++;
				if (MAC_ADDR_EQUAL(mh->h_source, entry->h_source)) {
					/* mean the source has changed to other side no matter it comes from wireless or ethernet */
					if ((easy_setup_intf_count != 0) || !bMulticast) {
						wf_fwd_delete_entry_inform(entry->h_source);
						return 0;
					} else {
						if (!entry->bfrom_wireless) { /* for ethernet target */
							/* mean the ethernet source has changed to other side */
							if (((entry->eth_tx_band == eth_traffic_band_2g) && IS_PACKET_FROM_2G(band_from)) ||
							   ((entry->eth_tx_band == eth_traffic_band_5g) && IS_PACKET_FROM_5G(band_from)) ||
							   ((entry->eth_tx_band == eth_traffic_band_5g_H) && IS_PACKET_FROM_5G_H(band_from))) {
								wf_fwd_delete_entry_inform(entry->h_source);
								return 0;
							}
						}
					}

					/*printk("FWD module: wifi_fwd_check_looping=> Grp packet from own connected client : %02x-%02x-%02x-%02x-%02x-%02x \n",
					*	entry->h_source[0],entry->h_source[1],entry->h_source[2],entry->h_source[3],entry->h_source[4],entry->h_source[5]);*/
					return 1;
				}

				if (count >= tx_src_count)
					break;
			}
		}
	}

	return 0;
}

/*
*	return value:
*	0:	do nothing
*	1:	forward to bridge
*/
static unsigned char wifi_fwd_pkt_to_bridge_by_dest_addr(struct sk_buff *skb, struct net_device *dev)
{
	struct ethhdr *mh = eth_hdr(skb);
	struct TxSourceEntry *tx_src_entry = NULL;
	unsigned int recv_from;
	int i = 0, count = 0;

	recv_from = WIFI_FWD_GET_PACKET_RECV_FROM(skb, recv_from_cb_offset);

	if ((tx_src_count > 0) && IS_PACKET_FROM_APCLI(recv_from) && mh) {
		for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
			tx_src_entry = tx_src_tbl + i;
			if (tx_src_entry->valid) {
				count++;
				if ((tx_src_entry->bfrom_wireless == 0) && MAC_ADDR_EQUAL(tx_src_entry->h_source, mh->h_dest))
					return 1;

				if (count >= tx_src_count)
					break;
			}
		}
	}

	return 0;
}

void wifi_fwd_check_device(struct net_device *net_dev, signed int type, signed int mbss_idx, unsigned char channel, unsigned char link)
{
	struct DevInfo *DeviceEntry = NULL;
	struct DevInfo *DeviceEntryTmp = NULL;
	struct DevInfo *DeviceTmp = NULL;
	struct DevInfo *DevicePartnerCheck = NULL;
	unsigned char idx = 0, i = 0, j = 0, valid_index = 0;
	bool get_idx = FALSE;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	if (!net_dev || !type)
		return;

	FWD_IRQ_LOCK(&priority_tbl_lock, priority_tbl_irq_flags);

	for (idx = 0; idx < WIFI_FWD_DEVICE_SIZE; idx++) {
		DeviceEntry = FwdDevice + idx;

		if (IS_VALID(DeviceEntry->valid) && (DeviceEntry->dev == net_dev)) {
			if (link == 0) {
				DevicePartnerCheck = DeviceEntry->partner;

				/* to avoid there is one more partner point to this device */
				if (DevicePartnerCheck != NULL) {
					for (j = 0; j < WIFI_FWD_DEVICE_SIZE; j++) {
						DeviceTmp = FwdDevice + j;

						if (idx == j)
							continue;

						if ((DeviceTmp->band == DevicePartnerCheck->band) && (DeviceTmp->type != DevicePartnerCheck->type)) {
							DevicePartnerCheck->partner = DeviceTmp;
							DeviceTmp->partner = DevicePartnerCheck;
						}
					}
				}

				wifi_fwd_clear_device(idx);
		   }

		   FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
		   return;
		}

		if ((DeviceEntry->valid == FALSE) && (get_idx != TRUE)) {
			valid_index = idx;
			get_idx = TRUE;
		}
	}

	if (link == 0) {
		FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
		return;
	}

	if (get_idx != TRUE) {
		FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
		WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] No free space for inserting device info.\n", __func__));
		return;
	}

	DeviceEntry = FwdDevice + valid_index;
	DeviceEntry->dev = net_dev;
	DeviceEntry->index = valid_index;
	DeviceEntry->type = type;
	DeviceEntry->mbss_idx = mbss_idx;

	if (IS_5G_HIGH(channel)) {
		DeviceEntry->band = band_5g_h;
		DeviceEntry->second_band = band_5g;
		DeviceEntry->third_band = band_2g;
	} else if (channel > 14) {
		DeviceEntry->band = band_5g;
		DeviceEntry->second_band = band_5g_h;
		DeviceEntry->third_band = band_2g;
	} else {
		DeviceEntry->band = band_2g;
		DeviceEntry->second_band = band_5g;
		DeviceEntry->third_band = band_5g_h;
	}

	DeviceEntry->partner = NULL;

	for (i = 0; i < WIFI_FWD_DEVICE_SIZE; i++) {
		DeviceEntryTmp = FwdDevice + i;

		if (i == DeviceEntry->index)
			continue;

		if ((DeviceEntryTmp->band == DeviceEntry->band) && (DeviceEntryTmp->type != DeviceEntry->type)) {
			DeviceEntry->partner = DeviceEntryTmp;
			DeviceEntryTmp->partner = DeviceEntry;
		}
	}

	DeviceEntry->valid = TRUE;

	FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
	return;
}

static int wifi_fwd_find_fastlane(struct DevInfo *src, struct DevInfo *dest)
{
	struct DevInfo *DeviceOthSrc = NULL;
	int idx = 0, i = 0;

	if (dest == NULL)
		return -1;

	for (i = 0; i < WIFI_FWD_DEVICE_SIZE; i++) {
		DeviceOthSrc = FwdDevice + i;

		if ((DeviceOthSrc == NULL) || (DeviceOthSrc == src))
			continue;

		if ((DeviceOthSrc->valid == TRUE) && IS_APCLI_DEV(DeviceOthSrc->type)) {
			idx = wifi_fwd_find_entry(DeviceOthSrc->dev, dest->dev);

			if (idx >= 0)
				return idx;
		}
	}

	return -1;

}

/*
*	return value:
*	1:	delete success
*	0:	delete failed
*/
static unsigned char wifi_fwd_delete_entry(struct net_device *src, struct net_device *dest, unsigned char link_down)
{
	int i, j;
	int src_index, tmp_index, second_src_index, third_src_index;
	bool need_flush = FALSE;
	struct DevInfo *DeviceTmp = NULL, *DeviceSecondSrc = NULL, *DeviceThirdSrc = NULL;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return 0;

	if (!src)
		return 0;

	FWD_IRQ_LOCK(&priority_tbl_lock, priority_tbl_irq_flags);

	/* if AP interface is down */
	if (link_down == 0) {
		for (i = 0; i < WIFI_FWD_DEVICE_SIZE; i++) {
			DeviceTmp = FwdDevice + i;

			if (DeviceTmp->valid != TRUE)
				continue;

			if (!IS_APCLI_DEV(DeviceTmp->type))
				continue;

			tmp_index = wifi_fwd_find_entry(DeviceTmp->dev, dest);

			if (tmp_index >= 0) {
				if (wifi_fwd_clear_entry(tmp_index) != 0) {
					cal_link_cnt_by_idx(i, 0);
					need_flush = TRUE;
				}
			}
		}
		FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
		goto done;
	}

	src_index = wifi_fwd_find_device(src);

	if (src_index < 0) {
		FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d]Error!!!!!!!!!!!!src_index(%d) src(%p)\n", __func__, __LINE__, src_index, src));
		return 0;
	}
	/*
	*	Delete the src path
	*/
	for (j = 0; j < WIFI_FWD_DEVICE_SIZE; j++) {
		DeviceTmp = FwdDevice + j;

		if (DeviceTmp->valid != TRUE)
			continue;

		if (!IS_AP_DEV(DeviceTmp->type))
			continue;

		tmp_index = wifi_fwd_find_entry(src, DeviceTmp->dev);

		if (tmp_index >= 0) {
			if (wifi_fwd_clear_entry(tmp_index) != 0) {
				cal_link_cnt_by_idx(src_index, 0);
				need_flush = TRUE;

				second_src_index = wifi_fwd_find_device_by_info(DeviceTmp->second_band, INT_APCLI);
				DeviceSecondSrc = FwdDevice + second_src_index;

				if ((second_src_index >= 0) && (second_src_index != src_index))
					wifi_fwd_establish_entry(DeviceSecondSrc->dev, DeviceTmp->dev);
				else {
					third_src_index = wifi_fwd_find_device_by_info(DeviceTmp->third_band, INT_APCLI);

					DeviceThirdSrc = FwdDevice + third_src_index;

					if ((third_src_index >= 0) && (third_src_index != src_index))
						wifi_fwd_establish_entry(DeviceThirdSrc->dev, DeviceTmp->dev);

				}
			}
		}
	}

	FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);

done:

	if (need_flush) {
		/* flush packet source table */
		packet_source_delete_entry(WIFI_FWD_TBL_SIZE + 1);

		/* flush tx packet source table */
		packet_tx_source_flush_all();
	}

	WIFI_FWD_CLEAR_FLAG(fOP_GET_NET_DEVICE_STATUS_DONE);
	return 1;
}

/*
*	return value:
*	1:	insert success
*	0:	insert failed
*/
static unsigned char wifi_fwd_insert_entry(struct net_device *src, struct net_device *dest, void *adapter)
{
	struct DevInfo *DeviceEntrySrc = NULL, *DeviceEntryDest = NULL, *DeviceEntryOthDest = NULL, *DeviceEntryOthSrc = NULL;
	int idx = 0, k = 0, index = 0, src_index = 0;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return 0;

	if (!src || !dest)
		return 0;

	if (!WIFI_FWD_TEST_FLAG(fOP_GET_NET_DEVICE_STATUS_DONE)) {
		dump_bridge_by_name();
		WIFI_FWD_SET_FLAG(fOP_GET_NET_DEVICE_STATUS_DONE);
	}

	FWD_IRQ_LOCK(&priority_tbl_lock, priority_tbl_irq_flags);

	/*
	*	src named as DeviceEntrySrc is the net device of ap-client trying to insert the entry into WiFi forwarding table
	*	based on forwarding device table, find out the partner of src and name it as DeviceEntryDest
	*/
	for (src_index = 0; src_index < WIFI_FWD_DEVICE_SIZE; src_index++) {
		DeviceEntrySrc = FwdDevice + src_index;

		if ((DeviceEntrySrc != NULL) && (DeviceEntrySrc->valid == TRUE) && (DeviceEntrySrc->dev == src) && IS_APCLI_DEV(DeviceEntrySrc->type)) {
			DeviceEntryDest = DeviceEntrySrc->partner;
			break;
		}
	}

	if (src_index >= WIFI_FWD_DEVICE_SIZE) 	{
		FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d]cannot find DevSrc(%p) in FwdDevice\n", __func__, __LINE__, src));
		return 0;
	}

	/*
	*	check if there exists a fastlane path by searching forwarding device table
	*	if yes, delete the cross path of fastlane and will establish a new path later
	*/

	idx = wifi_fwd_find_fastlane(DeviceEntrySrc, DeviceEntryDest);

	if (idx >= 0)
		wifi_fwd_delete_entry_by_idx(idx);

	if (DeviceEntryDest != NULL) {
		/* try to establish the path between src and dest */
		if (wifi_fwd_establish_entry(DeviceEntrySrc->dev, DeviceEntryDest->dev) == 0) {
			FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d]forwarding path establishing failed\n", __func__, __LINE__));
			return 0;
		}
	}

	/*
	*	check if there exists a higher priority forwarding path leading to root ap for other dest named as DeviceEntryOthDest
	*	if yes, change the link from DeviceEntryOthDest(dest) to the input src
	*/
	for (k = 0; k < WIFI_FWD_DEVICE_SIZE; k++) {
		DeviceEntryOthDest = FwdDevice + k;

		if (DeviceEntryOthDest->valid != TRUE)
			continue;

		if (!IS_AP_DEV(DeviceEntryOthDest->type))
			continue;

		/* find out other dest which is not equal to the partner of the input src */
		if (DeviceEntryOthDest != DeviceEntryDest) {
			index = -1;
			DeviceEntryOthSrc = NULL;

			if ((DeviceEntryOthDest->partner != NULL) && (DeviceEntryOthDest->partner->valid == TRUE))
				DeviceEntryOthSrc = DeviceEntryOthDest->partner;

			if ((DeviceEntryOthSrc != NULL) && IS_VALID(DeviceEntryOthSrc->valid))
				index = wifi_fwd_find_entry(DeviceEntryOthSrc->dev, DeviceEntryOthDest->dev);

			if (index >= 0)
				continue;

			index = wifi_fwd_find_fastlane(DeviceEntryOthSrc, DeviceEntryOthDest);

			if (index < 0) {
				if (DeviceEntrySrc != NULL)
					wifi_fwd_establish_entry(DeviceEntrySrc->dev, DeviceEntryOthDest->dev);

				continue;
			}
#if 0
			wifi_fwd_delete_entry_by_idx(index);
			if (DeviceEntrySrc != NULL)
				wifi_fwd_establish_entry(DeviceEntrySrc->dev, DeviceEntryOthDest->dev);

#else
			if (DeviceEntrySrc->band == band_5g) {
				WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d]input source is @ band_5g\n", __func__, __LINE__));

				if (((DeviceEntryOthDest->band == band_2g) && (DeviceEntryOthDest->second_band == band_5g)) || (DeviceEntryOthDest->band == band_5g_h)) {
					wifi_fwd_delete_entry_by_idx(index);

					if (DeviceEntrySrc != NULL)
						wifi_fwd_establish_entry(DeviceEntrySrc->dev, DeviceEntryOthDest->dev);
				}
			} else if (DeviceEntrySrc->band == band_5g_h) {
				WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d]input source is @ band_5g_h\n", __func__, __LINE__));

				if (((DeviceEntryOthDest->band == band_2g) && (DeviceEntryOthDest->second_band == band_5g_h)) || (DeviceEntryOthDest->band == band_5g)) {
					wifi_fwd_delete_entry_by_idx(index);

					if (DeviceEntrySrc != NULL)
						wifi_fwd_establish_entry(DeviceEntrySrc->dev, DeviceEntryOthDest->dev);
				}
			} else if (DeviceEntrySrc->band == band_2g) {
				/* adding 2.4g ap-client link shall not change the original state of 5g links */
				WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d]input source is @ band_2g\n", __func__, __LINE__));
			}
#endif
		}
	}

	FWD_IRQ_UNLOCK(&priority_tbl_lock, priority_tbl_irq_flags);

	/* flush packet source table */
	packet_source_delete_entry(WIFI_FWD_TBL_SIZE + 1);
	return 1;
}

/*
*	return value:
*	0:	Success, skb is handled by wifi_fwd module.
*	1:	FAIL, do nothing
*/
static struct net_device *wifi_fwd_insert_packet_source(struct sk_buff *skb, struct net_device *dev, struct net_device *peer)
{
	struct ethhdr *mh = eth_hdr(skb);
	struct PacketSource *entry = NULL;
	bool need_flush = false;
	int i = 0;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = pkt_src + i;

		if (entry->valid == 1) {
			if (peer == entry->peer) {
				if (MAC_ADDR_EQUAL(mh->h_source, entry->h_source)) {
					if (dev == entry->src) {
						return peer;
					} else {
						need_flush = true;
						break;
					}
				}
			}
		}
	}

	if (need_flush == true)
		packet_source_delete_entry(WIFI_FWD_TBL_SIZE + 1);
	else
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] packet source cannot be found\n", __func__));

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = pkt_src + i;

		if (entry->valid == 0) {
			COPY_MAC_ADDR(entry->h_source, mh->h_source);
			COPY_MAC_ADDR(entry->h_dest, mh->h_dest);
			entry->valid = 1;
			entry->peer = peer;
			entry->src = dev;

			WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s %d] valid=%d, from=0x%p, will send to=0x%p, src_mac=%02X:%02X:%02X:%02X:%02X:%02X, dest_mac=%02X:%02X:%02X:%02X:%02X:%02X\n",
				__func__, i, entry->valid, entry->src, entry->peer,
				entry->h_source[0], entry->h_source[1], entry->h_source[2],
				entry->h_source[3], entry->h_source[4], entry->h_source[5],
						      entry->h_dest[0], entry->h_dest[1], entry->h_dest[2], entry->h_dest[3], entry->h_dest[4], entry->h_dest[5]));

			return entry->peer;
		}
	}
	return NULL;
}

static void wifi_fwd_tx_count_by_source_addr(struct sk_buff *skb, struct net_device *tx_dev, unsigned char *found_path, unsigned char *entry_cnt)
{
	struct PacketSource *ppkt_src = NULL;
	struct FwdPair *entry = NULL;
	int i = 0;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		ppkt_src = pkt_src + i;

		if (ppkt_src->valid == 1) {
			if (MAC_ADDR_EQUAL(&skb->data[6], ppkt_src->h_source)) {
				*entry_cnt = *entry_cnt + 1;

				if ((ppkt_src->src == tx_dev) || (ppkt_src->peer == tx_dev)) {
					for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
						entry = WiFiFwdBase + i;

						if (entry->valid == 1) {
							if (((entry->src == ppkt_src->src) && (entry->dest == ppkt_src->peer)) || ((entry->src == ppkt_src->peer) && (entry->dest == ppkt_src->src)))
								*found_path = *found_path + 1;
						}
					}
				}
			}
		} else
			break;
	}
}

/*
*	return value:
*	1:	find entry
*	0:	find nothing
*/
static unsigned char wifi_fwd_find_sa_entry_by_sa_and_nd(struct sk_buff *skb, struct net_device *sender_dev, unsigned char idx)
{
	struct PacketSource *entry = NULL;
	struct ethhdr *mh = eth_hdr(skb);
	int i = 0;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = pkt_src + i;

		if (entry->valid == 1) {
			if (MAC_ADDR_EQUAL(mh->h_source, entry->h_source)) {
				if (idx == 0) {
					if (entry->src == sender_dev)
						return 1;
				} else {
					if (entry->peer == sender_dev)
						return 1;
				}
			}
		} else
			break;
	}

	return 0;
}

#if 0
/*
*	return value: # of found entries
*/
static unsigned char wifi_fwd_sa_count_by_source_addr(struct sk_buff *skb, struct net_device *receive_dev)
{
	struct PacketSource *entry = NULL;
	struct ethhdr *mh = eth_hdr(skb);
	int i = 0, sa_cnt = 0;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = pkt_src + i;

		if (entry->valid == 1) {
			if ((mh->h_source[0] & 0x2) == 0x2) {
				if (memcmp(&mh->h_source[3], &entry->h_source[3], 3) == 0)
					sa_cnt++;
			} else {
				if (MAC_ADDR_EQUAL(mh->h_source, entry->h_source))
					sa_cnt++;
			}
		} else
			break;
	}

	return sa_cnt;
}
#endif

/*
*	return value:
*	0:	suppose it is forwarded from bridge
*	1:	not forwarded from bridge
*	2:	should be forwarded to bridge
*/
static unsigned char wifi_fwd_check_from_bridge_by_dest_addr(struct sk_buff *skb, struct DevInfo *sender_dev_info)
{
	struct PacketSource *entry = NULL;
	struct ethhdr *mh = eth_hdr(skb);
	struct DevInfo *entry_dev_info = NULL;
	int i = 0, idx = 0;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = pkt_src + i;

		if (entry->valid == 1) {
			idx = wifi_fwd_find_device(entry->src);

			if (idx < 0) {
				WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] idx(%d)\n", __func__, idx));
				return 0;
			}

			entry_dev_info = FwdDevice + idx;

			if ((sender_dev_info->valid == TRUE) && (entry_dev_info->valid == TRUE)) {
				if ((IS_AP_DEV(sender_dev_info->type) && IS_AP_DEV(entry_dev_info->type)) && (sender_dev_info->index != entry_dev_info->index)) {
					if (MAC_ADDR_EQUAL(mh->h_dest, entry->h_source))
						return 2;
				} else {
					if (MAC_ADDR_EQUAL(mh->h_dest, entry->h_source)) {
						/*printk("wifi_fwd_check_from_bridge_by_dest_addr: pkt dest matched pkt_src entry->source\n");*/
						return 1;
					}
				}
			}
		} else
			break;
	}

	return 0;
}

static void wifi_fwd_clone_and_redirect_pkt(struct sk_buff *skb, BAND_INDEX band)
{
	struct sk_buff *clone_pkt = NULL;
	struct DevInfo *rdt_dev = NULL;
	int rdt_dev_idx = 0;

	clone_pkt = skb_clone(skb, GFP_ATOMIC);

	if (clone_pkt == NULL)
		return;

	rdt_dev_idx = wifi_fwd_find_device_by_info(band, INT_APCLI);

	if (rdt_dev_idx < 0)
		goto error;

	rdt_dev = FwdDevice + rdt_dev_idx;

	if (rdt_dev == NULL)
		goto error;

	clone_pkt->dev = rdt_dev->dev;
	dev_queue_xmit(clone_pkt);
	return;

error:

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] cannnot find redirect device(%d)\n", __func__, rdt_dev_idx));
	kfree_skb(clone_pkt);
	return;
}

/*
*	return value:
*	0:	no need to drop this packet
*	1:	drop this packet
*/
static unsigned char wifi_fwd_tx_lookup_entry(struct net_device *tx_dev, struct sk_buff *skb)
{
	struct DevInfo *device_tx = NULL;
	unsigned char found = 0, entry_cnt = 0, rep = 0;
	unsigned int recv_from = 0, band_from = 0;
	int idx = 0, need_redirect = 0;

	band_from = WIFI_FWD_GET_PACKET_BAND(skb, band_cb_offset);
	recv_from = WIFI_FWD_GET_PACKET_RECV_FROM(skb, recv_from_cb_offset);

	/* drop the packet due to there's no forwarding links between ap-client and ap */
	if (IS_PACKET_FROM_APCLI(recv_from) && !MAIN_SSID_OP(tx_dev))
		return 1;

	/*
	*	in FastLane topology:
	*	1. forward the packet to driver and handle without any drop
	*	2. only for unicast
	*/
	if (IS_UNICAST_PACKET(skb) && is_fastlane_mode(tx_dev))
		return 0;

	idx = wifi_fwd_find_device(tx_dev);
	if (idx < 0)
		return 1;

	device_tx = FwdDevice + idx;

	/*
	*	drop the packet from other band if
	*	1. @ concurrent mode
	*	2. the tx device is valid
	*	3. the tx device is ap-client, not ap
	*/
	if (is_concurrent_mode(tx_dev)
		&& IS_VALID(device_tx->valid)) {
		if (IS_APCLI_DEV(device_tx->type)) {
			/* double check the rep setting due to tri-band topology */
			rep = wifi_fwd_check_rep(rep_net_dev);

			if (device_tx->band == band_5g_h) {
				if (IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt != 0)) {
					if (IS_TAG_PACKET(band_from)) {
						need_redirect = band_2g;
					} else {
						if (easy_setup_intf_count == 0)
							return 1;
					}
				}

				if ((IS_PACKET_FROM_5G(band_from) && (main_5g_link_cnt != 0)) || ((IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt == 0)) && (next_2g_band == band_5g))) {
					if (IS_TAG_PACKET(band_from)) {
						need_redirect = band_5g;
					} else {
						if (easy_setup_intf_count == 0)
							return 1;

					}
				}

				if (IS_PACKET_FROM_ETHER(band_from)) {
					if (IS_UNICAST_PACKET(skb)) {
						if (easy_setup_intf_count == 0) {

							if (rep == eth_traffic_band_2g) {
								eth_rep2g_wrg_uni_cnt++;
								need_redirect = band_2g;
							} else if (rep == eth_traffic_band_5g) {
								eth_rep5g_wrg_uni_cnt++;
								need_redirect = band_5g;
							}

						}
					} else {
						if (rep == eth_traffic_band_2g)
							eth_rep2g_wrg_bct_cnt++;
						else if (rep == eth_traffic_band_5g)
							eth_rep5g_wrg_bct_cnt++;
					}
				}
			} else if (device_tx->band == band_5g) {
				if (IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt != 0)) {
					if (IS_TAG_PACKET(band_from)) {
						need_redirect = band_2g;
					} else {
						if (easy_setup_intf_count == 0)
							return 1;
					}
				}

				if ((IS_PACKET_FROM_5G_H(band_from) && (main_5g_h_link_cnt != 0)) || ((IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt == 0)) && (next_2g_band == band_5g_h))) {
					if (IS_TAG_PACKET(band_from)) {
						need_redirect = band_5g_h;
					} else {
						if (easy_setup_intf_count == 0) {
							return 1;
						}
					}
				}
				if (IS_PACKET_FROM_ETHER(band_from)) {
					if (IS_UNICAST_PACKET(skb)) {
						if (easy_setup_intf_count == 0) {

							if (rep == eth_traffic_band_2g) {
								eth_rep2g_wrg_uni_cnt++;
								need_redirect = band_2g;
							} else if (rep == eth_traffic_band_5g_H) {
								eth_rep5g_h_wrg_uni_cnt++;
								need_redirect = band_5g_h;
							}

						}
					} else {
						if (rep == eth_traffic_band_2g)
							eth_rep2g_wrg_bct_cnt++;
						else if (rep == eth_traffic_band_5g_H)
							eth_rep5g_h_wrg_bct_cnt++;
					}
				}
			} else if (device_tx->band == band_2g) {
				if (IS_PACKET_FROM_5G(band_from) && (main_5g_link_cnt != 0)) {
					if (IS_TAG_PACKET(band_from)) {
						need_redirect = band_5g;
					} else {
						if (easy_setup_intf_count == 0)
							return 1;
					}
				}

				if (IS_PACKET_FROM_5G_H(band_from) && (main_5g_h_link_cnt != 0)) {
					if (IS_TAG_PACKET(band_from)) {
						need_redirect = band_5g_h;
					} else {
						if (easy_setup_intf_count == 0)
							return 1;
					}
				}

				if (IS_PACKET_FROM_ETHER(band_from)) {
					if (IS_UNICAST_PACKET(skb)) {
						if (easy_setup_intf_count == 0) {

							if (rep == eth_traffic_band_5g) {
								eth_rep5g_wrg_uni_cnt++;
								need_redirect = band_5g;
							} else if (rep == eth_traffic_band_5g_H) {
								eth_rep5g_h_wrg_uni_cnt++;
								need_redirect = band_5g_h;
							}

						}
					} else {
						if (rep == eth_traffic_band_5g)
							eth_rep5g_wrg_bct_cnt++;
						else if (rep == eth_traffic_band_5g_H)
							eth_rep5g_h_wrg_bct_cnt++;
					}
				}
			}

			if (need_redirect != 0) {
				/*printk("FWD module: redirecting packet to :%x\n",need_redirect);*/
				wifi_fwd_clone_and_redirect_pkt(skb, need_redirect);
				return 1;
			}
		} else { /* for non-apcli interface*/
			/*check to avoid this case for Easy Setup*/
			if ((easy_setup_intf_count == 0)) {
				if (IS_PACKET_FROM_APCLI(recv_from) && IS_BROADCAST_PACKET(skb)) {
					if (device_tx->band == band_2g) {
						if (IS_PACKET_FROM_5G(band_from) && (main_5g_link_cnt != 0)) {
							/*printk("FWD module: [Case 1]: We can't forward to different band for BC/MC packets received from apcli!\n"); */
							return 1;
						}

						if (IS_PACKET_FROM_5G_H(band_from) && (main_5g_h_link_cnt != 0)) {
							/*printk("FWD module: [Case 2]: We can't forward to different band for BC/MC packets received from apcli!\n"); */
							return 1;
						}
					} else if (device_tx->band == band_5g) {
						if (IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt != 0)) {
						/*printk("FWD module: [Case 3]: We can't forward to different band for BC/MC packets received from apcli!\n"); */
							return 1;
						}

						if ((IS_PACKET_FROM_5G_H(band_from) && (main_5g_h_link_cnt != 0)) ||
						    ((IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt == 0)) && (next_2g_band == band_5g_h))) {
						/*printk("FWD module: [Case 4]: We can't forward to different band for BC/MC packets received from apcli!\n"); */
							return 1;
						}
					} else if (device_tx->band == band_5g_h) {
						if (IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt != 0)) {
						/*printk("FWD module: [Case 5]: We can't forward to different band for BC/MC packets received from apcli!\n"); */
							return 1;
						}

						if ((IS_PACKET_FROM_5G(band_from) && (main_5g_link_cnt != 0)) ||
						    ((IS_PACKET_FROM_2G(band_from) && (main_2g_link_cnt == 0)) && (next_2g_band == band_5g))) {
						/*printk("FWD module: [Case 6]: We can't forward to different band for BC/MC packets received from apcli!\n"); */
							return 1;
						}
					}
				}
			
			} /* avoid this case for easy setup*/
		}
	}

	wifi_fwd_tx_count_by_source_addr(skb, tx_dev, &found, &entry_cnt);

	if (IS_BROADCAST_PACKET(skb)) {
		if (wifi_fwd_insert_tx_source_entry(skb, device_tx) == 0)
			return 0;

		if (found == 0) {
			if (entry_cnt == 0) {
				if (is_concurrent_mode(tx_dev) && IS_PACKET_FROM_ETHER(band_from)) {
					rep = wifi_fwd_check_rep(rep_net_dev);

					if (IS_VALID(device_tx->valid) && IS_APCLI_DEV(device_tx->type) && (easy_setup_intf_count == 0)) {
						if (device_tx->band == band_5g_h) {
							if (rep == eth_traffic_band_5g_H)
								return 0;
							else {
								/*printk("FWD_MODULE: wifi_fwd_tx_lookup_entry band_5g_h => drop as rep !eth_traffic_band_5g_H\n"); */
								return 1;
							}
						} else if (device_tx->band == band_5g) {
							if (rep == eth_traffic_band_5g)
								return 0;
							else {
								/*printk("FWD_MODULE: wifi_fwd_tx_lookup_entry band_5g => drop as rep !eth_traffic_band_5g\n"); */
								return 1;
							}
						} else if (device_tx->band == band_2g) {
							if (rep == eth_traffic_band_2g)
								return 0;
							else {
								/*printk("FWD_MODULE: wifi_fwd_tx_lookup_entry band_5g => drop as rep !eth_traffic_band_2g\n");*/
								return 1;
							}
						}
					}
				}
			}

			return 0;
		} else {
			if (entry_cnt >= 2)
				return 0;
			else {
				if (wifi_fwd_find_sa_entry_by_sa_and_nd(skb, tx_dev, 1) == 0) {
					if (easy_setup_intf_count == 0) {
					/*printk("FWD_MODULE: wifi_fwd_tx_lookup_entry Drop pkt due to For path present, this sender is not the intended sender.\n"); */
						return 1;
					}
				} else
					return 0;
			}
		}
	} else 	{
		if (found == 0) {
			if (entry_cnt == 0) {
				if (is_concurrent_mode(tx_dev) && IS_PACKET_FROM_ETHER(band_from)) {
					rep = wifi_fwd_check_rep(rep_net_dev);

					if (rep == eth_traffic_band_5g_H) {
						if (IS_VALID(device_tx->valid) && REP_IS_5G_H(device_tx->band))
							return 0;
					} else if (rep == eth_traffic_band_5g) {
						if (IS_VALID(device_tx->valid) && REP_IS_5G(device_tx->band))
							return 0;
					} else if (rep == eth_traffic_band_2g) {
						if (IS_VALID(device_tx->valid) && REP_IS_2G(device_tx->band))
							return 0;
					}
				}
			}
		}

		return 0;
	}

	return 0;
}

/*
*	return value of struct net_device:
*	NULL:	search failed
*	other:	partner net_device
*/
static struct net_device *wifi_fwd_lookup_entry(struct net_device *dev, unsigned char *type, struct sk_buff *skb)
{
	struct FwdPair *entry = NULL;
	struct net_device *src = NULL;
	int i = 0;

	for (i = 0; i < WIFI_FWD_TBL_SIZE; i++) {
		entry = WiFiFwdBase + i;

		if (entry->valid == 1) {
			if (entry->src == dev) {
				*type = ENTRY_TYPE_SRC;
				src = wifi_fwd_insert_packet_source(skb, dev, entry->dest);

				if (src != NULL)
					return src;
			} else if (entry->dest == dev) {
				*type = ENTRY_TYPE_DEST;
				src = wifi_fwd_insert_packet_source(skb, dev, entry->src);

				if (src != NULL)
					return src;
			}
		}
	}

	*type = ENTRY_TYPE_INVALID;
	return NULL;
}

void wifi_fwd_probe_adapter(void *adapter)
{
	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	if ((adapter != NULL) && (adapter_tmp_1 == NULL)) {
		adapter_tmp_1 = adapter;
	} else if ((adapter != NULL) && (adapter_tmp_2 == NULL)) {
		if (adapter != adapter_tmp_1)
			adapter_tmp_2 = adapter;
	} else if ((adapter != NULL) && (adapter_tmp_3 == NULL)) {
		if ((adapter != adapter_tmp_1) && (adapter != adapter_tmp_2))
			adapter_tmp_3 = adapter;

	}
}

void wifi_fwd_feedback_map_table(void *adapter, void **peer, void **opp_peer, void **oth_peer)
{
	unsigned char index;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	if (!adapter)
		return;

	FWD_IRQ_LOCK(&global_band_tbl_lock, global_band_tbl_irq_flags);

	for (index = 0; index < num_rep_tbl; index++) {
		if (adapter == *adapter_table[index]) {
			if (rep_table[index]->Enabled == TRUE)
				*peer = rep_table[index];
			else
				*peer = NULL;

		} else {
			if (*opp_peer == NULL) {
				if (rep_table[index] && rep_table[index]->Enabled == TRUE)
					*opp_peer = rep_table[index];
				else
				*opp_peer = NULL;
			} else {
				if (rep_table[index] && rep_table[index]->Enabled == TRUE)
					*oth_peer = rep_table[index];
				else
					*oth_peer = NULL;

			}
		}
	}
	FWD_IRQ_UNLOCK(&global_band_tbl_lock, global_band_tbl_irq_flags);

}

void wifi_fwd_insert_repeater_mapping(void *adapter, void *lock, void *cli_mapping, void *map_mapping, void *ifAddr_mapping)
{
	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	FWD_IRQ_LOCK(&global_band_tbl_lock, global_band_tbl_irq_flags);

	if ((adapter == adapter_tmp_1)) {
		global_map_tbl_1.EntryLock = lock;
		global_map_tbl_1.CliHash = cli_mapping;
		global_map_tbl_1.MapHash = map_mapping;
		if ((global_map_tbl_1.Wdev_ifAddr == NULL) || (global_map_tbl_1.Wdev_ifAddr == ifAddr_mapping))
			global_map_tbl_1.Wdev_ifAddr = ifAddr_mapping;
		else
			global_map_tbl_1.Wdev_ifAddr_DBDC = ifAddr_mapping;
		global_map_tbl_1.Enabled = TRUE;

	} else if ((adapter == adapter_tmp_2)) {
		global_map_tbl_2.EntryLock = lock;
		global_map_tbl_2.CliHash = cli_mapping;
		global_map_tbl_2.MapHash = map_mapping;
		if ((global_map_tbl_2.Wdev_ifAddr == NULL) || (global_map_tbl_2.Wdev_ifAddr == ifAddr_mapping))
			global_map_tbl_2.Wdev_ifAddr = ifAddr_mapping;
		else
			global_map_tbl_2.Wdev_ifAddr_DBDC = ifAddr_mapping;
		global_map_tbl_2.Enabled = TRUE;

	} else if ((adapter == adapter_tmp_3)) {
		global_map_tbl_3.EntryLock = lock;
		global_map_tbl_3.CliHash = cli_mapping;
		global_map_tbl_3.MapHash = map_mapping;
		if ((global_map_tbl_3.Wdev_ifAddr == NULL) || (global_map_tbl_3.Wdev_ifAddr == ifAddr_mapping))
			global_map_tbl_3.Wdev_ifAddr = ifAddr_mapping;
		else
			global_map_tbl_3.Wdev_ifAddr_DBDC = ifAddr_mapping;

		global_map_tbl_3.Enabled = TRUE;
	}

	FWD_IRQ_UNLOCK(&global_band_tbl_lock, global_band_tbl_irq_flags);
}

void wifi_fwd_insert_bridge_mapping(struct sk_buff *skb)
{
	struct net_device *net_dev = NULL;
	struct APCLI_BRIDGE_LEARNING_MAPPING_STRUCT *map_tbl_entry = NULL;
	unsigned char pkt_from, tbl_size = 0;

	net_dev = skb->dev;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return;

	if ((net_dev == ap_5g) || (net_dev == apcli_5g))
		pkt_from = WIFI_FWD_PACKET_SPECIFIC_5G;
	else
		pkt_from = WIFI_FWD_PACKET_SPECIFIC_2G;

	FWD_IRQ_LOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);

	if (global_map_tbl.entry_num < 0) {
		FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] no entry found in the list\n", __func__));
		return;
	}

	tbl_size = sizeof(struct APCLI_BRIDGE_LEARNING_MAPPING_STRUCT);
	map_tbl_entry = (struct APCLI_BRIDGE_LEARNING_MAPPING_STRUCT *)kmalloc(tbl_size, GFP_ATOMIC);

	if (map_tbl_entry) {
		memset(map_tbl_entry, 0, tbl_size);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] size of map_tbl_entry = %dbytes\n", __func__, tbl_size));
	} else
		return;

	map_tbl_entry->rcvd_net_dev = net_dev;
	map_tbl_entry->entry_from = pkt_from;
	memcpy(map_tbl_entry->src_addr, &((unsigned char *)skb->data)[6], ETH_ALEN);

	if (global_map_tbl.entry_num == 0) {
		global_map_tbl.pHead = map_tbl_entry;
		global_map_tbl.pTail = map_tbl_entry;
		map_tbl_entry->pBefore = NULL;
		map_tbl_entry->pNext = NULL;
	} else if (global_map_tbl.entry_num > 0) {
		global_map_tbl.pTail->pNext = map_tbl_entry;
		map_tbl_entry->pBefore = global_map_tbl.pTail;
		global_map_tbl.pTail = map_tbl_entry;
	}

	global_map_tbl.entry_num++;
	FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);

	if (0) {
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] %s\n", __func__, (pkt_from == WIFI_FWD_PACKET_SPECIFIC_5G ? "5G" : "2.4G")));
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] inserting mac addr = %02X:%02X:%02X:%02X:%02X:%02X\n", __func__, PRINT_MAC(map_tbl_entry->src_addr)));
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] rcvd from %s\n", __func__, WIFI_FWD_NETDEV_GET_DEVNAME(map_tbl_entry->rcvd_net_dev)));
	}

	return;
}

/*
*	return value:
*	1:	success
*	0:	fail
*/
static int wifi_fwd_search_mapping_table(struct sk_buff *skb, struct APCLI_BRIDGE_LEARNING_MAPPING_STRUCT **tbl_entry)
{
	struct net_device *net_dev = NULL;
	struct APCLI_BRIDGE_LEARNING_MAPPING_STRUCT *map_tbl_entry = NULL;
	unsigned char pkt_from, idx = 0;

	net_dev = skb->dev;

	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED))
		return 0;

	if ((net_dev == ap_5g) || (net_dev == apcli_5g))
		pkt_from = WIFI_FWD_PACKET_SPECIFIC_5G;
	else
		pkt_from = WIFI_FWD_PACKET_SPECIFIC_2G;

	FWD_IRQ_LOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);

	if (global_map_tbl.entry_num <= 0) {
		FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] no entry found in the list\n", __func__));
		return 0;
	} else {
		if (global_map_tbl.pHead != NULL) {
			map_tbl_entry = global_map_tbl.pHead;
		} else {
			FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);
			return 0;
		}

		for (idx = 0; idx < global_map_tbl.entry_num; idx++) {
			if (MAC_ADDR_EQUAL(map_tbl_entry->src_addr, &((unsigned char *)skb->data)[6]) && (net_dev != map_tbl_entry->rcvd_net_dev)) {
				if (map_tbl_entry->entry_from == WIFI_FWD_PACKET_SPECIFIC_5G) {
					/* indicate this entry exist in dual band. packets sending to this entry need to be monitored */
					map_tbl_entry->entry_from |= pkt_from;
					map_tbl_entry->rcvd_net_dev = net_dev;
				} else {
					/* make sure the net device reported in the packet is up */
					if (dev_get_flags(map_tbl_entry->rcvd_net_dev) & IFF_UP) {
						map_tbl_entry->entry_from |= pkt_from; /* indicate this entry exist in dual band. packet need to send to this */
						SET_OS_PKT_NETDEV(RTPKT_TO_OSPKT(skb), map_tbl_entry->rcvd_net_dev); /* change net_device of packet to 2G */
					}
				}

				*tbl_entry = map_tbl_entry;
				FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);
				return 1;
			} else if (MAC_ADDR_EQUAL(map_tbl_entry->src_addr, &((unsigned char *)skb->data)[6]) && (net_dev == map_tbl_entry->rcvd_net_dev)) {
				*tbl_entry = map_tbl_entry;
				FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);
				return 1;
			}

			map_tbl_entry = map_tbl_entry->pNext;
		}

		FWD_IRQ_UNLOCK(&global_map_tbl_lock, global_map_tbl_irq_flags);
	}

	return 0;
}

/*
*	return value:
*	1:	Allocate Success
*	0:	Allocate FAIL
*/
static int wifi_fwd_alloc_tbl(unsigned int NumOfEntry)
{
	unsigned int TblSize = 0;

	TblSize = NumOfEntry * sizeof(struct FwdPair);
	WiFiFwdBase = (struct FwdPair *)kmalloc(TblSize, GFP_ATOMIC);
	if (WiFiFwdBase) {
		memset(WiFiFwdBase, 0, TblSize);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] size of WiFiFwdBase = %dbytes\n", __func__, TblSize));
	} else
		return 0;

	TblSize = NumOfEntry * sizeof(struct PacketSource);
	pkt_src = (struct PacketSource *)kmalloc(TblSize, GFP_ATOMIC);
	if (pkt_src) {
		memset(pkt_src, 0, TblSize);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] size of pkt_src = %dbytes\n", __func__, TblSize));
	} else
		return 0;

	TblSize = NumOfEntry * sizeof(struct TxSourceEntry);
	tx_src_tbl = (struct TxSourceEntry *)kmalloc(TblSize, GFP_ATOMIC);
	if (tx_src_tbl) {
		memset(tx_src_tbl, 0, TblSize);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] size of tx_src_tbl = %dbytes\n", __func__, TblSize));
	} else
		return 0;

	TblSize = sizeof(struct APCLI_BRIDGE_LEARNING_MAPPING_MAP);
	memset(&global_map_tbl, 0, TblSize);

	TblSize = sizeof(struct _REPEATER_ADAPTER_DATA_TABLE);
	memset(&global_map_tbl_1, 0, TblSize);
	memset(&global_map_tbl_2, 0, TblSize);
	memset(&global_map_tbl_3, 0, TblSize);

	return 1;
}

static int wifi_fwd_alloc_device_info(unsigned int NumOfDevice)
{
	unsigned int DeviceSize = 0;

	DeviceSize = NumOfDevice * sizeof(struct DevInfo);
	FwdDevice = (struct DevInfo *) kmalloc(DeviceSize, GFP_ATOMIC);
	if (FwdDevice) {
		memset(FwdDevice, 0, DeviceSize);
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s] size of FwdDevice = %dbytes\n", __func__, DeviceSize));
	} else
		return 0;

	return 1;
}

static unsigned char wifi_fwd_check_and_forward(struct sk_buff *skb)
{
	struct ethhdr *mh = NULL;
	struct iphdr *iph = NULL;
	struct udphdr *uh = NULL;
	struct tcphdr *th = NULL;
	struct ipv6hdr *ipv6h = NULL;
	void *ptr = NULL;
	unsigned short type = 0;

	mh = eth_hdr(skb);
	if (mh) {
		type = ntohs(mh->h_proto);
		switch (type) {
			/*
			*	Forward LLTD EthType: 0x88D9
			*/
		case LLTD_ETH_TYPE:
#if 0
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("Forward - LLTD_ETH_TYPE: 0x%02x\n", type));
#endif
			if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE))
				return TRUE;
			break;

			/*
			*   Forward ARP EthType: 0x0806
			 */
		case ARP_ETH_TYPE:
#if 0
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("Forward - ARP_ETH_TYPE: 0x%02x\n", type));
#endif
			if (WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE))
				return TRUE;
			break;

		case ETH_P_IP:
			iph = (struct iphdr *)(skb->data);
#if 0
			hex_dump_("Data", skb->data, (iph->ihl << 2));
#endif
			if (iph) {
				ptr = (void *)(skb->data + (iph->ihl << 2));
				if (ptr) {
#if 0
					WF_FWD_PRINT(WF_DEBUG_TRACE, ("wifi_fwd_check_and_forward - iph->protocol: 0x%02x\n", iph->protocol));
#endif
					switch (iph->protocol) {
					case UDP:
						/*
						 *  Forward UDP port 53 and 67
						 */
						uh = (struct udphdr *)(ptr);
						if ((ntohs(uh->source) == 53) || (ntohs(uh->dest) == 53) || (ntohs(uh->source) == 67) || (ntohs(uh->dest) == 67)) {
#if 0
							WF_FWD_PRINT(WF_DEBUG_TRACE, ("Forward - udp source port: %d, dest port: %d\n", ntohs(uh->source), ntohs(uh->dest)));
#endif
							return TRUE;
						}
						break;

					case TCP:
						/*
						 *  Forward TCP port 80 and 5000
						 */
						th = (struct tcphdr *)(ptr);
						if ((ntohs(th->source) == 80) || (ntohs(th->dest) == 80) || (ntohs(th->source) == 5000) || (ntohs(th->dest) == 5000)) {
#if 0
							WF_FWD_PRINT(WF_DEBUG_TRACE, ("Forward - tcp source port: %d, dest port: %d\n", ntohs(th->source), ntohs(th->dest)));
#endif
									if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE))
										return TRUE;
								}
								break;

					default:
						break;
						}
					}
				}
				break;

		case ETH_P_IPV6:
#if 0
			WF_FWD_PRINT(WF_DEBUG_TRACE, ("IPv6: IPV6_HDR_LEN = %d\n", IPV6_HDR_LEN));
			hex_dump_("Data", skb->data, LENGTH_802_3 + IPV6_HDR_LEN);
#endif
			ipv6h = (struct ipv6hdr *)(skb->data);
			if (ipv6h) {
#if 0
				WF_FWD_PRINT(WF_DEBUG_TRACE, ("ipv6h->version = 0x%x\n", ipv6h->version));
				WF_FWD_PRINT(WF_DEBUG_TRACE, ("ipv6h->nexthdr = 0x%x\n", ipv6h->nexthdr));
#endif
				ptr = (void *)(skb->data + IPV6_HDR_LEN);
				if (ptr) {
					switch (ipv6h->nexthdr) {
						/*
						 *  Forward IPv6 UDP port 53
						 */
					case IPV6_NEXT_HEADER_UDP:
						uh = (struct udphdr *)(ptr);
#if 0
						WF_FWD_PRINT(WF_DEBUG_TRACE, ("udp source port: %d, dest port: %d\n", ntohs(uh->source), ntohs(uh->dest)));
#endif
						if ((ntohs(uh->source) == 53) || (ntohs(uh->dest) == 53))
							return TRUE;
						break;

					default:
						break;
					}
				}
			}
			break;

		default:
			break;
		}
	}

	return FALSE;
}

/*
*	return value:
*	0:	return to driver and handle
*	1:	return to driver and release
*/
static int wifi_fwd_tx_handler(struct sk_buff *skb)
{
	struct net_device *net_dev = NULL;
	unsigned char ret = 0;

	net_dev = skb->dev;

	/*
	*	return this skb to driver and handle while:
	*	1. path of WiFi forwarding is inactive
	*	2. the skb does not exist
	*	3. no forwarding connection is established
	*/
	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACTIVE)
		|| !WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED)
	    || !skb || (fwd_counter == 0))
		return 0;

	ret = wifi_fwd_tx_lookup_entry(net_dev, skb);
	return ret;
}

/*
*	return value:
*	0:	skb is handled by wifi_fwd module
*	1:	return to driver and handle
*	2:   	return to driver and release
*/
static int wifi_fwd_rx_handler(struct sk_buff *skb)
{
	struct net_device *net_dev = NULL;
	struct net_device *target = NULL;
	struct ethhdr *mh = eth_hdr(skb);
	struct DevInfo *device_info = NULL;
	unsigned char type = ENTRY_TYPE_INVALID;
	unsigned int recv_from = 0, band_from = 0, ret = 0;
	int idx = 0;

	net_dev = skb->dev;
	recv_from = WIFI_FWD_GET_PACKET_RECV_FROM(skb, recv_from_cb_offset);
	band_from = WIFI_FWD_GET_PACKET_BAND(skb, band_cb_offset);

	/*
	*	return this skb to driver and bridge while:
	*	1. path of WiFi forwarding is inactive
	*	2. the skb does not exist
	*	3. no forwarding connection is established
	*	4. the destination is to bridge or ethernet
	*	5. in FastLane topology
	*	6. handling multicast/broadcast Rx
	*	7. hit hijack
	*/
	if (!WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACTIVE)
		|| !WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ENABLED)
	    || !skb || (fwd_counter == 0))
		return 1;

	/* drop the packet due to there's no forwarding links between ap-client and ap */
	if (IS_PACKET_FROM_APCLI(recv_from) && !MAIN_SSID_OP(net_dev)) {
		/*printk("FWD Module: wifi_fwd_rx_handler: No CLI link\n"); */
		return 2;
	}

	/* handle access schedule */
	if (WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE)) {
		if (IS_PACKET_FROM_APCLI(recv_from)) {
			if (wifi_fwd_check_and_forward(skb) == FALSE)
				return 2;
		}
	}

	/* forward the packet to bridge no matter unicast or broadcast */
	if ((br0->dev_addr != NULL) && MAC_ADDR_EQUAL(mh->h_dest, br0->dev_addr))
		return 1;

	/*
	*	in FastLane topology:
	*	1. forward Rx packets to bridge and let flooding work done by bridge
	*	2. no matter unicast or multicast/broadcast
	*	or in MBSS support
	*/
	if (is_fastlane_mode(net_dev) || is_mbss_mode(band_from))
		return 1;

	idx = wifi_fwd_find_device(net_dev);
	if (idx < 0) {
		WF_FWD_PRINT(WF_DEBUG_TRACE, ("[%s][%d] cannot find device information\n", __func__, __LINE__));
		return 1;
	}

	device_info = FwdDevice + idx;

	/*
	*	prevent from looping
	*	0: no looping
	*	1: looping found and need to drop the packet
	*	2: forward to bridge and handle
	*/
	ret = wifi_fwd_check_looping(skb, device_info);
	if (ret == 1)
		return 2;
	else if (ret == 2)
		return 1;

	/* after checking looping, handle multicast/broadcast Rx */
	if ((mh->h_dest[0] & 0x1) == 0x1) {
		/* report bpdu following ether traffic band to avoid apcliX's STP state as blocking on apcliX while dual band */
		if (WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_BPDU_ACTIVE)) {
			if (IS_PACKET_FROM_APCLI(recv_from) && !IS_PACKET_FROM_REP_APCLI(recv_from, rep_net_dev)
			    && (!memcmp(mh->h_dest, bpdu_group_address, ETH_ALEN)))
				return 2;
		}

		return 1;
	}

	/* handle hijack */
	if (WIFI_FWD_TEST_FLAG(fOP_WIFI_FWD_HIJACK_ACTIVE)) {
		if (wifi_fwd_check_and_forward(skb) == TRUE) {
			WIFI_FWD_SET_PACKET_BAND(skb, WIFI_FWD_PACKET_SPECIFIC_TAG, band_cb_offset);
			return 1;
		}
	}

	/* forward the packet to bridge by comparing destination address @ concurrent mode */
	if (wifi_fwd_pkt_to_bridge_by_dest_addr(skb, net_dev))
		return 1;

	target = wifi_fwd_lookup_entry(net_dev, &type, skb);

	/* handle unicast Rx for non-FastLane cases */
	if (target != NULL) {
		/* prevent from looping */
		if (wifi_fwd_find_sa_entry_by_sa_and_nd(skb, net_dev, 0) != 0) {
			unsigned char hit = 0;

			/* forward to bridge if it is not a WiFi net device back-end packet */
			hit = wifi_fwd_check_from_bridge_by_dest_addr(skb, device_info);

			if (hit == 0) {
				/* this packet should be forwarded to bridge due to the destination is addessing to bridge or ethernet */
				if (is_concurrent_mode(net_dev) && IS_PACKET_FROM_ETHER(band_from))
					WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s][%d] this case is not reasonable\n", __func__, __LINE__));

				return 1;
			} else if (hit == 2) {
				/* match inner communication case */
				return 1;
			} else {
				/* this packet should be delivered to WiFi interface and transmitted directly */
				skb_push(skb, ETH_HLEN);
				skb->dev = target;
				dev_queue_xmit(skb);
				/*printk("wifi_fwd_rx_handler: dev_queue_xmit called directly\n");*/
				return 0;
			}

			return 1;
		} else {
			/*printk("FWD Module: wifi_fwd_rx_handler: entry not found in pkt_src database\n");*/
			return 2;
		}
	} else
		return 1;
}

static void wifi_fwd_set_easy_setup_mode(unsigned int easy_setup_mode)
{
	if (easy_setup_mode) {
		easy_setup_intf_count += 1;
	} else {
		easy_setup_intf_count -= 1;
	}

	WF_FWD_PRINT(WF_DEBUG_OFF, ("[%s] easy_setup_mode=%d easy_setup_intf_count=%d\n", __func__, easy_setup_mode, easy_setup_intf_count));
}
#if 1
static void wifi_fwd_func_table_init(struct wifi_fwd_func_table *Fptable)
{
Fptable->wf_fwd_tx_hook = wifi_fwd_tx_handler;
Fptable->wf_fwd_rx_hook = wifi_fwd_rx_handler;
Fptable->wf_fwd_entry_insert_hook = wifi_fwd_insert_entry;
Fptable->wf_fwd_entry_delete_hook = wifi_fwd_delete_entry;
Fptable->wf_fwd_set_cb_num  = wifi_fwd_set_cb_num;
Fptable->wf_fwd_set_debug_level_hook = wifi_fwd_set_debug_level;
Fptable->wf_fwd_check_active_hook = wifi_fwd_check_active;
Fptable->wf_fwd_get_rep_hook = wifi_fwd_get_rep;
Fptable->wf_fwd_pro_active_hook = wifi_fwd_pro_active;
Fptable->wf_fwd_pro_halt_hook = wifi_fwd_pro_halt;
Fptable->wf_fwd_pro_enabled_hook = wifi_fwd_pro_enabled;
Fptable->wf_fwd_pro_disabled_hook = wifi_fwd_pro_disabled;
Fptable->wf_fwd_access_schedule_active_hook = wifi_fwd_access_schedule_active;
Fptable->wf_fwd_access_schedule_halt_hook = wifi_fwd_access_schedule_halt;
Fptable->wf_fwd_hijack_active_hook = wifi_fwd_hijack_active;
Fptable->wf_fwd_hijack_halt_hook = wifi_fwd_hijack_halt;
Fptable->wf_fwd_bpdu_active_hook = wifi_fwd_bpdu_active;
Fptable->wf_fwd_bpdu_halt_hook = wifi_fwd_bpdu_halt;
Fptable->wf_fwd_show_entry_hook = wifi_fwd_show_entry;
Fptable->wf_fwd_needed_hook = wifi_fwd_needed;
Fptable->wf_fwd_delete_entry_hook = wifi_fwd_delete_entry_by_idx;
Fptable->packet_source_show_entry_hook = packet_source_show_entry;
Fptable->packet_source_delete_entry_hook = packet_source_delete_entry;
Fptable->wf_fwd_feedback_map_table = wifi_fwd_feedback_map_table;
Fptable->wf_fwd_probe_adapter = wifi_fwd_probe_adapter;
Fptable->wf_fwd_insert_bridge_mapping_hook = wifi_fwd_insert_bridge_mapping;
Fptable->wf_fwd_insert_repeater_mapping_hook = wifi_fwd_insert_repeater_mapping;
Fptable->wf_fwd_search_mapping_table_hook = wifi_fwd_search_mapping_table;
Fptable->wf_fwd_delete_entry_inform_hook = wf_fwd_delete_entry_inform;
Fptable->wf_fwd_check_device_hook = wifi_fwd_check_device;
Fptable->wf_fwd_add_entry_inform_hook = wf_fwd_add_entry_inform;
Fptable->wf_fwd_set_easy_setup_mode = wifi_fwd_set_easy_setup_mode;

}
#endif

static int wifi_fwd_init_mod(void)
{
	if (!wifi_fwd_alloc_tbl(WIFI_FWD_TBL_SIZE))
		return -ENOMEM; /* memory allocation failed */

	if (!wifi_fwd_alloc_device_info(WIFI_FWD_DEVICE_SIZE))
		return -ENOMEM; /* memory allocation failed */

	WIFI_FWD_CLEAR_FLAG(fOP_GET_NET_DEVICE_STATUS_DONE);
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_ACCESS_SCHED_ACTIVE);
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_ENABLED);
	WIFI_FWD_SET_FLAG(fOP_WIFI_FWD_ACTIVE);
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_HIJACK_ACTIVE);
	WIFI_FWD_CLEAR_FLAG(fOP_WIFI_FWD_BPDU_ACTIVE);

	wifi_fwd_reset_link_count();

	eth_rep5g_wrg_uni_cnt = 0;
	eth_rep5g_wrg_bct_cnt = 0;
	eth_rep2g_wrg_uni_cnt = 0;
	eth_rep2g_wrg_bct_cnt = 0;
	rep_net_dev = eth_traffic_band_5g;
	next_2g_band = band_5g;
	band_cb_offset = DEFAULT_BAND_CB_OFFSET;
	recv_from_cb_offset = DEFAULT_RECV_FROM_CB_OFFSET;
	wf_debug_level = WF_DEBUG_OFF;
	easy_setup_intf_count = 0;

	wf_fwd_tx_hook = wifi_fwd_tx_handler;
	wf_fwd_rx_hook = wifi_fwd_rx_handler;
	wf_fwd_entry_insert_hook = wifi_fwd_insert_entry;
	wf_fwd_entry_delete_hook = wifi_fwd_delete_entry;
	wf_fwd_set_cb_num  = wifi_fwd_set_cb_num;
	wf_fwd_set_debug_level_hook = wifi_fwd_set_debug_level;
	wf_fwd_check_active_hook = wifi_fwd_check_active;
	wf_fwd_get_rep_hook = wifi_fwd_get_rep;
	wf_fwd_pro_active_hook = wifi_fwd_pro_active;
	wf_fwd_pro_halt_hook = wifi_fwd_pro_halt;
	wf_fwd_pro_enabled_hook = wifi_fwd_pro_enabled;
	wf_fwd_pro_disabled_hook = wifi_fwd_pro_disabled;
	wf_fwd_access_schedule_active_hook = wifi_fwd_access_schedule_active;
	wf_fwd_access_schedule_halt_hook = wifi_fwd_access_schedule_halt;
	wf_fwd_hijack_active_hook = wifi_fwd_hijack_active;
	wf_fwd_hijack_halt_hook = wifi_fwd_hijack_halt;
	wf_fwd_bpdu_active_hook = wifi_fwd_bpdu_active;
	wf_fwd_bpdu_halt_hook = wifi_fwd_bpdu_halt;
	wf_fwd_show_entry_hook = wifi_fwd_show_entry;
	wf_fwd_needed_hook = wifi_fwd_needed;
	wf_fwd_delete_entry_hook = wifi_fwd_delete_entry_by_idx;
	packet_source_show_entry_hook = packet_source_show_entry;
	packet_source_delete_entry_hook = packet_source_delete_entry;
	wf_fwd_feedback_map_table = wifi_fwd_feedback_map_table;
	wf_fwd_probe_adapter = wifi_fwd_probe_adapter;
	wf_fwd_insert_bridge_mapping_hook = wifi_fwd_insert_bridge_mapping;
	wf_fwd_insert_repeater_mapping_hook = wifi_fwd_insert_repeater_mapping;
	wf_fwd_search_mapping_table_hook = wifi_fwd_search_mapping_table;
	wf_fwd_delete_entry_inform_hook = wf_fwd_delete_entry_inform;
	wf_fwd_check_device_hook = wifi_fwd_check_device;
	wf_fwd_add_entry_inform_hook = wf_fwd_add_entry_inform;
	wf_fwd_set_easy_setup_mode = wifi_fwd_set_easy_setup_mode;
	wifi_fwd_func_table_init(&wf_fp_tbl);
	wifi_fwd_register(&wf_fp_tbl);	
	return 0;
}
static void wifi_fwd_cleanup_mod(void)
{
	kfree(WiFiFwdBase);
	WiFiFwdBase = NULL;

	kfree(FwdDevice);
	FwdDevice = NULL;

	kfree(pkt_src);
	pkt_src = NULL;

	kfree(tx_src_tbl);
	tx_src_tbl = NULL;

	wf_fwd_tx_hook = NULL;
	wf_fwd_rx_hook = NULL;
	wf_fwd_entry_insert_hook = NULL;
	wf_fwd_entry_delete_hook = NULL;
	wf_fwd_set_cb_num = NULL;
	wf_fwd_set_debug_level_hook = NULL;
	wf_fwd_check_active_hook = NULL;
	wf_fwd_get_rep_hook = NULL;
	wf_fwd_pro_active_hook = NULL;
	wf_fwd_pro_halt_hook = NULL;
	wf_fwd_pro_enabled_hook = NULL;
	wf_fwd_pro_disabled_hook = NULL;
	wf_fwd_access_schedule_active_hook = NULL;
	wf_fwd_access_schedule_halt_hook = NULL;
	wf_fwd_hijack_active_hook = NULL;
	wf_fwd_hijack_halt_hook = NULL;
	wf_fwd_bpdu_active_hook = NULL;
	wf_fwd_bpdu_halt_hook = NULL;
	wf_fwd_show_entry_hook = NULL;
	wf_fwd_needed_hook = NULL;
	wf_fwd_delete_entry_hook = NULL;
	packet_source_show_entry_hook = NULL;
	packet_source_delete_entry_hook = NULL;
	wf_fwd_feedback_map_table = NULL;
	wf_fwd_probe_adapter = NULL;
	wf_fwd_insert_bridge_mapping_hook = NULL;
	wf_fwd_insert_repeater_mapping_hook = NULL;
	wf_fwd_search_mapping_table_hook = NULL;
	wf_fwd_delete_entry_inform_hook = NULL;
	wf_fwd_check_device_hook = NULL;
	wf_fwd_add_entry_inform_hook = NULL;
	wf_fwd_debug_level_hook = NULL;

	return;
}

module_init(wifi_fwd_init_mod);
module_exit(wifi_fwd_cleanup_mod);

MODULE_AUTHOR("MediaTek Inc");
MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek WiFi Packet Forwarding Module\n");

