#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h265_stream.h"
#include "bs.h"

 #define min(a,b) (((a) < (b)) ? (a) : (b))

// 7.3.4  Scaling list data syntax
void h265_read_scaling_list(scaling_list_data_t *sld, bs_t *b)
{
    for (int sizeId = 0; sizeId < 4; sizeId++)
    {
        for (int matrixId = 0; matrixId < 6; matrixId += (sizeId == 3) ? 3 : 1)
        {
            sld->scaling_list_pred_mode_flag[sizeId][matrixId] = bs_read_u1(b);
            if (!sld->scaling_list_pred_mode_flag[sizeId][matrixId])
            {
                sld->scaling_list_pred_matrix_id_delta[sizeId][matrixId] = bs_read_ue(b);
            }
            else
            {
                int nextCoef = 8;
                int coefNum = min(64, (1 << (4 + (sizeId << 1))));
                sld->coefNum = coefNum; // tmp store
                if (sizeId > 1)
                {
                    sld->scaling_list_dc_coef_minus8[sizeId - 2][matrixId] = bs_read_se(b);
                    nextCoef = sld->scaling_list_dc_coef_minus8[sizeId - 2][matrixId] + 8;
                }
                for (int i = 0; i < sld->coefNum; i++)
                {
                    int scaling_list_delta_coef = bs_read_se(b);
                    nextCoef = (nextCoef + scaling_list_delta_coef + 256) % 256;
                    sld->ScalingList[sizeId][matrixId][i] = nextCoef;
                }
            }
        }
    }
}

/// <summary>
///
/// </summary>
/// <param name="ptl"></param>
/// <param name="b"></param>
/// <param name="profilePresentFlag"></param>
/// <param name="max_sub_layers_minus1">max value 7</param>
void h265_read_ptl(profile_tier_level_t *ptl, bs_t *b, int profilePresentFlag, int max_sub_layers_minus1)
{
    int i = 0;
    if (profilePresentFlag)
    {
        ptl->general_profile_space = bs_read_u(b, 2);
        ptl->general_tier_flag = bs_read_u1(b);
        ptl->general_profile_idc = bs_read_u(b, 5);
        for (i = 0; i < 32; i++)
        {
            ptl->general_profile_compatibility_flag[i] = bs_read_u1(b);
        }
        ptl->general_progressive_source_flag = bs_read_u1(b);
        ptl->general_interlaced_source_flag = bs_read_u1(b);
        ptl->general_non_packed_constraint_flag = bs_read_u1(b);
        ptl->general_frame_only_constraint_flag = bs_read_u1(b);
        if (ptl->general_profile_idc == 4 || ptl->general_profile_compatibility_flag[4] ||
            ptl->general_profile_idc == 5 || ptl->general_profile_compatibility_flag[5] ||
            ptl->general_profile_idc == 6 || ptl->general_profile_compatibility_flag[6] ||
            ptl->general_profile_idc == 7 || ptl->general_profile_compatibility_flag[7])
        {
            ptl->general_max_12bit_constraint_flag = bs_read_u1(b);
            ptl->general_max_10bit_constraint_flag = bs_read_u1(b);
            ptl->general_max_8bit_constraint_flag = bs_read_u1(b);
            ptl->general_max_422chroma_constraint_flag = bs_read_u1(b);
            ptl->general_max_420chroma_constraint_flag = bs_read_u1(b);
            ptl->general_max_monochrome_constraint_flag = bs_read_u1(b);
            ptl->general_intra_constraint_flag = bs_read_u1(b);
            ptl->general_one_picture_only_constraint_flag = bs_read_u1(b);
            ptl->general_lower_bit_rate_constraint_flag = bs_read_u1(b);
            uint64_t tmp1 = bs_read_u(b, 32);
            uint64_t tmp2 = bs_read_u(b, 2);
            ptl->general_reserved_zero_34bits = tmp1 + tmp2;
        }
        else
        {
            uint64_t tmp1 = bs_read_u(b, 32);
            uint64_t tmp2 = bs_read_u(b, 11);
            ptl->general_reserved_zero_43bits = tmp1 + tmp2;
        }
        if ((ptl->general_profile_idc >= 1 && ptl->general_profile_idc <= 5) ||
            ptl->general_profile_compatibility_flag[1] || ptl->general_profile_compatibility_flag[2] ||
            ptl->general_profile_compatibility_flag[3] || ptl->general_profile_compatibility_flag[4] ||
            ptl->general_profile_compatibility_flag[5])
        {
            ptl->general_inbld_flag = bs_read_u1(b);
        }
        else
        {
            ptl->general_reserved_zero_bit = bs_read_u1(b);
        }
    }
    ptl->general_level_idc = bs_read_u8(b);

#if 0
    ptl->sub_layer_profile_present_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_level_present_flag.resize(max_sub_layers_minus1);
#endif
    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        ptl->sub_layer_profile_present_flag[i] = bs_read_u1(b);
        ptl->sub_layer_level_present_flag[i] = bs_read_u1(b);
    }

    if (max_sub_layers_minus1 > 0)
    {
        for (i = max_sub_layers_minus1; i < 8; i++)
        {
            ptl->reserved_zero_2bits[i] = bs_read_u(b, 2);
        }
    }
#if 0
    ptl->sub_layer_profile_space.resize(max_sub_layers_minus1);
    ptl->sub_layer_tier_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_profile_idc.resize(max_sub_layers_minus1);
    ptl->sub_layer_profile_compatibility_flag.resize(max_sub_layers_minus1);

    for (int j = 0; j < max_sub_layers_minus1; j++)
    {
        ptl->sub_layer_profile_compatibility_flag[j].resize(32);
    }

    ptl->sub_layer_progressive_source_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_interlaced_source_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_non_packed_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_frame_only_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_12bit_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_10bit_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_8bit_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_422chroma_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_420chroma_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_max_monochrome_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_intra_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_one_picture_only_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_lower_bit_rate_constraint_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_reserved_zero_34bits.resize(max_sub_layers_minus1);
    ptl->sub_layer_reserved_zero_43bits.resize(max_sub_layers_minus1);
    ptl->sub_layer_inbld_flag.resize(max_sub_layers_minus1);
    ptl->sub_layer_reserved_zero_bit.resize(max_sub_layers_minus1);
    ptl->sub_layer_level_idc.resize(max_sub_layers_minus1);
#endif

