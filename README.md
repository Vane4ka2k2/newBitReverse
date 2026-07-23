<div align="center">

# 📷 SobelCam-Qt — Захват и видеообработка с веб-камеры на Qt 6

**Десктопное графическое приложение на Qt 6 для захвата видеопотока с веб-камеры и многопоточной обработки кадров в реальном времени (Оператор Собеля, пикселизация, бинарный реверс) с использованием QThread и OpenMP.**

![Qt 6](https://img.shields.io/badge/Qt-6.2%2B-41CD52?logo=qt)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![OpenMP](https://img.shields.io/badge/Parallel-OpenMP-FF6F00)
![License](https://img.shields.io/badge/License-MIT-green)

</div>

---

## 📑 Оглавление
- [🌟 Основные возможности](#-основные-возможности)
- [🏗 Архитектура приложения](#-архитектура-приложения)
- [🛠 Требования и Сборка](#-требования-и-сборка)
- [📁 Структура проекта](#-структура-проекта)
- [📜 Лицензия](#-лицензия)

---

## 🌟 Основные возможности

- 📹 **Захват видео в реальном времени** — Интеграция с веб-камерами через модуль Qt Multimedia.
- ⚡ **Многопоточная фильтрация** — Вынесение трудоемких алгоритмов видеообработки в отдельный поток ProcessorWorker (QThread).
- 🧮 **Параллельные вычисления (OpenMP)** — Оптимизированные алгоритмы сверстки изображения на процессоре.
- 🎨 **Графический фильтр Собеля** — Выделение границ объектов на изображении на лету.

---

## 🏗 Архитектура приложения

`mermaid
flowchart LR
    A[QCamera / QMediaCaptureSession] -->|Видеокадры| B[ProcessorWorker Thread]
    B -->|OpenMP Алгоритмы Собеля| C[QImage Result]
    C -->|Сигнал frameProcessed| D[MainWindow UI]
`

---

## 🛠 Требования и Сборка

### Требования:
- C++17 компилятор
- Qt 6.2+ (модули Core, Gui, Widgets, Multimedia)
- CMake 3.16+
- Поддержка OpenMP (опционально)

### Сборка через CMake:
`ash
git clone https://github.com/Vane4ka2k2/SobelCam-Qt.git
cd SobelCam-Qt
cmake -B build
cmake --build build
`

---

## 📁 Структура проекта

`	ext
SobelCam-Qt/
├── cpp/               # Исходные файлы алгоритмов
├── h/                 # Заголовочные файлы и stb_image
├── MainWindow.cpp     # Главное окно Qt
├── ProcessorWorker.h  # Рабочий поток обработки кадров
├── CMakeLists.txt     # Скрипт сборки CMake
├── LICENSE            # Лицензия MIT
└── README.md          # Документация
`

---

## 📜 Лицензия

Распространяется под лицензией **MIT**. Подробнее см. в файле [LICENSE](LICENSE).