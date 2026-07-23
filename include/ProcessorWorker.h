#ifndef PROCESSORWORKER_H
#define PROCESSORWORKER_H

#include <QObject>
#include <QImage>
#include "h/Image.h"

class ProcessorWorker : public QObject
{
    Q_OBJECT
public:
    explicit ProcessorWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void processFrame(const QImage &img,
                      bool isBitReverse, bool isThreshold, bool isPixelated,
                      bool isSobel, bool isGray) {
        // 1. Запоминаем оригинальные размеры до всех манипуляций
        int origW = img.width();
        int origH = img.height();

        // Загружаем данные
        // img.depth() / 8 для RGB32 будет равно 4
        int channels = img.depth() / 8;
        m_processor.loadFromMemory(origW, origH, channels, img.bits());

        if (isGray)
        {
            m_processor.applyGrayscale();
        }

        if (isSobel)
        {
            m_processor.applySobel();
        }

        // 2. Дополняем до степени двойки (появляются черные полосы)
        m_processor.padToPowerOfTwo();

        if (isPixelated) {
            m_processor.applyPixelate(8);
        }

        m_processor.bitReverse();

        if (isThreshold)
        {
            m_processor.applyThreshold(127);
        }

        // 3. Если нужно, делаем перестановку
        if (isBitReverse)
        {
            m_processor.bitReverse();
        }

        // 4. Формируем результат
        // Если включен BitReverse - мы обязаны показать ВСЁ изображение (квадратное),
        // так как пиксели разбросаны по всей площади.
        // Если выключен - мы хотим видеть только оригинальную часть без черных полей.
        int finalW = !isBitReverse ? m_processor.getWidth() : origW;
        int finalH = !isBitReverse ? m_processor.getHeight() : origH;

        // ВАЖНЫЙ МОМЕНТ:
        // bytesPerLine (шаг строки) ВСЕГДА должен быть равен ширине БОЛЬШОГО (дополненного) буфера.
        // Даже если мы показываем только кусочек 640 пикселей, в памяти строка занимает 1024 пикселя.
        int bytesPerLine = m_processor.getWidth() * channels;

        QImage result(m_processor.getData(),
                      finalW,
                      finalH,
                      bytesPerLine,
                      QImage::Format_RGB32);

        emit frameProcessed(result.copy());
    }

signals:
    void frameProcessed(QImage img);

private:
    Image m_processor;
};

#endif
