/*
 * sps_decode.h
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

#ifndef SPS_DECODE_H_
#define SPS_DECODE_H_

#ifdef __cplusplus
extern "C"
{
#endif

int h264_decode_sps(unsigned char * buf, unsigned int nLen, int *width, int *height, int *fps);

#ifdef __cplusplus
}
#endif

#endif /* SPS_DECODE_H_ */