#if 1
    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        if (ptl->sub_layer_profile_present_flag[i])
        {
            ptl->sub_layer_profile_space[i] = bs_read_u(b, 2);
            ptl->sub_layer_tier_flag[i] = bs_read_u1(b);
            ptl->sub_layer_profile_idc[i] = bs_read_u(b, 5);
            for (int j = 0; j < 32; j++)
            {
                ptl->sub_layer_profile_compatibility_flag[i][j] = bs_read_u1(b);
            }
            ptl->sub_layer_progressive_source_flag[i] = bs_read_u1(b);
            ptl->sub_layer_interlaced_source_flag[i] = bs_read_u1(b);
            ptl->sub_layer_non_packed_constraint_flag[i] = bs_read_u1(b);
            ptl->sub_layer_frame_only_constraint_flag[i] = bs_read_u1(b);
            if (ptl->sub_layer_profile_idc[i] == 4 || ptl->sub_layer_profile_compatibility_flag[i][4] ||
                ptl->sub_layer_profile_idc[i] == 5 || ptl->sub_layer_profile_compatibility_flag[i][5] ||
                ptl->sub_layer_profile_idc[i] == 6 || ptl->sub_layer_profile_compatibility_flag[i][6] ||
                ptl->sub_layer_profile_idc[i] == 7 || ptl->sub_layer_profile_compatibility_flag[i][7])
            {
                ptl->sub_layer_max_12bit_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_max_10bit_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_max_8bit_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_max_422chroma_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_max_420chroma_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_max_monochrome_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_intra_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_one_picture_only_constraint_flag[i] = bs_read_u1(b);
                ptl->sub_layer_lower_bit_rate_constraint_flag[i] = bs_read_u1(b);
                uint64_t tmp1 = bs_read_u(b, 32);
                uint64_t tmp2 = bs_read_u(b, 2);
                ptl->sub_layer_reserved_zero_34bits[i] = tmp1 + tmp2;
            }
            else
            {
                uint64_t tmp1 = bs_read_u(b, 32);
                uint64_t tmp2 = bs_read_u(b, 12);
                ptl->sub_layer_reserved_zero_43bits[i] = tmp1 + tmp2;
            }
            // to check
            if ((ptl->sub_layer_profile_idc[i] >= 1 && ptl->sub_layer_profile_idc[i] <= 5) ||
                ptl->sub_layer_profile_compatibility_flag[i][1] ||
                ptl->sub_layer_profile_compatibility_flag[i][2] ||
                ptl->sub_layer_profile_compatibility_flag[i][3] ||
                ptl->sub_layer_profile_compatibility_flag[i][4] ||
                ptl->sub_layer_profile_compatibility_flag[i][5])
            {
                ptl->sub_layer_inbld_flag[i] = bs_read_u1(b);
            }
            else
            {
                ptl->sub_layer_reserved_zero_bit[i] = bs_read_u1(b);
            }
        }
        if (ptl->sub_layer_level_present_flag[i])
        {
            ptl->sub_layer_level_idc[i] = bs_read_u8(b);
        }
    }
#else
    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        if (ptl->sub_layer_profile_present_flag[i])
        {
            /*
             * sub_layer_profile_space[i]                     u(2)
             * sub_layer_tier_flag[i]                         u(1)
             * sub_layer_profile_idc[i]                       u(5)
             * sub_layer_profile_compatibility_flag[i][0..31] u(32)
             * sub_layer_progressive_source_flag[i]           u(1)
             * sub_layer_interlaced_source_flag[i]            u(1)
             * sub_layer_non_packed_constraint_flag[i]        u(1)
             * sub_layer_frame_only_constraint_flag[i]        u(1)
             * sub_layer_reserved_zero_44bits[i]              u(44) 12+44=56= 32+24
             */
            skip_bits_long(b, 32);
            skip_bits_long(b, 32);
            skip_bits(b, 24);
        }

        if (ptl->sub_layer_level_present_flag[i])
            skip_bits(gb, 8);
    }
#endif
}

//7.3.2.1 Sequence parameter set RBSP syntax
void h265_read_sps_rbsp(h265_sps_t *sps, bs_t *b, int *pw, int *ph)
{
    // NOTE 不能直接赋值给sps，因为还未知是哪一个sps。
    int width, height;
    int sps_video_parameter_set_id = 0;
    int sps_max_sub_layers_minus1 = 0;
    int sps_temporal_id_nesting_flag = 0;
    int sps_seq_parameter_set_id = 0;
    profile_tier_level_t profile_tier_level;

    sps_video_parameter_set_id = bs_read_u(b, 4);
    sps_max_sub_layers_minus1 = bs_read_u(b, 3); //3bit : 421 , 7
    sps_temporal_id_nesting_flag = bs_read_u1(b);

    // profile tier level...
    memset(&profile_tier_level, '\0', sizeof(profile_tier_level_t));

    h265_read_ptl(&profile_tier_level, b, 1, sps_max_sub_layers_minus1);

    sps_seq_parameter_set_id = bs_read_ue(b);
    // 选择正确的sps表
    //h->sps = h->sps_table[sps_seq_parameter_set_id];

    sps->sps_video_parameter_set_id = sps_video_parameter_set_id;
    sps->sps_max_sub_layers_minus1 = sps_max_sub_layers_minus1;
    sps->sps_temporal_id_nesting_flag = sps_temporal_id_nesting_flag;

    memcpy(&(sps->ptl), &profile_tier_level, sizeof(profile_tier_level_t)); // ptl

    sps->sps_seq_parameter_set_id = sps_seq_parameter_set_id;
    sps->chroma_format_idc = bs_read_ue(b);

    /// h->info->chroma_format_idc = sps->chroma_format_idc;
    if (sps->chroma_format_idc == 3)
    {
        sps->separate_colour_plane_flag = bs_read_u1(b);
    }
    sps->pic_width_in_luma_samples = bs_read_ue(b);
    sps->pic_height_in_luma_samples = bs_read_ue(b);

    width = sps->pic_width_in_luma_samples;
    height = sps->pic_height_in_luma_samples;

    //printf("pic_width_in_luma_samples=%d \n", sps->pic_width_in_luma_samples);
    //printf("pic_height_in_luma_samples=%d \n", sps->pic_height_in_luma_samples);

    sps->conformance_window_flag = bs_read_u1(b);
    if (sps->conformance_window_flag)
    {
        sps->conf_win_left_offset = bs_read_ue(b);
        sps->conf_win_right_offset = bs_read_ue(b);
        sps->conf_win_top_offset = bs_read_ue(b);
        sps->conf_win_bottom_offset = bs_read_ue(b);

        // calc width & height again...
        //h->info->crop_left = sps->conf_win_left_offset;
        //h->info->crop_right = sps->conf_win_right_offset;
        //h->info->crop_top = sps->conf_win_top_offset;
        //h->info->crop_bottom = sps->conf_win_bottom_offset;

        // 根据Table6-1及7.4.3.2.1重新计算宽、高
        // 注意：手册里加1，实际上不用
        // 参考：https://github.com/mbunkus/mkvtoolnix/issues/1152
        int sub_width_c = ((1 == sps->chroma_format_idc) || (2 == sps->chroma_format_idc)) && (0 == sps->separate_colour_plane_flag) ? 2 : 1;
        int sub_height_c = (1 == sps->chroma_format_idc) && (0 == sps->separate_colour_plane_flag) ? 2 : 1;

        width -= (sub_width_c * sps->conf_win_right_offset + sub_width_c * sps->conf_win_left_offset);
        height -= (sub_height_c * sps->conf_win_bottom_offset + sub_height_c * sps->conf_win_top_offset);
    }
    if (pw)
        *pw = width;
    if (ph)
        *ph = height;

    sps->bit_depth_luma_minus8 = bs_read_ue(b);
    sps->bit_depth_chroma_minus8 = bs_read_ue(b);

    // bit depth
    //h->info->bit_depth_luma = sps->bit_depth_luma_minus8 + 8;
    //h->info->bit_depth_chroma = sps->bit_depth_chroma_minus8 + 8;

    sps->log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b);

    sps->sps_sub_layer_ordering_info_present_flag = bs_read_u1(b);
    for (int i = (sps->sps_sub_layer_ordering_info_present_flag ? 0 : sps->sps_max_sub_layers_minus1);
         i <= sps->sps_max_sub_layers_minus1; i++)
    {
        sps->sps_max_dec_pic_buffering_minus1[i] = bs_read_ue(b);
        sps->sps_max_num_reorder_pics[i] = bs_read_ue(b);
        sps->sps_max_latency_increase_plus1[i] = bs_read_ue(b);
    }

    sps->log2_min_luma_coding_block_size_minus3 = bs_read_ue(b);
    sps->log2_diff_max_min_luma_coding_block_size = bs_read_ue(b);
    sps->log2_min_luma_transform_block_size_minus2 = bs_read_ue(b);
    sps->log2_diff_max_min_luma_transform_block_size = bs_read_ue(b);
    sps->max_transform_hierarchy_depth_inter = bs_read_ue(b);
    sps->max_transform_hierarchy_depth_intra = bs_read_ue(b);

    sps->scaling_list_enabled_flag = bs_read_u1(b);
    if (sps->scaling_list_enabled_flag)
    {
        sps->sps_scaling_list_data_present_flag = bs_read_u1(b);
        if (sps->sps_scaling_list_data_present_flag)
        {
            // scaling_list_data()
            h265_read_scaling_list(&(sps->scaling_list_data), b);
        }
    }

    sps->amp_enabled_flag = bs_read_u1(b);
    sps->sample_adaptive_offset_enabled_flag = bs_read_u1(b);
    sps->pcm_enabled_flag = bs_read_u1(b);
    if (sps->pcm_enabled_flag)
    {
        sps->pcm_sample_bit_depth_luma_minus1 = bs_read_u(b, 4);
        sps->pcm_sample_bit_depth_chroma_minus1 = bs_read_u(b, 4);
        sps->log2_min_pcm_luma_coding_block_size_minus3 = bs_read_ue(b);
        sps->log2_diff_max_min_pcm_luma_coding_block_size = bs_read_ue(b);
        sps->pcm_loop_filter_disabled_flag = bs_read_u1(b);
    }

    sps->num_short_term_ref_pic_sets = bs_read_ue(b);

