/*
 * flv_out.h
 *
 *  Created on: 2021年3月2日
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

#ifndef SRC_LIBMEDIA_FLV_OUT_H_
#define SRC_LIBMEDIA_FLV_OUT_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct FlvOut *FLVHandle;

FLVHandle flv_out_build(int (*_write)(struct FlvOut *out, void *data, int size, void *usr));

void flv_out_free(FLVHandle h);

void flv_out_set_usr(FLVHandle flv, void *usr);
void flv_out_start(FLVHandle flv);

void flv_out_clear(FLVHandle flv);

int flv_out_video_h264(FLVHandle flv, uint8_t *data, int size, int ms);

int flv_out_video_h265(FLVHandle flv, uint8_t *data, int size, int ms);

#ifdef __cplusplus
}
#endif

#endif /* SRC_LIBMEDIA_FLV_OUT_H_ */
