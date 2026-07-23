#include "../h/Image.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

// Подключаем реализации библиотек STB
#define STB_IMAGE_IMPLEMENTATION
#include "../h/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../h/stb_image_write.h"

Image::Image() {}

Image::~Image() {}

bool Image::load(const std::string& filename) {
    int w, h, c;
    unsigned char* data = stbi_load(filename.c_str(), &w, &h, &c, 0);

    if (data == nullptr) {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return false;
    }

    loadFromMemory(w, h, c, data);
    stbi_image_free(data);
    return true;
}

bool Image::save(const std::string& filename) {
    int stride = m_width * m_channels;
    int result = stbi_write_png(filename.c_str(), m_width, m_height, m_channels, m_data.data(), stride);
    return result != 0;
}

void Image::applyGrayscale() {
    if (m_channels < 3) return;

    // Переиспользуем scratchBuffer, если он подходит по размеру
    size_t newSize = m_width * m_height;
    if (m_scratchBuffer.size() < newSize) {
        m_scratchBuffer.resize(newSize);
    }

    unsigned char* dest = m_scratchBuffer.data();
    const unsigned char* src = m_data.data();
    int channels = m_channels;
    size_t totalPixels = m_width * m_height;

// Распараллеливаем конвертацию
#pragma omp parallel for
    for (int i = 0; i < (int)totalPixels; ++i) {
        int idx = i * channels;
        unsigned char r = src[idx];
        unsigned char g = src[idx+1];
        unsigned char b = src[idx+2];
        // Быстрая целочисленная аппроксимация: (R*77 + G*150 + B*29) >> 8
        dest[i] = (unsigned char)((r * 77 + g * 150 + b * 29) >> 8);
    }

    m_data = m_scratchBuffer;
    m_data.resize(newSize);
    m_channels = 1;
}

void Image::loadFromMemory(int w, int h, int c, const unsigned char *data)
{
    m_width = w;
    m_height = h;
    m_channels = c;

    size_t size = (size_t)w * h * c;

    // Избегаем переаллокации, если вектор уже достаточного размера
    if (m_data.size() < size) {
        m_data.resize(size);
    }

    std::memcpy(m_data.data(), data, size);
}

void Image::updateLuts()
{
    // Если размеры не изменились, не пересчитываем
    if (m_width == m_cachedWidth && m_height == m_cachedHeight) return;

    int bitsW = static_cast<int>(std::ceil(std::log2(m_width)));
    int bitsH = static_cast<int>(std::ceil(std::log2(m_height)));

    m_lutX.resize(m_width);
#pragma omp parallel for
    for (int x = 0; x < m_width; ++x) {
        m_lutX[x] = bitRev(x, bitsW);
    }

    m_lutY.resize(m_height);
#pragma omp parallel for
    for (int y = 0; y < m_height; ++y) {
        m_lutY[y] = bitRev(y, bitsH);
    }

    m_cachedWidth = m_width;
    m_cachedHeight = m_height;
}

void Image::bitReverse()
{
    if (m_data.empty()) return;

    updateLuts(); // Обновляем таблицы только если размер изменился

    // Подготавливаем буфер назначения
    if (m_scratchBuffer.size() < m_data.size()) {
        m_scratchBuffer.resize(m_data.size());
    }

    const unsigned char* src = m_data.data();
    unsigned char* dst = m_scratchBuffer.data();

    const int w = m_width;
    const int h = m_height;
    const int channels = m_channels;
    const int* lutX = m_lutX.data();
    const int* lutY = m_lutY.data();

// Основной цикл оптимизирован:
// 1. OpenMP для использования всех ядер
// 2. Используем предвычисленные LUT
// 3. Убраны лишние вычисления индексов

#pragma omp parallel for
    for (int y = 0; y < h; ++y) {
        int revY = lutY[y];
        // Предвычисляем смещения строк
        size_t srcRowOffset = (size_t)y * w * channels;
        size_t dstRowOffset = (size_t)revY * w * channels;

        for (int x = 0; x < w; ++x) {
            int revX = lutX[x];

            size_t srcIdx = srcRowOffset + x * channels;
            size_t dstIdx = dstRowOffset + revX * channels;

            // Копирование пикселя (3 или 4 байта)
            if (channels == 4) {
                // Для 32-битного цвета (RGB32/RGBA) компилятор оптимизирует это в одну инструкцию
                *(uint32_t*)(dst + dstIdx) = *(const uint32_t*)(src + srcIdx);
            } else {
                for (int c = 0; c < channels; ++c) {
                    dst[dstIdx + c] = src[srcIdx + c];
                }
            }
        }
    }

    // Быстрый обмен данными между векторами (без копирования)
    std::swap(m_data, m_scratchBuffer);
}

void Image::applyThreshold(unsigned char limit) {
    unsigned char* data = m_data.data();
    int totalElements = m_width * m_height * m_channels;

#pragma omp parallel for
    for (int i = 0; i < totalElements; i += m_channels) {
        // Вычисляем среднюю яркость пикселя
        unsigned char gray = (data[i] + data[i+1] + data[i+2]) / 3;

        unsigned char result = (gray > limit) ? 255 : 0;

        data[i]     = result; // R
        data[i+1]   = result; // G
        data[i+2]   = result; // B
        // Alpha-канал (i+3) не трогаем
    }
}

