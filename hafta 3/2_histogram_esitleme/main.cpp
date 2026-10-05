#include <opencv2/imgcodecs.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    const std::string giris = argc > 1 ? argv[1] : "fotograflar/dusuk_kontrast.png";
    const std::string cikis = argc > 2 ? argv[2] : "sonuclar/2_histogram_esitleme.png";
    const std::string tablo = argc > 3 ? argv[3] : "sonuclar/2_histogram_cdf.csv";

    cv::Mat resim = cv::imread(giris, cv::IMREAD_GRAYSCALE);
    if (resim.empty())
    {
        std::cerr << "Resim acilamadi: " << giris << '\n';
        return 1;
    }

    std::array<long long, 256> histogram{};
    std::array<long long, 256> cdf{};
    std::array<unsigned char, 256> donusum{};

    for (int y = 0; y < resim.rows; ++y)
    {
        const unsigned char* satir = resim.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
            ++histogram[satir[x]];
    }

    cdf[0] = histogram[0];
    for (int i = 1; i < 256; ++i)
        cdf[i] = cdf[i - 1] + histogram[i];

    long long cdfMin = 0;
    for (int i = 0; i < 256; ++i)
    {
        if (cdf[i] != 0)
        {
            cdfMin = cdf[i];
            break;
        }
    }

    const long long pikselSayisi = static_cast<long long>(resim.rows) * resim.cols;
    const long long payda = pikselSayisi - cdfMin;

    for (int i = 0; i < 256; ++i)
    {
        if (cdf[i] == 0 || payda == 0)
        {
            donusum[i] = 0;
        }
        else
        {
            const double deger = static_cast<double>(cdf[i] - cdfMin) * 255.0 / payda;
            int yuvarlanmis = static_cast<int>(std::round(deger));
            if (yuvarlanmis < 0)
                yuvarlanmis = 0;
            if (yuvarlanmis > 255)
                yuvarlanmis = 255;
            donusum[i] = static_cast<unsigned char>(yuvarlanmis);
        }
    }

    cv::Mat sonuc(resim.size(), CV_8UC1);
    for (int y = 0; y < resim.rows; ++y)
    {
        const unsigned char* girisSatiri = resim.ptr<unsigned char>(y);
        unsigned char* cikisSatiri = sonuc.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
            cikisSatiri[x] = donusum[girisSatiri[x]];
    }

    std::filesystem::create_directories(std::filesystem::path(cikis).parent_path());
    if (!cv::imwrite(cikis, sonuc))
    {
        std::cerr << "Cikti kaydedilemedi.\n";
        return 2;
    }

    std::ofstream dosya(tablo);
    if (!dosya)
    {
        std::cerr << "Histogram tablosu kaydedilemedi.\n";
        return 3;
    }

    dosya << "deger,histogram,cdf,esitlenmis_deger\n";
    for (int i = 0; i < 256; ++i)
        dosya << i << ',' << histogram[i] << ',' << cdf[i] << ',' << static_cast<int>(donusum[i]) << '\n';

    std::cout << "Piksel sayisi: " << pikselSayisi << '\n';
    std::cout << "Kaydedildi: " << cikis << '\n';
    std::cout << "Tablo: " << tablo << '\n';
    return 0;
}