#if 0
    // 根据num_short_term_ref_pic_sets创建数组
    sps->st_ref_pic_set.resize(sps->num_short_term_ref_pic_sets);
    sps->m_RPSList.resize(sps->num_short_term_ref_pic_sets); // 确定一共有多少个RPS列表
    referencePictureSets_t* rps = NULL;
    st_ref_pic_set_t* st = NULL;
    for (int i = 0; i < sps->num_short_term_ref_pic_sets; i++)
    {
        st = &sps->st_ref_pic_set[i];
        rps = &sps->m_RPSList[i];
        h265_read_short_term_ref_pic_set(b, sps, st, rps, i);
    }

    sps->long_term_ref_pics_present_flag = bs_read_u1(b);
    if (sps->long_term_ref_pics_present_flag)
    {
        sps->num_long_term_ref_pics_sps = bs_read_ue(b);
        sps->lt_ref_pic_poc_lsb_sps.resize(sps->num_long_term_ref_pics_sps);
        sps->used_by_curr_pic_lt_sps_flag.resize(sps->num_long_term_ref_pics_sps);
        for (int i = 0; i < sps->num_long_term_ref_pics_sps; i++)
        {
            sps->lt_ref_pic_poc_lsb_sps_bytes = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
            sps->lt_ref_pic_poc_lsb_sps[i] = bs_read_u(b, sps->log2_max_pic_order_cnt_lsb_minus4 + 4); // u(v)
            sps->used_by_curr_pic_lt_sps_flag[i] = bs_read_u1(b);
        }
    }

    sps->sps_temporal_mvp_enabled_flag = bs_read_u1(b);
    sps->strong_intra_smoothing_enabled_flag = bs_read_u1(b);
    sps->vui_parameters_present_flag = bs_read_u1(b);
    if (sps->vui_parameters_present_flag)
    {
        h265_read_vui_parameters(&(sps->vui), b, sps->sps_max_sub_layers_minus1);
        // calc fps
        if (sps->vui.vui_num_units_in_tick != 0)
            h->info->max_framerate = (float)(sps->vui.vui_time_scale) / (float)(sps->vui.vui_num_units_in_tick);
    }

    sps->sps_extension_present_flag = bs_read_u1(b);
    if (sps->sps_extension_present_flag)
    {
        sps->sps_range_extension_flag = bs_read_u1(b);
        sps->sps_multilayer_extension_flag = bs_read_u1(b);
        sps->sps_3d_extension_flag = bs_read_u1(b);
        sps->sps_extension_5bits = bs_read_u(b, 5);
    }

    if (sps->sps_range_extension_flag)
    {
        sps->sps_range_extension.transform_skip_rotation_enabled_flag = bs_read_u1(b);
        sps->sps_range_extension.transform_skip_context_enabled_flag = bs_read_u1(b);
        sps->sps_range_extension.implicit_rdpcm_enabled_flag = bs_read_u1(b);
        sps->sps_range_extension.explicit_rdpcm_enabled_flag = bs_read_u1(b);
        sps->sps_range_extension.extended_precision_processing_flag = bs_read_u1(b);
        sps->sps_range_extension.intra_smoothing_disabled_flag = bs_read_u1(b);
        sps->sps_range_extension.high_precision_offsets_enabled_flag = bs_read_u1(b);
        sps->sps_range_extension.persistent_rice_adaptation_enabled_flag = bs_read_u1(b);
        sps->sps_range_extension.cabac_bypass_alignment_enabled_flag = bs_read_u1(b);
    }
    if (sps->sps_multilayer_extension_flag)
    {
        // sps_multilayer_extension()
        sps->inter_view_mv_vert_constraint_flag = bs_read_u1(b);
    }
    if (sps->sps_3d_extension_flag)
    {
        // todo sps_3d_extension( )
    }
    if (sps->sps_extension_5bits)
    {
        while (h265_more_rbsp_trailing_data(b))
        {
            int sps_extension_data_flag = bs_read_u1(b);
        }
    }
    h265_read_rbsp_trailing_bits(b);
#endif
}

int h265_decode_sps_t(h265_sps_t *sps, void *spsdata, int size, int *width, int *height)
{
    bs_t bs;
    bs_t *b = &bs;
    memset(&bs, 0, sizeof(bs));
    bs_init(&bs, spsdata, size);
    int forbidden_zero_bit = bs_read_f(b, 1);
    int nal_unit_type = bs_read_u(b, 6);
    int nuh_layer_id = bs_read_u(b, 6);
    int nuh_temporal_id_plus1 = bs_read_u(b, 3);

    if (nal_unit_type == 33)
    {
        int nal_size = size;
        int rbsp_size = size;
        uint8_t* rbsp_buf = (uint8_t*)malloc(rbsp_size + 1);

        int rc = nal_to_rbsp(2, spsdata, &nal_size, rbsp_buf, &rbsp_size);

        if (rc < 0)
        {
            free(rbsp_buf);
            return -1;
        } // handle conversion error

        //b = bs_new(rbsp_buf, rbsp_size);
        bs_init(&bs, rbsp_buf, rbsp_size);
        h265_read_sps_rbsp(sps, &bs, width, height);

        free(rbsp_buf);
    }
    return 0;
}

