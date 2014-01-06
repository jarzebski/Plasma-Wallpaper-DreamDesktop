#define __STDC_CONSTANT_MACROS 1

#include <QPainter>
#include <QtGui/QMessageBox>
#include <QTimer>
#include <QThread>
#include <QUuid>
#include <qdatetime.h>
#include <KDirSelectDialog>
#include <KDirWatch>
#include <KFileDialog>
#include <KFileMetaInfo>
#include <KRandom>
#include <KStandardDirs>
#include <KIO/Job>

#include "dreamdesktop.h"

enum DreamDesktopFpsRate {
    DD_FPS_AUTO,
    DD_FPS_LIMIT_30,
    DD_FPS_LIMIT_25,
    DD_FPS_LIMIT_20,
    DD_FPS_LIMIT_15,
    DD_FPS_LIMIT_10
};

enum DreamDesktopSwsFilter {
    DD_F_FAST_BILINEAR,
    DD_F_BILINEAR,
    DD_F_BICUBIC,
    DD_F_X,
    DD_F_POINT,
    DD_F_AREA,
    DD_F_BICUBLIN,
    DD_F_GAUSS,
    DD_F_SINC,
    DD_F_LANCZOS,
    DD_F_SPLINE
};

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

int our_get_buffer(struct AVCodecContext *c, AVFrame *pic)
{
    int ret = avcodec_default_get_buffer(c, pic);
    uint64_t *pts = new uint64_t;
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    return ret;
}

double VideoPlayer::synchronize_video(double pts)
{
    double frame_delay;

    if (pts != 0)
    {
        video_clock = pts;
    } else
    {
        pts = video_clock;
    }

    frame_delay = av_q2d(video_st->codec->time_base);
    frame_delay += pFrame->repeat_pict * (frame_delay * 0.5);
    video_clock += frame_delay;

    return pts;
}

void our_release_buffer(struct AVCodecContext *c, AVFrame *pic)
{
    if (pic)
    {
        delete reinterpret_cast<uint64_t*>(pic->opaque);
    }

    avcodec_default_release_buffer(c, pic);
}

VideoPlayer::VideoPlayer(QObject *parent, DreamDesktop* tdd) : QThread(parent), dd(tdd), pCodecCtx(NULL), buffer(NULL), pFrame(NULL), pFormatCtx(NULL)
{
}

VideoPlayer::~VideoPlayer()
{
    deinitCodec();
}

bool VideoPlayer::initCodec()
{
    av_register_all();

    if (avformat_open_input(&pFormatCtx, dd->m_wallpaperFile.toLocal8Bit().data(), NULL, NULL) != 0)
    {
        return false;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        return false;
    }

    streamIndex=-1;

    for (size_t i=0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            streamIndex = i;
            break;
        }
    }

    if (streamIndex == -1)
    {
        return false;
    }

    video_st = pFormatCtx->streams[streamIndex];
    pCodecCtx = pFormatCtx->streams[streamIndex]->codec;
    pCodecCtx->get_buffer = our_get_buffer;
    pCodecCtx->release_buffer = our_release_buffer;

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL)
    {
        return false;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        return false;
    }

    pFrame = avcodec_alloc_frame();

    dd->mutex->lock();
    dd->pFrameRGB = avcodec_alloc_frame();

    if (dd->pFrameRGB == NULL)
    {
        return false;
    }

    QRect screenGeometry = QApplication::desktop()->screenGeometry();

    screenWidth = screenGeometry.width();
    screenHeight = screenGeometry.height();

    bufferSize = avpicture_get_size(PIX_FMT_RGB32, screenWidth, screenHeight);

    buffer = (uint8_t *)av_malloc(bufferSize*sizeof(uint8_t));
    avpicture_fill((AVPicture *)dd->pFrameRGB, buffer, PIX_FMT_RGB32, screenWidth, screenHeight);

    dd->mutex->unlock();

    codecReady = true;

    return true;
}

void VideoPlayer::deinitCodec()
{
    dd->mutex->lock();
    codecReady = false;
    
    if (pFormatCtx)
    {
        av_free(buffer);
        av_free(dd->pFrameRGB);
        av_free(pFrame);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);
    }

    dd->mutex->unlock();
}

void VideoPlayer::garbageCollection()
{
    pFormatCtx = NULL;
    streamIndex = 0;
    video_st = NULL;
    pCodecCtx = NULL;
    pCodec = NULL;
    pFrame = NULL;
    dd->pFrameRGB = NULL;
    screenWidth = 0;
    screenHeight = 0;
    bufferSize = 0;
    buffer = NULL;
}

