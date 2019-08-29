// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Cheng-Jung Ho <cheng-jung.ho@mediatek.com>
 */

#include "vcodec_bw.h"

bool validate_bw(struct vcodec_bw *bw, int type)
{
	if (bw == 0)
		return false;

	if (bw->smi_bw_mon[type] == 0xFFFFFFFF)
		return false;
	else
		return true;
}

struct vcodec_bw *find_bw_by_id(struct vcodec_bw *bw_list, int id)
{
	struct vcodec_bw *cur_bw = bw_list;

	if (cur_bw == 0)
		return 0;

	while (cur_bw->id != id) {
		cur_bw = cur_bw->next;
		if (cur_bw == 0)
			return 0;
	}

	return cur_bw;
}

struct vcodec_bw *add_bw_by_id(struct vcodec_bw **bw_list, int id)
{
	struct vcodec_bw *bw = 0;

	bw = find_bw_by_id(*bw_list, id);

	if (bw == 0) {
		bw = kmalloc(sizeof(struct vcodec_bw), GFP_KERNEL);
		if (bw == 0)
			return 0;
		memset(bw, 0, sizeof(struct vcodec_bw));
		bw->id = id;
		bw->next = *bw_list;
		*bw_list = bw;
	}

	if (*bw_list == 0)
		*bw_list = bw;

	return bw;
}

struct vcodec_bw *remove_bw_by_id(struct vcodec_bw **bw_list, int id)
{
	struct vcodec_bw *cur_bw = *bw_list;
	struct vcodec_bw *next_bw = 0;

	if (cur_bw == 0)
		return 0;

	if (cur_bw->id == id) {
		*bw_list = cur_bw->next;
		return cur_bw;
	}

	while (cur_bw->next != 0 && cur_bw->next->id != id)
		cur_bw = cur_bw->next;

	if (cur_bw->next == 0)
		return 0;

	if (cur_bw->next->id == id) {
		next_bw = cur_bw->next;
		cur_bw->next = next_bw->next;
	}
	return next_bw;
}

void free_all_bw(struct vcodec_bw **bw_list)
{
	struct vcodec_bw *bw = 0;

	while (*bw_list != 0) {
		bw = *bw_list;
		*bw_list = bw->next;
		kfree(bw);
	}
}
