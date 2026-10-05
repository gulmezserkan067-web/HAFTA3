#include <opencv2/imgcodecs.hpp>

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    const std::string giris = argc > 1 ? argv[1] : "fotograflar/dusuk_kontrast.png";
    const std::string cikis = argc > 2 ? argv[2] : "sonuclar/1_dogrusal_olcekleme.png";

    cv::Mat resim = cv::imread(giris, cv::IMREAD_GRAYSCALE);
    if (resim.empty())
    {
        std::cerr << "Resim acilamadi: " << giris << '\n';
        return 1;
    }

    int enKucuk = 255;
    int enBuyuk = 0;

    for (int y = 0; y < resim.rows; ++y)
    {
        const unsigned char* satir = resim.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
        {
            const int deger = satir[x];
            if (deger < enKucuk)
                enKucuk = deger;
            if (deger > enBuyuk)
                enBuyuk = deger;
        }
    }

    cv::Mat sonuc(resim.size(), CV_8UC1);

    if (enBuyuk == enKucuk)
    {
        sonuc.setTo(0);
    }
    else
    {
        for (int y = 0; y < resim.rows; ++y)
        {
            const unsigned char* girisSatiri = resim.ptr<unsigned char>(y);
            unsigned char* cikisSatiri = sonuc.ptr<unsigned char>(y);
            for (int x = 0; x < resim.cols; ++x)
            {
                const int yeniDeger = (girisSatiri[x] - enKucuk) * 255 / (enBuyuk - enKucuk);
                cikisSatiri[x] = static_cast<unsigned char>(yeniDeger);
            }
        }
    }

    std::filesystem::create_directories(std::filesystem::path(cikis).parent_path());
    if (!cv::imwrite(cikis, sonuc))
    {
        std::cerr << "Cikti kaydedilemedi.\n";
        return 2;
    }

    std::cout << "Minimum deger: " << enKucuk << '\n';
    std::cout << "Maksimum deger: " << enBuyuk << '\n';
    std::cout << "Kaydedildi: " << cikis << '\n';
    return 0;
}

