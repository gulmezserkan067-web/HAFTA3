#include "islemler.h"
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

void kaydet(const fs::path& yol, const cv::Mat& r) {
    if (!cv::imwrite(yol.string(), r)) throw std::runtime_error("Resim kaydedilemedi.");
}

int main(int argc, char** argv) {
    try {
        fs::path giris = argc > 1 ? argv[1] : "resimler";
        fs::path cikis = argc > 2 ? argv[2] : "sonuclar";
        std::string aygit;
        if (!cudaVar(aygit)) throw std::runtime_error(aygit);
        cv::Mat r = cv::imread((giris / "dusuk.png").string(), cv::IMREAD_GRAYSCALE);
        cv::Mat g = cv::imread((giris / "gurultu.png").string(), cv::IMREAD_GRAYSCALE);
        cv::Mat t = cv::imread((giris / "tuz_biber.png").string(), cv::IMREAD_GRAYSCALE);
        if (r.empty() || g.empty() || t.empty()) throw std::runtime_error("Resimler acilamadi.");
        gpuIsle(r, g, t);
        auto bas = std::chrono::steady_clock::now();
        Sonuc s = gpuIsle(r, g, t);
        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - bas).count();
        fs::create_directories(cikis);
        kaydet(cikis / "1_germe.png", s.germe);
        kaydet(cikis / "2_esitleme.png", s.esit);
        kaydet(cikis / "3_clahe.png", s.clahe);
        kaydet(cikis / "4_ortalama.png", s.ortalama);
        kaydet(cikis / "5_medyan.png", s.medyan);
        std::ofstream f(cikis / "histogram_cdf.csv");
        if (!f) throw std::runtime_error("CSV acilamadi.");
        f << "deger,histogram,kumulatif_toplam,cdf\n";
        for (int i = 0; i < 256; i++)
            f << i << ',' << s.hist[i] << ',' << s.cdf[i] << ','
              << std::setprecision(10) << double(s.cdf[i]) / r.total() << '\n';
        if (!f) throw std::runtime_error("CSV yazilamadi.");
        std::ofstream rapor(cikis / "bilgiler.txt");
        if (!rapor) throw std::runtime_error("Rapor acilamadi.");
        rapor << "GPU: " << aygit << '\n'
              << "Minimum: " << s.alt << '\n'
              << "Maksimum: " << s.ust << '\n'
              << "Sure: " << ms << " ms (bellek ayirma ve aktarimlar dahil)\n";
        std::cout << "Minimum: " << s.alt << " Maksimum: " << s.ust << '\n';
        std::cout << "CUDA: " << ms << " ms\n";
        std::cout << "Sonuclar: " << cikis.string() << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
