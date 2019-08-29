/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _VDEC_IPI_MSG_H_
#define _VDEC_IPI_MSG_H_

#define MTK_MAX_DEC_CODECS_SUPPORT       (128)
#define DEC_MAX_FB_NUM              VIDEO_MAX_FRAME
#define DEC_MAX_BS_NUM              VIDEO_MAX_FRAME
/**
 * enum vdec_src_chg_type - decoder src change type
 * @VDEC_NO_CHANGE      : no change
 * @VDEC_RES_CHANGE     : resolution change
 * @VDEC_REALLOC_MV_BUF : realloc mv buf
 * @VDEC_HW_NOT_SUPPORT : hw not support
 * @VDEC_NEED_MORE_OUTPUT_BUF: bs need more fm buffer to decode
 *          kernel should send the same bs buffer again with new fm buffer
 * @VDEC_CROP_CHANGED: notification to update frame crop info
 */
enum vdec_src_chg_type {
	VDEC_NO_CHANGE              = (0 << 0),
	VDEC_RES_CHANGE             = (1 << 0),
	VDEC_REALLOC_MV_BUF         = (1 << 1),
	VDEC_HW_NOT_SUPPORT         = (1 << 2),
	VDEC_NEED_SEQ_HEADER        = (1 << 3),
	VDEC_NEED_MORE_OUTPUT_BUF   = (1 << 4),
	VDEC_CROP_CHANGED           = (1 << 5),
};

/*
 * enum vdec_set_param_type -
 *                  The type of set parameter used in vdec_if_set_param()
 * (VCU related: If you change the order, you must also update the VCU codes.)
 * SET_PARAM_DECODE_MODE: set decoder mode
 * SET_PARAM_FRAME_SIZE: set container frame size
 * SET_PARAM_SET_FIXED_MAX_OUTPUT_BUFFER: set fixed maximum buffer size
 * SET_PARAM_UFO_MODE: set UFO mode
 * SET_PARAM_CRC_PATH: set CRC path used for UT
 * SET_PARAM_GOLDEN_PATH: set Golden YUV path used for UT
 * SET_PARAM_FB_NUM_PLANES                      : frame buffer plane count
 */
enum vdec_set_param_type {
	SET_PARAM_DECODE_MODE,
	SET_PARAM_FRAME_SIZE,
	SET_PARAM_SET_FIXED_MAX_OUTPUT_BUFFER,
	SET_PARAM_UFO_MODE,
	SET_PARAM_CRC_PATH,
	SET_PARAM_GOLDEN_PATH,
	SET_PARAM_FB_NUM_PLANES,
	SET_PARAM_WAIT_KEY_FRAME,
	SET_PARAM_NAL_SIZE_LENGTH,
	SET_PARAM_OPERATING_RATE,
	SET_PARAM_FREE_FRAME_BUFQ_COUNT,
	SET_PARAM_TOTAL_FRAME_BUFQ_COUNT
};

/**
 * enum vdec_ipi_msgid - message id between AP and VPU
 * @AP_IPIMSG_XXX	: AP to VPU cmd message id
 * @VPU_IPIMSG_XXX_ACK	: VPU ack AP cmd message id
 */
enum vdec_ipi_msgid {
	AP_IPIMSG_DEC_INIT = 0xA000,
	AP_IPIMSG_DEC_START = 0xA001,
	AP_IPIMSG_DEC_END = 0xA002,
	AP_IPIMSG_DEC_DEINIT = 0xA003,
	AP_IPIMSG_DEC_RESET = 0xA004,
	AP_IPIMSG_DEC_SET_PARAM = 0xA005,
	AP_IPIMSG_DEC_QUERY_CAP = 0xA006,

	VPU_IPIMSG_DEC_INIT_ACK = 0xB000,
	VPU_IPIMSG_DEC_START_ACK = 0xB001,
	VPU_IPIMSG_DEC_END_ACK = 0xB002,
	VPU_IPIMSG_DEC_DEINIT_ACK = 0xB003,
	VPU_IPIMSG_DEC_RESET_ACK = 0xB004,
	VPU_IPIMSG_DEC_SET_PARAM_ACK = 0xB005,
	VPU_IPIMSG_DEC_QUERY_CAP_ACK = 0xB006,

	VPU_IPIMSG_DEC_WAITISR = 0xC000,
	VPU_IPIMSG_DEC_GET_FRAME_BUFFER = 0xC001,
	VPU_IPIMSG_DEC_CLOCK_ON = 0xC002,
	VPU_IPIMSG_DEC_CLOCK_OFF = 0xC003
};

