/*
 * main.c
 *
 *  Created on: 2017年12月30日
 *      Author: cj
 */

#include <stdio.h>
#include <stdlib.h>
#include "clog.h"

#include "zbar/include/zbar.h"
#include "jpeg-9/jpeglib.h"
#include "jpeg-9/jerror.h"

/* 将jpg buffer 转为点阵buffer */
static void _convert_data (const unsigned char *p_jpg_buffer, unsigned int jpg_size, int *width, int *height, void **raw)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr err;
	cinfo.err = jpeg_std_error(&err);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, (unsigned char *)p_jpg_buffer, jpg_size);
	(void)jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_GRAYSCALE;
	(void)jpeg_start_decompress(&cinfo);
	*width = cinfo.image_width;
	*height = cinfo.image_height;
	*raw = (void *)malloc(cinfo.output_width * cinfo.output_height * 3);
	unsigned bpl = cinfo.output_width * cinfo.output_components;
	JSAMPROW buf = (void *)*raw;
	JSAMPARRAY line = &buf;
	for (; cinfo.output_scanline < cinfo.output_height; buf += bpl)
    	{
		jpeg_read_scanlines(&cinfo, line, 1);
	}
	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

int qrcode_process_test() {
	int width = 0;
	int height = 0;
	void *raw = NULL;
	unsigned char *p_ReadBuffer = NULL;
	int freadResult = 0;

	zbar_image_scanner_t *scanner = NULL;
	scanner = zbar_image_scanner_create();
	zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);
	zbar_image_t *image = zbar_image_create();
	zbar_image_set_format(image, *(int*) "Y800");

	log_d("qrcode start..");
	while (1) {
		FILE *fp = fopen("/tmp/qrcode.jpg", "rb");
		if (fp) {
			fseek(fp, 0, SEEK_SET);
			fseek(fp, 0, SEEK_END);
			freadResult = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			p_ReadBuffer = malloc(freadResult);
			if (p_ReadBuffer)
				fread(p_ReadBuffer, freadResult, 1, fp);
			fclose(fp);
		} else {
			continue;
		}
		if (!p_ReadBuffer)
			continue;

		log_d("...... raw=%d", raw);
		_convert_data(p_ReadBuffer, freadResult, &width, &height, &raw);
		log_d("raw=%d", raw);

		zbar_image_set_size(image, width, height);
		zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

		log_d("zbar_scan_image--%d, freadResult=%d", raw, freadResult);
		int n = zbar_scan_image(scanner, image); //error.....
		log_d("width:%d  height:%d n:%d\r\n", width, height, n);
		if (n > 0) {
			const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
			for (; symbol; symbol = zbar_symbol_next(symbol)) {
				zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
				const char *data = zbar_symbol_get_data(symbol);
				log_d("decoded %s symbol:%s", zbar_get_symbol_name(typ), data);
				//if (tuya_ipc_direct_connect(data, 0) == 0) {
				//goto zbar_finish;
				//}
			}
		}

		/*
		 释放sensor里面的数据
		 */
		if (p_ReadBuffer)
			free(p_ReadBuffer);
		p_ReadBuffer = NULL;
	}

	remove("/tmp/qrcode.jpg");

	zbar_finish:
	/* clean up */
	zbar_image_destroy(image);
	zbar_image_scanner_destroy(scanner);
	return NULL;
}

int main(int argc, char **argv) {
	qrcode_process_test();
	return 0;
}
