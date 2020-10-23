#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zbar.h"
#include "Tool_ffmpeg.h"

#define _max_(a, b, c)  (a>b)? (a>c?a:c):(b>c?b:c)
#define _min_(a, b, c)  (a<b)? (a<c?a:c):(b<c?b:c)

void rgb2hsv(int R, int G, int B, int *H, int *S, int *V)
{
	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	int _max = _max_(R, G, B);
	int _min = _min_(R, G, B);

	double rd = (double) R / 255;
	double gd = (double) G / 255;
	double bd = (double) B / 255;

	double max = (double) _max / 255;
	double min = (double) _min / 255;
	double h, s, v = max;
	double d = max - min;

	s = _max == 0 ? 0 : d / max;

	if (_max == _min)
	{
		h = 0; // achromatic
	}
	else
	{
		if (_max == R)
		{
			//h = (gd - bd) / d + (gd < bd ? 6 : 0);
			h = (gd - bd) / d;
		}
		else if (_max == G)
		{
			h = (bd - rd) / d + 2;
		}
		else if (_max == B)
		{
			h = (rd - gd) / d + 4;
		}
		//h /= 6;
		h *= 60;
		if (h < 0)
			h = h + 360;
	}
//H需要转换成 0-360
	*H = h;
	*S = s * 100;
	*V = v * 100;

	if ((int) (h * 10) % 10 >= 5)
		(*H)++;
	if ((int) (s * 1000) % 10 >= 5)
		(*S)++;
	if ((int) (v * 1000) % 10 >= 5)
		(*V)++;
}

void rgb2yuv(int R, int G, int B, uint8_t *Y, uint8_t *U, uint8_t *V)
{
	*Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
	*U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
	*V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

//	*Y = 0.299 * R + 0.587 * G + 0.114 * B;
//	*U = -0.1687 * R - 0.3313 * G + 0.5 * B + 128;
//	*V = 0.5 * R - 0.4187 * G - 0.0813 * B + 128;
}

void yuv2rgb(uint8_t Y, uint8_t U, uint8_t V, int *R, int *G, int *B)
{
//	B = 1.164383 * (Y - 16) + 2.017232 * (U - 128);
//	G = 1.164383 * (Y - 16) - 0.391762 * (U - 128) - 0.812968 * (V - 128);
//	R = 1.164383 * (Y - 16) + 1.596027 * (V - 128);
	int r, g, b;
	r = (int) ((Y & 0xff) + 1.4075 * ((V & 0xff) - 128));
	g = (int) ((Y & 0xff) - 0.3455 * ((U & 0xff) - 128) - 0.7169 * ((V & 0xff) - 128));
	b = (int) ((Y & 0xff) + 1.779 * ((U & 0xff) - 128));
	r = (r < 0 ? 0 : r > 255 ? 255 : r);
	g = (g < 0 ? 0 : g > 255 ? 255 : g);
	b = (b < 0 ? 0 : b > 255 ? 255 : b);
	*R = r;
	*G = g;
	*B = b;
}

int image_color_max(uint8_t *imagePtr, int format, int width, int height, int x, int y, int w, int h)
{
	uint8_t *imgY, *imgU, *imgV, *imgUV;
	imgY = imagePtr;
	imgU = imgY + (width * height);
	imgV = imgU + (width * height / 4);

	int xline = 0;
	int yline = 0;

	if (AV_PIX_FMT_YUV420P == format)
	{
		for (xline = x; xline < x + h; xline++)
		{
			for (yline = y; yline < y + w; yline++)
			{
				uint8_t y = imgY[xline * width + yline];
				uint8_t u = imgU[xline / 2 * width / 2 + yline / 2];
				uint8_t v = imgV[xline / 2 * width / 2 + yline / 2];
				uint8_t *pY = &imgY[xline * width + yline];
				uint8_t *pU = &imgU[xline / 2 * width / 2 + yline / 2];
				uint8_t *pV = &imgV[xline / 2 * width / 2 + yline / 2];

				int R, G, B;
				int H, S, V;

				yuv2rgb(y, u, v, &R, &G, &B);
				rgb2hsv(R, G, B, &H, &S, &V);

				//(%d,%d  %d,%d) yline * width, hline, x / 2 * width / 2, hline / 2
				//printf("%d %d %d ", R, G, B);
				printf("(%d %d %d)", H, S, V);
				//亮度<30为黑色, 不统计
				if (V <= 35)
				{
					printf("黑");
					//rgb2yuv(255, 255, 255, pY, pU, pV);
					continue;
				}
				if (S <= 10)
				{
					printf("白");
					//rgb2yuv(255, 255, 255, pY, pU, pV);
					continue;
				}
				if (H >= 25 && H <= 130)
				{
					printf("黄");
					rgb2yuv(255, 255, 0, pY, pU, pV);
					continue;
				}
				printf("NUL ");
			}
			printf("\n");
		}
	}
	return 0;
}

