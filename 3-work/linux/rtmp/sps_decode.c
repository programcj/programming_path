/*
 * sps_decode.c
 *
 *  Created on: 2020年11月7日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "sps_decode.h"
#include "bs.h"

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned long DWORD;

void get_profile(int profile_idc, char* profile_str)
{
	switch (profile_idc)
	{
		case 66:
			strcpy(profile_str, "Baseline");
		break;
		case 77:
			strcpy(profile_str, "Main");
		break;
		case 88:
			strcpy(profile_str, "Extended");
		break;
		case 100:
			strcpy(profile_str, "High(FRExt)");
		break;
		case 110:
			strcpy(profile_str, "High10(FRExt)");
		break;
		case 122:
			strcpy(profile_str, "High4:2:2(FRExt)");
		break;
		case 144:
			strcpy(profile_str, "High4:4:4(FRExt)");
		break;
		default:
			strcpy(profile_str, "Unknown");
	}
}

UINT Ue(BYTE *pBuff, UINT nLen, UINT *nStartBit)
{
	UINT nZeroNum = 0;
	while (*nStartBit < nLen * 8)
	{
		if (pBuff[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
		{
			break;
		}
		nZeroNum++;
		(*nStartBit)++;
	}
	(*nStartBit)++;

	DWORD dwRet = 0;
	UINT i;
	for (i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[*nStartBit / 8] & (0x80 >> ((*nStartBit) % 8)))
		{
			dwRet += 1;
		}
		(*nStartBit)++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}

int Se(BYTE *pBuff, UINT nLen, UINT *nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}

DWORD u(UINT BitCount, BYTE * buf, UINT *nStartBit)
{
	DWORD dwRet = 0;
	UINT i;
	for (i = 0; i < BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
		{
			dwRet += 1;
		}
		(*nStartBit)++;
	}
	return dwRet;
}

void de_emulation_prevention(BYTE* buf, unsigned int* buf_size)
{
	int i = 0, j = 0;
	BYTE* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i < (tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j < tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}
}

#if 0
int h264_decode_sps(BYTE * buf, unsigned int nLen, int *width, int *height, int *fps)
{
	UINT StartBit = 0;
	*fps = 0;
	de_emulation_prevention(buf, &nLen);

	int timing_info_present_flag = 0;
	int forbidden_zero_bit = u(1, buf, &StartBit);
	int nal_ref_idc = u(2, buf, &StartBit);
	int nal_unit_type = u(5, buf, &StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, buf, &StartBit);
		int constraint_set0_flag = u(1, buf, &StartBit); //(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, buf, &StartBit); //(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, buf, &StartBit); //(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, buf, &StartBit); //(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, buf, &StartBit);
		int level_idc = u(8, buf, &StartBit);

		int seq_parameter_set_id = Ue(buf, nLen, &StartBit);
		int chroma_format_idc = 0;
		if (profile_idc == 100 || profile_idc == 110 ||
					profile_idc == 122 || profile_idc == 144)
		{
			chroma_format_idc = Ue(buf, nLen, &StartBit);
			if (chroma_format_idc == 3)
			{
				DWORD residual_colour_transform_flag = u(1, buf, &StartBit);
			}
			int bit_depth_luma_minus8 = Ue(buf, nLen, &StartBit);
			int bit_depth_chroma_minus8 = Ue(buf, nLen, &StartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, buf, &StartBit);
			int seq_scaling_matrix_present_flag = u(1, buf, &StartBit);

			int seq_scaling_list_present_flag[8];
			int i;
			if (seq_scaling_matrix_present_flag)
			{
				for (i = 0; i < 8; i++)
				{
					seq_scaling_list_present_flag[i] = u(1, buf, &StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4 = Ue(buf, nLen, &StartBit);
		int pic_order_cnt_type = Ue(buf, nLen, &StartBit);
		if (pic_order_cnt_type == 0)
		{
			UINT log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, &StartBit);
		}
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, buf, &StartBit);
			int offset_for_non_ref_pic = Se(buf, nLen, &StartBit);
			int offset_for_top_to_bottom_field = Se(buf, nLen, &StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, &StartBit);
			int i;
			int *offset_for_ref_frame = (int*) malloc(num_ref_frames_in_pic_order_cnt_cycle * sizeof(int));
			for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, &StartBit);
			free(offset_for_ref_frame);
		}
		int num_ref_frames = Ue(buf, nLen, &StartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, buf, &StartBit);
		int pic_width_in_mbs_minus1 = Ue(buf, nLen, &StartBit);
		int pic_height_in_map_units_minus1 = Ue(buf, nLen, &StartBit);

		int frame_mbs_only_flag = u(1, buf, &StartBit);

		*width = (pic_width_in_mbs_minus1 + 1) * 16;
		*height = (pic_height_in_map_units_minus1 + 1) * 16; //* (2 - frame_mbs_only_flag)

		if (!frame_mbs_only_flag)
		{
			DWORD mb_adaptive_frame_field_flag = u(1, buf, &StartBit);
		}

		int direct_8x8_inference_flag = u(1, buf, &StartBit);
		int frame_cropping_flag = u(1, buf, &StartBit);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset = Ue(buf, nLen, &StartBit);
			int frame_crop_right_offset = Ue(buf, nLen, &StartBit);
			int frame_crop_top_offset = Ue(buf, nLen, &StartBit);
			int frame_crop_bottom_offset = Ue(buf, nLen, &StartBit);

//			int hsub = (chroma_format_idc == 1 ||
//						chroma_format_idc == 2) ? 1 : 0;
//			int step_x = 1 << hsub;
//			int vsub = (chroma_format_idc == 1) ? 1 : 0;
//			int step_y = (2 - frame_mbs_only_flag) << vsub;
//
//			printf("step_y=%d\n", step_y);
//			frame_crop_left_offset = frame_crop_left_offset * step_x;
//			frame_crop_right_offset = frame_crop_right_offset * step_x;
//			frame_crop_top_offset = frame_crop_top_offset * step_y;
//			frame_crop_bottom_offset = frame_crop_bottom_offset * step_y;

			*width = *width - 2 * (frame_crop_right_offset + frame_crop_left_offset);
			*height = *height - 2 * (frame_crop_top_offset + frame_crop_bottom_offset);
		}
		int vui_parameter_present_flag = u(1, buf, &StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, buf, &StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, buf, &StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, buf, &StartBit);
					int sar_height = u(16, buf, &StartBit);
				}
			}
			int overscan_info_present_flag = u(1, buf, &StartBit);
			if (overscan_info_present_flag)
			{
				DWORD overscan_appropriate_flagu = u(1, buf, &StartBit);
			}
			int video_signal_type_present_flag = u(1, buf, &StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, buf, &StartBit);
				int video_full_range_flag = u(1, buf, &StartBit);
				int colour_description_present_flag = u(1, buf, &StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, buf, &StartBit);
					int transfer_characteristics = u(8, buf, &StartBit);
					int matrix_coefficients = u(8, buf, &StartBit);
				}
			}
			int chroma_loc_info_present_flag = u(1, buf, &StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(buf, nLen, &StartBit);
				int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, &StartBit);
			}
			timing_info_present_flag = u(1, buf, &StartBit);

			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, buf, &StartBit);
				int time_scale = u(32, buf, &StartBit);
				int fixed_frame_rate_flag = u(1, buf, &StartBit);

				*fps = time_scale / num_units_in_tick;
				if (fixed_frame_rate_flag)
				{
					*fps = *fps / 2;
				}
			}
		}

		char profile_str[32] =
					{ 0 };
		get_profile(profile_idc, &profile_str[0]);

		if (timing_info_present_flag)
		{
			//printf("H.264 SPS: -> video size %dx%d, %d fps, profile(%d) %s\n",
			//			*width, *height, fps, profile_idc, profile_str);
		}
		else
		{
			//printf("H.264 SPS: -> video size %dx%d, unknown fps, profile(%d) %s\n",
			//			*width, *height, profile_idc, profile_str);
		}

		return 0;
	}
	else
		return -1;
}
#endif

//https://github.com/wexiangis/rtsp_to_h264/tree/6826f646306f3458d0a773bcd45a1ca861eff13f
int h265_decode_sps(unsigned char *buf, unsigned int nLen, int *width, int *height, int *fps)
{

	unsigned int StartBit = 0;
	de_emulation_prevention(buf, &nLen);

	//--- nal_uint_header ---
	int forbidden_zero_bit = u(1, buf, &StartBit);
	int nal_unit_type = u(6, buf, &StartBit);
	if (nal_unit_type != 33)
		return 0;
	int nuh_layer_id = u(6, buf, &StartBit);
	int nuh_temporal_id_plus = u(3, buf, &StartBit);

	//--- seq_parameter_set_rbsp ---
	int sps_video_parameter_set_id = u(4, buf, &StartBit);
	int sps_max_sub_layers_minus1 = u(3, buf, &StartBit);
	int sps_temporal_id_nesting_flag = u(1, buf, &StartBit);
	// printf("sps_video_parameter_set_id/%d\n"
	//     "sps_max_sub_layers_minus1/%d\n"
	//     "sps_temporal_id_nesting_flag/%d\n",
	//     sps_video_parameter_set_id,
	//     sps_max_sub_layers_minus1,
	//     sps_temporal_id_nesting_flag);
	if (sps_temporal_id_nesting_flag) //--- profile_tier_level ---
	{
		int general_profile_space = u(2, buf, &StartBit);
		int general_tier_flag = u(1, buf, &StartBit);
		int general_profile_idc = u(5, buf, &StartBit);
		int general_profile_compatibility_flag[32];
		// printf("general_profile_space/%d\n"
		//     "general_tier_flag/%d\n"
		//     "general_profile_idc/%d\n",
		//     general_profile_space,
		//     general_tier_flag,
		//     general_profile_idc);
		int j;
		for (j = 0; j < 32; j++)
		{
			general_profile_compatibility_flag[j] = u(1, buf, &StartBit);
			// printf("bit[%d]: %d\n", j, general_profile_compatibility_flag[j]);
		}
		int general_progressive_source_flag = u(1, buf, &StartBit);
		int general_interlaced_source_flag = u(1, buf, &StartBit);
		int general_non_packed_constraint_flag = u(1, buf, &StartBit);
		int general_frame_only_constraint_flag = u(1, buf, &StartBit);
		if (general_profile_idc == 4 || general_profile_compatibility_flag[4] ||
					general_profile_idc == 5 || general_profile_compatibility_flag[5] ||
					general_profile_idc == 6 || general_profile_compatibility_flag[6] ||
					general_profile_idc == 7 || general_profile_compatibility_flag[7] ||
					general_profile_idc == 8 || general_profile_compatibility_flag[8] ||
					general_profile_idc == 9 || general_profile_compatibility_flag[9] ||
					general_profile_idc == 10 || general_profile_compatibility_flag[10])
		{
			// printf("> hit 1-1\n");
			int general_max_12bit_constraint_flag = u(1, buf, &StartBit);
			int general_max_10bit_constraint_flag = u(1, buf, &StartBit);
			int general_max_8bit_constraint_flag = u(1, buf, &StartBit);
			int general_max_422chroma_constraint_flag = u(1, buf, &StartBit);
			int general_max_420chroma_constraint_flag = u(1, buf, &StartBit);
			int general_max_monochrome_constraint_flag = u(1, buf, &StartBit);
			int general_intra_constraint_flag = u(1, buf, &StartBit);
			int general_one_picture_only_constraint_flag = u(1, buf, &StartBit);
			int general_lower_bit_rate_constraint_flag = u(1, buf, &StartBit);
			if (general_profile_idc == 5 || general_profile_compatibility_flag[5] ||
						general_profile_idc == 9 || general_profile_compatibility_flag[9] ||
						general_profile_idc == 10 || general_profile_compatibility_flag[10])
			{
				int general_max_14bit_constraint_flag = u(1, buf, &StartBit);
				int general_reserved_zero_33bits = u(33, buf, &StartBit);
			}
			else
			{
				int general_reserved_zero_34bits = u(34, buf, &StartBit);
			}
		}
		else if (general_profile_idc == 2 || general_profile_compatibility_flag[2])
		{
			// printf("> hit 1-2\n");
			int general_reserved_zero_7bits = u(7, buf, &StartBit);
			int general_one_picture_only_constraint_flag = u(1, buf, &StartBit);
			int general_reserved_zero_35bits = u(35, buf, &StartBit);
		}
		else
		{
			// printf("> hit 1-3\n");
			int general_reserved_zero_43bits = u(43, buf, &StartBit);
		}
		if ((general_profile_idc >= 1 && general_profile_idc <= 5) ||
					general_profile_idc == 9 ||
					general_profile_compatibility_flag[1] || general_profile_compatibility_flag[2] ||
					general_profile_compatibility_flag[3] || general_profile_compatibility_flag[4] ||
					general_profile_compatibility_flag[5] || general_profile_compatibility_flag[9])
		{
			// printf("> hit 2-1\n");
			int general_inbld_flag = u(1, buf, &StartBit);
		}
		else
		{
			// printf("> hit 2-2\n");
			int general_reserved_zero_bit = u(1, buf, &StartBit);
		}
		int general_level_idc = u(8, buf, &StartBit);
		if (sps_max_sub_layers_minus1 > 0)
		{
			fprintf(stderr, "error: sps_max_sub_layers_minus1 must 0 (%d)\n",
						sps_max_sub_layers_minus1);
			return 0;
		}
	}
	int sps_seq_parameter_set_id = Ue(buf, nLen, &StartBit);
	int chroma_format_idc = Ue(buf, nLen, &StartBit);
	if (chroma_format_idc == 3)
	{
		int separate_colour_plane_flag = u(1, buf, &StartBit);
	}
	int pic_width_in_luma_samples = Ue(buf, nLen, &StartBit);
	int pic_height_in_luma_samples = Ue(buf, nLen, &StartBit);
	int conformance_window_flag = u(1, buf, &StartBit);

	int conf_win_left_offset = 0;
	int conf_win_right_offset = 0;
	int conf_win_top_offset = 0;
	int conf_win_bottom_offset = 0;
	if (conformance_window_flag)
	{
		int conf_win_left_offset = Ue(buf, nLen, &StartBit);
		int conf_win_right_offset = Ue(buf, nLen, &StartBit);
		int conf_win_top_offset = Ue(buf, nLen, &StartBit);
		int conf_win_bottom_offset = Ue(buf, nLen, &StartBit);
	}

	// printf("forbidden_zero_bit/%d,\n"
	// "nal_unit_type/%d, nuh_layer_id/%d,\n"
	// "sps_video_parameter_set_id/%d,\n"
	// "sps_max_sub_layers_minus1/%d,\n"
	// "sps_temporal_id_nesting_flag/%d\n"
	// "sps_seq_parameter_set_id/%d\n"
	// "chroma_format_idc/%d\n",
	//     forbidden_zero_bit,
	//     nal_unit_type,
	//     nuh_layer_id,
	//     sps_video_parameter_set_id,
	//     sps_max_sub_layers_minus1,
	//     sps_temporal_id_nesting_flag,
	//     sps_seq_parameter_set_id,
	//     chroma_format_idc);

	*width = pic_width_in_luma_samples - (conf_win_left_offset + conf_win_right_offset);
	*height = pic_height_in_luma_samples - (conf_win_top_offset + conf_win_bottom_offset);
	*fps = 0;
	return 0;
}


/**
 Sequence Parameter Set
 @see 7.3.2.1 Sequence parameter set RBSP syntax
 @see read_seq_parameter_set_rbsp
 @see write_seq_parameter_set_rbsp
 @see debug_sps
 */