#define MAX_SPATIAL_SEGMENTATION 4096 // max. value of u(12) field
#define AV_INPUT_BUFFER_PADDING_SIZE 64

static uint8_t *nal_unit_extract_rbsp(const uint8_t *src, uint32_t src_len,
                                      uint32_t *dst_len)
{
    uint8_t *dst;
    uint32_t i, len;

    dst = (uint8_t *)malloc(src_len + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!dst)
        return NULL;

    /* NAL unit header (2 bytes) */
    i = len = 0;
    while (i < 2 && i < src_len) //NAL type
        dst[len++] = src[i++];

    while (i + 2 < src_len)
        if (!src[i] && !src[i + 1] && src[i + 2] == 3)
        {
            dst[len++] = src[i++];
            dst[len++] = src[i++];
            i++; // remove emulation_prevention_three_byte
        }
        else
            dst[len++] = src[i++];

    while (i < src_len)
        dst[len++] = src[i++];

    memset(dst + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    *dst_len = len;
    return dst;
}

static void hvcc_init(HEVCDecoderConfigurationRecord *hvcc)
{
    memset(hvcc, 0, sizeof(HEVCDecoderConfigurationRecord));
    hvcc->configurationVersion = 1;
    hvcc->lengthSizeMinusOne = 3; // 4 bytes

    /*
	 * The following fields have all their valid bits set by default,
	 * the ProfileTierLevel parsing code will unset them when needed.
	 */
    hvcc->general_profile_compatibility_flags = 0xffffffff;
    hvcc->general_constraint_indicator_flags = 0xffffffffffff;

    /*
	 * Initialize this field with an invalid value which can be used to detect
	 * whether we didn't see any VUI (in which case it should be reset to zero).
	 */
    hvcc->min_spatial_segmentation_idc = MAX_SPATIAL_SEGMENTATION + 1;
}

#define FFMAX(a, b) ((a) > (b) ? (a) : (b))
#define FFMIN(a, b) ((a) > (b) ? (b) : (a))

#define HEVC_MAX_SHORT_TERM_REF_PIC_SETS 64

#define get_bits1(b)            bs_read_u1(b)
#define get_ue_golomb_long(b)   bs_read_ue(b)
#define skip_bits_long(b, len)  bs_skip_u(b, len)
#define skip_bits(b, len)       bs_skip_u(b, len)
#define skip_bits1(b)           bs_skip_u1(b)
#define get_bits(b, len)        bs_read_u(b, len)
#define get_se_golomb_long(b)   bs_read_se(b)
#define get_bits_left(b)        bs_bytes_left(b)

typedef bs_t GetBitContext;

#define AVERROR_INVALIDDATA -1

typedef struct HVCCProfileTierLevel
{
    uint8_t profile_space;
    uint8_t tier_flag;
    uint8_t profile_idc;
    uint32_t profile_compatibility_flags;
    uint64_t constraint_indicator_flags;
    uint8_t level_idc;
} HVCCProfileTierLevel;

static void hvcc_update_ptl(HEVCDecoderConfigurationRecord *hvcc,
                            HVCCProfileTierLevel *ptl)
{
    /*
     * The value of general_profile_space in all the parameter sets must be
     * identical.
     */
    hvcc->general_profile_space = ptl->profile_space;

    /*
     * The level indication general_level_idc must indicate a level of
     * capability equal to or greater than the highest level indicated for the
     * highest tier in all the parameter sets.
     */
    if (hvcc->general_tier_flag < ptl->tier_flag)
        hvcc->general_level_idc = ptl->level_idc;
    else
        hvcc->general_level_idc = FFMAX(hvcc->general_level_idc, ptl->level_idc);

    /*
     * The tier indication general_tier_flag must indicate a tier equal to or
     * greater than the highest tier indicated in all the parameter sets.
     */
    hvcc->general_tier_flag = FFMAX(hvcc->general_tier_flag, ptl->tier_flag);

    /*
     * The profile indication general_profile_idc must indicate a profile to
     * which the stream associated with this configuration record conforms.
     *
     * If the sequence parameter sets are marked with different profiles, then
     * the stream may need examination to determine which profile, if any, the
     * entire stream conforms to. If the entire stream is not examined, or the
     * examination reveals that there is no profile to which the entire stream
     * conforms, then the entire stream must be split into two or more
     * sub-streams with separate configuration records in which these rules can
     * be met.
     *
     * Note: set the profile to the highest value for the sake of simplicity.
     */
    hvcc->general_profile_idc = FFMAX(hvcc->general_profile_idc, ptl->profile_idc);

    /*
     * Each bit in general_profile_compatibility_flags may only be set if all
     * the parameter sets set that bit.
     */
    hvcc->general_profile_compatibility_flags &= ptl->profile_compatibility_flags;

    /*
     * Each bit in general_constraint_indicator_flags may only be set if all
     * the parameter sets set that bit.
     */
    hvcc->general_constraint_indicator_flags &= ptl->constraint_indicator_flags;
}

static void hvcc_parse_ptl(bs_t *b, HEVCDecoderConfigurationRecord *hvcc, unsigned int max_sub_layers_minus1)
{
    unsigned int i;
    HVCCProfileTierLevel general_ptl;
    uint8_t sub_layer_profile_present_flag[HEVC_MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[HEVC_MAX_SUB_LAYERS];

    general_ptl.profile_space = bs_read_u(b, 2);                //get_bits(gb, 2);
    general_ptl.tier_flag = bs_read_u1(b);                      //get_bits1(gb);
    general_ptl.profile_idc = bs_read_u(b, 5);                  //get_bits(gb, 5);
    general_ptl.profile_compatibility_flags = bs_read_u(b, 32); // get_bits_long(gb, 32);
    general_ptl.constraint_indicator_flags = bs_read_u(b, 48);  //get_bits64(gb, 48);
    general_ptl.level_idc = bs_read_u8(b);                      // get_bits(gb, 8);
    hvcc_update_ptl(hvcc, &general_ptl);

    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        sub_layer_profile_present_flag[i] = bs_read_u1(b); // get_bits1(gb);
        sub_layer_level_present_flag[i] = bs_read_u1(b);   // get_bits1(gb);
    }

    if (max_sub_layers_minus1 > 0)
        for (i = max_sub_layers_minus1; i < 8; i++)
            bs_read_u(b, 2); // skip_bits(gb, 2); // reserved_zero_2bits[i]

    for (i = 0; i < max_sub_layers_minus1; i++)
    {
        if (sub_layer_profile_present_flag[i])
        {
            /*
             * sub_layer_profile_space[i]                     u(2)
             * sub_layer_tier_flag[i]                         u(1)
             * sub_layer_profile_idc[i]                       u(5)
             * sub_layer_profile_compatibility_flag[i][0..31] u(32)
             * sub_layer_progressive_source_flag[i]           u(1)
             * sub_layer_interlaced_source_flag[i]            u(1)
             * sub_layer_non_packed_constraint_flag[i]        u(1)
             * sub_layer_frame_only_constraint_flag[i]        u(1)
             * sub_layer_reserved_zero_44bits[i]              u(44)
             */
            bs_read_u(b, 32); //skip_bits_long(gb, 32);
            bs_read_u(b, 32); //skip_bits_long(gb, 32);
            bs_read_u(b, 24); //skip_bits     (gb, 24);
        }

        if (sub_layer_level_present_flag[i])
            bs_read_u8(b); //skip_bits(gb, 8);
    }
}

static void skip_sub_layer_ordering_info(GetBitContext *gb)
{
    get_ue_golomb_long(gb); // max_dec_pic_buffering_minus1
    get_ue_golomb_long(gb); // max_num_reorder_pics
    get_ue_golomb_long(gb); // max_latency_increase_plus1
}

static int hvcc_parse_vps(bs_t *gb, HEVCDecoderConfigurationRecord *hvcc)
{
    unsigned int vps_max_sub_layers_minus1;

    /*
     * vps_video_parameter_set_id u(4)
     * vps_reserved_three_2bits   u(2)
     * vps_max_layers_minus1      u(6)
     */
    bs_read_u(gb, 12); //skip_bits(gb, 12);

    vps_max_sub_layers_minus1 = bs_read_u(gb, 3); // get_bits(gb, 3);

    /*
     * numTemporalLayers greater than 1 indicates that the stream to which this
     * configuration record applies is temporally scalable and the contained
     * number of temporal layers (also referred to as temporal sub-layer or
     * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
     * indicates that the stream is not temporally scalable. Value 0 indicates
     * that it is unknown whether the stream is temporally scalable.
     */
    hvcc->numTemporalLayers = FFMAX(hvcc->numTemporalLayers, vps_max_sub_layers_minus1 + 1);

    /*
     * vps_temporal_id_nesting_flag u(1)
     * vps_reserved_0xffff_16bits   u(16)
     */
    bs_read_u(gb, 17); //skip_bits(gb, 17);

    hvcc_parse_ptl(gb, hvcc, vps_max_sub_layers_minus1);

    /* nothing useful for hvcC past this point */
    return 0;
}

#if 0
static void skip_scaling_list_data(bs_t *b)
{
    int i, j, k, num_coeffs;

    for (i = 0; i < 4; i++)
        for (j = 0; j < (i == 3 ? 2 : 6); j++)
            if (!bs_read_u1(b)) // scaling_list_pred_mode_flag[i][j]
                bs_read_ue(b);  //get_ue_golomb_long(gb); // scaling_list_pred_matrix_id_delta[i][j]
            else
            {
                num_coeffs = FFMIN(64, 1 << (4 + (i << 1)));

                if (i > 1)
                    get_se_golomb_long(b); // scaling_list_dc_coef_minus8[i-2][j]

                for (k = 0; k < num_coeffs; k++)
                    get_se_golomb_long(b); // scaling_list_delta_coef
            }
}
#endif

static int parse_rps(bs_t *gb,
                     unsigned int rps_idx,
                     unsigned int num_rps,
                     unsigned int num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS])
{
    unsigned int i;

    if (rps_idx && bs_read_u1(gb))
    { // inter_ref_pic_set_prediction_flag
        /* this should only happen for slice headers, and this isn't one */
        if (rps_idx >= num_rps)
            return -1;

        bs_read_u1(gb); // delta_rps_sign
        bs_read_ue(gb); // abs_delta_rps_minus1

        num_delta_pocs[rps_idx] = 0;

        /*
         * From libavcodec/hevc_ps.c:
         *
         * if (is_slice_header) {
         *    //foo
         * } else
         *     rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];
         *
         * where:
         * rps:             &sps->st_rps[rps_idx]
         * sps->st_rps:     &sps->st_rps[0]
         * is_slice_header: rps_idx == num_rps
         *
         * thus:
         * if (num_rps != rps_idx)
         *     rps_ridx = &sps->st_rps[rps_idx - 1];
         *
         * NumDeltaPocs[RefRpsIdx]: num_delta_pocs[rps_idx - 1]
         */
        for (i = 0; i <= num_delta_pocs[rps_idx - 1]; i++)
        {
            uint8_t use_delta_flag = 0;
            uint8_t used_by_curr_pic_flag = get_bits1(gb);
            if (!used_by_curr_pic_flag)
                use_delta_flag = get_bits1(gb);

            if (used_by_curr_pic_flag || use_delta_flag)
                num_delta_pocs[rps_idx]++;
        }
    }
    else
    {
        unsigned int num_negative_pics = bs_read_ue(gb);
        unsigned int num_positive_pics = bs_read_ue(gb);

        if ((num_positive_pics + (uint64_t)num_negative_pics) * 2 > get_bits_left(gb))
            return AVERROR_INVALIDDATA;

        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++)
        {
            bs_read_ue(gb); // delta_poc_s0_minus1[rps_idx]
            bs_read_u1(gb); // used_by_curr_pic_s0_flag[rps_idx]
        }

        for (i = 0; i < num_positive_pics; i++)
        {
            bs_read_ue(gb); // delta_poc_s1_minus1[rps_idx]
            bs_read_u1(gb); // used_by_curr_pic_s1_flag[rps_idx]
        }
    }

    return 0;
}

