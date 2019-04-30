#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#define CONFIG_CLOG_BASIC
#include "../public_tools/clog.h"

#include <errno.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int xioctl(int fd, int IOCTL_X, void* arg) {
#define IOCTL_RETRY 4
  int ret = 0;
  int tries = IOCTL_RETRY;
  do {
    ret = ioctl(fd, IOCTL_X, arg);
  } while (ret && tries-- &&
           ((errno == EINTR) || (errno == EAGAIN) || (errno == ETIMEDOUT)));

  if (ret && (tries <= 0))
    fprintf(stderr, "ioctl (%i) retried %i times - giving up: %s)\n", IOCTL_X,
            IOCTL_RETRY, strerror(errno));

  return (ret);
}

void out2file(const void* buf, int len) {
  int fd = 0;
  int ret = 0;
  char* file = "/tmp/uvc_test.mjpeg";

  if (access(file, F_OK) == 0) {
    long flag = 0;
#if 1
	fd = open(file, O_WRONLY);
#else
	fd = open(file, O_WRONLY | O_APPEND);
#endif
    //		struct stat sf;
    //		memset(&sf, 0, sizeof(sf));
    //		stat(file, &sf);
    //		printf("MODE=%o", sf.st_mode);
    //		chmod(file, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    
    //		log_d("APPEND=%X",  O_APPEND| O_WRONLY); O_TRUNC
    //		fcntl(fd, F_GETFL, &flag);
    //		log_d("flag=%X, %o", flag, O_NONBLOCK);
    // ret=lseek(fd, 0, SEEK_END);
    // log_d("ret=%d, %s", ret, ret==-1? strerror(errno):"OK");
  } else {
    fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  }
  ret = write(fd, buf, len);
  if (ret < 0) log_e("ret=%d, %s", ret, ret == -1 ? strerror(errno) : "OK");
  close(fd);
}

int v4l2_init(int fd) {
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;
  struct v4l2_streamparm frameint;

  int ret = 0;
  int width = 640, height = 480, fps = -1, formatIn = V4L2_PIX_FMT_MJPEG, i;

  if ((ret = xioctl(fd, VIDIOC_QUERYCAP, &cap)) < 0) {
    log_e("err..VIDIOC_QUERYCAP.");
    return -1;
  }
  log_i(
      "DriverName:%s\nCard Name:%s\nBus info:%s\nDriverVersion:%u.%u.%u\n "
      "cap:%02X",
      cap.driver, cap.card, cap.bus_info, (cap.version >> 16) & 0XFF,
      (cap.version >> 8) & 0XFF, (cap.version & 0xFF), cap.capabilities);
  struct v4l2_fmtdesc fmtdesc;
  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.index = 0; /* the number to check */
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
    log_d("support device %d.%s ", fmtdesc.index + 1, fmtdesc.description);
    fmtdesc.index++;
  }

  memset(&fmt, 0, sizeof(struct v4l2_format));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = formatIn;                // V4L2_PIX_FMT_MJPEG
  fmt.fmt.pix.field = V4L2_FIELD_ANY;                //
  if ((ret = xioctl(fd, VIDIOC_S_FMT, &fmt)) < 0) {  //设置频格式
    log_e("VIDIOC_S_FMT");
    return -1;
  }
  log_i("format: %X", fmt.fmt.pix.pixelformat);

  struct v4l2_streamparm* setfps;
  setfps = (struct v4l2_streamparm*)calloc(1, sizeof(struct v4l2_streamparm));
  memset(setfps, 0, sizeof(struct v4l2_streamparm));
  setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  /*
   * first query streaming parameters to determine that the FPS selection is
   * supported
   */
  ret = xioctl(fd, VIDIOC_G_PARM, setfps);  //获取视频帧率

  memset(setfps, 0, sizeof(struct v4l2_streamparm));
  setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  setfps->parm.capture.timeperframe.numerator = 1;
  setfps->parm.capture.timeperframe.denominator =
      fps == -1 ? 30 : fps;  // if no default fps set set it to maximum
  ret = xioctl(fd, VIDIOC_S_PARM, setfps);
  return 0;
}

