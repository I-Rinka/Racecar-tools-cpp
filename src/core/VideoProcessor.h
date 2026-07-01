#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <functional>
#include "DigitRecognizer.h"
#include "SDAnalyzer.h"

#ifdef HAS_TESSERACT
#include <tesseract/baseapi.h>
#endif

class VideoProcessor {
public:
    explicit VideoProcessor(double frameRate, int maxWorkers = 4);
    ~VideoProcessor();

    double processFrame(const cv::Mat &frame, int frameIndex);
    std::vector<std::tuple<int, double, double>> processBatch(
        const std::vector<std::pair<cv::Mat, int>> &frames);

    SpeedData getSpeedData() const;
    void saveCSV(const std::string &path);
    void restart();

private:
    double smartOCR(const cv::Mat &frame);
    double tesseractOCR(const cv::Mat &frame);
    bool sanityCheck(double value) const;

    double m_timeInterval;
    double m_lastSpeed = -1.0;
    std::vector<std::tuple<double, double, int>> m_timeSpeed; // time, speed, frame
    DigitTemplateCache m_digitCache;
    int m_maxWorkers;

#ifdef HAS_TESSERACT
    tesseract::TessBaseAPI m_tess;
#endif
};
