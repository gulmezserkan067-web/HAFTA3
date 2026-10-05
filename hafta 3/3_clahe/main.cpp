#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int yansit(int konum, int uzunluk)
{
    if (uzunluk <= 1)
        return 0;

    while (konum < 0 || konum >= uzunluk)
    {
        if (konum < 0)
            konum = -konum;
        else
            konum = 2 * uzunluk - konum - 2;
    }
    return konum;
}

cv::Mat claheElle(const cv::Mat& resim, double sinir, int yatayBolme, int dikeyBolme)
{
    const int parcaGenisligi = (resim.cols + yatayBolme - 1) / yatayBolme;
    const int parcaYuksekligi = (resim.rows + dikeyBolme - 1) / dikeyBolme;
    const int parcaAlani = parcaGenisligi * parcaYuksekligi;
    const int kirpmaSiniri = std::max(1, static_cast<int>(sinir * parcaAlani / 256.0));

    std::vector<std::array<unsigned char, 256>> tablolar(yatayBolme * dikeyBolme);

    for (int py = 0; py < dikeyBolme; ++py)
    {
        for (int px = 0; px < yatayBolme; ++px)
        {
            std::array<int, 256> histogram{};

            for (int y = 0; y < parcaYuksekligi; ++y)
            {
                const int kaynakY = yansit(py * parcaYuksekligi + y, resim.rows);
                const unsigned char* satir = resim.ptr<unsigned char>(kaynakY);
                for (int x = 0; x < parcaGenisligi; ++x)
                {
                    const int kaynakX = yansit(px * parcaGenisligi + x, resim.cols);
                    ++histogram[satir[kaynakX]];
                }
            }

            int tasan = 0;
            for (int i = 0; i < 256; ++i)
            {
                if (histogram[i] > kirpmaSiniri)
                {
                    tasan += histogram[i] - kirpmaSiniri;
                    histogram[i] = kirpmaSiniri;
                }
            }

            const int herkese = tasan / 256;
            int kalan = tasan % 256;
            for (int i = 0; i < 256; ++i)
                histogram[i] += herkese;

            if (kalan > 0)
            {
                const int adim = std::max(256 / kalan, 1);
                for (int i = 0; i < 256 && kalan > 0; i += adim, --kalan)
                    ++histogram[i];
            }

            int toplam = 0;
            std::array<unsigned char, 256>& tablo = tablolar[py * yatayBolme + px];
            for (int i = 0; i < 256; ++i)
            {
                toplam += histogram[i];
                int deger = static_cast<int>(std::round(toplam * 255.0 / parcaAlani));
                if (deger < 0)
                    deger = 0;
                if (deger > 255)
                    deger = 255;
                tablo[i] = static_cast<unsigned char>(deger);
            }
        }
    }

    cv::Mat sonuc(resim.size(), CV_8UC1);

    for (int y = 0; y < resim.rows; ++y)
    {
        const double parcaY = static_cast<double>(y) / parcaYuksekligi - 0.5;
        int ust = static_cast<int>(std::floor(parcaY));
        int alt = ust + 1;
        const double dikeyOran = parcaY - ust;
        ust = std::max(0, std::min(ust, dikeyBolme - 1));
        alt = std::max(0, std::min(alt, dikeyBolme - 1));

        const unsigned char* girisSatiri = resim.ptr<unsigned char>(y);
        unsigned char* cikisSatiri = sonuc.ptr<unsigned char>(y);

        for (int x = 0; x < resim.cols; ++x)
        {
            const double parcaX = static_cast<double>(x) / parcaGenisligi - 0.5;
            int sol = static_cast<int>(std::floor(parcaX));
            int sag = sol + 1;
            const double yatayOran = parcaX - sol;
            sol = std::max(0, std::min(sol, yatayBolme - 1));
            sag = std::max(0, std::min(sag, yatayBolme - 1));

            const int piksel = girisSatiri[x];
            const double ustDeger = (1.0 - yatayOran) * tablolar[ust * yatayBolme + sol][piksel]
                                  + yatayOran * tablolar[ust * yatayBolme + sag][piksel];
            const double altDeger = (1.0 - yatayOran) * tablolar[alt * yatayBolme + sol][piksel]
                                  + yatayOran * tablolar[alt * yatayBolme + sag][piksel];
            int deger = static_cast<int>(std::round((1.0 - dikeyOran) * ustDeger + dikeyOran * altDeger));
            deger = std::max(0, std::min(deger, 255));
            cikisSatiri[x] = static_cast<unsigned char>(deger);
        }
    }

    return sonuc;
}

int main(int argc, char** argv)
{
    const std::string giris = argc > 1 ? argv[1] : "fotograflar/dusuk_kontrast.png";
    const std::string hazirCikis = argc > 2 ? argv[2] : "sonuclar/3_clahe_opencv.png";
    const std::string elleCikis = argc > 3 ? argv[3] : "sonuclar/3_clahe_elle.png";

    cv::Mat resim = cv::imread(giris, cv::IMREAD_GRAYSCALE);
    if (resim.empty())
    {
        std::cerr << "Resim acilamadi: " << giris << '\n';
        return 1;
    }

    const double sinir = 2.0;
    const cv::Size bolme(8, 8);

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(sinir, bolme);
    cv::Mat hazirSonuc;
    clahe->apply(resim, hazirSonuc);

    cv::Mat elleSonuc = claheElle(resim, sinir, bolme.width, bolme.height);

    double toplamFark = 0.0;
    for (int y = 0; y < resim.rows; ++y)
    {
        const unsigned char* birinci = hazirSonuc.ptr<unsigned char>(y);
        const unsigned char* ikinci = elleSonuc.ptr<unsigned char>(y);
        for (int x = 0; x < resim.cols; ++x)
            toplamFark += std::abs(static_cast<int>(birinci[x]) - static_cast<int>(ikinci[x]));
    }

    std::filesystem::create_directories(std::filesystem::path(hazirCikis).parent_path());
    if (!cv::imwrite(hazirCikis, hazirSonuc) || !cv::imwrite(elleCikis, elleSonuc))
    {
        std::cerr << "Ciktilar kaydedilemedi.\n";
        return 2;
    }

    const double ortalamaFark = toplamFark / (resim.rows * resim.cols);
    std::cout << "OpenCV sonucu: " << hazirCikis << '\n';
    std::cout << "Elle yazilan sonuc: " << elleCikis << '\n';
    std::cout << "Ortalama mutlak fark: " << ortalamaFark << '\n';
    return 0;
}

