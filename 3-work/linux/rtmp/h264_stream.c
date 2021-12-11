#include <stdio.h>
#include "bs.h"
#include "h264_stream.h"

//7.3.2.1.1 Scaling list syntax
void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag)
{
    int j;
    if (scalingList == NULL)
    {
        return;
    }

    int lastScale = 8;
    int nextScale = 8;
    for (j = 0; j < sizeOfScalingList; j++)
    {
        if (nextScale != 0)
        {
            int delta_scale = bs_read_se(b);
            nextScale = (lastScale + delta_scale + 256) % 256;
            useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
        }
        scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
        lastScale = scalingList[j];
    }
}


//Appendix E.1.2 HRD parameters syntax
void read_hrd_parameters(sps_t* sps, bs_t* b)
{
    int SchedSelIdx;

    sps->hrd.cpb_cnt_minus1 = bs_read_ue(b);
    sps->hrd.bit_rate_scale = bs_read_u(b, 4);
    sps->hrd.cpb_size_scale = bs_read_u(b, 4);
    for (SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++)
    {
        sps->hrd.bit_rate_value_minus1[SchedSelIdx] = bs_read_ue(b);
        sps->hrd.cpb_size_value_minus1[SchedSelIdx] = bs_read_ue(b);
        sps->hrd.cbr_flag[SchedSelIdx] = bs_read_u1(b);
    }
    sps->hrd.initial_cpb_removal_delay_length_minus1 = bs_read_u(b, 5);
    sps->hrd.cpb_removal_delay_length_minus1 = bs_read_u(b, 5);
    sps->hrd.dpb_output_delay_length_minus1 = bs_read_u(b, 5);
    sps->hrd.time_offset_length = bs_read_u(b, 5);
}

//Appendix E.1.1 VUI parameters syntax
void read_vui_parameters(sps_t* sps, bs_t* b)
{
    sps->vui.aspect_ratio_info_present_flag = bs_read_u1(b);
    if (sps->vui.aspect_ratio_info_present_flag)
    {
        sps->vui.aspect_ratio_idc = bs_read_u8(b);
        if (sps->vui.aspect_ratio_idc == SAR_Extended)
        {
            sps->vui.sar_width = bs_read_u(b, 16);
            sps->vui.sar_height = bs_read_u(b, 16);
        }
    }
    sps->vui.overscan_info_present_flag = bs_read_u1(b);
    if (sps->vui.overscan_info_present_flag)
    {
        sps->vui.overscan_appropriate_flag = bs_read_u1(b);
    }
    sps->vui.video_signal_type_present_flag = bs_read_u1(b);
    if (sps->vui.video_signal_type_present_flag)
    {
        sps->vui.video_format = bs_read_u(b, 3);
        sps->vui.video_full_range_flag = bs_read_u1(b);
        sps->vui.colour_description_present_flag = bs_read_u1(b);
        if (sps->vui.colour_description_present_flag)
        {
            sps->vui.colour_primaries = bs_read_u8(b);
            sps->vui.transfer_characteristics = bs_read_u8(b);
            sps->vui.matrix_coefficients = bs_read_u8(b);
        }
    }
    sps->vui.chroma_loc_info_present_flag = bs_read_u1(b);
    if (sps->vui.chroma_loc_info_present_flag)
    {
        sps->vui.chroma_sample_loc_type_top_field = bs_read_ue(b);
        sps->vui.chroma_sample_loc_type_bottom_field = bs_read_ue(b);
    }
    sps->vui.timing_info_present_flag = bs_read_u1(b);
    if (sps->vui.timing_info_present_flag)
    {
        sps->vui.num_units_in_tick = bs_read_u(b, 32);
        sps->vui.time_scale = bs_read_u(b, 32);
        sps->vui.fixed_frame_rate_flag = bs_read_u1(b);
    }
    sps->vui.nal_hrd_parameters_present_flag = bs_read_u1(b);
    if (sps->vui.nal_hrd_parameters_present_flag)
    {
        read_hrd_parameters(sps, b);
    }
    sps->vui.vcl_hrd_parameters_present_flag = bs_read_u1(b);
    if (sps->vui.vcl_hrd_parameters_present_flag)
    {
        read_hrd_parameters(sps, b);
    }
    if (sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag)
    {
        sps->vui.low_delay_hrd_flag = bs_read_u1(b);
    }
    sps->vui.pic_struct_present_flag = bs_read_u1(b);
    sps->vui.bitstream_restriction_flag = bs_read_u1(b);
    if (sps->vui.bitstream_restriction_flag)
    {
        sps->vui.motion_vectors_over_pic_boundaries_flag = bs_read_u1(b);
        sps->vui.max_bytes_per_pic_denom = bs_read_ue(b);
        sps->vui.max_bits_per_mb_denom = bs_read_ue(b);
        sps->vui.log2_max_mv_length_horizontal = bs_read_ue(b);
        sps->vui.log2_max_mv_length_vertical = bs_read_ue(b);
        sps->vui.num_reorder_frames = bs_read_ue(b);
        sps->vui.max_dec_frame_buffering = bs_read_ue(b);
    }
}

