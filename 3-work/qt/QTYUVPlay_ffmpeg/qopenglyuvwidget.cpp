#include "qopenglyuvwidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QDebug>

// 调用FFmpeg的头文件
extern "C"{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"

}

#define VERTEXIN 0
#define TEXTUREIN 1
#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

QOpenGLYUVWidget::QOpenGLYUVWidget(QWidget *parent, Qt::WindowFlags f): QOpenGLWidget(parent,f)
{

}

void QOpenGLYUVWidget::play()
{
    this->m_ffplay=new QFFmpegRead();
    connect(this->m_ffplay, SIGNAL(showYuv(uchar *, int , int )), this, SLOT(slotShowYuv(uchar *, int , int )));
    qDebug()<<"start...";
    m_ffplay->start();
}

QOpenGLYUVWidget::~QOpenGLYUVWidget()
{
    makeCurrent();
    vbo.destroy();
    textureY->destroy();
    textureU->destroy();
    textureV->destroy();
    doneCurrent();
}

void QOpenGLYUVWidget::slotShowYuv(uchar *ptr, int width, int height)
{
    yuvPtr = ptr;
    videoW = width;
    videoH = height;
    update();
}

void QOpenGLYUVWidget::initializeGL()
{
    qDebug() << "initializeGL";

    //初始化opengl （QOpenGLFunctions继承）函数
    initializeOpenGLFunctions();

    //顶点shader
    const char *vString =
            "attribute vec4 vertexPosition;\
            attribute vec2 textureCoordinate;\
    varying vec2 texture_Out;\
    void main(void)\
    {\
        gl_Position = vertexPosition;\
        texture_Out = textureCoordinate;\
    }";
    //片元shader
    const char *tString =
            "varying vec2 texture_Out;\
            uniform sampler2D tex_y;\
    uniform sampler2D tex_u;\
    uniform sampler2D tex_v;\
    void main(void)\
    {\
        vec3 YUV;\
        vec3 RGB;\
        YUV.x = texture2D(tex_y, texture_Out).r;\
        YUV.y = texture2D(tex_u, texture_Out).r - 0.5;\
        YUV.z = texture2D(tex_v, texture_Out).r - 0.5;\
        RGB = mat3(1.0, 1.0, 1.0,\
                   0.0, -0.39465, 2.03211,\
                   1.13983, -0.58060, 0.0) * YUV;\
        gl_FragColor = vec4(RGB, 1.0);\
    }";

    //m_program加载shader（顶点和片元）脚本
    //片元（像素）
    qDebug()<<m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, tString);
    //顶点shader
    qDebug() << m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vString);

    //设置顶点位置
    m_program.bindAttributeLocation("vertexPosition",ATTRIB_VERTEX);
    //设置纹理位置
    m_program.bindAttributeLocation("textureCoordinate",ATTRIB_TEXTURE);

    //编译shader
    qDebug() << "m_program.link() = " << m_program.link();

    qDebug() << "m_program.bind() = " << m_program.bind();

    //传递顶点和纹理坐标
    //顶点
    static const GLfloat ver[] = {
        -1.0f,-1.0f,
        1.0f,-1.0f,
        -1.0f, 1.0f,
        1.0f,1.0f
        //        -1.0f,-1.0f,
        //        0.9f,-1.0f,
        //        -1.0f, 1.0f,
        //        0.9f,1.0f
    };
    //纹理
    static const GLfloat tex[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    //设置顶点,纹理数组并启用
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, ver);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, tex);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    //从shader获取地址
    m_textureUniformY = m_program.uniformLocation("tex_y");
    m_textureUniformU = m_program.uniformLocation("tex_u");
    m_textureUniformV = m_program.uniformLocation("tex_v");

    //创建纹理
    glGenTextures(1, &m_idy);
    //Y
    glBindTexture(GL_TEXTURE_2D, m_idy);
    //放大过滤，线性插值   GL_NEAREST(效率高，但马赛克严重)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //U
    glGenTextures(1, &m_idu);
    glBindTexture(GL_TEXTURE_2D, m_idu);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //V
    glGenTextures(1, &m_idv);
    glBindTexture(GL_TEXTURE_2D, m_idv);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void QOpenGLYUVWidget::paintGL()
{
    if(yuvPtr==nullptr)
        return;
    uchar *ptr=this->yuvPtr;
    int width=this->videoW;
    int height=this->videoH;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_idy);
    //修改纹理内容(复制内存内容)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE,ptr);
    //与shader 关联
    glUniform1i(m_textureUniformY, 0);

    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, m_idu);
    //修改纹理内容(复制内存内容)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width/2, height/2, 0, GL_RED, GL_UNSIGNED_BYTE, ptr+width*height);
    //与shader 关联
    glUniform1i(m_textureUniformU,1);

    glActiveTexture(GL_TEXTURE0+2);
    glBindTexture(GL_TEXTURE_2D, m_idv);
    //修改纹理内容(复制内存内容)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width/2, height/2, 0, GL_RED, GL_UNSIGNED_BYTE, ptr+width*height*5/4);
    //与shader 关联
    glUniform1i(m_textureUniformV, 2);

    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}