bool VideoPlayer::playback()
{
    clock.restart();

    while (!stopPlayback && pFormatCtx && av_read_frame(pFormatCtx, &(packet))>=0)
    {
        if (!stopPlayback && (packet.stream_index == streamIndex))
        {
            AVPacket avpkt;
            av_init_packet(&avpkt);
            avpkt.data = packet.data;
            avpkt.size = packet.size;
            avcodec_decode_video2(pCodecCtx, pFrame, &(frameFinished), &avpkt);

            double pts = 0;

            if (packet.dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE)
            {
                pts = *(uint64_t *)pFrame->opaque;
            } else 
            if (packet.dts != AV_NOPTS_VALUE) 
            {
                pts = packet.dts;
            } else 
            {
                pts = 0;
            }

            pts *= av_q2d(video_st->time_base);

            if (frameFinished)
            {
                dd->boundingWidth = dd->boundingRect().width();
                dd->boundingHeight = dd->boundingRect().height();

                if (dd->boundingWidth > screenWidth)
                {
                    dd->boundingWidth = screenWidth;
                }

                if (dd->boundingHeight > screenHeight)
                {
                    dd->boundingHeight = screenHeight;
                }

                int useFilter = SWS_FAST_BILINEAR;

                switch (dd->m_swsFilter)
                {
                    case DD_F_FAST_BILINEAR: useFilter = SWS_FAST_BILINEAR; break;
                    case DD_F_BILINEAR: useFilter = SWS_BILINEAR; break;
                    case DD_F_BICUBIC: useFilter = SWS_BICUBIC; break;
                    case DD_F_X: useFilter = SWS_X; break;
                    case DD_F_POINT: useFilter = SWS_POINT; break;
                    case DD_F_AREA: useFilter = SWS_AREA; break;
                    case DD_F_BICUBLIN: useFilter = SWS_BICUBLIN; break;
                    case DD_F_GAUSS: useFilter = SWS_GAUSS; break;
                    case DD_F_SINC: useFilter = SWS_SINC; break;
                    case DD_F_LANCZOS: useFilter = SWS_LANCZOS; break;
                    case DD_F_SPLINE: useFilter = SWS_SPLINE; break;
                }

                SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, dd->boundingWidth, dd->boundingHeight, PIX_FMT_RGB32, useFilter, NULL, NULL, NULL);

                dd->mutex->lock();

                sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, dd->pFrameRGB->data, dd->pFrameRGB->linesize);
                sws_freeContext(img_convert_ctx);

                dd->mutex->unlock();

                pts = synchronize_video(pts);
                double delay = 0;

                switch (dd->m_fpsRate)
                {
                    case DD_FPS_AUTO: delay = (pts - last_pts); break;
                    case DD_FPS_LIMIT_30: delay = 0.0333; break;
                    case DD_FPS_LIMIT_25: delay = 0.04; break;
                    case DD_FPS_LIMIT_20: delay = 0.05; break;
                    case DD_FPS_LIMIT_15: delay = 0.0666; break;
                    case DD_FPS_LIMIT_10: delay = 0.1; break;
                }

                if (delay <= 0 || delay >= 1.0)
                {
                    delay = last_delay;
                }

                last_pts = pts;
                last_delay = delay;

                int elapsed = clock.restart();

                int wait = (delay*1000)-elapsed;

                dd->updateFrame();

                if (wait > 0)
                {
                    QThread::msleep(wait);
                }

                clock.restart();

            }
        }

        av_free_packet(&(packet));
    }

    if (pFormatCtx)
    { 
        av_seek_frame(pFormatCtx, streamIndex, 0,  AVSEEK_FLAG_FRAME);
        return true;
    } else
    {
        return false;
    }
}

void VideoPlayer::run()
{
    stopPlayback = false;

    while(true && !stopPlayback)
    {
        garbageCollection();
        deinitCodec();
        initCodec();

        while(playback() && !stopPlayback)
        {
        }
    }
}

DreamDesktop::DreamDesktop(QObject *parent, const QVariantList &args):Plasma::Wallpaper(parent, args), videoPlayer(parent, this), m_dialog(0), m_configWidget(0)
{
    mutex = new QMutex;
    pFrameRGB = NULL;
}

DreamDesktop::~DreamDesktop()
{
    videoPlayer.stopPlayback = true;
    videoPlayer.wait();
}

void DreamDesktop::updateFrame()
{
    emit(update(boundingRect()));
}

void DreamDesktop::paint(QPainter *painter, const QRectF &exposedRect)
{
    if (pFrameRGB)
    {
        mutex->lock();
        const QImage imag(pFrameRGB->data[0], boundingWidth, boundingHeight, pFrameRGB->linesize[0], QImage::Format_RGB32);
        painter->drawImage(exposedRect, imag, exposedRect);
        painter->setPen(QPen(QColor(255,255,255)));
        mutex->unlock();
    } else
    {
        painter->setBrush(Qt::black);
        painter->drawRect(exposedRect);
    }
}

void DreamDesktop::init(const KConfigGroup &config)
{
    m_wallpaperList = config.readEntry("wallpaperList", QStringList());
    m_wallpaperFile = config.readEntry("wallpaperFile", QString());
    m_fpsRate = config.readEntry("fpsRate", (int)DD_FPS_AUTO);
    m_swsFilter = config.readEntry("swsFilter", (int)DD_F_FAST_BILINEAR);

    if (QFile(m_wallpaperFile).exists())
    {
        if (!videoPlayer.isRunning())
        {
            videoPlayer.start();
        } else
        {
            videoPlayer.stopPlayback = true;
            videoPlayer.wait();
            videoPlayer.start();
        }
    }
}