static void read_seq_parameter_set_rbsp(sps_t* sps, bs_t* b)
{
    int profile_idc = bs_read_u8(b);
    int constraint_set0_flag = bs_read_u1(b);
    int constraint_set1_flag = bs_read_u1(b);
    int constraint_set2_flag = bs_read_u1(b);
    int constraint_set3_flag = bs_read_u1(b);
    int constraint_set4_flag = bs_read_u1(b);
    int constraint_set5_flag = bs_read_u1(b);
    int reserved_zero_2bits = bs_read_u(b, 2);  /* all 0's */
    int level_idc = bs_read_u8(b);
    int seq_parameter_set_id = bs_read_ue(b);
    int i = 0;

    sps->chroma_format_idc = 1;

    sps->profile_idc = profile_idc; // bs_read_u8(b);
    sps->constraint_set0_flag = constraint_set0_flag;//bs_read_u1(b);
    sps->constraint_set1_flag = constraint_set1_flag;//bs_read_u1(b);
    sps->constraint_set2_flag = constraint_set2_flag;//bs_read_u1(b);
    sps->constraint_set3_flag = constraint_set3_flag;//bs_read_u1(b);
    sps->constraint_set4_flag = constraint_set4_flag;//bs_read_u1(b);
    sps->constraint_set5_flag = constraint_set5_flag;//bs_read_u1(b);
    sps->reserved_zero_2bits = reserved_zero_2bits;//bs_read_u(b,2);
    sps->level_idc = level_idc; //bs_read_u8(b);
    sps->seq_parameter_set_id = seq_parameter_set_id; // bs_read_ue(b);

    if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 144)
    {
        sps->chroma_format_idc = bs_read_ue(b);
        sps->ChromaArrayType = sps->chroma_format_idc;
        if (sps->chroma_format_idc == 3)
        {
            sps->separate_colour_plane_flag = bs_read_u1(b);
            if (sps->separate_colour_plane_flag) sps->ChromaArrayType = 0;
        }
        sps->bit_depth_luma_minus8 = bs_read_ue(b);
        sps->bit_depth_chroma_minus8 = bs_read_ue(b);
        sps->qpprime_y_zero_transform_bypass_flag = bs_read_u1(b);
        sps->seq_scaling_matrix_present_flag = bs_read_u1(b);
        if (sps->seq_scaling_matrix_present_flag)
        {
            for (i = 0; i < ((sps->chroma_format_idc != 3) ? 8 : 12); i++)
            {
                sps->seq_scaling_list_present_flag[i] = bs_read_u1(b);
                if (sps->seq_scaling_list_present_flag[i])
                {
                    if (i < 6)
                    {
                        read_scaling_list(b, &sps->ScalingList4x4[i], 16,
                            sps->UseDefaultScalingMatrix4x4Flag[i]);
                    }
                    else
                    {
                        read_scaling_list(b, &sps->ScalingList8x8[i - 6], 64,
                            sps->UseDefaultScalingMatrix8x8Flag[i - 6]);
                    }
                }
            }
        }
    }
    sps->log2_max_frame_num_minus4 = bs_read_ue(b);
    sps->pic_order_cnt_type = bs_read_ue(b);
    if (sps->pic_order_cnt_type == 0)
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b);
    }
    else if (sps->pic_order_cnt_type == 1)
    {
        sps->delta_pic_order_always_zero_flag = bs_read_u1(b);
        sps->offset_for_non_ref_pic = bs_read_se(b);
        sps->offset_for_top_to_bottom_field = bs_read_se(b);
        sps->num_ref_frames_in_pic_order_cnt_cycle = bs_read_ue(b);
        for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            sps->offset_for_ref_frame[i] = bs_read_se(b);
        }
    }
    sps->max_num_ref_frames = bs_read_ue(b);
    sps->gaps_in_frame_num_value_allowed_flag = bs_read_u1(b);
    sps->pic_width_in_mbs_minus1 = bs_read_ue(b);
    sps->pic_height_in_map_units_minus1 = bs_read_ue(b);
    sps->frame_mbs_only_flag = bs_read_u1(b);
    if (!sps->frame_mbs_only_flag)
    {
        sps->mb_adaptive_frame_field_flag = bs_read_u1(b);
    }
    sps->direct_8x8_inference_flag = bs_read_u1(b);
    sps->frame_cropping_flag = bs_read_u1(b);
    if (sps->frame_cropping_flag)
    {
        sps->frame_crop_left_offset = bs_read_ue(b);
        sps->frame_crop_right_offset = bs_read_ue(b);
        sps->frame_crop_top_offset = bs_read_ue(b);
        sps->frame_crop_bottom_offset = bs_read_ue(b);
    }
    sps->vui_parameters_present_flag = bs_read_u1(b);
    if (sps->vui_parameters_present_flag)
    {
        read_vui_parameters(sps, b);
        /* 注：这里的帧率计算还有问题，x264编码25fps，time_scale为50，num_units_in_tick为1，计算得50fps
        网上说法，当nuit_field_based_flag为1时，再除以2，又说x264将该值设置为0.
        地址：http://forum.doom9.org/showthread.php?t=153019
        */
        float max_framerate;
        if (sps->vui.num_units_in_tick != 0)
            max_framerate = (float)(sps->vui.time_scale) / (float)(sps->vui.num_units_in_tick);
    }
    //read_rbsp_trailing_bits(h, b);

    // add by Late Lee
    //h->info->crop_left = sps->frame_crop_left_offset;
    //h->info->crop_right = sps->frame_crop_right_offset;
    //h->info->crop_top = sps->frame_crop_top_offset;
    //h->info->crop_bottom = sps->frame_crop_bottom_offset;