static void skip_sub_layer_hrd_parameters(GetBitContext *gb,
                                          unsigned int cpb_cnt_minus1,
                                          uint8_t sub_pic_hrd_params_present_flag)
{
    unsigned int i;

    for (i = 0; i <= cpb_cnt_minus1; i++) {
        get_ue_golomb_long(gb); // bit_rate_value_minus1
        get_ue_golomb_long(gb); // cpb_size_value_minus1

        if (sub_pic_hrd_params_present_flag) {
            get_ue_golomb_long(gb); // cpb_size_du_value_minus1
            get_ue_golomb_long(gb); // bit_rate_du_value_minus1
        }

        skip_bits1(gb); // cbr_flag
    }
}

static int skip_hrd_parameters(GetBitContext *gb, uint8_t cprms_present_flag,
                                unsigned int max_sub_layers_minus1)
{
    unsigned int i;
    uint8_t sub_pic_hrd_params_present_flag = 0;
    uint8_t nal_hrd_parameters_present_flag = 0;
    uint8_t vcl_hrd_parameters_present_flag = 0;

    if (cprms_present_flag) {
        nal_hrd_parameters_present_flag = get_bits1(gb);
        vcl_hrd_parameters_present_flag = get_bits1(gb);

        if (nal_hrd_parameters_present_flag ||
            vcl_hrd_parameters_present_flag) {
            sub_pic_hrd_params_present_flag = get_bits1(gb);

            if (sub_pic_hrd_params_present_flag)
                /*
                 * tick_divisor_minus2                          u(8)
                 * du_cpb_removal_delay_increment_length_minus1 u(5)
                 * sub_pic_cpb_params_in_pic_timing_sei_flag    u(1)
                 * dpb_output_delay_du_length_minus1            u(5)
                 */
                skip_bits(gb, 19);

            /*
             * bit_rate_scale u(4)
             * cpb_size_scale u(4)
             */
            skip_bits(gb, 8);

            if (sub_pic_hrd_params_present_flag)
                skip_bits(gb, 4); // cpb_size_du_scale

            /*
             * initial_cpb_removal_delay_length_minus1 u(5)
             * au_cpb_removal_delay_length_minus1      u(5)
             * dpb_output_delay_length_minus1          u(5)
             */
            skip_bits(gb, 15);
        }
    }

    for (i = 0; i <= max_sub_layers_minus1; i++) {
        unsigned int cpb_cnt_minus1            = 0;
        uint8_t low_delay_hrd_flag             = 0;
        uint8_t fixed_pic_rate_within_cvs_flag = 0;
        uint8_t fixed_pic_rate_general_flag    = get_bits1(gb);

        if (!fixed_pic_rate_general_flag)
            fixed_pic_rate_within_cvs_flag = get_bits1(gb);

        if (fixed_pic_rate_within_cvs_flag)
            get_ue_golomb_long(gb); // elemental_duration_in_tc_minus1
        else
            low_delay_hrd_flag = get_bits1(gb);

        if (!low_delay_hrd_flag) {
            cpb_cnt_minus1 = get_ue_golomb_long(gb);
            if (cpb_cnt_minus1 > 31)
                return AVERROR_INVALIDDATA;
        }

        if (nal_hrd_parameters_present_flag)
            skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                          sub_pic_hrd_params_present_flag);

        if (vcl_hrd_parameters_present_flag)
            skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                          sub_pic_hrd_params_present_flag);
    }

    return 0;
}

