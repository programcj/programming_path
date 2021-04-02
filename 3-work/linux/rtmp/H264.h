/*
 * H264.h
 *
 *  Created on: 2020年12月19日
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

#ifndef H264_H_
#define H264_H_

#ifdef __cplusplus
extern "C"
{
#endif

//H264定义的类型 values for nal_unit_type
//H264 NAL Type
typedef enum
{

	NALU_TYPE_SLICE = 1,
	NALU_TYPE_DPA = 2,
	NALU_TYPE_DPB = 3,
	NALU_TYPE_DPC = 4,
	NALU_TYPE_IDR = 5,
	NALU_TYPE_SEI = 6,
	NALU_TYPE_SPS = 7,
	NALU_TYPE_PPS = 8,
	NALU_TYPE_AUD = 9,
	NALU_TYPE_EOSEQ = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL = 12,
#if (MVC_EXTENSION_ENABLE)
NALU_TYPE_PREFIX = 14,
NALU_TYPE_SUB_SPS = 15,
NALU_TYPE_SLC_EXT = 20,
NALU_TYPE_VDRD = 24  // View and Dependency Representation Delimiter NAL Unit
#endif
} AvcNaluType;

//H265 NAL Type
typedef enum
{
NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0
NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1

NAL_UNIT_CODED_SLICE_TSA_N,     // 2
NAL_UNIT_CODED_SLICE_TLA,       // 3   // Current name in the spec: TSA_R

NAL_UNIT_CODED_SLICE_STSA_N,    // 4
NAL_UNIT_CODED_SLICE_STSA_R,    // 5

NAL_UNIT_CODED_SLICE_RADL_N,    // 6
NAL_UNIT_CODED_SLICE_DLP,       // 7 // Current name in the spec: RADL_R

NAL_UNIT_CODED_SLICE_RASL_N,    // 8
NAL_UNIT_CODED_SLICE_TFD,       // 9 // Current name in the spec: RASL_R

NAL_UNIT_RESERVED_10,
NAL_UNIT_RESERVED_11,
NAL_UNIT_RESERVED_12,
NAL_UNIT_RESERVED_13,
NAL_UNIT_RESERVED_14,
NAL_UNIT_RESERVED_15,
NAL_UNIT_CODED_SLICE_BLA,       // 16   // Current name in the spec: BLA_W_LP
NAL_UNIT_CODED_SLICE_BLANT,     // 17   // Current name in the spec: BLA_W_DLP
NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18
NAL_UNIT_CODED_SLICE_IDR,       // 19  // Current name in the spec: IDR_W_DLP
NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20
NAL_UNIT_CODED_SLICE_CRA,       // 21
NAL_UNIT_RESERVED_22,
NAL_UNIT_RESERVED_23,

NAL_UNIT_RESERVED_24,
NAL_UNIT_RESERVED_25,
NAL_UNIT_RESERVED_26,
NAL_UNIT_RESERVED_27,
NAL_UNIT_RESERVED_28,
NAL_UNIT_RESERVED_29,
NAL_UNIT_RESERVED_30,
NAL_UNIT_RESERVED_31,

NAL_UNIT_VPS,                   // 32
NAL_UNIT_SPS,                   // 33
NAL_UNIT_PPS,                   // 34
NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
NAL_UNIT_EOS,                   // 36
NAL_UNIT_EOB,                   // 37
NAL_UNIT_FILLER_DATA,           // 38
NAL_UNIT_SEI,                   // 39 Prefix SEI
NAL_UNIT_SEI_SUFFIX,            // 40 Suffix SEI
NAL_UNIT_RESERVED_41,
NAL_UNIT_RESERVED_42,
NAL_UNIT_RESERVED_43,
NAL_UNIT_RESERVED_44,
NAL_UNIT_RESERVED_45,
NAL_UNIT_RESERVED_46,
NAL_UNIT_RESERVED_47,
NAL_UNIT_UNSPECIFIED_48,
NAL_UNIT_UNSPECIFIED_49,
NAL_UNIT_UNSPECIFIED_50,
NAL_UNIT_UNSPECIFIED_51,
NAL_UNIT_UNSPECIFIED_52,
NAL_UNIT_UNSPECIFIED_53,
NAL_UNIT_UNSPECIFIED_54,
NAL_UNIT_UNSPECIFIED_55,
NAL_UNIT_UNSPECIFIED_56,
NAL_UNIT_UNSPECIFIED_57,
NAL_UNIT_UNSPECIFIED_58,
NAL_UNIT_UNSPECIFIED_59,
NAL_UNIT_UNSPECIFIED_60,
NAL_UNIT_UNSPECIFIED_61,
NAL_UNIT_UNSPECIFIED_62,
NAL_UNIT_UNSPECIFIED_63,
NAL_UNIT_INVALID,
} HevcNaluType;

const char *HevcNaluType_ToString(int type);

typedef struct t_h264_nalu_header
{
//小端模式哦(反的)
#if __BYTE_ORDER == __LITTLE_ENDIAN
unsigned char nal_unit_type :5;    //type&0x1F
unsigned char nal_reference_idc :2;
unsigned char forbidden_bit :1;
#endif
} H264_NALU_HEADER;

typedef struct t_h265_nalu_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
unsigned char F :1;
unsigned char nal_unit_type :6; //type&0x7E >>1
unsigned char LayerId :6;
unsigned char TID :3;
#endif
} H265_NALU_HEADER;

#define H264NaluType(v) (v&0x1F)
#define H265NaluType(v) ((v&0x7E)>>1)

#ifdef __cplusplus
}
#endif

#endif /* H264_H_ */
