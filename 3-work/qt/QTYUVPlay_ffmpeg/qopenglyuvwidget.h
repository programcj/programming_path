#ifndef QOPENGLYUVWIDGET_H
#define QOPENGLYUVWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QThread>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class QFFmpegRead:public QThread {
    Q_OBJECT

public:
   explicit QFFmpegRead(QObject *parent = nullptr);

signals:
    void showYuv(uchar *ptr, int width, int height);

protected:

    void run() Q_DECL_OVERRIDE;
};


class QOpenGLYUVWidget : public QOpenGLWidget, protected QOpenGLFunctions {

    Q_OBJECT

    ~QOpenGLYUVWidget();
public:
    explicit QOpenGLYUVWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void play();

public slots:
    void slotShowYuv(uchar *ptr,int width,int height); //update一帧yuv图像

protected:
    void initializeGL() Q_DECL_OVERRIDE;

    void paintGL() Q_DECL_OVERRIDE;

private:

    //shader程序
    QOpenGLShaderProgram m_program;
    //shader中yuv变量地址
    GLuint m_textureUniformY, m_textureUniformU , m_textureUniformV;
    //创建纹理
    GLuint m_idy , m_idu , m_idv;

    QOpenGLShaderProgram *program; //shader程序
    QOpenGLBuffer vbo;

    GLuint textureUniformY,textureUniformU,textureUniformV; //opengl中y、u、v分量位置
    QOpenGLTexture *textureY = nullptr,*textureU = nullptr,*textureV = nullptr;
    GLuint idY,idU,idV; //自己创建的纹理对象ID，创建错误返回0
    uint videoW,videoH;
    uchar *yuvPtr = nullptr;

    QFFmpegRead *m_ffplay=nullptr;
};

#endif // QOPENGLYUVWIDGET_H
