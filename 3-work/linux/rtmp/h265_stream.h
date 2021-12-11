#ifndef _H265_STREAM_H
#define _H265_STREAM_H

//sampile: https://github.com/latelee/H264BSAnalyzer/blob/master/H264BSAnalyzer/h264_stream.h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    int nal_to_rbsp(const int nal_header_size, const uint8_t* nal_buf, int* nal_size, uint8_t* rbsp_buf, int* rbsp_size);

    typedef struct HEVCDecoderConfigurationRecord
    {
        uint8_t configurationVersion;
        uint8_t general_profile_space;
        uint8_t general_tier_flag;
        uint8_t general_profile_idc;
        uint32_t general_profile_compatibility_flags;
        uint64_t general_constraint_indicator_flags;
        uint8_t general_level_idc;
        uint16_t min_spatial_segmentation_idc;
        uint8_t parallelismType;
        uint8_t chromaFormat;
        uint8_t bitDepthLumaMinus8;
        uint8_t bitDepthChromaMinus8;
        uint16_t avgFrameRate;
        uint8_t constantFrameRate;
        uint8_t numTemporalLayers;
        uint8_t temporalIdNested;
        uint8_t lengthSizeMinusOne;
        uint8_t numOfArrays;
        //    HVCCNALUnitArray *array;
    } HEVCDecoderConfigurationRecord;

    int ff_buff_write_hvcc(uint8_t* buff,
        unsigned char* pps, int pps_len,
        unsigned char* sps, int sps_len,
        unsigned char* vps, int vps_len);

#define HEVC_MAX_SUB_LAYERS 7

    /**
   Profile, tier and level
   @see 7.3.3 Profile, tier and level syntax
*/
    typedef struct
    {
        uint8_t general_profile_space;
        uint8_t general_tier_flag;
        uint8_t general_profile_idc;
        uint8_t general_profile_compatibility_flag[32];
        uint8_t general_progressive_source_flag;
        uint8_t general_interlaced_source_flag;
        uint8_t general_non_packed_constraint_flag;
        uint8_t general_frame_only_constraint_flag;
        uint8_t general_max_12bit_constraint_flag;
        uint8_t general_max_10bit_constraint_flag;
        uint8_t general_max_8bit_constraint_flag;
        uint8_t general_max_422chroma_constraint_flag;
        uint8_t general_max_420chroma_constraint_flag;
        uint8_t general_max_monochrome_constraint_flag;
        uint8_t general_intra_constraint_flag;
        uint8_t general_one_picture_only_constraint_flag;
        uint8_t general_lower_bit_rate_constraint_flag;
        uint64_t general_reserved_zero_34bits; // todo
        uint64_t general_reserved_zero_43bits; // todo
        uint8_t general_inbld_flag;
        uint8_t general_reserved_zero_bit;
        uint8_t general_level_idc;
        ///      
        uint8_t sub_layer_profile_present_flag[HEVC_MAX_SUB_LAYERS];
        uint8_t sub_layer_level_present_flag[HEVC_MAX_SUB_LAYERS];
        uint8_t reserved_zero_2bits[8];

#if 1
        uint8_t sub_layer_profile_space[7];
        uint8_t sub_layer_tier_flag[7];
        uint8_t sub_layer_profile_idc[7];

        uint8_t sub_layer_profile_compatibility_flag[7][32];

        uint8_t sub_layer_progressive_source_flag[7];
        uint8_t sub_layer_interlaced_source_flag[7];
        uint8_t sub_layer_non_packed_constraint_flag[7];
        uint8_t sub_layer_frame_only_constraint_flag[7];
        uint8_t sub_layer_max_12bit_constraint_flag[7];
        uint8_t sub_layer_max_10bit_constraint_flag[7];
        uint8_t sub_layer_max_8bit_constraint_flag[7];
        uint8_t sub_layer_max_422chroma_constraint_flag[7];
        uint8_t sub_layer_max_420chroma_constraint_flag[7];
        uint8_t sub_layer_max_monochrome_constraint_flag[7];
        uint8_t sub_layer_intra_constraint_flag[7];
        uint8_t sub_layer_one_picture_only_constraint_flag[7];
        uint8_t sub_layer_lower_bit_rate_constraint_flag[7];
        uint64_t sub_layer_reserved_zero_34bits[7];
        uint64_t sub_layer_reserved_zero_43bits[7];
        uint8_t sub_layer_inbld_flag[7];
        uint8_t sub_layer_reserved_zero_bit[7];
        uint8_t sub_layer_level_idc[7];
#endif

    } profile_tier_level_t;

