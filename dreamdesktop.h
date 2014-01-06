#ifndef PLASMA_WALLPAPER_DREAMDESKTOP
#define PLASMA_WALLPAPER_DREAMDESKTOP

#include <Plasma/Wallpaper>
#include <stdint.h>
#include <QMutex>
#include <QWaitCondition>
#include <QDesktopWidget>
#include <keditlistbox.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include "ui_dreamdesktop.h"

class DreamDesktop;

class VideoPlayer : public QThread
{
    Q_OBJECT

    public:
        DreamDesktop* dd;
        explicit VideoPlayer(QObject *parent, DreamDesktop* tdd);
        ~VideoPlayer();

    protected:
        AVStream *video_st;
        AVCodecContext *pCodecCtx;
        uint8_t *buffer;
        int streamIndex;
        AVCodec *pCodec;
        AVFrame *pFrame;
        int bufferSize;
        AVPacket packet;
        int frameFinished;
        bool codecReady;
        double synchronize_video(double pts);
        double last_pts;
        int last_delay;
        double video_clock;
        int screenWidth;
        int screenHeight;
        void run();

    public:
        QTime clock;
        bool initCodec();
        bool stopPlayback; 
        bool playback();
        void deinitCodec();
        void garbageCollection();
        AVFormatContext *pFormatCtx;//*//
};

class DreamDesktop : public Plasma::Wallpaper
{
    Q_OBJECT//*//

    public://*//

        DreamDesktop(QObject* parent, const QVariantList& args);
        ~DreamDesktop();
        void updateFrame();
        virtual void paint(QPainter* painter, const QRectF& exposedRect);
        virtual QWidget *createConfigurationInterface(QWidget* parent);
        virtual void save(KConfigGroup &config);
        QMutex* mutex;                                                                                                                                                                                                                                                       
        QString str;
        QString m_wallpaperFile;
        QStringList m_wallpaperList;
        int boundingWidth;
        int boundingHeight;
        int m_fpsRate;
        int m_swsFilter;
        AVFrame *pFrameRGB;

    private:
            VideoPlayer videoPlayer;
            KFileDialog* m_dialog;

    protected:
            void init(const KConfigGroup &config);
            Ui::DreamDesktopConfig m_uiDreamDesktopConfig;
            QWidget* m_configWidget;
            QString fileDialogFile;

    protected slots:
            void configWidgetDestroyed();
            void settingsModified();
            void slotAddVideo();
            void slotRemoveVideo();
            void videoSelected();
            void fileDialogFinished();
            void fileDialogSelected();

    signals:
            void settingsChanged(bool);
};

K_EXPORT_PLASMA_WALLPAPER(dreamdesktop, DreamDesktop)

#endif
