#ifndef _DECODER_H
#define _DECODER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>


#include "dec_buf.h"
#include "nvdec.h"


#define data_abs(a,b) ((a>=b)?(a-b):(b-a))
typedef struct decoder_st *decoder_t;
typedef struct rtpPkg_st  *rtpPkg_t;
typedef struct rtspChnStatus_st *rtspChnStatus_t;
typedef struct decEnv_st *decEnv_t; 
typedef struct HEVCPPS_st *HEVCPPS_t;
#define PKG_LIST_LEN  (6)

#define FILE_RECORD_EN 
#undef  FILE_RECORD_EN
typedef enum{PKG_BAD=-1,PKG_MIN=0,PKG_TXT=1} PKG_TYPE;

#define  VDEC_CHN_NUM_25 (25)
#define  VDEC_CHN_NUM_16 (16)
#define  VDEC_CHN_NUM_4 (4)
#define  VDEC_CHN_NUM_2 (2)
#define  VDEC_CHN_NUM_1 (1)

#define CHNS (VDEC_CHN_NUM_1)


extern decEnv_t  decEnv;
struct decoder_st{
	int 	id;
    context_t ctx;
	unsigned int timestamp;
	unsigned long long time40ms;
	unsigned short last_seqc;
	//VDEC_STREAM_S decStream;
	dec_buf_t 	buf;
	//rtpPkg_t   rtpPkgList;
	pthread_mutex_t decLock;

	int slice_I_counter;
	int slice_P_counter;
	HEVCPPS_t hevcPPS;
	unsigned char EN_PPS;
	unsigned char EN_VPS;
	unsigned char EN_SPS;
	unsigned PKG_STARTED:1;			//BUFER 是否接收到了  包头
	unsigned err:1;  				//表示当前画面为无视频状态，当有视频流来的时候，应该清除该标志
	unsigned startRcv:1;			//表示开始接收码流
	unsigned refused:1;				//表示此时不接受视频
	unsigned waitIfream:1;
};



struct HEVCPPS_st {
    unsigned int sps_id; ///< seq_parameter_set_id

    uint8_t sign_data_hiding_flag;

    uint8_t cabac_init_present_flag;

    int num_ref_idx_l0_default_active; ///< num_ref_idx_l0_default_active_minus1 + 1
    int num_ref_idx_l1_default_active; ///< num_ref_idx_l1_default_active_minus1 + 1
    int pic_init_qp_minus26;

    uint8_t constrained_intra_pred_flag;
    uint8_t transform_skip_enabled_flag;

    uint8_t cu_qp_delta_enabled_flag;
    int diff_cu_qp_delta_depth;

    int cb_qp_offset;
    int cr_qp_offset;
    uint8_t pic_slice_level_chroma_qp_offsets_present_flag;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_flag;
    uint8_t output_flag_present_flag;
    uint8_t transquant_bypass_enable_flag;

    uint8_t dependent_slice_segments_enabled_flag;
    uint8_t tiles_enabled_flag;
    uint8_t entropy_coding_sync_enabled_flag;

    int num_tile_columns;   ///< num_tile_columns_minus1 + 1
    int num_tile_rows;      ///< num_tile_rows_minus1 + 1
    uint8_t uniform_spacing_flag;
    uint8_t loop_filter_across_tiles_enabled_flag;

    uint8_t seq_loop_filter_across_slices_enabled_flag;

    uint8_t deblocking_filter_control_present_flag;
    uint8_t deblocking_filter_override_enabled_flag;
    uint8_t m_inferScalingListFlag;
    int m_scalingListRefLayerId; 
    uint8_t disable_dbf;
    int8_t beta_offset;    ///< beta_offset_div2 * 2
    int8_t tc_offset;      ///< tc_offset_div2 * 2

    uint8_t scaling_list_data_present_flag;
    //ScalingList scaling_list;

    uint8_t lists_modification_present_flag;
    int log2_parallel_merge_level; ///< log2_parallel_merge_level_minus2 + 2
    int num_extra_slice_header_bits;
    uint8_t slice_header_extension_present_flag;
    uint8_t log2_max_transform_skip_block_size;
    uint8_t cross_component_prediction_enabled_flag;
    uint8_t chroma_qp_offset_list_enabled_flag;
    uint8_t diff_cu_chroma_qp_offset_depth;
    uint8_t chroma_qp_offset_list_len_minus1;
    int8_t  cb_qp_offset_list[5];
    int8_t  cr_qp_offset_list[5];
    uint8_t log2_sao_offset_scale_luma;
    uint8_t log2_sao_offset_scale_chroma;


    // Inferred parameters
    unsigned int *column_width;  ///< ColumnWidth
    unsigned int *row_height;    ///< RowHeight
    int *col_idxX;

    int *ctb_addr_rs_to_ts; ///< CtbAddrRSToTS
    int *ctb_addr_ts_to_rs; ///< CtbAddrTSToRS
    int *ctb_row_to_rs;
    int *tile_id;           ///< TileId
    int *tile_width;           ///< TileWidth
    int *tile_pos_rs;       ///< TilePosRS
    int *min_tb_addr_zs;    ///< MinTbAddrZS
    int *min_tb_addr_zs_tab;///< MinTbAddrZS

};

struct rtpPkg_st
{
	char magic[8];
	int cmd;
	int len;
	u_char data[1600];
};

struct rtspChnStatus_st{ //和RTSPclient通信的接口，当不是RTSP数据包的时候就会出现这个包
	int id;
	int err;
	int update; //表示该通道切换了视频源，可以进行reset操作
	int subwin;
};


struct decEnv_st{
	decoder_t decs;
};

decEnv_t create_dec_chns(void);

void free_dec_chns(decEnv_t);

#endif