/* For GET_PARAM_DISP_FRAME_BUFFER and GET_PARAM_FREE_FRAME_BUFFER,
 * the caller does not own the returned buffer. The buffer will not be
 *                              released before vdec_if_deinit.
 * GET_PARAM_DISP_FRAME_BUFFER  : get next displayable frame buffer,
 *                              struct vdec_fb**
 * GET_PARAM_FREE_FRAME_BUFFER  : get non-referenced framebuffer, vdec_fb**
 * GET_PARAM_PIC_INFO           : get picture info, struct vdec_pic_info*
 * GET_PARAM_CROP_INFO          : get crop info, struct v4l2_crop*
 * GET_PARAM_DPB_SIZE           : get dpb size, __s32*
 * GET_PARAM_FRAME_INTERVAL     : get frame interval info*
 * GET_PARAM_ERRORMB_MAP        : get error mocroblock when decode error*
 * GET_PARAM_CAPABILITY_SUPPORTED_FORMATS: get codec supported format capability
 * GET_PARAM_CAPABILITY_FRAME_SIZES:
 *                       get codec supported frame size & alignment info
 */
enum vdec_get_param_type {
	GET_PARAM_DISP_FRAME_BUFFER,
	GET_PARAM_FREE_FRAME_BUFFER,
	GET_PARAM_FREE_BITSTREAM_BUFFER,
	GET_PARAM_PIC_INFO,
	GET_PARAM_CROP_INFO,
	GET_PARAM_DPB_SIZE,
	GET_PARAM_FRAME_INTERVAL,
	GET_PARAM_ERRORMB_MAP,
	GET_PARAM_CAPABILITY_SUPPORTED_FORMATS,
	GET_PARAM_CAPABILITY_FRAME_SIZES,
	GET_PARAM_COLOR_DESC,
	GET_PARAM_ASPECT_RATIO,
	GET_PARAM_PLATFORM_SUPPORTED_FIX_BUFFERS,
	GET_PARAM_PLATFORM_SUPPORTED_FIX_BUFFERS_SVP,
	GET_PARAM_INTERLACING,
	GET_PARAM_CODEC_TYPE
};
/**
 * struct vdec_ap_ipi_cmd - generic AP to VPU ipi command format
 * @msg_id	: vdec_ipi_msgid
 * @vpu_inst_addr	: VPU decoder instance address
 */
struct vdec_ap_ipi_cmd {
	uint32_t msg_id;
	uint32_t vpu_inst_addr;
};

/**
 * struct vdec_vpu_ipi_ack - generic VPU to AP ipi command format
 * @msg_id	: vdec_ipi_msgid
 * @status	: VPU exeuction result
 * @ap_inst_addr	: AP video decoder instance address
 */
struct vdec_vpu_ipi_ack {
	uint32_t msg_id;
	int32_t status;
	uint64_t ap_inst_addr;
};

/**
 * struct vdec_ap_ipi_init - for AP_IPIMSG_DEC_INIT
 * @msg_id	: AP_IPIMSG_DEC_INIT
 * @reserved	: Reserved field
 * @ap_inst_addr	: AP video decoder instance address
 */
struct vdec_ap_ipi_init {
	uint32_t msg_id;
	uint32_t reserved;
	uint64_t ap_inst_addr;
};

/**
 * struct vdec_ap_ipi_dec_start - for AP_IPIMSG_DEC_START
 * @msg_id	: AP_IPIMSG_DEC_START
 * @vpu_inst_addr	: VPU decoder instance address
 * @data	: Header info
 *	H264 decoder [0]:buf_sz [1]:nal_start
 *	VP8 decoder  [0]:width/height
 *	VP9 decoder  [0]:profile, [1][2] width/height
 * @reserved	: Reserved field
 */
struct vdec_ap_ipi_dec_start {
	uint32_t msg_id;
	uint32_t vpu_inst_addr;
	uint32_t data[3];
	uint32_t reserved;
};

/**
 * struct vdec_vpu_ipi_init_ack - for VPU_IPIMSG_DEC_INIT_ACK
 * @msg_id	: VPU_IPIMSG_DEC_INIT_ACK
 * @status	: VPU exeuction result
 * @ap_inst_addr	: AP vcodec_vpu_inst instance address
 * @vpu_inst_addr	: VPU decoder instance address
 */
struct vdec_vpu_ipi_init_ack {
	uint32_t msg_id;
	int32_t status;
	uint64_t ap_inst_addr;
	uint32_t vpu_inst_addr;
};
/**
 * struct vdec_ap_ipi_set_param - for AP_IPIMSG_DEC_SET_PARAM
 * @msg_id        : AP_IPIMSG_DEC_SET_PARAM
 * @vpu_inst_addr : VPU decoder instance address
 * @id            : set param  type
 * @data          : param data
 */
struct vdec_ap_ipi_set_param {
	uint32_t msg_id;
	uint32_t vpu_inst_addr;
	uint32_t id;
	uint32_t data[8];
};

/**
 * struct vdec_ap_ipi_query_cap - for AP_IPIMSG_DEC_QUERY_CAP
 * @msg_id        : AP_IPIMSG_DEC_QUERY_CAP
 * @id      : query capability type
 * @vdec_inst     : AP query data address
 */
struct vdec_ap_ipi_query_cap {
	uint32_t msg_id;
	uint32_t id;
#ifndef CONFIG_64BIT
	union {
		uint64_t ap_inst_addr_64;
		uint32_t ap_inst_addr;
	};
	union {
		uint64_t ap_data_addr_64;
		uint32_t ap_data_addr;
	};
#else
	uint64_t ap_inst_addr;
	uint64_t ap_data_addr;
#endif
};