#if 0
    // 根据Table6-1及7.4.2.1.1计算宽、高
    int width = (sps->pic_width_in_mbs_minus1 + 1) * 16;
    int height = (2 - sps->frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1) * 16;

    if (sps->frame_cropping_flag)
    {
        unsigned int crop_unit_x;
        unsigned int crop_unit_y;
        if (0 == sps->chroma_format_idc) // monochrome
        {
            crop_unit_x = 1;
            crop_unit_y = 2 - sps->frame_mbs_only_flag;
        }
        else if (1 == sps->chroma_format_idc) // 4:2:0
        {
            crop_unit_x = 2;
            crop_unit_y = 2 * (2 - sps->frame_mbs_only_flag);
        }
        else if (2 == sps->chroma_format_idc) // 4:2:2
        {
            crop_unit_x = 2;
            crop_unit_y = 2 - sps->frame_mbs_only_flag;
        }
        else // 3 == sps.chroma_format_idc   // 4:4:4
        {
            crop_unit_x = 1;
            crop_unit_y = 2 - sps->frame_mbs_only_flag;
        }

        width -= crop_unit_x * (sps->frame_crop_left_offset + sps->frame_crop_right_offset);
        height -= crop_unit_y * (sps->frame_crop_top_offset + sps->frame_crop_bottom_offset);
    }