int v4l2_create_mmap(int fd, void* membuf[], int* membuf_len,
                     int membuf_count) {
  int ret;
  int i = 0;
  /*
   * request buffers
   */
  struct v4l2_requestbuffers rb;
  memset(&rb, 0, sizeof(struct v4l2_requestbuffers));
  rb.count = membuf_count;
  rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rb.memory = V4L2_MEMORY_MMAP;
  ret = xioctl(fd, VIDIOC_REQBUFS, &rb);  //申请帧缓冲区
  log_d("RB.Count=%d", rb.count);

  struct v4l2_buffer buf;

  for (i = 0; i < membuf_count; i++) {
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.index = i;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = xioctl(fd, VIDIOC_QUERYBUF, &buf);  //查询帧缓冲区
    if (ret < 0) {
      perror("Unable to query buffer");
      return -1;
    }
    // fprintf(stderr, "length: %u offset: %u\n", buf.length, buf.m.offset);
    // log_d("length: %u offset: %u", buf.length, buf.m.offset);
    //内存映射
    membuf[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                     buf.m.offset);
    if (membuf[i] == MAP_FAILED) {
      log_e("mmap >> Unable to map buffer");
      return -1;
    }
    membuf_len[i] = buf.length;
    log_d("Buffer mapped at address %p.", membuf[i]);
  }

  /*
   * Queue the buffers.
   */
  for (i = 0; i < membuf_count; ++i) {
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.index = i;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = xioctl(fd, VIDIOC_QBUF, &buf);  //重新排入队列
    if (ret < 0) {
      perror("Unable to queue buffer");
      return -1;
    }
  }
  return 0;
}

static int vrl2_streamon(int fd) {
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int ret;
  ret = xioctl(fd, VIDIOC_STREAMON, &type);  //启动数据流
  if (ret < 0) {
    perror("Unable to start capture");
    return ret;
  }
  return 0;
}

static int vrl2_streamoff(int fd) {
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int ret;
  log_d("Stopping capture\n");
  ret = xioctl(fd, VIDIOC_STREAMOFF, &type);  //停止数据流
  if (ret != 0) {
    perror("Unable to stop capture");
    return ret;
  }
  log_d("Stopping capture done\n");
  return 0;
}

int v4l2_process(int fd, void* mem[]) {
  int ret = 0;
  int i = 0;
  struct v4l2_buffer buf;
  struct v4l2_format format;

  memset(&format, 0, sizeof format);
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  xioctl(fd, VIDIOC_G_FMT, &format);

  memset(&buf, 0, sizeof(struct v4l2_buffer));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  ret = xioctl(fd, VIDIOC_DQBUF, &buf);  // 从缓冲区取出一个缓冲帧

  switch (format.fmt.pix.pixelformat) {
    case V4L2_PIX_FMT_MJPEG:
      // log_d("MJPEG");
      break;
    case V4L2_PIX_FMT_YUV420:
      break;
  }

  /*
     log_d("out > file: index:%d, addr: %p , len:%d", buf.index,
     mem[buf.index], buf.bytesused);*/
  //	    buf.bytesused;
  //	       buf.timestamp;
  out2file(mem[buf.index], buf.bytesused);

  ret = xioctl(fd, VIDIOC_QBUF, &buf);  // 将取出的缓冲帧放回缓冲区
  return 0;
}

struct run_arg {
  int fd;
  void* membuf;
  volatile int run;
};

void* _run(void* arg) {
  struct run_arg* a = (struct run_arg*)arg;
  vrl2_streamon(a->fd);
  while (a->run) {
    v4l2_process(a->fd, a->membuf);
    usleep(10);
  }
  vrl2_streamoff(a->fd);
  return NULL;
}

//彷徨 茫然
int main(int argc, char** argv) {
  log_i("start..");
  int i = 0;
  int fd = 0;

  fd = v4l2_open("/dev/video0", O_RDWR);
  if (fd < 0) return -1;
  void* membuf[4];
  int membuf_len[4] = {0};
  memset(membuf, 0, 4 * 4);

  if (v4l2_init(fd) < 0) goto __quit;

  if (v4l2_create_mmap(fd, membuf, membuf_len, 4) < 0) goto _quit_mmap;

  {
    pthread_t pt;
    struct run_arg arg;
    arg.fd = fd;
    arg.membuf = membuf;
    arg.run = 1;
    pthread_create(&pt, NULL, _run, &arg);

    while (1) {
      char buf[100];
      memset(buf, 0, sizeof(buf));
      printf("Please input e to exit.\n");
      scanf("%s", buf);
      if (strcmp(buf, "e") == 0) break;
    }
    arg.run = 0;
    pthread_join(pt, NULL);
  }

  log_d("========================");
_quit_mmap:
  for (i = 0; i < 4; i++) {
    if (membuf[i] != NULL) munmap(membuf[i], membuf_len[i]);
    log_d("unmap  address:%p, len:%d", membuf[i], membuf_len[i]);
  }
__quit:
  v4l2_close(fd);
  log_d("exit");
  return 0;
}