/**
 * struct vdec_vpu_ipi_query_cap_ack - for VCU_IPIMSG_DEC_QUERY_CAP_ACK
 * @msg_id      : VPU_IPIMSG_DEC_QUERY_CAP_ACK
 * @status      : VPU exeuction result
 * @ap_data_addr   : AP query data address
 * @vpu_data_addr  : VCU query data address
 */
struct vdec_vpu_ipi_query_cap_ack {
	uint32_t msg_id;
	int32_t status;
#ifndef CONFIG_64BIT
	union {
		uint64_t ap_inst_addr_64;
		uint32_t ap_inst_addr;
	};
	uint32_t id;
	union {
		uint64_t ap_data_addr_64;
		uint32_t ap_data_addr;
	};
#else
	uint64_t ap_inst_addr;
	uint32_t id;
	uint64_t ap_data_addr;
#endif
	uint32_t vpu_data_addr;
};
/*
 * struct vdec_ipi_fb - decoder frame buffer information
 * @vdec_fb_va  : virtual address of struct vdec_fb
 * @y_fb_dma    : dma address of Y frame buffer
 * @c_fb_dma    : dma address of C frame buffer
 * @poc         : picture order count of frame buffer
 * @reserved    : for 8 bytes alignment
 */
struct vdec_ipi_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};

/**
 * struct ring_bs_list - ring bitstream buffer list
 * @vdec_bs_va_list   : bitstream buffer arrary
 * @read_idx  : read index
 * @write_idx : write index
 * @count	  : buffer count in list
 */
struct ring_bs_list {
	uint64_t vdec_bs_va_list[DEC_MAX_BS_NUM];
	int32_t read_idx;
	int32_t write_idx;
	int32_t count;
	int32_t reserved;
};

/**
 * struct ring_fb_list - ring frame buffer list
 * @fb_list   : frame buffer arrary
 * @read_idx  : read index
 * @write_idx : write index
 * @count     : buffer count in list
 */
struct ring_fb_list {
	struct vdec_ipi_fb fb_list[DEC_MAX_FB_NUM];
	int32_t read_idx;
	int32_t write_idx;
	int32_t count;
	int32_t reserved;
};
/**
 * struct vdec_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @vdec_changed_info  : some changed flags
 * @bs_dma		: Input bit-stream buffer dma address
 * @bs_fd               : Input bit-stream buffer dmabuf fd
 * @fb_dma		: Y frame buffer dma address
 * @fb_fd             : Y frame buffer dmabuf fd
 * @vdec_bs_va		: VDEC bitstream buffer struct virtual address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 * @fb_num_planes	: frame buffer plane count
 * @reserved		: reserved variable for 64bit align
 */
struct vdec_dec_info {
	__u32 dpb_sz;
	__u32 vdec_changed_info;
	__u64 bs_dma;
	__u64 bs_fd;
	__u64 fb_dma[VIDEO_MAX_PLANES];
	__u64 fb_fd[VIDEO_MAX_PLANES];
	__u64 vdec_bs_va;
	__u64 vdec_fb_va;
	__u32 fb_num_planes;
	__u32 index;
	__u32 wait_key_frame;
	__u32 error_map;
};

struct mtk_color_desc {
	__u32	full_range;
	__u32	color_primaries;
	__u32	transform_character;
	__u32	matrix_coeffs;
	__u32	display_primaries_x[3];
	__u32	display_primaries_y[3];
	__u32	white_point_x;
	__u32	white_point_y;
	__u32	max_display_mastering_luminance;
	__u32	min_display_mastering_luminance;
	__u32	max_content_light_level;
	__u32	max_pic_light_level;
	__u32	is_hdr;
};
/**
 * struct vdec_vsi - shared memory for decode information exchange
 *                        between VCU and Host.
 *                        The memory is allocated by VCU and mapping to Host
 *                        in vpu_dec_init()
 * @ppl_buf_dma : HW working buffer ppl dma address
 * @mv_buf_dma  : HW working buffer mv dma address
 * @list_free   : free frame buffer ring list
 * @list_disp   : display frame buffer ring list
 * @dec         : decode information
 * @pic         : picture information
 * @crop        : crop information
 * @video_formats        : codec supported format info
 * @vdec_framesizes    : codec supported resolution info
 */
struct vdec_vsi {
	struct ring_bs_list list_free_bs;
	struct ring_fb_list list_free;
	struct ring_fb_list list_disp;
	struct vdec_dec_info dec;
	struct vdec_pic_info pic;
	struct mtk_color_desc color_desc;
	struct v4l2_rect crop;
	struct mtk_video_fmt video_formats[MTK_MAX_DEC_CODECS_SUPPORT];
	struct mtk_codec_framesizes vdec_framesizes[MTK_MAX_DEC_CODECS_SUPPORT];
	uint32_t aspect_ratio;
	uint32_t fix_buffers;
	uint32_t fix_buffers_svp;
	uint32_t interlacing;
	uint32_t codec_type;
	uint8_t crc_path[256];
	uint8_t golden_path[256];
};
#endif
