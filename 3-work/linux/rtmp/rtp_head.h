/*
 * rtp_head.h
 *
 *  Created on: 2019年10月29日
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

#ifndef RTP_HEAD_H_
#define RTP_HEAD_H_

#include <stdint.h>
#include <netinet/in.h>
/******************************************************************
 RTP_FIXED_HEADER
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           timestamp                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |           synchronization source (SSRC) identifier            |
 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 |            contributing source (CSRC) identifiers             |
 |                             ....                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ******************************************************************/
typedef struct {
	unsigned char csrc_len :4; /* CSRC计数器，占4位，指示CSRC 标识符的个数 */
	unsigned char extension :1; /* 扩展标志，占1位，如果X=1，则在RTP报头后跟有一个扩展报头 */
	unsigned char padding :1; /* 填充标志，占1位，如果P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分。 */
	unsigned char version :2; /* RTP协议的版本号，占2位，当前协议版本号为2 */

	unsigned char payload_type :7; /* 有效荷载类型，占7位 */
	unsigned char marker :1; /*标记，占1位，不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始。 */
	uint16_t seq_no; /*序列号：占16位*/
	uint32_t timestamp;
	uint32_t ssrc; /*同步信源(SSRC)标识符 */
}__attribute__((packed)) RTP_FIXED_HEADER;

//H264 RTP
/******************************************************************
 NALU_HEADER
 +---------------+
 |0|1|2|3|4|5|6|7|
 +-+-+-+-+-+-+-+-+
 |F|NRI|  Type   |
 +---------------+
F: 1个比特.
  forbidden_zero_bit. 在 H.264 规范中规定了这一位必须为 0.
NRI: 2个比特.
  nal_ref_idc. 取00~11,似乎指示这个NALU的重要性,如00的NALU解码器可以丢弃它而不影响图像的回放,0～3，取值越大，表示当前NAL越重要，需要优先受到保护。
  如果当前NAL是属于参考帧的片，或是序列参数集，或是图像参数集这些重要的单位时，本句法元素必需大于0。
Type: 5个比特.
  nal_unit_type. 这个NALU单元的类型,1～12由H.264使用，24～31由H.264以外的应用使用,简述如下:

 ******************************************************************/
typedef struct {
	//byte 0
	unsigned char TYPE :5;
	unsigned char NRI :2;
	unsigned char F :1;
}__attribute__((packed)) NALU_HEADER; /* 1 byte */

/******************************************************************
 FU_INDICATOR
 +---------------+
 |0|1|2|3|4|5|6|7|
 +-+-+-+-+-+-+-+-+
 |F|NRI|  Type   |
 +---------------+
 ******************************************************************/
typedef struct {
	//byte 0
	unsigned char TYPE :5;
	unsigned char NRI :2;
	unsigned char F :1;
}__attribute__((packed)) FU_INDICATOR; /*1 byte */

/******************************************************************
 FU_HEADER
 +---------------+
 |0|1|2|3|4|5|6|7|
 +-+-+-+-+-+-+-+-+
 |S|E|R|  Type   |
 +---------------+
 ******************************************************************/
typedef struct {
	//byte 0
	unsigned char TYPE :5;
	unsigned char R :1;
	unsigned char E :1;
	unsigned char S :1;
}__attribute__((packed)) FU_HEADER; /* 1 byte */

//H264 PS RTP
typedef struct ps_header {
	unsigned char pack_start_code[4];  //'0x000001BA'

	unsigned char system_clock_reference_base21 :2;
	unsigned char marker_bit :1;
	unsigned char system_clock_reference_base1 :3;
	unsigned char fix_bit :2;    //'01'

	unsigned char system_clock_reference_base22;

	unsigned char system_clock_reference_base31 :2;
	unsigned char marker_bit1 :1;
	unsigned char system_clock_reference_base23 :5;

	unsigned char system_clock_reference_base32;
	unsigned char system_clock_reference_extension1 :2;
	unsigned char marker_bit2 :1;
	unsigned char system_clock_reference_base33 :5; //system_clock_reference_base 33bit

	unsigned char marker_bit3 :1;
	unsigned char system_clock_reference_extension2 :7; //system_clock_reference_extension 9bit

	unsigned char program_mux_rate1;

	unsigned char program_mux_rate2;
	unsigned char marker_bit5 :1;
	unsigned char marker_bit4 :1;
	unsigned char program_mux_rate3 :6;

	unsigned char pack_stuffing_length :3;
	unsigned char reserved :5;
}__attribute__((packed)) ps_header_t;  //14