#else
    // 根据Table6-1及7.4.2.1.1计算宽、高
   // int sub_width_c = ((1 == sps->chroma_format_idc) || (2 == sps->chroma_format_idc)) && (0 == sps->separate_colour_plane_flag) ? 2 : 1;
   // int sub_height_c = (1 == sps->chroma_format_idc) && (0 == sps->separate_colour_plane_flag) ? 2 : 1;
   // h->info->width = ((sps->pic_width_in_mbs_minus1 + 1) * 16) - sps->frame_crop_left_offset * sub_width_c - sps->frame_crop_right_offset * sub_width_c;
   // h->info->height = ((2 - sps->frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1) * 16) - (sps->frame_crop_top_offset * sub_height_c) - (sps->frame_crop_bottom_offset * sub_height_c);
#endif
    //h->info->bit_depth_luma = sps->bit_depth_luma_minus8 + 8;
    //h->info->bit_depth_chroma = sps->bit_depth_chroma_minus8 + 8;
    //h->info->profile_idc = sps->profile_idc;
    //h->info->level_idc = sps->level_idc;
    // YUV空间
    //h->info->chroma_format_idc = sps->chroma_format_idc;
}

int h264_decode_sps_t(sps_t* sps, void* spsdata, int size, int *pwidth, int *pheight)
{
    //bs_t* b = bs_new(spsdata, size);
    bs_t bs;
    bs_t* b=&bs;

    memset(b, 0, sizeof(bs));
    bs_init(b, spsdata, size);

    int forbidden_zero_bit = bs_read_f(b, 1);
    int nal_ref_idc = bs_read_u(b, 2);
    int nal_unit_type = bs_read_u(b, 5);
    
    int nal_size = size;
    int rbsp_size = size;
    uint8_t* rbsp_buf = (uint8_t*)malloc(rbsp_size + 1);

    int rc = nal_to_rbsp(1, spsdata, &nal_size, rbsp_buf, &rbsp_size);

    if (rc < 0) { free(rbsp_buf); return -1; } // handle conversion error

    //b = bs_new(rbsp_buf, rbsp_size);
    bs_init(&bs, rbsp_buf, rbsp_size);

    if (nal_unit_type == 0x07) 
    {
        read_seq_parameter_set_rbsp(sps, b);
        int width = (sps->pic_width_in_mbs_minus1 + 1) * 16;
        int height = (2 - sps->frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1) * 16;

        if (sps->frame_cropping_flag)
        {
            unsigned int crop_unit_x;
            unsigned int crop_unit_y;
            if (0 == sps->chroma_format_idc) // monochrome
            {
                crop_unit_x = 1;
                crop_unit_y = 2 - sps->frame_mbs_only_flag;
            }
            else if (1 == sps->chroma_format_idc) // 4:2:0
            {
                crop_unit_x = 2;
                crop_unit_y = 2 * (2 - sps->frame_mbs_only_flag);
            }
            else if (2 == sps->chroma_format_idc) // 4:2:2
            {
                crop_unit_x = 2;
                crop_unit_y = 2 - sps->frame_mbs_only_flag;
            }
            else // 3 == sps.chroma_format_idc   // 4:4:4
            {
                crop_unit_x = 1;
                crop_unit_y = 2 - sps->frame_mbs_only_flag;
            }

            width -= crop_unit_x * (sps->frame_crop_left_offset + sps->frame_crop_right_offset);
            height -= crop_unit_y * (sps->frame_crop_top_offset + sps->frame_crop_bottom_offset);
        }
        if(pwidth)
            *pwidth = width;
        if(pheight)
            *pheight = height;
    }
    free(rbsp_buf);
    return 0;
}

