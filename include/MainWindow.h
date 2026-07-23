#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QLabel>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVBoxLayout>
#include "ProcessorWorker.h"
#include "h/Image.h" // Твой класс

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void requestFrameProcessing(QImage img,
                                bool isBitReverse, bool isThreshold, bool isPixelated,
                                bool isSobel, bool isGray); // Сигнал для передачи кадра воркеру

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    // Слот, который вызывается каждый раз, когда камера присылает новый кадр
    void processFrame();

private:
    // Элементы интерфейса
    QLabel *m_displayLabel;

    // Объекты мультимедиа (Qt 6)
    QCamera *m_camera;
    QMediaCaptureSession *m_captureSession;
    QVideoSink *m_videoSink;

    QThread *m_workerThread;
    ProcessorWorker *m_worker;

    // Экземпляр твоего класса для обработки
    Image m_processor;

    bool bitReverse = true;
    bool thresholdOn = false;
    bool pixelated = false;
    bool sobel = false;
    bool gray = false;
};

#endif // MAINWINDOW_H
