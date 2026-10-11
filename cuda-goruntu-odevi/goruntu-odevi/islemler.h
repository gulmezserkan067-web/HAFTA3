#pragma once
#include <opencv2/core.hpp>
#include <array>
#include <string>

struct Sonuc {
    cv::Mat germe, esit, clahe, ortalama, medyan;
    std::array<int, 256> hist{}, cdf{};
    int alt = 255, ust = 0;
};

bool cudaVar(std::string& ad);
Sonuc gpuIsle(const cv::Mat& resim, const cv::Mat& gurultu, const cv::Mat& tuz,
    float limit = 2.0f, int nx = 8, int ny = 8);
