/*
 * udp_ps_recv.c
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#include <poll.h>
#include <time.h>
#include "socket_opt.h"
#include "rtp_head.h"

void log_print(const char *fun, int line, const char *format, ...)
{
	va_list v;
	va_start(v, format);
	printf("[%s:%4d]", fun, line);
	vprintf(format, v);
	va_end(v);
	fflush(stdout);
}

#define log_d(format,...) log_print(__FUNCTION__, __LINE__,format, ##__VA_ARGS__)

typedef struct
{
	uint8_t *data;
	int size;
	uint8_t *pos; //当前位置
	uint8_t *pend;
} DataBuf;

void databuf_init(DataBuf *buf, void *data, int size)
{
	buf->data = data;
	buf->size = size;
	buf->pos = buf->data;
	buf->pend = buf->data + size;
}

int databuf_read(DataBuf *buf, void *data, int size)
{
	memcpy(data, buf->pos, size);
	buf->pos += size;
	return 0;
}

uint8_t databuf_read_char(DataBuf *buf)
{
	uint8_t v = buf->pos[0];
	buf->pos += 1;
	return v;
}

void print_hex(void *data, int len)
{
	printf("(%d)", len);
	for (int i = 0; i < len; i++)
	{
		printf("%02X ", ((uint8_t*) data)[i]);
	}
	printf("\n");
}

#define uint8buf_rchar(p)  { char v=*p;p+sizeof(char); v; }

volatile int video_flag = 0;
volatile int video_h264_iframe = -1;

static FILE *fp_264 = NULL;

enum
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
	NALU_TYPE_FILL = 12
};

void h264_cache_write(void *data, int size)
{
	if (fp_264 == NULL)
	{
		fp_264 = fopen("aa.h264", "wb");
		//int outfd = open("video", O_WRONLY);
		//fp_264 = fdopen(outfd, "wb");
		log_d("h264 cache write start\n");
	}
	if (fp_264)
	{
		if (video_h264_iframe == 1)
		{
			uint8_t *p = (uint8_t*) data;
			NALU_HEADER *nal_h = NULL;
			if (p[2] == 0x01)
			{
				nal_h = (NALU_HEADER *) (p + 3);
			}
			else if (p[3] == 0x01)
			{
				nal_h = (NALU_HEADER *) (p + 4);
			}
			printf("H264:");
			print_hex(data, 5);

			if (nal_h)
			{
				log_d("NAL type:%d  ", nal_h->TYPE);
				switch (nal_h->TYPE)
				{
					case NALU_TYPE_SPS:
						printf("NALU_TYPE_SPS\n");
					break;
					case NALU_TYPE_PPS:
						printf("NALU_TYPE_PPS\n");
					break;
					case NALU_TYPE_IDR:
						printf("NALU_TYPE_IDR\n");
					break;
					case NALU_TYPE_SEI:
						printf("NALU_TYPE_SEI\n");
					break;
					case NALU_TYPE_SLICE:
						printf("NALU_TYPE_SLICE\n");
					break;
					default:
						printf("UNKNOW\n");
					break;
				}

				//nal_h->TYPE=5;
			}
			video_h264_iframe = -1;

		}
		else if (video_h264_iframe == 0)
		{
			uint8_t *p = (uint8_t*) data;
			NALU_HEADER *nal_h = NULL;
			if (p[2] == 0x01)
			{
				nal_h = (NALU_HEADER *) (p + 3);
			}
			else if (p[3] == 0x01)
			{
				nal_h = (NALU_HEADER *) (p + 4);
			}
			printf("-----------------p--------\n");
			print_hex(data, 5);
			if (nal_h)
				log_d("NAL type:%d\n", nal_h->TYPE);
			video_h264_iframe = -1;
		}
		fwrite(data, 1, size, fp_264);
	}
}

int hex_exists(void *data, int dlen, const void *fdata, int flen)
{
	uint8_t *p = data;
	uint8_t *pend = p + dlen;

	int ret = 0;
	for (; pend - p > flen; p++)
	{
		ret = memcmp(p, fdata, flen);
		if (ret == 0)
			return 1;
	}
	return 0;
}

struct encode_stream
{

};

static int rawdata_length = 0;

uint8_t* pes_parse(uint8_t *pesptr, uint8_t *pend)
{
	uint8_t *pnext = pesptr;
	pes_header_t pes_head; // = (pes_header_t *) pnext;
	memcpy(&pes_head, pesptr, sizeof(pes_header_t));

	pes_head.PES_packet_length = ntohs(pes_head.PES_packet_length); //剩下的大小
	log_d("PES-video头, streamID:%02X, lenght:%d\n", pes_head.stream_id,
			pes_head.PES_packet_length);
	pnext += sizeof(pes_header_t);

	optional_pes_header_t *opes_head = (optional_pes_header_t *) pnext;
	log_d("opes len:%d\n", opes_head->PES_header_data_length);

	print_hex(pesptr,
			sizeof(pes_header_t) + sizeof(optional_pes_header_t)
					+ opes_head->PES_header_data_length);
	pnext += sizeof(optional_pes_header_t) + opes_head->PES_header_data_length;

	rawdata_length = pes_head.PES_packet_length - sizeof(optional_pes_header_t)
			- opes_head->PES_header_data_length;

	printf("Video Data(%d): (%ld)\n", rawdata_length, pend - pnext);
	log_d(">>>(%ld)\n", pend - pnext);

	video_flag = 1;

	int wlen = 0;
	int psize = pend - pnext;
	if (psize >= rawdata_length)
	{
		h264_cache_write(pnext, rawdata_length);
		pnext += rawdata_length;
		rawdata_length = 0;
	}
	else
	{
		h264_cache_write(pnext, psize);
		rawdata_length -= psize;
		pnext += psize; //后续需要的数据长度
	}
	return pnext;
}

void ps_parse(uint8_t *ps_data, uint8_t *pend)
{
	int size = pend - ps_data;
	ps_header_t *ps_head = (ps_header_t*) ps_data; //
	uint8_t *pnext = ps_data;
	uint32_t *head = NULL;

	log_d("PS包 HEAD:%d (%d) lenght:%d\n", sizeof(ps_header_t),
			size - sizeof(ps_header_t), ps_head->pack_stuffing_length);
	print_hex(pnext, sizeof(ps_header_t) + ps_head->pack_stuffing_length);

	pnext += sizeof(ps_header_t) + ps_head->pack_stuffing_length;
	head = (uint32_t*) pnext;

	if (ps_is_sh_header(pnext)) //0x000001BB: system_header
	{
		uint8_t *shptr = pnext;
		sh_header_t shpack;
		memcpy(&shpack, shptr, sizeof(sh_header_t));
		shpack.header_length = ntohs(shpack.header_length);

		log_d("SH头(BB) , lenght:%d\n", shpack.header_length);
		pnext += 4 + 2 + shpack.header_length;

		print_hex(shptr, sizeof(sh_header_t));

		if (ps_is_psm_header(pnext)) //0x000001BC: PSM, Program Stream Map , PSM只有在关键帧打包的时候，才会存在
		{
			uint8_t *psmptr = pnext;
			psm_header_t psmpack;
			memcpy(&psmpack, psmptr, sizeof(psm_header_t));

			psmpack.elementary_stream_info_length = ntohs(
					psmpack.elementary_stream_info_length);
			psmpack.elementary_stream_map_length = ntohs(
					psmpack.elementary_stream_map_length);
			psmpack.program_stream_info_length = ntohs(
					psmpack.program_stream_info_length);
			psmpack.program_stream_map_length = ntohs(
					psmpack.program_stream_map_length);

			log_d(
					"PSM头(BC), length:%d , Info len:%d, ES Len:%d, ES Info len:%d, SType:%02X, ESID:%02X\n",
					psmpack.program_stream_map_length,
					psmpack.program_stream_info_length,
					psmpack.elementary_stream_map_length,
					psmpack.elementary_stream_info_length, //需要往下增加长度
					psmpack.stream_type,//0x1B H264
					psmpack.elementary_stream_id//0x(C0~DF)指音频，0x(E0~EF)为视频；
					);

			print_hex(psmptr, 4 + 2 + psmpack.program_stream_map_length);

			pnext += 4 + 2 + psmpack.program_stream_map_length;

			while (pnext < pend)
			{
				if (ps_is_pes_header(pnext))
				{
					pnext = pes_parse(pnext, pend);
					if (pnext < pend)
					{
						log_d("PES Next...\n");
						print_hex(pnext, 4);
					}
				}
				else
				{
					log_d("PSM decode err[not find PSM(BC)]\n");
					print_hex(pnext, 4);
					break;
				}
			}
			return;
		}
		else
		{
			log_d("PSM decode err\n");
			print_hex(shptr, sizeof(sh_header_t) + shpack.header_length);
		}
	}

	//00 00 01 E0 14 9D 88 80 05 21 00 03 F0 E1
	if (ps_is_pes_video_header(pnext))
	{
		pnext = pes_parse(pnext, pend);
		return;
	}
	video_h264_iframe = -1;
	h264_cache_write(pnext, pend - pnext);
}

int ps_parse_loop(uint8_t *pdata, uint8_t *pend)
{
	if (pdata >= pend)
		return -1;

	if (ps_is_header(pdata)) //PS流总是以0x000001BA开始，以0x000001B9结束
	{
		ps_header_t *ps_head = (ps_header_t*) pdata; //
		log_d("PS包 lenght:%d\n", ps_head->pack_stuffing_length);
		pdata += sizeof(ps_header_t) + ps_head->pack_stuffing_length;

		return ps_parse_loop(pdata, pend);
	}
	if (ps_is_sh_header(pdata)) //0x000001BB: system_header
	{
		uint8_t *shptr = pdata;
		sh_header_t shpack;
		memcpy(&shpack, shptr, sizeof(sh_header_t));
		shpack.header_length = ntohs(shpack.header_length);

		log_d("SH头(BB) , lenght:%d\n", shpack.header_length);
		pdata += 4 + 2 + shpack.header_length;
		print_hex(shptr, sizeof(sh_header_t));

		return ps_parse_loop(pdata, pend);
	}

	if (ps_is_psm_header(pdata)) //0x000001BC: PSM, Program Stream Map , PSM只有在关键帧打包的时候，才会存在
	{
		uint8_t *psmptr = pdata;
		psm_header_t psmpack;
		memcpy(&psmpack, psmptr, sizeof(psm_header_t));

		psmpack.elementary_stream_info_length = ntohs(
				psmpack.elementary_stream_info_length);
		psmpack.elementary_stream_map_length = ntohs(
				psmpack.elementary_stream_map_length);
		psmpack.program_stream_info_length = ntohs(
				psmpack.program_stream_info_length);
		psmpack.program_stream_map_length = ntohs(
				psmpack.program_stream_map_length);

		log_d(
				"PSM头(BC), length:%d , Info len:%d, ES Len:%d, ES Info len:%d, SType:%02X, ESID:%02X\n",
				psmpack.program_stream_map_length,
				psmpack.program_stream_info_length,
				psmpack.elementary_stream_map_length,
				psmpack.elementary_stream_info_length, //需要往下增加长度
				psmpack.stream_type,//0x1B H264
				psmpack.elementary_stream_id//0x(C0~DF)指音频，0x(E0~EF)为视频；
				);
		print_hex(psmptr, 4 + 2 + psmpack.program_stream_map_length);
		pdata += 4 + 2 + psmpack.program_stream_map_length;
		return ps_parse_loop(pdata, pend);
	}

	if (ps_is_pes_header(pdata))
	{
		uint8_t *pesptr = pdata;

		pes_header_t pes_head; // = (pes_header_t *) pnext;
		memcpy(&pes_head, pesptr, sizeof(pes_header_t));

		pes_head.PES_packet_length = ntohs(pes_head.PES_packet_length); //剩下的大小
		log_d("PES-video头, streamID:%02X, lenght:%d\n", pes_head.stream_id,
				pes_head.PES_packet_length);
		pdata += sizeof(pes_header_t);

		optional_pes_header_t *opes_head = (optional_pes_header_t *) pdata;
		log_d("opes len:%d\n", opes_head->PES_header_data_length);

		print_hex(pesptr,
				sizeof(pes_header_t) + sizeof(optional_pes_header_t)
						+ opes_head->PES_header_data_length);
		pdata += sizeof(optional_pes_header_t)
				+ opes_head->PES_header_data_length;

		rawdata_length = pes_head.PES_packet_length
				- sizeof(optional_pes_header_t)
				- opes_head->PES_header_data_length;

		printf("Video Data(%d): (%ld)\n", rawdata_length, pend - pdata);

		video_flag = 1;

		int wlen = 0;
		int psize = pend - pdata;
		if (psize >= rawdata_length)
		{
			h264_cache_write(pdata, rawdata_length);
			pdata += rawdata_length;
			rawdata_length = 0;
		}
		else
		{
			h264_cache_write(pdata, psize);
			rawdata_length -= psize;
			pdata += psize; //后续需要的数据长度
		}
		return ps_parse_loop(pdata, pend);
	}

	if (video_flag && rawdata_length > 0)
	{
		int len = pend - pdata;

		if (len > rawdata_length)
		{
			log_d(">>>>>(%d)\n", rawdata_length);
			h264_cache_write(pdata, rawdata_length);
			pdata += rawdata_length;
			rawdata_length = 0;
			video_flag = 0;
			return ps_parse_loop(pdata, pend);
		}
		else
		{
			log_d(">>>>>(%d)\n", len);
			h264_cache_write(pdata, len);
			pdata += len;
			rawdata_length -= len;

			return 0;
		}
	}
	log_d("PS Parse err:[unknow]\n");

#if 0
	if (ps_is_header(pdata)) //PS流总是以0x000001BA开始，以0x000001B9结束
	{
		ps_parse(pdata, pend);
		return 0;
	}
	if (ps_is_pes_video_header(pdata))
	{
		pdata = pes_parse(pdata, pend);
		return 0;
	}

	int len = pend - pdata;

	if (video_flag && rawdata_length > 0)
	{
		if (len > rawdata_length)
		{
			log_d(">>>>>(%d)\n", rawdata_length);
			h264_cache_write(pdata, rawdata_length);
			pdata += rawdata_length;
			rawdata_length = 0;
			video_flag = 0;

			print_hex(pdata, 4);
			if (ps_is_pes_video_header(pdata))
			{
				pdata = pes_parse(pdata, pend);
				return 0;
			}
		}
		else
		{
			log_d(">>>>>(%d)\n", len);
			h264_cache_write(pdata, len);
			pdata += len;
			rawdata_length -= len;
		}
	}
#endif
	return 0;
}

int rtp_parse(uint8_t *data, int size)
{
	RTP_FIXED_HEADER *rtp_head = (RTP_FIXED_HEADER*) data;

	uint8_t *ps_data = data + sizeof(RTP_FIXED_HEADER);
	uint8_t *pnext = ps_data;
	uint8_t *pend = data + size;
	uint8_t *point = NULL;
	printf("===========RTP(%d) type:%d===========: mark=%d, exten:%d\n", size,
			rtp_head->payload_type, rtp_head->marker, rtp_head->extension);

	if (rtp_head->extension)
	{
		unsigned short rtp_ex_profile = 0;
		int rtp_ex_length = 0;
		uint32_t header;

		memcpy(&rtp_ex_profile, ps_data, 2);
		ps_data += 2;
		memcpy(&rtp_ex_length, ps_data, 2);
		ps_data += 2;
		rtp_ex_profile = ntohs(rtp_ex_profile);
		rtp_ex_length = ntohs(rtp_ex_length);

		printf("扩展: %04X len:%d ", rtp_ex_profile, rtp_ex_length);
		int ll = 0;
		for (ll = 0; ll < rtp_ex_length; ll++)
		{
			memcpy(&header, ps_data, 4);
			header = ntohl(header);
			printf("header:%08X ", header);
			ps_data += 4;
		}
		pnext = ps_data;
	}
	//print_hex(pnext, pend - pnext);

	switch (rtp_head->payload_type)
	{
		case 96:
		{
			return ps_parse_loop(pnext, pend);
		}
		break;
	}

#if 0
//00 00 01 BA :PS包起始码字段
	if (ps_is_header(ps_data))//PS流总是以0x000001BA开始，以0x000001B9结束
	{
		ps_parse(ps_data, pend);

		ps_header_t *ps_head = (ps_header_t*) ps_data; //
		pnext = ps_data + sizeof(ps_header_t);

		log_d("ps包 HEAD:%d (%d) stulen:%d\n", sizeof(ps_header_t),
				size - sizeof(ps_header_t), ps_head->pack_stuffing_length);
		print_hex(ps_head, sizeof(ps_header_t) + ps_head->pack_stuffing_length);

		pnext += ps_head->pack_stuffing_length;

		print_hex(pnext, 4);

		if (ps_is_sh_header(pnext))
		{
			sh_header_t *sh_head = (sh_header_t*) pnext;
			pnext += ntohs(sh_head->header_length) + 4 + 2;

			log_d("\t系统标题: len:%d\n", ntohs(sh_head->header_length));
			print_hex(sh_head, sizeof(sh_header_t));

			//PES包=PES header+code raw data;
			psm_header_t *psm_head = (psm_header_t*) pnext;
			if (ps_is_psm_header(psm_head))
			{
				pnext += ntohs(psm_head->program_stream_map_length) + 4 + 2;

				log_d("\t节目映射流: len=%d\n",
						ntohs(psm_head->program_stream_map_length));
				print_hex(psm_head, sizeof(*psm_head));
				log_d("\t节目类型:%02X id:%02X\n", psm_head->stream_type,
						psm_head->elementary_stream_id);

				if (PSM_STREAM_H264 == psm_head->stream_type)
				{
					log_d("\t\t流类型H264\n");
				}

				point = (uint8_t*) &psm_head->elementary_stream_info_length;
				point += ntohs(*(uint16_t*) point) + 2;
				log_d("\t音频:%02X id:%02X\n", *point, point[1]);

				pes_header_t *pes_head = NULL;
				print_hex(pnext, pend - pnext);  //

				while (pnext < pend)
				{
					pes_head = (pes_header_t*) pnext;
					if (ps_is_pes_header(pes_head))
					{
						PES_packet_length = ntohs(pes_head->PES_packet_length);
						log_d("\tPES流ID:%02X\n", pes_head->stream_id);
						log_d("\tPES len:%d\n", PES_packet_length); //整个长度,接下来的数据不能超过它
						//print_hex(pnext, PES_packet_length + 4 + 2);  //
						pnext += sizeof(pes_header_t);//video_header

						optional_pes_header_t *opes_head =
						(optional_pes_header_t*) pnext;
						log_d("\t OPES_header_data_length:%d\n",
								opes_head->PES_header_data_length);

						pnext = pnext + sizeof(optional_pes_header_t)
						+ opes_head->PES_header_data_length;
						int packlen = PES_packet_length
						- opes_head->PES_header_data_length - 3;
						log_d(" video len:%d\n", packlen);

						if (packlen >= pend - pnext)
						packlen = pend - pnext;

						print_hex(pnext, packlen);//
						video_flag = 1;
						video_length = 0;
						video_h264_iframe = 1;

						if (pnext < pend)
						h264_cache_write(pnext, packlen);
						pnext += packlen;
					}
					else
					{
						printf("Unknow\n");
						break;
					}
				}
			}
		}
		else
		{
			//log_d("非系统标题 \n"); //0x(C0~DF)指音频，0x(E0~EF)为视频
			pes_header_t *pes_head = (pes_header_t *) pnext;
			if (ps_is_pes_video_header(pes_head))
			{
				int vlen = ntohs(pes_head->PES_packet_length);
				log_d("0x000001E0 stream...:%d\n", vlen);

				pnext += sizeof(pes_header_t);
				struct video_header
				{
					uint8_t _8[2];
					uint8_t pendlen;
					uint8_t pts[5];
				};
				struct video_header *vhead = (struct video_header *) pnext;
				pnext += 2 + 1 + vhead->pendlen;

				video_h264_iframe = 1;
				//video_h264_iframe = 0;
				h264_cache_write(pnext, pend - pnext);
			}
			else
			{
				video_flag = 0;
			}
		}
	}
	else
	{
		if (ps_is_pes_video_header(pnext))
		{
			pes_header_t *pes_head = NULL;
			pes_head = (pes_header_t *) pnext;

			while (pnext < pend)
			{
				pes_head = (pes_header_t*) pnext;
				if (ps_is_pes_header(pes_head))
				{
					PES_packet_length = ntohs(pes_head->PES_packet_length);
					log_d("\tPES流ID:%02X\n", pes_head->stream_id);
					log_d("\tPES len:%d\n", PES_packet_length); //整个长度,接下来的数据不能超过它
					//print_hex(pnext, PES_packet_length + 4 + 2);  //
					pnext += sizeof(pes_header_t);//video_header

					optional_pes_header_t *opes_head =
					(optional_pes_header_t*) pnext;
					log_d("\t OPES_header_data_length:%d\n",
							opes_head->PES_header_data_length);

					pnext = pnext + sizeof(optional_pes_header_t)
					+ opes_head->PES_header_data_length;
					int packlen = PES_packet_length
					- opes_head->PES_header_data_length - 3;
					log_d(" video len:%d\n", packlen);

					if (packlen >= pend - pnext)
					packlen = pend - pnext;

					print_hex(pnext, packlen);//
					video_flag = 1;
					video_length = 0;
					video_h264_iframe = 1;

					if (pnext < pend)
					h264_cache_write(pnext, packlen);
					pnext += packlen;
				}
				else
				{
					printf("Unknow\n");
					break;
				}
			}
			return 0;
		}

		log_d(" write seq=%d marker=%d\n", htons(rtp_head->seq_no),
				rtp_head->marker);
		h264_cache_write(ps_data, pend - ps_data);
	}
#endif
	return 0;
}

void RTPSendPs(void *ps, int len, int marker)
{
	static int fd = 0;
	if (fd == 0)
	{
		int ret;
		fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		socket_set_reuseaddr(fd, 1);
		socket_set_reuseport(fd, 1);
		socket_nonblock(fd, 1);

		struct sockaddr_in saddr;
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr("192.168.0.14");
		saddr.sin_port = htons(9704);
		ret = bind(fd, (__CONST_SOCKADDR_ARG) &saddr, sizeof(saddr));
	}
	if (fd == 0)
		return;

	struct sockaddr_in addr;
	uint8_t *buffer;
	int buffsize = 0;

	buffer = (uint8_t*) malloc(len + sizeof(RTP_FIXED_HEADER));

	static int seq = 1;
	RTP_FIXED_HEADER *rtp_head = (RTP_FIXED_HEADER *) (buffer);
	memset(rtp_head, 0, sizeof(RTP_FIXED_HEADER));
	rtp_head->version = 2;
	rtp_head->padding = 0;
	rtp_head->timestamp = 12;
	rtp_head->seq_no = seq++;
	rtp_head->payload_type = 97;
	rtp_head->marker = marker;
	rtp_head->ssrc = 1024;
	memcpy(buffer + sizeof(RTP_FIXED_HEADER), ps, len);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("192.168.0.29");
	addr.sin_port = htons(6000);

	sendto(fd, buffer, buffsize, 0, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
}

int main(int argc, char **argv)
{
	log_d("rtp head len:%d\n", sizeof(RTP_FIXED_HEADER)); //20
	log_d("ps head len:%d\n", sizeof(ps_header_t)); //14
	log_d("NALU_HEADER len:%d\n", sizeof(NALU_HEADER));
	log_d("sizeof(int)=%d\n", sizeof(int));

#if 1

	if (argc > 1)
	{
		const char *fileName = argv[1];
		if (access(fileName, F_OK) == 0)
		{
			FILE *fp = fopen(fileName, "rb");
			if (!fp)
				return -1;
			int len;
			uint8_t rtp_data[4096];

			while (!feof(fp))
			{
				fread(&len, sizeof(len), 1, fp);
				fread(rtp_data, 1, len, fp);
				//printf("len=%d \n", len);
				rtp_parse(rtp_data, len);
			}
			fclose(fp);

			if (fp_264)
			{
				fclose(fp_264);
			}
			return 0;
		}
		else
		{
			printf("file not exists:%s\n", fileName);
		}
		return 0;
	}
#endif

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
	{
		log_d("err fd:%d\n", fd);
		return -1;
	}
	int ret;
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	saddr.sin_port = htons(6002);

	printf("recv port:%d\n", 6002);

	socket_set_reuseaddr(fd, 1);
	socket_set_reuseport(fd, 1);
	socket_nonblock(fd, 1);

	log_d("start fd=%d\n", fd);
	ret = bind(fd, (__CONST_SOCKADDR_ARG) &saddr, sizeof(saddr));
	if (ret < 0)
	{
		log_d("bind err ret=%d\n", ret);
	}
	struct pollfd pofd[1];

	uint8_t buffer[65535];
	int i;

	struct sockaddr_in caddr;
	socklen_t caddrlen;
	FILE *fp = fopen("ps.264", "wb");

	time_t cur = time(NULL);
	while (1)
	{
		pofd[0].fd = fd;
		pofd[0].events = POLLIN;
//		if (time(NULL) - cur > 20)
//		{
//			break;
//		}
		ret = poll(pofd, 1, 300);
		if (ret == 0)
			continue;
		log_d("poll ret=%d\n", ret);
		caddrlen = sizeof(caddr);
		ret = recvfrom(fd, &buffer, sizeof(buffer), 0,
				(struct sockaddr *) &caddr, &caddrlen);
		fwrite(&ret, sizeof(ret), 1, fp);
		fwrite(buffer, 1, ret, fp); //写入ps流

		printf("ret=%d, ", ret);
		for (i = 0; i < ret; i++)
		{
			printf("%02X ", buffer[i]);
		}
		printf("\n");

		rtp_parse(buffer, ret); //解析ps流到 aa.h264

	}
	fclose(fp);

	if (fp_264)
		fclose(fp_264);
	close(fd);
}
