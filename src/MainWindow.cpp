#include "MainWindow.h"
#include <QMediaDevices>
#include <QVideoFrame>
#include <QImage>
#include <QPixmap>
#include <QThread>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    m_displayLabel = new QLabel("Camera loading...", this);
    m_displayLabel->setAlignment(Qt::AlignCenter);
    m_displayLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    layout->addWidget(m_displayLabel);
    setCentralWidget(centralWidget);
    resize(800, 600);

    m_camera = new QCamera(QMediaDevices::defaultVideoInput(), this);
    m_captureSession = new QMediaCaptureSession(this);
    m_videoSink = new QVideoSink(this);

    QList<QCameraFormat> formats = m_camera->cameraDevice().videoFormats();

    if (!formats.isEmpty()) {
        QCameraFormat bestFormat;
        for (const auto &fmt : formats) {
            // Ищем формат 512x512 или близкий, желательно 32-битный если есть
            if (fmt.resolution() == QSize(512, 512)) {
                bestFormat = fmt;
                break;
            }
        }
        if (bestFormat.isNull()) {
            bestFormat = formats.first();
        }
        m_camera->setCameraFormat(bestFormat);
        qDebug() << "Selected resolution:" << bestFormat.resolution();
    }

    m_captureSession->setCamera(m_camera);
    m_captureSession->setVideoOutput(m_videoSink);

    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &MainWindow::processFrame);

    m_workerThread = new QThread(this);
    m_worker = new ProcessorWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, &MainWindow::requestFrameProcessing, m_worker, &ProcessorWorker::processFrame);

    // Qt::QueuedConnection используется по умолчанию между потоками, но укажем явно для ясности
    connect(m_worker, &ProcessorWorker::frameProcessed, this, [this](QImage img){
        // setPixmap в главном потоке довольно тяжелый, если картинка большая
        // scaled лучше делать тут, но если размер окна не меняется часто, можно оптимизировать и это
        m_displayLabel->setPixmap(QPixmap::fromImage(img).scaled(m_displayLabel->size(), Qt::KeepAspectRatio));
    });

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();
    m_camera->start();
}

MainWindow::~MainWindow()
{
    m_workerThread->quit();
    m_workerThread->wait();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_W)
    {
        bitReverse = !bitReverse;
    }
    if (event->key() == Qt::Key_T)
    {
        thresholdOn = !thresholdOn;
    }
    if (event->key() == Qt::Key_P)
    {
        pixelated = !pixelated;
    }
    if (event->key() == Qt::Key_S)
    {
        sobel = !sobel;
    }
    if (event->key() == Qt::Key_G)
    {
        gray = !gray;
    }
}

void MainWindow::processFrame()
{
    QVideoFrame frame = m_videoSink->videoFrame();
    if (!frame.isValid()) return;

    // ВАЖНО: Используем Format_RGB32 вместо Format_RGB888.
    // Процессору намного быстрее копировать и обрабатывать блоки по 4 байта (int),
    // чем по 3 байта (не выровненная память).
    QImage qtImage = frame.toImage().convertToFormat(QImage::Format_RGB32);

    emit requestFrameProcessing(qtImage,
                                bitReverse, thresholdOn, pixelated,
                                sobel, gray);
}