/**
7.3.4  Scaling list data syntax
*/
    typedef struct
    {
        int scaling_list_pred_mode_flag[4][6];
        int scaling_list_pred_matrix_id_delta[4][6];
        int scaling_list_dc_coef_minus8[4][6];
        int ScalingList[4][6][64];
        int coefNum;
    } scaling_list_data_t;

    /**
       Sequence Parameter Set
       @see 7.3.2.2 Sequence parameter set RBSP syntax
    */
    typedef struct
    {
        uint8_t sps_video_parameter_set_id;
        uint8_t sps_max_sub_layers_minus1;
        uint8_t sps_temporal_id_nesting_flag;
        profile_tier_level_t ptl;
        int sps_seq_parameter_set_id;
        int chroma_format_idc;
        uint8_t separate_colour_plane_flag;
        int pic_width_in_luma_samples;
        int pic_height_in_luma_samples;
        int conformance_window_flag;
        int conf_win_left_offset;
        int conf_win_right_offset;
        int conf_win_top_offset;
        int conf_win_bottom_offset;
        int bit_depth_luma_minus8;
        int bit_depth_chroma_minus8;
        int log2_max_pic_order_cnt_lsb_minus4;
        uint8_t sps_sub_layer_ordering_info_present_flag;
        int sps_max_dec_pic_buffering_minus1[8]; // max u(3)
        int sps_max_num_reorder_pics[8];
        int sps_max_latency_increase_plus1[8];
        int log2_min_luma_coding_block_size_minus3;
        int log2_diff_max_min_luma_coding_block_size;
        int log2_min_luma_transform_block_size_minus2;
        int log2_diff_max_min_luma_transform_block_size;
        int max_transform_hierarchy_depth_inter;
        int max_transform_hierarchy_depth_intra;
        uint8_t scaling_list_enabled_flag;
        uint8_t sps_infer_scaling_list_flag;
        int sps_scaling_list_ref_layer_id;
        int sps_scaling_list_data_present_flag;
        scaling_list_data_t scaling_list_data;
        uint8_t amp_enabled_flag;
        uint8_t sample_adaptive_offset_enabled_flag;
        uint8_t pcm_enabled_flag;
        uint8_t pcm_sample_bit_depth_luma_minus1;
        uint8_t pcm_sample_bit_depth_chroma_minus1;
        int log2_min_pcm_luma_coding_block_size_minus3;
        int log2_diff_max_min_pcm_luma_coding_block_size;
        uint8_t pcm_loop_filter_disabled_flag;
        int num_short_term_ref_pic_sets;
#if 0
        vector<st_ref_pic_set_t> st_ref_pic_set;
        vector<referencePictureSets_t> m_RPSList; // store
        uint8_t long_term_ref_pics_present_flag;
        int num_long_term_ref_pics_sps;
        int lt_ref_pic_poc_lsb_sps_bytes;
        vector<int> lt_ref_pic_poc_lsb_sps;
        vector<uint8_t> used_by_curr_pic_lt_sps_flag;
        uint8_t sps_temporal_mvp_enabled_flag;
        uint8_t strong_intra_smoothing_enabled_flag;
        uint8_t vui_parameters_present_flag;
        vui_parameters_t vui;
        uint8_t sps_extension_present_flag;
        uint8_t sps_range_extension_flag;
        uint8_t sps_multilayer_extension_flag;
        uint8_t sps_3d_extension_flag;
        uint8_t sps_extension_5bits;
        sps_range_extension_t sps_range_extension;
        uint8_t inter_view_mv_vert_constraint_flag; //sps_multilayer_extension_t sps_multilayer_extension;
        //sps_3d_extension_t sps_3d_extension;
        //int sps_extension_data_flag; // no need
        // rbsp_trailing_bits()...
#endif
    } h265_sps_t;


    int h265_decode_sps_t(h265_sps_t* sps, void* spsdata, int size, int* width, int* height);

    void h265_sps_deubg_out(h265_sps_t* sps);

#ifdef __cplusplus
}
#endif

#endif