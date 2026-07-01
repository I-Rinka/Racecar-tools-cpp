#pragma once
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <array>
#include <mutex>
#include <optional>

constexpr int DIGIT_W = 32;
constexpr int DIGIT_H = 48;
constexpr int MAX_SAMPLES = 20;

class DigitTemplateCache {
public:
    void addTemplate(int digit, const cv::Mat &img);
    bool hasTemplates() const;
    std::pair<int, double> matchDigit(const cv::Mat &img) const;

private:
    cv::Mat getMeanTemplate(int digit) const;
    static cv::Mat normalizeDigit(const cv::Mat &img);

    mutable std::mutex m_mutex;
    std::array<std::vector<cv::Mat>, 10> m_slots;
    mutable std::array<std::optional<cv::Mat>, 10> m_meanCache;
};

std::vector<cv::Mat> segmentDigits(const cv::Mat &roi);
void learnFromOCRResult(DigitTemplateCache &cache, const cv::Mat &roi, int number);
std::pair<double, double> recognizeByTemplate(const DigitTemplateCache &cache, const cv::Mat &roi);
