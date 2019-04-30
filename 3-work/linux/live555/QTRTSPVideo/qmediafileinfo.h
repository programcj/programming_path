/**
  * 多媒体文件信息，需要ffmpeg来读
  */
#ifndef QMEDIAFILEINFO_H
#define QMEDIAFILEINFO_H

#include <QObject>
#include <QtCore>
#include <QMap>

/**
 * @brief The QMediaFileInfo class
 * 多媒体文件信息类，采用ffmpeg读取信息
 *
 */
class QMediaFileInfo : public QObject
{

    int fWidth;
    int fHeight;
    int fViewBitRae;

    Q_OBJECT

    QMap<QString,QString> videoInfoMap;
    void addVideoInfo(int v,const QString &info);

public:
    enum {
        SUM_FRAME,  //总帧数
        VIDEO_BIT_RATE //码流(kbps)
    };


    QString getVideoInfo(int key);

    static void Init();
    explicit QMediaFileInfo(QObject *parent = 0);

    QMediaFileInfo(const QString &filePath, QObject *parent=0);

    int getHeight() const;

    int getWidth() const;

    void debugInfo(const QString &filePath);

    int getVideoBitRae() const;

signals:

public slots:
};

#endif // QMEDIAFILEINFO_H