//系统标题
typedef struct sh_header {
	uint32_t system_header_start_code; //32
	uint16_t header_length;            //16 uimsbf

	uint32_t marker_bit1 :1;   //1  bslbf
	uint32_t rate_bound :22;   //22 uimsbf
	uint32_t marker_bit2 :1;   //1 bslbf
	uint32_t audio_bound :6;   //6 uimsbf
	uint32_t fixed_flag :1;    //1 bslbf
	uint32_t CSPS_flag :1;     //1 bslbf

	uint16_t system_audio_lock_flag :1;  // bslbf
	uint16_t system_video_lock_flag :1;  // bslbf
	uint16_t marker_bit3 :1;             // bslbf
	uint16_t video_bound :5;             // uimsbf
	uint16_t packet_rate_restriction_flag :1; //bslbf
	uint16_t reserved_bits :7;                //bslbf
	unsigned char reserved[6];
}__attribute__((packed)) sh_header_t; //18

//PSM（节目流映射
typedef struct psm_header {  //0x000001BC
	uint32_t promgram_stream_map_start_code;
	uint16_t program_stream_map_length; //表示此字段之后PSM的总长度，最大值为1018(0x3FA)；
	///////////////////////////////////////////////////////////////////////////
	unsigned char program_stream_map_version :5;
	unsigned char reserved1 :2;
	unsigned char current_next_indicator :1; //置位1表示当前PSM是可用的，置位0则表示当前PSM不可以，下一个可用；

	unsigned char marker_bit :1;
	unsigned char reserved2 :7;

	uint16_t program_stream_info_length;   //表示此字段后面的descriptor字段的长度；
	uint16_t elementary_stream_map_length; //表示在这个PSM中所有ES流信息的总长度；包括stream_type,
	unsigned char stream_type;
	unsigned char elementary_stream_id;
	uint16_t elementary_stream_info_length;
	uint32_t CRC_32;
	unsigned char reserved[16];
}__attribute__((packed)) psm_header_t; //36

enum PSM_STREAM_TYPE {
	PSM_STREAM_H264 = 0x1B, PSM_STREAM_G711 = 0x90
};

typedef struct pes_header {
	unsigned char pes_start_code_prefix[3];
	unsigned char stream_id;
	unsigned short PES_packet_length;
}__attribute__((packed)) pes_header_t; //6

typedef struct optional_pes_header {
	unsigned char original_or_copy :1;
	unsigned char copyright :1;
	unsigned char data_alignment_indicator :1;
	unsigned char PES_priority :1;
	unsigned char PES_scrambling_control :2;
	unsigned char fix_bit :2;

	unsigned char PES_extension_flag :1;
	unsigned char PES_CRC_flag :1;
	unsigned char additional_copy_info_flag :1;
	unsigned char DSM_trick_mode_flag :1;
	unsigned char ES_rate_flag :1;
	unsigned char ESCR_flag :1;
	unsigned char PTS_DTS_flags :2;

	unsigned char PES_header_data_length;
}__attribute__((packed)) optional_pes_header_t;

static int inline ps_is_header(void *p) {
	if (*(uint32_t*) p == ntohl(0x000001BA))
		return 1;
	return 0;
}

static inline int ps_is_sh_header(void *p) {
	if (*(uint32_t*) p == ntohl(0x000001BB))
		return 1;
	return 0;
}

static inline int ps_is_psm_header(void *p) {
	if (*(uint32_t*) p == ntohl(0x000001BC))
		return 1;
	return 0;
}

//a)   0x000001E0   -  0x000001EF:   Video   PES  start   code();
//b)   0x000001C0   -  0x000001DF:   Audio   PES  start   code(ISO/IEC   13818-3  or   11172-3);
//c)   0x000001BD:   Private  Stream(AC3)
static inline int ps_is_pes_video_header(void *p) {
	if (*(uint32_t*) p == ntohl(0x000001E0))
		return 1;
	return 0;
}

static inline int ps_is_pes_audio_header(void *p) {
	if (*(uint32_t*) p == ntohl(0x000001C0))
		return 1;
	return 0;
}

static inline int ps_is_pes_header(void *p) {
	if (*(uint32_t*) p == ntohl(0x000001E0))
		return 1;
	if (*(uint32_t*) p == ntohl(0x000001C0))
		return 1;
	return 0;
}

#endif /* RTP_HEAD_H_ */
