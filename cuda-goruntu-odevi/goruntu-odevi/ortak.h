#pragma once
#include <cmath>

#ifdef __CUDACC__
#define ORTAK __host__ __device__
#else
#define ORTAK
#endif

ORTAK inline int sinir(int x, int n) {
    return x < 0 ? 0 : (x >= n ? n - 1 : x);
}

ORTAK inline int yansit(int x, int n) {
    if (n == 1) return 0;
    int t = 2 * n - 2;
    x %= t;
    if (x < 0) x += t;
    return x < n ? x : t - x;
}

ORTAK inline unsigned char yuvarla(float x) {
    int a = int(x);
    float kalan = x - a;
    if (kalan > 0.5f || (kalan == 0.5f && a % 2 != 0)) a++;
    return (unsigned char)sinir(a, 256);
}

inline void karoBoyutu(int h, int w, int nx, int ny, int& kh, int& kw) {
    if (w % nx == 0 && h % ny == 0) {
        kw = w / nx;
        kh = h / ny;
    } else {
        kw = (w + nx - w % nx) / nx;
        kh = (h + ny - h % ny) / ny;
    }
}

ORTAK inline void karoTablosu(int* hist, unsigned char* tablo, int alan, float limit) {
    if (limit > 0) {
        int esik = int(limit * alan / 256);
        if (esik < 1) esik = 1;
        int fazla = 0;
        for (int i = 0; i < 256; i++) {
            if (hist[i] > esik) {
                fazla += hist[i] - esik;
                hist[i] = esik;
            }
        }
        int pay = fazla / 256;
        int kalan = fazla % 256;
        for (int i = 0; i < 256; i++) hist[i] += pay;
        if (kalan > 0) {
            int adim = 256 / kalan;
            for (int i = 0; i < 256 && kalan > 0; i += adim, kalan--) hist[i]++;
        }
    }
    int toplam = 0;
    float katsayi = 255.0f / alan;
    for (int i = 0; i < 256; i++) {
        toplam += hist[i];
        tablo[i] = yuvarla(toplam * katsayi);
    }
}

ORTAK inline unsigned char araDeger(const unsigned char* tablo, int v,
    int x, int y, int kw, int kh, int nx, int ny) {
    float tx = x * (1.0f / kw) - 0.5f;
    float ty = y * (1.0f / kh) - 0.5f;
    int x0 = int(floorf(tx));
    int y0 = int(floorf(ty));
    float dx = tx - x0;
    float dy = ty - y0;
    int x1 = sinir(x0 + 1, nx);
    int y1 = sinir(y0 + 1, ny);
    x0 = sinir(x0, nx);
    y0 = sinir(y0, ny);
    float a = tablo[(y0 * nx + x0) * 256 + v] * (1 - dx)
            + tablo[(y0 * nx + x1) * 256 + v] * dx;
    float b = tablo[(y1 * nx + x0) * 256 + v] * (1 - dx)
            + tablo[(y1 * nx + x1) * 256 + v] * dx;
    return yuvarla(a * (1 - dy) + b * dy);
}

ORTAK inline unsigned char ortalamaPiksel(const unsigned char* p, int h, int w, int y, int x) {
    int toplam = 0;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
            toplam += p[sinir(y - j, h) * w + sinir(x - i, w)];
    return (unsigned char)((toplam + 4) / 9);
}

ORTAK inline unsigned char medyanPiksel(const unsigned char* p, int h, int w, int y, int x) {
    unsigned char dizi[25];
    int k = 0;
    for (int j = -2; j <= 2; j++)
        for (int i = -2; i <= 2; i++)
            dizi[k++] = p[sinir(y + j, h) * w + sinir(x + i, w)];
    for (int i = 1; i < 25; i++) {
        unsigned char v = dizi[i];
        int j = i - 1;
        while (j >= 0 && dizi[j] > v) {
            dizi[j + 1] = dizi[j];
            j--;
        }
        dizi[j + 1] = v;
    }
    return dizi[12];
}