static void skip_timing_info(bs_t *gb)
{
    skip_bits_long(gb, 32); // num_units_in_tick
    skip_bits_long(gb, 32); // time_scale
    
    if (get_bits1(gb))          // poc_proportional_to_timing_flag
        get_ue_golomb_long(gb); // num_ticks_poc_diff_one_minus1
}

static void hvcc_parse_vui(bs_t *gb,
                           HEVCDecoderConfigurationRecord *hvcc,
                           unsigned int max_sub_layers_minus1)
{
    unsigned int min_spatial_segmentation_idc;

    if (get_bits1(gb))              // aspect_ratio_info_present_flag
        if (get_bits(gb, 8) == 255) // aspect_ratio_idc
            skip_bits_long(gb, 32); // sar_width u(16), sar_height u(16)

    if (get_bits1(gb))  // overscan_info_present_flag
        skip_bits1(gb); // overscan_appropriate_flag

    if (get_bits1(gb)) {  // video_signal_type_present_flag
        skip_bits(gb, 4); // video_format u(3), video_full_range_flag u(1)

        if (get_bits1(gb)) // colour_description_present_flag
            /*
             * colour_primaries         u(8)
             * transfer_characteristics u(8)
             * matrix_coeffs            u(8)
             */
            skip_bits(gb, 24);
    }

    if (get_bits1(gb)) {        // chroma_loc_info_present_flag
        get_ue_golomb_long(gb); // chroma_sample_loc_type_top_field
        get_ue_golomb_long(gb); // chroma_sample_loc_type_bottom_field
    }

    /*
     * neutral_chroma_indication_flag u(1)
     * field_seq_flag                 u(1)
     * frame_field_info_present_flag  u(1)
     */
    skip_bits(gb, 3);

    if (get_bits1(gb)) {        // default_display_window_flag
        get_ue_golomb_long(gb); // def_disp_win_left_offset
        get_ue_golomb_long(gb); // def_disp_win_right_offset
        get_ue_golomb_long(gb); // def_disp_win_top_offset
        get_ue_golomb_long(gb); // def_disp_win_bottom_offset
    }

    if (get_bits1(gb)) { // vui_timing_info_present_flag
        skip_timing_info(gb);

        if (get_bits1(gb)) // vui_hrd_parameters_present_flag
            skip_hrd_parameters(gb, 1, max_sub_layers_minus1);
    }

    if (get_bits1(gb)) { // bitstream_restriction_flag
        /*
         * tiles_fixed_structure_flag              u(1)
         * motion_vectors_over_pic_boundaries_flag u(1)
         * restricted_ref_pic_lists_flag           u(1)
         */
        skip_bits(gb, 3);

        min_spatial_segmentation_idc = get_ue_golomb_long(gb);

        /*
         * unsigned int(12) min_spatial_segmentation_idc;
         *
         * The min_spatial_segmentation_idc indication must indicate a level of
         * spatial segmentation equal to or less than the lowest level of
         * spatial segmentation indicated in all the parameter sets.
         */
        hvcc->min_spatial_segmentation_idc = FFMIN(hvcc->min_spatial_segmentation_idc,
                                                   min_spatial_segmentation_idc);

        get_ue_golomb_long(gb); // max_bytes_per_pic_denom
        get_ue_golomb_long(gb); // max_bits_per_min_cu_denom
        get_ue_golomb_long(gb); // log2_max_mv_length_horizontal
        get_ue_golomb_long(gb); // log2_max_mv_length_vertical
    }
}