typedef struct
{
	int profile_idc;
	int constraint_set0_flag;
	int constraint_set1_flag;
	int constraint_set2_flag;
	int constraint_set3_flag;
	int constraint_set4_flag;
	int constraint_set5_flag;
	int reserved_zero_2bits;
	int level_idc;
	int seq_parameter_set_id;
	int chroma_format_idc;
	int residual_colour_transform_flag;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	int qpprime_y_zero_transform_bypass_flag;
	int seq_scaling_matrix_present_flag;
	int seq_scaling_list_present_flag[8];
	int ScalingList4x4[6];
	int UseDefaultScalingMatrix4x4Flag[6];
	int ScalingList8x8[2];
	int UseDefaultScalingMatrix8x8Flag[2];
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int offset_for_ref_frame[256];
	int num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;
	int frame_cropping_flag;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;
	int vui_parameters_present_flag;

#if 0
	struct
	{
		int aspect_ratio_info_present_flag;
		int aspect_ratio_idc;
		int sar_width;
		int sar_height;
		int overscan_info_present_flag;
		int overscan_appropriate_flag;
		int video_signal_type_present_flag;
		int video_format;
		int video_full_range_flag;
		int colour_description_present_flag;
		int colour_primaries;
		int transfer_characteristics;
		int matrix_coefficients;
		int chroma_loc_info_present_flag;
		int chroma_sample_loc_type_top_field;
		int chroma_sample_loc_type_bottom_field;
		int timing_info_present_flag;
		int num_units_in_tick;
		int time_scale;
		int fixed_frame_rate_flag;
		int nal_hrd_parameters_present_flag;
		int vcl_hrd_parameters_present_flag;
		int low_delay_hrd_flag;
		int pic_struct_present_flag;
		int bitstream_restriction_flag;
		int motion_vectors_over_pic_boundaries_flag;
		int max_bytes_per_pic_denom;
		int max_bits_per_mb_denom;
		int log2_max_mv_length_horizontal;
		int log2_max_mv_length_vertical;
		int num_reorder_frames;
		int max_dec_frame_buffering;
	} vui;


	struct
	{
		int cpb_cnt_minus1;
		int bit_rate_scale;
		int cpb_size_scale;
		int bit_rate_value_minus1[32]; // up to cpb_cnt_minus1, which is <= 31
		int cpb_size_value_minus1[32];
		int cbr_flag[32];
		int initial_cpb_removal_delay_length_minus1;
		int cpb_removal_delay_length_minus1;
		int dpb_output_delay_length_minus1;
		int time_offset_length;
	} hrd;
#endif

} sps_t;

