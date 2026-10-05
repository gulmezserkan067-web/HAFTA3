#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <string>

cv::Mat medyanFiltreElle(const cv::Mat& resim)
{
    cv::Mat sonuc(resim.size(), CV_8UC1);

    for (int y = 0; y < resim.rows; ++y)
    {
        unsigned char* cikisSatiri = sonuc.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
        {
            std::array<unsigned char, 25> degerler{};
            int sira = 0;

            for (int ky = -2; ky <= 2; ++ky)
            {
                const int kaynakY = std::max(0, std::min(y + ky, resim.rows - 1));
                const unsigned char* girisSatiri = resim.ptr<unsigned char>(kaynakY);
                for (int kx = -2; kx <= 2; ++kx)
                {
                    const int kaynakX = std::max(0, std::min(x + kx, resim.cols - 1));
                    degerler[sira++] = girisSatiri[kaynakX];
                }
            }

            std::sort(degerler.begin(), degerler.end());
            cikisSatiri[x] = degerler[12];
        }
    }

    return sonuc;
}

int main(int argc, char** argv)
{
    const std::string giris = argc > 1 ? argv[1] : "fotograflar/salt_pepper.png";
    const std::string hazirCikis = argc > 2 ? argv[2] : "sonuclar/5_medyan_opencv.png";
    const std::string elleCikis = argc > 3 ? argv[3] : "sonuclar/5_medyan_elle.png";

    cv::Mat resim = cv::imread(giris, cv::IMREAD_GRAYSCALE);
    if (resim.empty())
    {
        std::cerr << "Resim acilamadi: " << giris << '\n';
        return 1;
    }

    cv::Mat hazirSonuc;
    cv::medianBlur(resim, hazirSonuc, 5);
    cv::Mat elleSonuc = medyanFiltreElle(resim);

    long long farkliPiksel = 0;
    for (int y = 0; y < resim.rows; ++y)
    {
        const unsigned char* birinci = hazirSonuc.ptr<unsigned char>(y);
        const unsigned char* ikinci = elleSonuc.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
        {
            if (birinci[x] != ikinci[x])
                ++farkliPiksel;
        }
    }

    std::filesystem::create_directories(std::filesystem::path(hazirCikis).parent_path());
    if (!cv::imwrite(hazirCikis, hazirSonuc) || !cv::imwrite(elleCikis, elleSonuc))
    {
        std::cerr << "Ciktilar kaydedilemedi.\n";
        return 2;
    }

    std::cout << "OpenCV sonucu: " << hazirCikis << '\n';
    std::cout << "Elle yazilan sonuc: " << elleCikis << '\n';
    std::cout << "Farkli piksel sayisi: " << farkliPiksel << '\n';
    return 0;
}