QWidget *DreamDesktop::createConfigurationInterface(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);

    connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(configWidgetDestroyed()));

    m_uiDreamDesktopConfig.setupUi(widget);

    m_uiDreamDesktopConfig.m_fpsRate->setCurrentIndex(m_fpsRate);
    m_uiDreamDesktopConfig.m_swsFilter->setCurrentIndex(m_swsFilter);
    m_uiDreamDesktopConfig.m_addVideo->setIcon(KIcon("list-add"));
    m_uiDreamDesktopConfig.m_removeVideo->setIcon(KIcon("list-remove"));

    for (size_t i = 0; i < m_wallpaperList.size(); i++)
    {
        QString videoName;
        videoName = QFileInfo(m_wallpaperList.at(i)).completeBaseName();
        m_uiDreamDesktopConfig.m_videoList->addItem(new QListWidgetItem(KIcon(KStandardDirs::locate("data", QLatin1String( "dreamdesktop/thumbs/default.png" ))), videoName));
    }

    m_uiDreamDesktopConfig.m_videoList->setCurrentRow(0);

    connect(m_uiDreamDesktopConfig.m_fpsRate, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsModified()));
    connect(m_uiDreamDesktopConfig.m_swsFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsModified()));
    connect(m_uiDreamDesktopConfig.m_addVideo, SIGNAL(clicked()), this, SLOT(slotAddVideo()));
    connect(m_uiDreamDesktopConfig.m_removeVideo, SIGNAL(clicked()), this, SLOT(slotRemoveVideo()));
    connect(m_uiDreamDesktopConfig.m_videoList, SIGNAL(clicked(QModelIndex)), this, SLOT(videoSelected()));

    connect(this, SIGNAL(settingsChanged(bool)), parent, SLOT(settingsChanged(bool)));

    return widget;
}

void DreamDesktop::save(KConfigGroup &config)
{
    config.writeEntry("wallpaperFile", m_wallpaperFile);
    config.writeEntry("wallpaperList", m_wallpaperList);
    config.writeEntry("fpsRate", m_fpsRate);
    config.writeEntry("swsFilter", m_swsFilter);
}

void DreamDesktop::configWidgetDestroyed()
{
    m_configWidget = 0;
}

void DreamDesktop::settingsModified()
{
    m_fpsRate = m_uiDreamDesktopConfig.m_fpsRate->currentIndex();
    m_swsFilter = m_uiDreamDesktopConfig.m_swsFilter->currentIndex();
    emit settingsChanged(true);
}

void DreamDesktop::videoSelected()
{
    m_uiDreamDesktopConfig.m_removeVideo->setEnabled(true);

    int row = m_uiDreamDesktopConfig.m_videoList->currentRow();

    if (row != -1) 
    {
        m_wallpaperFile = m_wallpaperList.at(row);
    }

    emit settingsChanged(true);
}

void DreamDesktop::slotRemoveVideo()
{
    int row = m_uiDreamDesktopConfig.m_videoList->currentRow();

    if (row != -1) 
    {
        m_uiDreamDesktopConfig.m_videoList->takeItem(row);
        m_wallpaperList.removeAt(row);

        int row = m_uiDreamDesktopConfig.m_videoList->currentRow();

        if (row != -1) 
        {
            m_wallpaperFile = m_wallpaperList.at(row);
        }

        m_uiDreamDesktopConfig.m_removeVideo->setEnabled(m_uiDreamDesktopConfig.m_videoList->currentRow() != -1);

    }

    emit settingsChanged(true);
}

void DreamDesktop::slotAddVideo()
{
    if (!m_dialog)
    {
        m_dialog = new KFileDialog(KUrl(), "*.avi *.mkv *.ogg *.mpg *.mpeg *.ogv *.mp4 *.ogm *.asf *.flv *.mp4 *.wmv", m_configWidget);
        m_dialog->setOperationMode(KFileDialog::Opening);
        m_dialog->setInlinePreviewShown(true);
        m_dialog->setCaption(i18n("Select Wallpaper Image File"));
        m_dialog->setModal(false);
        connect(m_dialog, SIGNAL(okClicked()), this, SLOT(fileDialogSelected()));
        connect(m_dialog, SIGNAL(destroyed(QObject*)), this, SLOT(fileDialogFinished()));
    }

    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
}

void DreamDesktop::fileDialogFinished()
{
    m_dialog = 0;
}

void DreamDesktop::fileDialogSelected()
{
    fileDialogFile = m_dialog->selectedFile();

    if (fileDialogFile.isEmpty()) 
    {
        return;
    } else
    {
        m_wallpaperList << fileDialogFile;
        QString videoName;
        videoName = QFileInfo(fileDialogFile).completeBaseName();
        m_uiDreamDesktopConfig.m_videoList->addItem(new QListWidgetItem(KIcon(KStandardDirs::locate("data", QLatin1String( "dreamdesktop/thumbs/default.png" ))), videoName));
    }

    emit(settingsChanged(true));
}

#include "dreamdesktop.moc"

