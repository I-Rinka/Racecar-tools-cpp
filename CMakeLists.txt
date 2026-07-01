cmake_minimum_required(VERSION 3.20)
project(RacecarTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets PrintSupport)
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio)

# Tesseract (optional — disable OCR if not found)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(TESSERACT tesseract)
    pkg_check_modules(LEPTONICA lept)
endif()

# QCustomPlot (bundled)
add_library(qcustomplot STATIC
    third_party/qcustomplot/qcustomplot.cpp
)
target_include_directories(qcustomplot PUBLIC third_party/qcustomplot)
target_link_libraries(qcustomplot PUBLIC Qt6::Widgets Qt6::PrintSupport)
target_compile_definitions(qcustomplot PUBLIC QCUSTOMPLOT_USE_LIBRARY)

set(SOURCES
    src/main.cpp
    src/core/SDAnalyzer.cpp
    src/core/VideoWrapper.cpp
    src/core/DigitRecognizer.cpp
    src/core/VideoProcessor.cpp
    src/widgets/MainWindow.cpp
    src/widgets/PlotWidget.cpp
    src/widgets/VideoPlayer.cpp
    src/widgets/PlotPage.cpp
    src/widgets/ROISelector.cpp
    src/widgets/DataEditor.cpp
)

add_executable(RacecarTools ${SOURCES})

target_include_directories(RacecarTools PRIVATE src)
target_link_libraries(RacecarTools PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    qcustomplot
    ${OpenCV_LIBS}
)

if(TESSERACT_FOUND)
    target_include_directories(RacecarTools PRIVATE ${TESSERACT_INCLUDE_DIRS} ${LEPTONICA_INCLUDE_DIRS})
    target_link_directories(RacecarTools PRIVATE ${TESSERACT_LIBRARY_DIRS} ${LEPTONICA_LIBRARY_DIRS})
    target_link_libraries(RacecarTools PRIVATE ${TESSERACT_LIBRARIES} ${LEPTONICA_LIBRARIES})
    target_compile_definitions(RacecarTools PRIVATE HAS_TESSERACT)
endif()