QFFmpegRead::QFFmpegRead(QObject *parent):QThread(parent)
{

}

void QFFmpegRead::run()
{
    int ret;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    static const char *filename =
            "abc.mp4";

    qDebug()<<"run.......";

    AVCodec *dec;

    AVFormatContext *fmt_ctx=NULL;
    AVCodecContext *dec_ctx=NULL;
    AVInputFormat *inputFmt = NULL;

    int video_stream_index = -1;

    //    inputFmt = av_find_input_format("video4linux2");
    //    if (inputFmt == NULL)    {
    //        printf("can not find_input_format\n");
    //    }

    if ((ret = avformat_open_input(&fmt_ctx, filename, inputFmt, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ;
    }

    /* select the video stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Cannot find a video stream in the input file\n");
        return ;
    }
    video_stream_index = ret;

    const AVCodecDescriptor *des=avcodec_descriptor_get(dec->id);
    printf("input file video type:%s\n", des->name);

    /* create decoding context */
    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx)
        return ;
    avcodec_parameters_to_context(dec_ctx,
                                  fmt_ctx->streams[video_stream_index]->codecpar);
    av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0);

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ;
    }

    int imagebufflen = av_image_get_buffer_size(
                (enum AVPixelFormat)fmt_ctx->streams[video_stream_index]->codecpar->format,
                fmt_ctx->streams[video_stream_index]->codecpar->width,
                fmt_ctx->streams[video_stream_index]->codecpar->height, 1);

    printf("format:%d, %dx%d\n",(enum AVPixelFormat)fmt_ctx->streams[video_stream_index]->codecpar->format,
           fmt_ctx->streams[video_stream_index]->codecpar->width,
           fmt_ctx->streams[video_stream_index]->codecpar->height);
    fflush(stdout);

    uint8_t *imagebuff=(uint8_t*)av_malloc(imagebufflen);

    /* read all packets */
    while (1) {
        QThread::msleep(10);
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;

        if (packet.stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "Error while sending a packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR,
                           "Error while receiving a frame from the decoder\n");
                    goto end;
                }

                if (ret >= 0) {

                    const AVPixFmtDescriptor *des = av_pix_fmt_desc_get((AVPixelFormat)frame->format);
                    //printf(" %s, %dx%d\n",  des->name, frame->width,
                    //       frame->height);

                    ret = av_image_copy_to_buffer(imagebuff, imagebufflen, (const uint8_t * const *)frame->data,
                                                  (const int *)frame->linesize, (enum AVPixelFormat)frame->format,
                                                  frame->width, frame->height, 1);

                    emit this->showYuv(imagebuff, frame->width, frame->height);

                    //avframe_yuv420_tojpgfile(frame, outfilename);
                    //					display_frame(frame,
                    //							frame->sample_aspect_ratio);

#if 0
                    frame->pts = frame->best_effort_timestamp;

                    /* push the decoded frame into the filtergraph */
                    if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
                                                     AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR,
                               "Error while feeding the filtergraph\n");
                        break;
                    }

                    /* pull filtered frames from the filtergraph */
                    while (1) {
                        ret = av_buffersink_get_frame(buffersink_ctx,
                                                      filt_frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        if (ret < 0)
                            goto end;
                        display_frame(filt_frame,
                                      buffersink_ctx->inputs[0]->time_base);
                        av_frame_unref(filt_frame);
                    }
                    av_frame_unref(frame);
#endif
                }
            }
        }
        av_packet_unref(&packet);
    }

end:
    av_free(imagebuff);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
}