static int hvcc_parse_sps(bs_t *b,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    unsigned int i, sps_max_sub_layers_minus1, log2_max_pic_order_cnt_lsb_minus4;
    unsigned int num_short_term_ref_pic_sets, num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS];

    bs_read_u(b, 4);                             //skip_bits(gb, 4); // sps_video_parameter_set_id
    sps_max_sub_layers_minus1 = bs_read_u(b, 3); //get_bits (gb, 3);

    /*
     * numTemporalLayers greater than 1 indicates that the stream to which this
     * configuration record applies is temporally scalable and the contained
     * number of temporal layers (also referred to as temporal sub-layer or
     * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
     * indicates that the stream is not temporally scalable. Value 0 indicates
     * that it is unknown whether the stream is temporally scalable.
     */
    hvcc->numTemporalLayers = FFMAX(hvcc->numTemporalLayers,
                                    sps_max_sub_layers_minus1 + 1);

    hvcc->temporalIdNested = bs_read_u(b, 1); //get_bits1(gb);

    hvcc_parse_ptl(b, hvcc, sps_max_sub_layers_minus1);

    bs_read_ue(b); //get_ue_golomb_long(gb); // sps_seq_parameter_set_id

    hvcc->chromaFormat = bs_read_ue(b); //get_ue_golomb_long(gb);

    if (hvcc->chromaFormat == 3)
        bs_read_u1(b); // skip_bits1(gb); // separate_colour_plane_flag

    bs_read_ue(b); //get_ue_golomb_long(gb); // pic_width_in_luma_samples
    bs_read_ue(b); //get_ue_golomb_long(gb); // pic_height_in_luma_samples

    if (bs_read_u1(b))
    {                  // conformance_window_flag
        bs_read_ue(b); //get_ue_golomb_long(gb); // conf_win_left_offset
        bs_read_ue(b); //get_ue_golomb_long(gb); // conf_win_right_offset
        bs_read_ue(b); //get_ue_golomb_long(gb); // conf_win_top_offset
        bs_read_ue(b); //get_ue_golomb_long(gb); // conf_win_bottom_offset
    }

    hvcc->bitDepthLumaMinus8 = bs_read_ue(b);          //get_ue_golomb_long(gb);
    hvcc->bitDepthChromaMinus8 = bs_read_ue(b);        //get_ue_golomb_long(gb);
    log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b); //get_ue_golomb_long(gb);

    /* sps_sub_layer_ordering_info_present_flag */
    i = bs_read_u1(b) ? 0 : sps_max_sub_layers_minus1;
    for (; i <= sps_max_sub_layers_minus1; i++)
        skip_sub_layer_ordering_info(b);

    bs_read_ue(b); //get_ue_golomb_long(gb); // log2_min_luma_coding_block_size_minus3
    bs_read_ue(b); //get_ue_golomb_long(gb); // log2_diff_max_min_luma_coding_block_size
    bs_read_ue(b); //get_ue_golomb_long(gb); // log2_min_transform_block_size_minus2
    bs_read_ue(b); //get_ue_golomb_long(gb); // log2_diff_max_min_transform_block_size
    bs_read_ue(b); //get_ue_golomb_long(gb); // max_transform_hierarchy_depth_inter
    bs_read_ue(b); //get_ue_golomb_long(gb); // max_transform_hierarchy_depth_intra

    scaling_list_data_t scaling_list_data;

    if (bs_read_u1(b) && // scaling_list_enabled_flag
        bs_read_u1(b))   // sps_scaling_list_data_present_flag
        //skip_scaling_list_data(b);
        h265_read_scaling_list(&scaling_list_data, b);

    bs_read_u1(b); //skip_bits1(gb); // amp_enabled_flag
    bs_read_u1(b); //skip_bits1(gb); // sample_adaptive_offset_enabled_flag

    if (bs_read_u1(b))
    {                    // pcm_enabled_flag
        bs_read_u(b, 4); //skip_bits(gb, 4);       // pcm_sample_bit_depth_luma_minus1
        bs_read_u(b, 4); // skip_bits(gb, 4);       // pcm_sample_bit_depth_chroma_minus1
        bs_read_ue(b);   // log2_min_pcm_luma_coding_block_size_minus3
        bs_read_ue(b);   // log2_diff_max_min_pcm_luma_coding_block_size
        bs_read_u1(b);   // pcm_loop_filter_disabled_flag
    }

    num_short_term_ref_pic_sets = bs_read_ue(b);
    if (num_short_term_ref_pic_sets > HEVC_MAX_SHORT_TERM_REF_PIC_SETS)
        return -1;

    for (i = 0; i < num_short_term_ref_pic_sets; i++)
    {
        int ret = parse_rps(b, i, num_short_term_ref_pic_sets, num_delta_pocs);
        if (ret < 0)
            return ret;
    }

    if (bs_read_u1(b))
    { // long_term_ref_pics_present_flag
        unsigned num_long_term_ref_pics_sps = get_ue_golomb_long(b);
        if (num_long_term_ref_pics_sps > 31U)
            return -1;
        for (i = 0; i < num_long_term_ref_pics_sps; i++)
        { // num_long_term_ref_pics_sps
            int len = FFMIN(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
            bs_read_u(b, len); // lt_ref_pic_poc_lsb_sps[i]
            bs_read_u1(b);     // used_by_curr_pic_lt_sps_flag[i]
        }
    }

    bs_read_u1(b); // sps_temporal_mvp_enabled_flag
    bs_read_u1(b); // strong_intra_smoothing_enabled_flag

    if (bs_read_u1(b)) // vui_parameters_present_flag
        hvcc_parse_vui(b, hvcc, sps_max_sub_layers_minus1);

    /* nothing useful for hvcC past this point */
    return 0;
}

static int hvcc_parse_pps(GetBitContext *gb,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t tiles_enabled_flag, entropy_coding_sync_enabled_flag;

    get_ue_golomb_long(gb); // pps_pic_parameter_set_id
    get_ue_golomb_long(gb); // pps_seq_parameter_set_id

    /*
     * dependent_slice_segments_enabled_flag u(1)
     * output_flag_present_flag              u(1)
     * num_extra_slice_header_bits           u(3)
     * sign_data_hiding_enabled_flag         u(1)
     * cabac_init_present_flag               u(1)
     */
    skip_bits(gb, 7);

    get_ue_golomb_long(gb); // num_ref_idx_l0_default_active_minus1
    get_ue_golomb_long(gb); // num_ref_idx_l1_default_active_minus1
    get_se_golomb_long(gb); // init_qp_minus26

    /*
     * constrained_intra_pred_flag u(1)
     * transform_skip_enabled_flag u(1)
     */
    skip_bits(gb, 2);

    if (get_bits1(gb))          // cu_qp_delta_enabled_flag
        get_ue_golomb_long(gb); // diff_cu_qp_delta_depth

    get_se_golomb_long(gb); // pps_cb_qp_offset
    get_se_golomb_long(gb); // pps_cr_qp_offset

    /*
     * pps_slice_chroma_qp_offsets_present_flag u(1)
     * weighted_pred_flag               u(1)
     * weighted_bipred_flag             u(1)
     * transquant_bypass_enabled_flag   u(1)
     */
    skip_bits(gb, 4);

    tiles_enabled_flag = get_bits1(gb);
    entropy_coding_sync_enabled_flag = get_bits1(gb);

    if (entropy_coding_sync_enabled_flag && tiles_enabled_flag)
        hvcc->parallelismType = 0; // mixed-type parallel decoding
    else if (entropy_coding_sync_enabled_flag)
        hvcc->parallelismType = 3; // wavefront-based parallel decoding
    else if (tiles_enabled_flag)
        hvcc->parallelismType = 2; // tile-based parallel decoding
    else
        hvcc->parallelismType = 1; // slice-based parallel decoding

    /* nothing useful for hvcC past this point */
    return 0;
}

typedef struct AVIOContext
{
	unsigned char *buffer; /**< Start of the buffer. */
	unsigned char *buf_ptr; /**< Current position in the buffer */
	unsigned char *buf_end; /**< End of the data, may be less than
	 buffer+buffer_size if the read function returned
	 less data than requested, e.g. for streams where
	 no more data has been received yet. */
} AVIOContext;

static void flush_buffer(AVIOContext *s)
{

}

static void avio_w8(AVIOContext *s, int b)
{
	//av_assert2(b>=-128 && b<=255);
	*s->buf_ptr++ = b;
	if (s->buf_ptr >= s->buf_end)
		flush_buffer(s);
}

static void avio_wb16(AVIOContext *s, unsigned int val)
{
	avio_w8(s, (int) val >> 8);
	avio_w8(s, (uint8_t) val);
}

static void avio_wb32(AVIOContext *s, unsigned int val)
{
	avio_w8(s, val >> 24);
	avio_w8(s, (uint8_t) (val >> 16));
	avio_w8(s, (uint8_t) (val >> 8));
	avio_w8(s, (uint8_t) val);
}

static void avio_write(AVIOContext *s, const unsigned char *buf, int size)
{
//    if (s->direct && !s->update_checksum) {
//        avio_flush(s);
//        writeout(s, buf, size);
//        return;
//    }
	while (size > 0)
	{
		int len = FFMIN(s->buf_end - s->buf_ptr, size);
		memcpy(s->buf_ptr, buf, len);
		s->buf_ptr += len;

		if (s->buf_ptr >= s->buf_end)
			flush_buffer(s);

		buf += len;
		size -= len;
	}
}