int ffmpeg_jpgfileread(const char *file, AVFrame **frame)
{
	int ret;
	AVFormatContext *pFormatCtx = NULL;
	avformat_open_input(&pFormatCtx, file, NULL, NULL);
	avformat_find_stream_info(pFormatCtx, NULL);
	ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	AVCodec *pCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);

	pCodec = avcodec_find_decoder(pFormatCtx->streams[ret]->codecpar->codec_id);
	AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[ret]->codecpar);
	ret = avcodec_open2(pCodecCtx, pCodec, NULL);

	AVPacket packet;
	AVFrame *avframe = av_frame_alloc();
	int flag = 0;

	av_init_packet(&packet);
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		printf("read ok\n");
		avcodec_flush_buffers(pCodecCtx);
		if (0 == avcodec_send_packet(pCodecCtx, &packet))
		{
			ret = avcodec_receive_frame(pCodecCtx, avframe); //内部会av_frame_unref;
			if (ret == 0)
			{
				*frame = avframe;
				flag = 1;
			}
		}
		av_packet_unref(&packet);
	}
	if (flag == 0)
	{
		av_frame_free(&avframe);
	}
	avformat_close_input(&pFormatCtx);
	avcodec_close(pCodecCtx);  //close,如果为rk3399的硬件编解码,则需要等待MPP_Buff释放完成后再关闭?是否需要这样不知道
	avcodec_free_context(&pCodecCtx);
	return ret;
}

void ffmpeg_yuvbuff_draw_box(int width, int height, int format,
			uint8_t *image, int x, int y, int w, int h, int R, int G,
			int B)
{
	/**
	 * NV12
	 *
	 //Y Y Y Y
	 //Y Y Y Y
	 //Y Y Y Y
	 //Y Y Y Y
	 //U V U V
	 //U V U V
	 */
	uint8_t *dstY = image;
	uint8_t *dstUV = image + (width * height);
	uint8_t *dstU = dstUV;
	uint8_t *dstV = dstU + (width * height) / 4;

	x = x % 2 ? x + 1 : x;
	y = y % 2 ? y + 1 : y;
	w = w % 2 ? w - 1 : w;
	h = h % 2 ? h - 1 : h;

	int maxx = x + w;
	int maxy = y + h;
	int px; //横线
	int py; //竖线

	uint8_t _YColor = R;
	uint8_t _UColor = B;
	uint8_t _VColor = G;

	//通过Y的起点计算UV点: (x=5, y=6)  (Y= W*y+x, U_420P=V420P= (y/2)*(width/2)+x/2

	for (px = x; px < maxx; px += 2) //画横线
	{
		//上横线
		dstY[width * y + px] = _YColor;
		dstY[width * y + px + 1] = _YColor;
		dstY[width * y + width + px] = _YColor;
		dstY[width * y + width + px + 1] = _YColor;

		//下横线
		dstY[width * maxy + px] = _YColor;
		dstY[width * maxy + px + 1] = _YColor;
		dstY[width * (maxy + 1) + px] = _YColor;
		dstY[width * (maxy + 1) + px + 1] = _YColor;

		if (AV_PIX_FMT_NV12 == format)
		{
			//上横线UV
			dstUV[width * (y / 2) + px] = _UColor;
			dstUV[width * (y / 2) + px + 1] = _VColor;
			//下横线UV
			dstUV[width * (maxy / 2) + px] = _UColor;
			dstUV[width * (maxy / 2) + px + 1] = _VColor;
		}

		if (AV_PIX_FMT_YUV420P == format)
		{
			//上横线UV
			dstU[width / 2 * (y / 2) + px / 2] = _UColor;
			dstV[width / 2 * (y / 2) + px / 2] = _VColor;

			//下横线UV
			dstU[width / 2 * (maxy / 2) + px / 2] = _UColor;
			dstV[width / 2 * (maxy / 2) + px / 2] = _VColor;
		}
	}

	for (py = y; py <= maxy; py += 2) //画竖线
	{
		//左竖线
		dstY[width * py + x] = _YColor;
		dstY[width * py + x + 1] = _YColor;
		dstY[width * py + x + width] = _YColor;
		dstY[width * py + x + width + 1] = _YColor;

		//右竖线
		dstY[width * py + maxx] = _YColor;
		dstY[width * py + maxx + 1] = _YColor;
		dstY[width * (py + 1) + maxx] = _YColor;
		dstY[width * (py + 1) + maxx + 1] = _YColor;

		if (AV_PIX_FMT_NV12 == format)
		{
			//左竖线
			dstUV[width * (py / 2) + x] = _UColor;
			dstUV[width * (py / 2) + x + 1] = _VColor;

			//右竖线
			dstUV[width * (py / 2) + maxx] = _UColor;
			dstUV[width * (py / 2) + maxx + 1] = _VColor;
		}
		if (AV_PIX_FMT_YUV420P == format)
		{ //左竖线
			dstU[width / 2 * (py / 2) + x / 2] = _UColor;
			dstV[width / 2 * (py / 2) + x / 2] = _VColor;

			//右竖线
			dstU[width / 2 * (py / 2) + maxx / 2] = _UColor;
			dstV[width / 2 * (py / 2) + maxx / 2] = _VColor;
		}
	}
}