#define SAR_Extended      255        // Extended_SAR

//7.3.2.1.1 Scaling list syntax
void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList,
	int useDefaultScalingMatrixFlag)
{
	int j;

	// OutputDebugString("read_scaling_list");

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

#if 0
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
#endif

#if 0
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
	if (sps->vui.nal_hrd_parameters_present_flag
		|| sps->vui.vcl_hrd_parameters_present_flag)
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


//7.3.2.11 RBSP trailing bits syntax
void read_rbsp_trailing_bits(bs_t* b)
{
	int rbsp_stop_one_bit = bs_read_u1(b); // equal to 1

	while (!bs_byte_aligned(b))
	{
		int rbsp_alignment_zero_bit = bs_read_u1(b); // equal to 0
	}
}
#endif

void read_seq_parameter_set_rbsp(sps_t* sps, bs_t* b)
{
	int i;

	int profile_idc = bs_read_u8(b);
	int constraint_set0_flag = bs_read_u1(b);
	int constraint_set1_flag = bs_read_u1(b);
	int constraint_set2_flag = bs_read_u1(b);
	int constraint_set3_flag = bs_read_u1(b);
	int constraint_set4_flag = bs_read_u1(b);
	int constraint_set5_flag = bs_read_u1(b);
	int reserved_zero_2bits = bs_read_u(b, 2); /* all 0's */
	int level_idc = bs_read_u8(b);
	int seq_parameter_set_id = bs_read_ue(b);

	// select the correct sps
	// h->sps = h->sps_table[seq_parameter_set_id];
	// sps_t* sps = h->sps;
	// memset(sps, 0, sizeof(sps_t));

	sps->chroma_format_idc = 1;

	sps->profile_idc = profile_idc; // bs_read_u8(b);
	sps->constraint_set0_flag = constraint_set0_flag; //bs_read_u1(b);
	sps->constraint_set1_flag = constraint_set1_flag; //bs_read_u1(b);
	sps->constraint_set2_flag = constraint_set2_flag; //bs_read_u1(b);
	sps->constraint_set3_flag = constraint_set3_flag; //bs_read_u1(b);
	sps->constraint_set4_flag = constraint_set4_flag; //bs_read_u1(b);
	sps->constraint_set5_flag = constraint_set5_flag; //bs_read_u1(b);
	sps->reserved_zero_2bits = reserved_zero_2bits; //bs_read_u(b,2);
	sps->level_idc = level_idc; //bs_read_u8(b);
	sps->seq_parameter_set_id = seq_parameter_set_id; // bs_read_ue(b);
	if (sps->profile_idc == 100 || sps->profile_idc == 110
		|| sps->profile_idc == 122 || sps->profile_idc == 144)
	{
		sps->chroma_format_idc = bs_read_ue(b);
		if (sps->chroma_format_idc == 3)
		{
			sps->residual_colour_transform_flag = bs_read_u1(b);
		}
		sps->bit_depth_luma_minus8 = bs_read_ue(b);
		sps->bit_depth_chroma_minus8 = bs_read_ue(b);
		sps->qpprime_y_zero_transform_bypass_flag = bs_read_u1(b);
		sps->seq_scaling_matrix_present_flag = bs_read_u1(b);
		if (sps->seq_scaling_matrix_present_flag)
		{
			for (i = 0; i < 8; i++)
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
	sps->num_ref_frames = bs_read_ue(b);
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

#if 0 //解码会导致栈错误
	sps->vui_parameters_present_flag = bs_read_u1(b);

	if (sps->vui_parameters_present_flag)
	{
		read_vui_parameters(sps, b);
	}
	read_rbsp_trailing_bits(b);
#endif
}

int h264_decode_sps(void* buf, unsigned int nLen, int* pwidth, int* pheight,
	int* pfps)
{
	unsigned char* h264ptr = (unsigned char*)buf;
	int type = h264ptr[0] & 0x1F;

	h264ptr++;
	nLen--;

	sps_t sps;
	bs_t b;
	memset(&sps, 0, sizeof(sps));
	memset(&b, 0, sizeof(b));
	bs_init(&b, h264ptr, nLen);
	read_seq_parameter_set_rbsp(&sps, &b);

	int width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
	int height = (sps.pic_height_in_map_units_minus1 + 1) * 16;

	if (sps.frame_cropping_flag)
	{
		width = width
			- 2
			* (sps.frame_crop_right_offset
				+ sps.frame_crop_left_offset);
		height = height
			- 2
			* (sps.frame_crop_top_offset
				+ sps.frame_crop_bottom_offset);
	}
	if (pwidth)
		*pwidth = width;
	if (pheight)
		*pheight = height;
	return 0;
}
