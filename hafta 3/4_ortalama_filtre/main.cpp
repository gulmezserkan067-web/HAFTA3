#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

cv::Mat ortalamaFiltre(const cv::Mat& resim)
{
    cv::Mat sonuc(resim.size(), CV_8UC1);

    for (int y = 0; y < resim.rows; ++y)
    {
        unsigned char* cikisSatiri = sonuc.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
        {
            int toplam = 0;
            for (int ky = -1; ky <= 1; ++ky)
            {
                const int kaynakY = std::max(0, std::min(y + ky, resim.rows - 1));
                const unsigned char* girisSatiri = resim.ptr<unsigned char>(kaynakY);
                for (int kx = -1; kx <= 1; ++kx)
                {
                    const int kaynakX = std::max(0, std::min(x + kx, resim.cols - 1));
                    toplam += girisSatiri[kaynakX];
                }
            }
            cikisSatiri[x] = static_cast<unsigned char>((toplam + 4) / 9);
        }
    }

    return sonuc;
}

int main(int argc, char** argv)
{
    const std::string giris = argc > 1 ? argv[1] : "fotograflar/gurultulu.png";
    const std::string cikis = argc > 2 ? argv[2] : "sonuclar/4_ortalama_filtre.png";

    cv::Mat resim = cv::imread(giris, cv::IMREAD_GRAYSCALE);
    if (resim.empty())
    {
        std::cerr << "Resim acilamadi: " << giris << '\n';
        return 1;
    }

    cv::Mat sonuc = ortalamaFiltre(resim);
    std::filesystem::create_directories(std::filesystem::path(cikis).parent_path());

    if (!cv::imwrite(cikis, sonuc))
    {
        std::cerr << "Cikti kaydedilemedi.\n";
        return 2;
    }

    std::cout << "3x3 ortalama filtre sonucu: " << cikis << '\n';
    return 0;
}

