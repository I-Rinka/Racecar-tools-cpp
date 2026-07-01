#include "DigitRecognizer.h"
#include <algorithm>
#include <numeric>

cv::Mat DigitTemplateCache::normalizeDigit(const cv::Mat &img) {
    if (img.empty()) return {};
    cv::Mat gray;
    if (img.channels() == 3)
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else
        gray = img.clone();

    cv::Mat resized;
    cv::resize(gray, resized, cv::Size(DIGIT_W, DIGIT_H), 0, 0, cv::INTER_AREA);
    cv::Mat binary;
    cv::threshold(resized, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    return binary;
}

void DigitTemplateCache::addTemplate(int digit, const cv::Mat &img) {
    auto norm = normalizeDigit(img);
    if (norm.empty()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto &slot = m_slots[digit];
    if (static_cast<int>(slot.size()) >= MAX_SAMPLES)
        slot.erase(slot.begin());
    slot.push_back(norm);
    m_meanCache[digit].reset();
}

bool DigitTemplateCache::hasTemplates() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &s : m_slots)
        if (!s.empty()) return true;
    return false;
}

cv::Mat DigitTemplateCache::getMeanTemplate(int digit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_meanCache[digit].has_value())
        return m_meanCache[digit].value();
    auto &slot = m_slots[digit];
    if (slot.empty()) return {};
    cv::Mat acc = cv::Mat::zeros(DIGIT_H, DIGIT_W, CV_64F);
    for (auto &s : slot) {
        cv::Mat tmp;
        s.convertTo(tmp, CV_64F);
        acc += tmp;
    }
    acc /= static_cast<double>(slot.size());
    cv::Mat mean;
    acc.convertTo(mean, CV_8U);
    m_meanCache[digit] = mean;
    return mean;
}

std::pair<int, double> DigitTemplateCache::matchDigit(const cv::Mat &img) const {
    auto norm = normalizeDigit(img);
    if (norm.empty()) return {-1, 0.0};
    int bestDigit = -1;
    double bestScore = -1.0;
    for (int d = 0; d < 10; ++d) {
        auto tmpl = getMeanTemplate(d);
        if (tmpl.empty()) continue;
        cv::Mat result;
        cv::matchTemplate(norm, tmpl, result, cv::TM_CCOEFF_NORMED);
        double score = result.at<float>(0, 0);
        if (score > bestScore) {
            bestScore = score;
            bestDigit = d;
        }
    }
    return {bestDigit, bestScore};
}

std::vector<cv::Mat> segmentDigits(const cv::Mat &roi) {
    if (roi.empty()) return {};
    cv::Mat gray;
    if (roi.channels() == 3)
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    else
        gray = roi.clone();

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    if (cv::mean(binary)[0] > 127)
        cv::bitwise_not(binary, binary);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int minH = static_cast<int>(roi.rows * 0.35);
    struct Box { int x, y, w, h; };
    std::vector<Box> boxes;
    for (auto &c : contours) {
        auto r = cv::boundingRect(c);
        if (r.height >= minH && r.width >= 3)
            boxes.push_back({r.x, r.y, r.width, r.height});
    }
    std::sort(boxes.begin(), boxes.end(), [](auto &a, auto &b) { return a.x < b.x; });

    std::vector<cv::Mat> digits;
    for (auto &b : boxes)
        digits.push_back(gray(cv::Rect(b.x, b.y, b.w, b.h)).clone());
    return digits;
}

void learnFromOCRResult(DigitTemplateCache &cache, const cv::Mat &roi, int number) {
    if (number <= 0) return;
    auto str = std::to_string(number);
    auto imgs = segmentDigits(roi);
    if (imgs.size() != str.size()) return;
    for (size_t i = 0; i < str.size(); ++i)
        cache.addTemplate(str[i] - '0', imgs[i]);
}

std::pair<double, double> recognizeByTemplate(const DigitTemplateCache &cache, const cv::Mat &roi) {
    if (!cache.hasTemplates()) return {0.0, 0.0};
    auto imgs = segmentDigits(roi);
    if (imgs.empty()) return {0.0, 0.0};

    std::string result;
    double totalScore = 0.0;
    for (auto &img : imgs) {
        auto [d, score] = cache.matchDigit(img);
        if (d < 0) return {0.0, 0.0};
        result += std::to_string(d);
        totalScore += score;
    }
    double number = std::stod(result);
    return {number, totalScore / imgs.size()};
}
