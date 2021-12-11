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

#include "bs.h"
#include "h264_stream.h"
#include "h265_stream.h"

int h264_decode_sps(unsigned char* buf, unsigned int nLen, int *width, int *height, int *fps)
{
	sps_t sps;
	h264_decode_sps_t(&sps, buf, nLen, width, height);
	return 0;
}

/**
   Convert NAL data (Annex B format) to RBSP data.
   The size of rbsp_buf must be the same as size of the nal_buf to guarantee the output will fit.
   If that is not true, output may be truncated and an error will be returned.
   Additionally, certain byte sequences in the input nal_buf are not allowed in the spec and also cause the conversion to fail and an error to be returned.
   @param[in] nal_header_size the nal header
   @param[in] nal_buf   the nal data
   @param[in,out] nal_size  as input, pointer to the size of the nal data; as output, filled in with the actual size of the nal data
   @param[in,out] rbsp_buf   allocated memory in which to put the rbsp data
   @param[in,out] rbsp_size  as input, pointer to the maximum size of the rbsp data; as output, filled in with the actual size of rbsp data
   @return  actual size of rbsp data, or -1 on error
 */
 // 7.3.1 NAL unit syntax
 // 7.4.1.1 Encapsulation of an SODB within an RBSP
int nal_to_rbsp(const int nal_header_size, const uint8_t* nal_buf, int* nal_size, uint8_t* rbsp_buf, int* rbsp_size)
{
	int i;
	int j = 0;
	int count = 0;

	for (i = nal_header_size; i < *nal_size; i++)
	{
		// in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
		if ((count == 2) && (nal_buf[i] < 0x03))
		{
			return -1;
		}

		if ((count == 2) && (nal_buf[i] == 0x03))
		{
			// check the 4th byte after 0x000003, except when cabac_zero_word is used, in which case the last three bytes of this NAL unit must be 0x000003
			if ((i < *nal_size - 1) && (nal_buf[i + 1] > 0x03))
			{
				return -1;
			}

			// if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded, and the last two bytes of RBSP must be 0x0000
			if (i == *nal_size - 1)
			{
				break;
			}

			i++;
			count = 0;
		}

		if (j >= *rbsp_size)
		{
			// error, not enough space
			return -1;
		}

		rbsp_buf[j] = nal_buf[i];
		if (nal_buf[i] == 0x00)
		{
			count++;
		}
		else
		{
			count = 0;
		}
		j++;
	}

	*nal_size = i;
	*rbsp_size = j;
	return j;
}