//HSV  色调（H, Hue)
//饱和度（S,Saturation）
//明度（V, Value） 0%~100%

//H 取值范围为0°～360°，红色为0°，绿色为120°,蓝色为240°。补色是：黄色为60°，青色为180°,品红为300°；
//红色的角度为0度，依次为黄色、绿色、青色、蓝色、橙色。连续两种颜色的角度相差60度。
//0<=h<20，    红色
//30<=h<45，   黄色
//45<=h<90，   绿色
//90<=h<125，  青色
//125<=h<150， 蓝色
//150<=h<175， 紫色
//175<=h<200， 粉红色
//200<=h<220， 砖红色
//220<=h<255， 品红色
struct HSV
{
	float h;
	float s;
	float v;
};

//https://c.runoob.com/wp-content/themes/toolrunoob/assets/js/color.js
//https://c.runoob.com/front-end/868
void _rgb2hsv(int R, int G, int B, struct HSV *hsv)
{
	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	int _max = _max_(R, G, B);
	int _min = _min_(R, G, B);

	double rd = (double) R / 255;
	double gd = (double) G / 255;
	double bd = (double) B / 255;

	double max = (double) _max / 255;
	double min = (double) _min / 255;
	double h, s, v = max;
	double d = max - min;

	s = _max == 0 ? 0 : d / max;

	if (_max == _min)
	{
		h = 0; // achromatic
	}
	else
	{
		if (_max == R)
		{
			//h = (gd - bd) / d + (gd < bd ? 6 : 0);
			h = (gd - bd) / d;
		}
		else if (_max == G)
		{
			h = (bd - rd) / d + 2;
		}
		else if (_max == B)
		{
			h = (rd - gd) / d + 4;
		}
		//h /= 6;
		h *= 60;
		if (h < 0)
			h = h + 360;
	}
//H需要转换成 0-360
	hsv->h = h;
	hsv->s = s;
	hsv->v = v;
}

//HSL代表色调(Hue)，饱和度(Saturation)和亮度(Lightness)
struct HSL
{
	float h;
	float s;
	float l;
};

//420P
struct RGB
{
	int r;
	int g;
	int b;
};

void yuvTorgb(uint8_t Y, uint8_t U, uint8_t V, struct RGB *rgb)
{
//	B = 1.164383 * (Y - 16) + 2.017232 * (U - 128);
//	G = 1.164383 * (Y - 16) - 0.391762 * (U - 128) - 0.812968 * (V - 128);
//	R = 1.164383 * (Y - 16) + 1.596027 * (V - 128);

	rgb->r = (int) ((Y & 0xff) + 1.4075 * ((V & 0xff) - 128));
	rgb->g = (int) ((Y & 0xff) - 0.3455 * ((U & 0xff) - 128) - 0.7169 * ((V & 0xff) - 128));
	rgb->b = (int) ((Y & 0xff) + 1.779 * ((U & 0xff) - 128));
	rgb->r = (rgb->r < 0 ? 0 : rgb->r > 255 ? 255 : rgb->r);
	rgb->g = (rgb->g < 0 ? 0 : rgb->g > 255 ? 255 : rgb->g);
	rgb->b = (rgb->b < 0 ? 0 : rgb->b > 255 ? 255 : rgb->b);
}

