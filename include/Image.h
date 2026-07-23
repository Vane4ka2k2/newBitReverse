#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <vector>

class Image {
public:
    Image();
    ~Image();

    // Загрузка изображения из файла
    bool load(const std::string& filename);
    // Сохранение изображения в файл
    bool save(const std::string& filename);

    // Загрузка изображения из памяти
    void loadFromMemory(int w, int h, int c, const unsigned char* data);

    // Двоично-реверсная перестановка
    void bitReverse();

    // Перевод изображения в оттенки серого
    void applyGrayscale();

    // Применение порогового фильтра
    void applyThreshold(unsigned char limit);

    // Пикселизация изображения
    void applyPixelate(int pixelSize);

    // Выделение границ объектов
    void applySobel();

    const unsigned char* getData() const;
    int getWidth() const;
    int getHeight() const;

    // Преведение ширины и высоты к степеням двойки
    void padToPowerOfTwo();

private:
    int bitRev(int num, int bits);
    int nextPowerOfTwo(int n);

    // Обновление таблиц поиска (LUT) при изменении размера
    void updateLuts();

    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;

    // Основные данные
    std::vector<unsigned char> m_data;

    // Временный буфер для алгоритмов (чтобы не выделять память каждый кадр)
    std::vector<unsigned char> m_scratchBuffer;

    // Таблицы предвычисленных индексов (Look-Up Tables)
    std::vector<int> m_lutX;
    std::vector<int> m_lutY;

    // Кэшированные размеры для проверки изменений
    int m_cachedWidth = -1;
    int m_cachedHeight = -1;
};

#endif // IMAGE_H