void Image::applyPixelate(int pixelSize)
{
    if (pixelSize <= 1) return;

    unsigned char* data = m_data.data();
    int w = m_width;
    int h = m_height;
    int c = m_channels;

// Идем по сетке блоков
#pragma omp parallel for
    for (int y = 0; y < h; y += pixelSize) {
        for (int x = 0; x < w; x += pixelSize) {

            // 1. Определяем индекс "эталонного" пикселя для этого блока (левый верхний угол блока)
            int mainIdx = (y * w + x) * c;

            // Сохраняем цвета этого пикселя
            unsigned char r = data[mainIdx];
            unsigned char g = data[mainIdx + 1];
            unsigned char b = data[mainIdx + 2];
            unsigned char a = (c == 4) ? data[mainIdx + 3] : 255;

            // 2. Теперь закрашиваем ВЕСЬ блок pixelSize x pixelSize этим цветом
            // Важно: ограничиваем циклы границами изображения (py < h и px < w)
            for (int py = y; py < y + pixelSize && py < h; ++py) {
                for (int px = x; px < x + pixelSize && px < w; ++px) {
                    int currentIdx = (py * w + px) * c;

                    data[currentIdx] = r;
                    data[currentIdx + 1] = g;
                    data[currentIdx + 2] = b;
                    if (c == 4) {
                        data[currentIdx + 3] = a;
                    }
                }
            }
        }
    }
}

void Image::applySobel()
{
    if (m_data.empty()) return;

    // 1. Создаем временный буфер для результата
    std::vector<unsigned char> resultData(m_data.size(), 0);

    int w = m_width;
    int h = m_height;
    int c = m_channels;

    // Матрицы Собеля (ядра)
    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

// 2. Проходим по всем пикселям, кроме самых крайних (чтобы не выйти за границы)
#pragma omp parallel for
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            float sumX = 0;
            float sumY = 0;

            // Применяем ядро к каналам R, G, B (усредняем их для поиска границ)
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int pixelIdx = ((y + ky) * w + (x + kx)) * c;

                    // Вычисляем яркость соседа (простое среднее)
                    float brightness = (m_data[pixelIdx] + m_data[pixelIdx + 1] + m_data[pixelIdx + 2]) / 3.0f;

                    sumX += brightness * Gx[ky + 1][kx + 1];
                    sumY += brightness * Gy[ky + 1][kx + 1];
                }
            }

            // Итоговая величина градиента
            int magnitude = static_cast<int>(std::sqrt(sumX * sumX + sumY * sumY));
            if (magnitude > 255) magnitude = 255; // Ограничиваем сверху

            int outIdx = (y * w + x) * c;
            resultData[outIdx]     = (unsigned char)magnitude; // R
            resultData[outIdx + 1] = (unsigned char)magnitude; // G
            resultData[outIdx + 2] = (unsigned char)magnitude; // B
            if (c == 4) resultData[outIdx + 3] = 255;         // Alpha
        }
    }

    m_data = std::move(resultData);
}

int Image::bitRev(int num, int bits)
{
    int reversedNum = 0;
    for (int i = 0; i < bits; ++i) {
        if ((num >> i) & 1) {
            reversedNum |= (1 << (bits - 1 - i));
        }
    }
    return reversedNum;
}

const unsigned char *Image::getData() const
{
    return m_data.data();
}

int Image::getWidth() const
{
    return m_width;
}

int Image::getHeight() const
{
    return m_height;
}

void Image::padToPowerOfTwo()
{
    int newWidth = nextPowerOfTwo(m_width);
    int newHeight = nextPowerOfTwo(m_height);

    if (newWidth == m_width && newHeight == m_height) return;

    size_t newSize = (size_t)newWidth * newHeight * m_channels;

    // Подготовка буфера назначения
    if (m_scratchBuffer.size() < newSize) {
        m_scratchBuffer.resize(newSize);
    }

    // Обнуляем буфер (черный фон) - memset очень быстр
    std::memset(m_scratchBuffer.data(), 0, newSize);

    unsigned char* dst = m_scratchBuffer.data();
    const unsigned char* src = m_data.data();
    size_t rowSize = (size_t)m_width * m_channels;

// Копируем построчно (memcpy сильно быстрее цикла по пикселям)
#pragma omp parallel for
    for (int y = 0; y < m_height; ++y) {
        const unsigned char* srcRow = src + (size_t)y * rowSize;
        unsigned char* dstRow = dst + (size_t)y * newWidth * m_channels;
        std::memcpy(dstRow, srcRow, rowSize);
    }

    m_width = newWidth;
    m_height = newHeight;
    std::swap(m_data, m_scratchBuffer);

    // Сбрасываем кэш LUT, так как размер изменился
    m_cachedWidth = -1;
}

int Image::nextPowerOfTwo(int n)
{
    if (n <= 0) return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return ++n;
}