void yuv2rgb_info(AVFrame *frame)
{
	uint8_t y, u, v;
	uint8_t *ptrY = NULL;
	uint8_t *ptrU = NULL;
	uint8_t *ptrV = NULL;
	int i;
	int j;

	ptrY = frame->data[0];
	ptrU = frame->data[1];
	ptrV = frame->data[2];

	for (i = 0; i < frame->height; i++)
	{
		for (j = 0; j < frame->width; j++)
		{
			y = ptrY[j];
			u = ptrU[j / 2];
			v = ptrV[j / 2];
			struct RGB rgb;
			struct HSV hsv;
			memset(&rgb, 0, sizeof(rgb));
			yuvTorgb(y, u, v, &rgb);
			_rgb2hsv(rgb.r, rgb.g, rgb.b, &hsv);
			//printf("Y[%d:%d] UV:[%d] \n", i, j, j / 2);
//			printf("#%02X%02X%02X (%.4f %.4f %.4f) ", rgb.r, rgb.g, rgb.b,
//						hsv.h, hsv.s, hsv.v);
			printf("%d %d %d  (%.4f %.4f %.4f)", rgb.r, rgb.g, rgb.b,
						hsv.h, hsv.s, hsv.v);
		}
		printf("\n");
		ptrY += frame->linesize[0];
		ptrU += frame->linesize[1];
		ptrV += frame->linesize[2];
	}
}

int main(int argc, char **argv)
{
	AVFrame *avframe = NULL;

//"carBoxNumber":"MSCU741413845R1","rectBox":{"x":1959,"y":1959,"w":24,"h":41}}}}
	int ret = ffmpeg_jpgfileread("test.jpg", &avframe);
	if (ret != 0)
	{
		printf("err decode\n");
		return 0;
	}

	if (ret == 0)
	{
		printf("jpg : %dx%d\n", avframe->width, avframe->height);
		{
			//yuv2rgb_info(avframe);
		}
		int imglen = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
					avframe->width,
					avframe->height, 1);
		printf("buff size:%d\n", imglen);
		fflush(stdout);

		AVFrame *dstframe = av_frame_alloc();
		uint8_t *yuv420Data = av_malloc(imglen);

		dstframe->format = AV_PIX_FMT_YUV420P;
		dstframe->width = avframe->width;
		dstframe->height = avframe->height;
		//
		av_image_fill_arrays(dstframe->data, dstframe->linesize,
					yuv420Data, AV_PIX_FMT_YUV420P, dstframe->width,
					dstframe->height, 1);
		ffmpeg_SwsScale(avframe, dstframe);

		printf("%dx%d\n", dstframe->width, dstframe->height);

		void decoder_test(uint8_t *yuv420Data, int width,
					int height);

		{
			FILE *fp = fopen("out.gray.yuv", "wb");
			if (fp)
			{
				fwrite(yuv420Data, 1, dstframe->width * dstframe->height, fp);
//				uint8_t b = 0;
//				for (int i = 0; i < dstframe->width * dstframe->height; i++)
//					fwrite(&b, 1, 1, fp);
				fclose(fp);
			}
		}
		decoder_test(yuv420Data, dstframe->width,
					dstframe->height);

//		image_color_max(yuv420Data, AV_PIX_FMT_YUV420P,
//					dstframe->width, dstframe->height, 0, 0,
//					dstframe->width, dstframe->height);

		av_frame_free(&avframe);
		printf("draw\n");
		//ffmpeg_yuvbuff_draw_box(dstframe->width, dstframe->height, AV_PIX_FMT_YUV420P,
		//			yvuData, 1959, 1959, 24, 41, 0xFF, 180, 0);
		printf("save...\n");
		ffmpeg_AVFrameSaveJpgFile(dstframe, "out.jpg");

		av_free(yuv420Data);
		av_frame_free(&dstframe);
	}

	return 0;
}

void decoder_test(uint8_t *yuv420Data, int width,
			int height)
{
	zbar_image_scanner_t *scanner = NULL;
	scanner = zbar_image_scanner_create();
	zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
	/* wrap image data */
	zbar_image_t *image = zbar_image_create();
	zbar_image_set_format(image, *(int*) "GREY"); //GREY
	zbar_image_set_size(image, width, height);
	zbar_image_set_data(image,
				yuv420Data, width * height,
				NULL);

	/* scan the image for barcodes */
	int n = zbar_scan_image(scanner, image);
	printf("n=%d\n", n);
	/* extract results */
	const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
	for (; symbol; symbol = zbar_symbol_next(symbol))
	{
		/* do something useful with results */
		zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
		const char *data = zbar_symbol_get_data(symbol);
		int pointCount = zbar_symbol_get_loc_size(symbol);
		int i;

		for (i = 0; i < pointCount; i++)
		{
			int x = zbar_symbol_get_loc_x(symbol, i);
			int y = zbar_symbol_get_loc_y(symbol, i);
			ffmpeg_yuvbuff_draw_box(width, height, AV_PIX_FMT_YUV420P, yuv420Data, x, y, 5, 5, 0xFF, 0xFF, 10);
			printf("point%d=(%d,%d)\n", i, x, y);
		}
		printf("decoded %s symbol \"%s\"\n",
					zbar_get_symbol_name(typ), data);
	}

	/* clean up */
	zbar_image_destroy(image);
	zbar_image_scanner_destroy(scanner);
}