void h264_sps_deubg_out(sps_t* sps)
{
    printf("======= SPS =======\n");
    printf(" profile_idc : %d \n", sps->profile_idc);
    printf(" constraint_set0_flag : %d \n", sps->constraint_set0_flag);
    printf(" constraint_set1_flag : %d \n", sps->constraint_set1_flag);
    printf(" constraint_set2_flag : %d \n", sps->constraint_set2_flag);
    printf(" constraint_set3_flag : %d \n", sps->constraint_set3_flag);
    printf(" constraint_set4_flag : %d \n", sps->constraint_set4_flag);
    printf(" constraint_set5_flag : %d \n", sps->constraint_set5_flag);
    printf(" reserved_zero_2bits : %d \n", sps->reserved_zero_2bits);
    printf(" level_idc : %d \n", sps->level_idc);
    printf(" seq_parameter_set_id : %d \n", sps->seq_parameter_set_id);
    printf(" chroma_format_idc : %d \n", sps->chroma_format_idc);
    printf(" separate_colour_plane_flag : %d \n", sps->separate_colour_plane_flag);
    printf(" bit_depth_luma_minus8 : %d \n", sps->bit_depth_luma_minus8);
    printf(" bit_depth_chroma_minus8 : %d \n", sps->bit_depth_chroma_minus8);
    printf(" qpprime_y_zero_transform_bypass_flag : %d \n", sps->qpprime_y_zero_transform_bypass_flag);
    printf(" seq_scaling_matrix_present_flag : %d \n", sps->seq_scaling_matrix_present_flag);
    //  int seq_scaling_list_present_flag[8];
    //  void* ScalingList4x4[6];
    //  int UseDefaultScalingMatrix4x4Flag[6];
    //  void* ScalingList8x8[2];
    //  int UseDefaultScalingMatrix8x8Flag[2];
    printf(" log2_max_frame_num_minus4 : %d \n", sps->log2_max_frame_num_minus4);
    printf(" pic_order_cnt_type : %d \n", sps->pic_order_cnt_type);
    printf("   log2_max_pic_order_cnt_lsb_minus4 : %d \n", sps->log2_max_pic_order_cnt_lsb_minus4);
    printf("   delta_pic_order_always_zero_flag : %d \n", sps->delta_pic_order_always_zero_flag);
    printf("   offset_for_non_ref_pic : %d \n", sps->offset_for_non_ref_pic);
    printf("   offset_for_top_to_bottom_field : %d \n", sps->offset_for_top_to_bottom_field);
    printf("   num_ref_frames_in_pic_order_cnt_cycle : %d \n", sps->num_ref_frames_in_pic_order_cnt_cycle);
    //  int offset_for_ref_frame[256];
    printf(" max_num_ref_frames : %d \n", sps->max_num_ref_frames);
    printf(" gaps_in_frame_num_value_allowed_flag : %d \n", sps->gaps_in_frame_num_value_allowed_flag);
    printf(" pic_width_in_mbs_minus1 : %d \n", sps->pic_width_in_mbs_minus1);
    printf(" pic_height_in_map_units_minus1 : %d \n", sps->pic_height_in_map_units_minus1);
    printf(" frame_mbs_only_flag : %d \n", sps->frame_mbs_only_flag);
    printf(" mb_adaptive_frame_field_flag : %d \n", sps->mb_adaptive_frame_field_flag);
    printf(" direct_8x8_inference_flag : %d \n", sps->direct_8x8_inference_flag);
    printf(" frame_cropping_flag : %d \n", sps->frame_cropping_flag);
    printf("   frame_crop_left_offset : %d \n", sps->frame_crop_left_offset);
    printf("   frame_crop_right_offset : %d \n", sps->frame_crop_right_offset);
    printf("   frame_crop_top_offset : %d \n", sps->frame_crop_top_offset);
    printf("   frame_crop_bottom_offset : %d \n", sps->frame_crop_bottom_offset);
    printf(" vui_parameters_present_flag : %d \n", sps->vui_parameters_present_flag);

    printf("=== VUI ===\n");
    printf(" aspect_ratio_info_present_flag : %d \n", sps->vui.aspect_ratio_info_present_flag);
    printf("   aspect_ratio_idc : %d \n", sps->vui.aspect_ratio_idc);
    printf("     sar_width : %d \n", sps->vui.sar_width);
    printf("     sar_height : %d \n", sps->vui.sar_height);
    printf(" overscan_info_present_flag : %d \n", sps->vui.overscan_info_present_flag);
    printf("   overscan_appropriate_flag : %d \n", sps->vui.overscan_appropriate_flag);
    printf(" video_signal_type_present_flag : %d \n", sps->vui.video_signal_type_present_flag);
    printf("   video_format : %d \n", sps->vui.video_format);
    printf("   video_full_range_flag : %d \n", sps->vui.video_full_range_flag);
    printf("   colour_description_present_flag : %d \n", sps->vui.colour_description_present_flag);
    printf("     colour_primaries : %d \n", sps->vui.colour_primaries);
    printf("   transfer_characteristics : %d \n", sps->vui.transfer_characteristics);
    printf("   matrix_coefficients : %d \n", sps->vui.matrix_coefficients);
    printf(" chroma_loc_info_present_flag : %d \n", sps->vui.chroma_loc_info_present_flag);
    printf("   chroma_sample_loc_type_top_field : %d \n", sps->vui.chroma_sample_loc_type_top_field);
    printf("   chroma_sample_loc_type_bottom_field : %d \n", sps->vui.chroma_sample_loc_type_bottom_field);
    printf(" timing_info_present_flag : %d \n", sps->vui.timing_info_present_flag);
    printf("   num_units_in_tick : %d \n", sps->vui.num_units_in_tick);
    printf("   time_scale : %d \n", sps->vui.time_scale);
    printf("   fixed_frame_rate_flag : %d \n", sps->vui.fixed_frame_rate_flag);
    printf(" nal_hrd_parameters_present_flag : %d \n", sps->vui.nal_hrd_parameters_present_flag);
    printf(" vcl_hrd_parameters_present_flag : %d \n", sps->vui.vcl_hrd_parameters_present_flag);
    printf("   low_delay_hrd_flag : %d \n", sps->vui.low_delay_hrd_flag);
    printf(" pic_struct_present_flag : %d \n", sps->vui.pic_struct_present_flag);
    printf(" bitstream_restriction_flag : %d \n", sps->vui.bitstream_restriction_flag);
    printf("   motion_vectors_over_pic_boundaries_flag : %d \n", sps->vui.motion_vectors_over_pic_boundaries_flag);
    printf("   max_bytes_per_pic_denom : %d \n", sps->vui.max_bytes_per_pic_denom);
    printf("   max_bits_per_mb_denom : %d \n", sps->vui.max_bits_per_mb_denom);
    printf("   log2_max_mv_length_horizontal : %d \n", sps->vui.log2_max_mv_length_horizontal);
    printf("   log2_max_mv_length_vertical : %d \n", sps->vui.log2_max_mv_length_vertical);
    printf("   num_reorder_frames : %d \n", sps->vui.num_reorder_frames);
    printf("   max_dec_frame_buffering : %d \n", sps->vui.max_dec_frame_buffering);

    printf("=== HRD ===\n");
    printf(" cpb_cnt_minus1 : %d \n", sps->hrd.cpb_cnt_minus1);
    printf(" bit_rate_scale : %d \n", sps->hrd.bit_rate_scale);
    printf(" cpb_size_scale : %d \n", sps->hrd.cpb_size_scale);
    int SchedSelIdx;
    for (SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++)
    {
        printf("   bit_rate_value_minus1[%d] : %d \n", SchedSelIdx, sps->hrd.bit_rate_value_minus1[SchedSelIdx]); // up to cpb_cnt_minus1, which is <= 31
        printf("   cpb_size_value_minus1[%d] : %d \n", SchedSelIdx, sps->hrd.cpb_size_value_minus1[SchedSelIdx]);
        printf("   cbr_flag[%d] : %d \n", SchedSelIdx, sps->hrd.cbr_flag[SchedSelIdx]);
    }
    printf(" initial_cpb_removal_delay_length_minus1 : %d \n", sps->hrd.initial_cpb_removal_delay_length_minus1);
    printf(" cpb_removal_delay_length_minus1 : %d \n", sps->hrd.cpb_removal_delay_length_minus1);
    printf(" dpb_output_delay_length_minus1 : %d \n", sps->hrd.dpb_output_delay_length_minus1);
    printf(" time_offset_length : %d \n", sps->hrd.time_offset_length);
}