static int hvcc_write(AVIOContext *pb, HEVCDecoderConfigurationRecord *hvcc)
{
	uint8_t i;
	uint16_t j, vps_count = 0, sps_count = 0, pps_count = 0;

	/*
	 * We only support writing HEVCDecoderConfigurationRecord version 1.
	 */
	hvcc->configurationVersion = 1;

	/*
	 * If min_spatial_segmentation_idc is invalid, reset to 0 (unspecified).
	 */
	if (hvcc->min_spatial_segmentation_idc > MAX_SPATIAL_SEGMENTATION)
		hvcc->min_spatial_segmentation_idc = 0;

	/*
	 * parallelismType indicates the type of parallelism that is used to meet
	 * the restrictions imposed by min_spatial_segmentation_idc when the value
	 * of min_spatial_segmentation_idc is greater than 0.
	 */
	if (!hvcc->min_spatial_segmentation_idc)
		hvcc->parallelismType = 0;

	/*
	 * It's unclear how to properly compute these fields, so
	 * let's always set them to values meaning 'unspecified'.
	 */
	hvcc->avgFrameRate = 0;
	hvcc->constantFrameRate = 0;

	/* unsigned int(8) configurationVersion = 1; */
	avio_w8(pb, hvcc->configurationVersion);

	/*
	 * unsigned int(2) general_profile_space;
	 * unsigned int(1) general_tier_flag;
	 * unsigned int(5) general_profile_idc;
	 */
	avio_w8(pb, hvcc->general_profile_space << 6 |
				hvcc->general_tier_flag << 5 |
				hvcc->general_profile_idc);

	/* unsigned int(32) general_profile_compatibility_flags; */
	avio_wb32(pb, hvcc->general_profile_compatibility_flags);

	/* unsigned int(48) general_constraint_indicator_flags; */
	avio_wb32(pb, hvcc->general_constraint_indicator_flags >> 16);
	avio_wb16(pb, hvcc->general_constraint_indicator_flags);

	/* unsigned int(8) general_level_idc; */
	avio_w8(pb, hvcc->general_level_idc);

	/*
	 * bit(4) reserved = ‘1111’b;
	 * unsigned int(12) min_spatial_segmentation_idc;
	 */
	avio_wb16(pb, hvcc->min_spatial_segmentation_idc | 0xf000);

	/*
	 * bit(6) reserved = ‘111111’b;
	 * unsigned int(2) parallelismType;
	 */
	avio_w8(pb, hvcc->parallelismType | 0xfc);

	/*
	 * bit(6) reserved = ‘111111’b;
	 * unsigned int(2) chromaFormat;
	 */
	avio_w8(pb, hvcc->chromaFormat | 0xfc);

	/*
	 * bit(5) reserved = ‘11111’b;
	 * unsigned int(3) bitDepthLumaMinus8;
	 */
	avio_w8(pb, hvcc->bitDepthLumaMinus8 | 0xf8);

	/*
	 * bit(5) reserved = ‘11111’b;
	 * unsigned int(3) bitDepthChromaMinus8;
	 */
	avio_w8(pb, hvcc->bitDepthChromaMinus8 | 0xf8);

	/* bit(16) avgFrameRate; */
	avio_wb16(pb, hvcc->avgFrameRate);

	/*
	 * bit(2) constantFrameRate;
	 * bit(3) numTemporalLayers;
	 * bit(1) temporalIdNested;
	 * unsigned int(2) lengthSizeMinusOne;
	 */
	avio_w8(pb, hvcc->constantFrameRate << 6 |
				hvcc->numTemporalLayers << 3 |
				hvcc->temporalIdNested << 2 |
				hvcc->lengthSizeMinusOne);

#if 0
	/* unsigned int(8) numOfArrays; */
	avio_w8(pb, hvcc->numOfArrays);

	for (i = 0; i < hvcc->numOfArrays; i++)
	{
		/*
		 * bit(1) array_completeness;
		 * unsigned int(1) reserved = 0;
		 * unsigned int(6) NAL_unit_type;
		 */
		avio_w8(pb, hvcc->array[i].array_completeness << 7 | hvcc->array[i].NAL_unit_type & 0x3f);

		/* unsigned int(16) numNalus; */
		avio_wb16(pb, hvcc->array[i].numNalus);

		for (j = 0; j < hvcc->array[i].numNalus; j++)
		{
			/* unsigned int(16) nalUnitLength; */
			avio_wb16(pb, hvcc->array[i].nalUnitLength[j]);

			/* bit(8*nalUnitLength) nalUnit; */
			avio_write(pb, hvcc->array[i].nalUnit[j],
						hvcc->array[i].nalUnitLength[j]);
		}
	}
#endif
	return 0;
}

int ff_buff_write_hvcc(uint8_t *buff,
                       unsigned char *pps, int pps_len,
                       unsigned char *sps, int sps_len,
                       unsigned char *vps, int vps_len)
{
    int ret;
    uint8_t nal_type;
    bs_t bs;

    HEVCDecoderConfigurationRecord hvcc;
    hvcc_init(&hvcc);
    {
        uint8_t *rbsp_buf;
        uint32_t rbsp_size;
        rbsp_buf = nal_unit_extract_rbsp(vps + 2, vps_len - 2, &rbsp_size);

        bs_t *b = &bs;
        memset(&bs, 0, sizeof(bs));
        bs_init(&bs, rbsp_buf, rbsp_size);

        hvcc_parse_vps(b, &hvcc);
        free(rbsp_buf);
    }
    {
        uint8_t *rbsp_buf;
        uint32_t rbsp_size;
        rbsp_buf = nal_unit_extract_rbsp(sps + 2, sps_len - 2, &rbsp_size);

        bs_t *b = &bs;
        memset(&bs, 0, sizeof(bs));
        bs_init(&bs, rbsp_buf, rbsp_size);

        hvcc_parse_sps(b, &hvcc);
        free(rbsp_buf);
    }
    {
        uint8_t *rbsp_buf;
        uint32_t rbsp_size;
        rbsp_buf = nal_unit_extract_rbsp(pps + 2, pps_len - 2, &rbsp_size);

        bs_t *b = &bs;
        memset(&bs, 0, sizeof(bs));
        bs_init(&bs, rbsp_buf, rbsp_size);

        hvcc_parse_pps(b, &hvcc);
        free(rbsp_buf);
    }

    AVIOContext io;
	io.buffer = buff;
	io.buf_ptr = buff;
	io.buf_end = NULL;
	hvcc_write(&io, &hvcc);

	return io.buf_ptr - io.buffer;
}

void h265_test_sps()
{
    uint8_t sps[] = {
        0x42, 0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0xB0,
        0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x7B, 0xA0, 0x03,
        0xC0, 0x80, 0x10, 0xE5, 0x8D, 0xAE, 0x49, 0x14, 0xBF, 0x37,
        0x01, 0x01, 0x01, 0x00, 0x80};

    {
        uint8_t *rbsp_buf;
        uint32_t rbsp_size;

        rbsp_buf = nal_unit_extract_rbsp(sps, sizeof(sps), &rbsp_size);
        if (rbsp_buf)
        {
            //解码VPS
            free(rbsp_buf);
        }
        for (int i = 0; i < rbsp_size; i++)
        {
            printf("%02X ", rbsp_buf[i]);
        }
        printf("\n");
    }
    {
        int nal_size = sizeof(sps);
        int rbsp_size = sizeof(sps);
        uint8_t *rbsp_buf = (uint8_t *)malloc(rbsp_size);
        int rc = nal_to_rbsp(2, sps, &nal_size, rbsp_buf, &rbsp_size);
        printf("%02X %02X ", sps[0], sps[1]);
        for (int i = 0; i < rbsp_size; i++)
        {
            printf("%02X ", rbsp_buf[i]);
        }
        printf("\n");
        free(rbsp_buf);
    }
}