//https://github.com/wexiangis/rtsp_to_h264/tree/6826f646306f3458d0a773bcd45a1ca861eff13f
int h265_decode_sps(unsigned char *buf, unsigned int nLen, int *width, int *height, int *fps)
{
#if 1
	h265_sps_t sps;
	h265_decode_sps_t(&sps, buf, nLen, width, height);
#else
	bs_t bs;
	bs_t* b = &bs;
	memset(&bs, 0, sizeof(bs));
	bs_init(&bs, buf, nLen);
	int forbidden_zero_bit = bs_read_f(b, 1);
	int nal_unit_type = bs_read_u(b, 6);
	int nuh_layer_id = bs_read_u(b, 6);
	int nuh_temporal_id_plus1 = bs_read_u(b, 3);

	//--- nal_uint_header ---
	if (nal_unit_type != 33)
		return 0;
	int nal_size = nLen;
	int rbsp_size = nLen;
	uint8_t* rbsp_buf = (uint8_t*)malloc(rbsp_size);

	int rc = nal_to_rbsp(2, buf, &nal_size, rbsp_buf, &rbsp_size);

	if (rc < 0) { free(rbsp_buf); return -1; } // handle conversion error
	//b = bs_new(rbsp_buf, rbsp_size);
	bs_init(&bs, rbsp_buf, rbsp_size);
	//--- seq_parameter_set_rbsp ---
	int sps_video_parameter_set_id = bs_read_u(b, 4);
	int sps_max_sub_layers_minus1 = bs_read_u(b, 3);
	int sps_temporal_id_nesting_flag = bs_read_u1(b);
	
	if (sps_temporal_id_nesting_flag) //--- profile_tier_level ---
	{
		int general_profile_space = bs_read_u(b, 2);
		int general_tier_flag = bs_read_u1(b);
		int general_profile_idc = bs_read_u(b, 5);
		int general_profile_compatibility_flag[32];
		
		int j;
		for (j = 0; j < 32; j++)
		{
			general_profile_compatibility_flag[j] = bs_read_u1(b);			
		}
		int general_progressive_source_flag = bs_read_u1(b);
		int general_interlaced_source_flag =  bs_read_u1(b);
		int general_non_packed_constraint_flag =  bs_read_u1(b);
		int general_frame_only_constraint_flag =  bs_read_u1(b);

		if (general_profile_idc == 4 || general_profile_compatibility_flag[4] ||
			general_profile_idc == 5 || general_profile_compatibility_flag[5] ||
			general_profile_idc == 6 || general_profile_compatibility_flag[6] ||
			general_profile_idc == 7 || general_profile_compatibility_flag[7] ||
			general_profile_idc == 8 || general_profile_compatibility_flag[8] ||
			general_profile_idc == 9 || general_profile_compatibility_flag[9] ||
			general_profile_idc == 10 || general_profile_compatibility_flag[10])
		{
			// printf("> hit 1-1\n");
			int general_max_12bit_constraint_flag =  bs_read_u1(b);
			int general_max_10bit_constraint_flag =  bs_read_u1(b);
			int general_max_8bit_constraint_flag =  bs_read_u1(b);
			int general_max_422chroma_constraint_flag =  bs_read_u1(b);
			int general_max_420chroma_constraint_flag =  bs_read_u1(b);
			int general_max_monochrome_constraint_flag =  bs_read_u1(b);
			int general_intra_constraint_flag =  bs_read_u1(b);
			int general_one_picture_only_constraint_flag =  bs_read_u1(b);
			int general_lower_bit_rate_constraint_flag =  bs_read_u1(b);

			if (general_profile_idc == 5 || general_profile_compatibility_flag[5] ||
				general_profile_idc == 9 || general_profile_compatibility_flag[9] ||
				general_profile_idc == 10 || general_profile_compatibility_flag[10])
			{
				int general_max_14bit_constraint_flag =  bs_read_u1(b);
				int general_reserved_zero_33bits = bs_read_u(b, 33); //u(33, buf, &StartBit);
			}
			else
			{
				int general_reserved_zero_34bits = bs_read_u(b, 34); //u(34, buf, &StartBit);
			}
		}
		else if (general_profile_idc == 2 || general_profile_compatibility_flag[2])
		{
			// printf("> hit 1-2\n");
			int general_reserved_zero_7bits = bs_read_u(b, 7); //u(7, buf, &StartBit);
			int general_one_picture_only_constraint_flag =  bs_read_u1(b);
			int general_reserved_zero_35bits = bs_read_u(b, 35); //u(35, buf, &StartBit);
		}
		else
		{
			// printf("> hit 1-3\n");
			int general_reserved_zero_43bits = bs_read_u(b, 43); //u(43, buf, &StartBit);
		}
		if ((general_profile_idc >= 1 && general_profile_idc <= 5) ||
			general_profile_idc == 9 ||
			general_profile_compatibility_flag[1] || general_profile_compatibility_flag[2] ||
			general_profile_compatibility_flag[3] || general_profile_compatibility_flag[4] ||
			general_profile_compatibility_flag[5] || general_profile_compatibility_flag[9])
		{
			// printf("> hit 2-1\n");
			int general_inbld_flag =  bs_read_u1(b);
		}
		else
		{
			// printf("> hit 2-2\n");
			int general_reserved_zero_bit =  bs_read_u1(b);
		}
		int general_level_idc = bs_read_u8(b); //u(8, buf, &StartBit);
		if (sps_max_sub_layers_minus1 > 0)
		{
			fprintf(stderr, "error: sps_max_sub_layers_minus1 must 0 (%d)\n",
				sps_max_sub_layers_minus1);
			return 0;
		}
	}
	int sps_seq_parameter_set_id = bs_read_ue(b);
	int chroma_format_idc = bs_read_ue(b);
	int separate_colour_plane_flag = 0;

	if (chroma_format_idc == 3)
	{
		separate_colour_plane_flag =  bs_read_u1(b);
	}
	int pic_width_in_luma_samples = bs_read_ue(b);
	int pic_height_in_luma_samples = bs_read_ue(b);
	int conformance_window_flag =  bs_read_u1(b);

	int conf_win_left_offset = 0;
	int conf_win_right_offset = 0;
	int conf_win_top_offset = 0;
	int conf_win_bottom_offset = 0;
	if (conformance_window_flag)
	{
		 conf_win_left_offset = bs_read_ue(b);
		 conf_win_right_offset = bs_read_ue(b);
		 conf_win_top_offset = bs_read_ue(b);
		 conf_win_bottom_offset = bs_read_ue(b);

		 int sub_width_c = ((1 == chroma_format_idc) || (2 == chroma_format_idc)) && (0 == separate_colour_plane_flag) ? 2 : 1;
		 int sub_height_c = (1 == chroma_format_idc) && (0 == separate_colour_plane_flag) ? 2 : 1;
		 *width = pic_width_in_luma_samples-(sub_width_c * conf_win_right_offset + sub_width_c * conf_win_left_offset);
		 *height = pic_height_in_luma_samples-(sub_height_c * conf_win_bottom_offset + sub_height_c * conf_win_top_offset);
	}
	else {
		*width = pic_width_in_luma_samples - (conf_win_left_offset + conf_win_right_offset);
		*height = pic_height_in_luma_samples - (conf_win_top_offset + conf_win_bottom_offset);
	}	

	if (bs_overrun(b)) { 
		free(rbsp_buf); 
		return -1;
	}

	free(rbsp_buf);
#endif
	return 0;
}
