#include "islemler.h"
#include "ortak.h"
#include <cuda_runtime.h>
#include <stdexcept>
#include <limits>

void kontrol(cudaError_t hata) {
    if (hata != cudaSuccess) throw std::runtime_error(cudaGetErrorString(hata));
}

template<class T> struct Bellek {
    T* p = nullptr;
    explicit Bellek(size_t n) { kontrol(cudaMalloc((void**)&p, n * sizeof(T))); }
    ~Bellek() { cudaFree(p); }
    Bellek(const Bellek&) = delete;
    Bellek& operator=(const Bellek&) = delete;
};

bool cudaVar(std::string& ad) {
    int sayi = 0;
    cudaError_t hata = cudaGetDeviceCount(&sayi);
    if (hata != cudaSuccess || sayi == 0) {
        ad = hata == cudaSuccess ? "CUDA aygiti bulunamadi" : cudaGetErrorString(hata);
        return false;
    }
    cudaDeviceProp bilgi;
    kontrol(cudaGetDeviceProperties(&bilgi, 0));
    kontrol(cudaSetDevice(0));
    ad = bilgi.name;
    return true;
}

__global__ void histogramKernel(const unsigned char* r, int n, int* hist, int* sinirlar) {
    __shared__ int yerel[256];
    int t = threadIdx.x;
    yerel[t] = 0;
    __syncthreads();
    for (int i = blockIdx.x * blockDim.x + t; i < n; i += blockDim.x * gridDim.x)
        atomicAdd(&yerel[r[i]], 1);
    __syncthreads();
    if (yerel[t] > 0) {
        atomicAdd(hist + t, yerel[t]);
        atomicMin(sinirlar, t);
        atomicMax(sinirlar + 1, t);
    }
}

__global__ void cdfKernel(const int* hist, int* cdf, unsigned char* tablo, int n) {
    __shared__ int a[256];
    __shared__ int ilk;
    int t = threadIdx.x;
    a[t] = hist[t];
    __syncthreads();
    for (int adim = 1; adim < 256; adim *= 2) {
        int ek = t >= adim ? a[t - adim] : 0;
        __syncthreads();
        a[t] += ek;
        __syncthreads();
    }
    if (t == 0) {
        ilk = 0;
        for (int i = 0; i < 256; i++) {
            if (a[i] > 0) { ilk = a[i]; break; }
        }
    }
    __syncthreads();
    cdf[t] = a[t];
    tablo[t] = n == ilk ? (unsigned char)t :
        yuvarla(float(a[t] - ilk) * (255.0f / (n - ilk)));
}

__global__ void donusumKernel(const unsigned char* r, unsigned char* germe,
    unsigned char* esit, int n, const int* sinirlar, const unsigned char* tablo) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    int alt = sinirlar[0], ust = sinirlar[1], v = r[i];
    germe[i] = alt == ust ? (unsigned char)v :
        (unsigned char)(((v - alt) * 255 + (ust - alt) / 2) / (ust - alt));
    esit[i] = tablo[v];
}

__global__ void karoKernel(const unsigned char* r, unsigned char* tablolar,
    int h, int w, int kh, int kw, int nx, float limit) {
    __shared__ int hist[256];
    int t = threadIdx.x;
    int k = blockIdx.x;
    hist[t] = 0;
    __syncthreads();
    int x0 = (k % nx) * kw, y0 = (k / nx) * kh;
    for (int i = t; i < kh * kw; i += blockDim.x) {
        int y = yansit(y0 + i / kw, h);
        int x = yansit(x0 + i % kw, w);
        atomicAdd(hist + r[y * w + x], 1);
    }
    __syncthreads();
    if (t == 0) karoTablosu(hist, tablolar + k * 256, kh * kw, limit);
}

__global__ void filtreKernel(const unsigned char* r, const unsigned char* g,
    const unsigned char* tuz, unsigned char* c, unsigned char* o, unsigned char* m,
    const unsigned char* tablolar, int h, int w, int kh, int kw, int nx, int ny) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= h * w) return;
    int y = i / w, x = i % w;
    c[i] = araDeger(tablolar, r[i], x, y, kw, kh, nx, ny);
    o[i] = ortalamaPiksel(g, h, w, y, x);
    m[i] = medyanPiksel(tuz, h, w, y, x);
}

Sonuc gpuIsle(const cv::Mat& resim, const cv::Mat& gurultu, const cv::Mat& tuz,
    float limit, int nx, int ny) {
    if (resim.empty() || resim.type() != CV_8UC1 || gurultu.type() != CV_8UC1 ||
        tuz.type() != CV_8UC1 || resim.size() != gurultu.size() || resim.size() != tuz.size() ||
        nx < 1 || ny < 1 || nx > 64 || ny > 64 ||
        resim.total() > size_t(std::numeric_limits<int>::max() / 4))
        throw std::runtime_error("Gecersiz resim veya parametre.");
    cv::Mat r = resim.isContinuous() ? resim : resim.clone();
    cv::Mat g = gurultu.isContinuous() ? gurultu : gurultu.clone();
    cv::Mat t = tuz.isContinuous() ? tuz : tuz.clone();
    int h = r.rows, w = r.cols, n = h * w, kh, kw;
    karoBoyutu(h, w, nx, ny, kh, kw);
    Bellek<unsigned char> dr(n), dg(n), dt(n), dgerme(n), desit(n), dc(n), dor(n), dm(n);
    Bellek<unsigned char> tablo(256), tablolar(nx * ny * 256);
    Bellek<int> hist(256), cdf(256), sinirlar(2);
    int baslangic[2] = {255, 0};
    kontrol(cudaMemcpy(dr.p, r.data, n, cudaMemcpyHostToDevice));
    kontrol(cudaMemcpy(dg.p, g.data, n, cudaMemcpyHostToDevice));
    kontrol(cudaMemcpy(dt.p, t.data, n, cudaMemcpyHostToDevice));
    kontrol(cudaMemset(hist.p, 0, 256 * sizeof(int)));
    kontrol(cudaMemcpy(sinirlar.p, baslangic, sizeof(baslangic), cudaMemcpyHostToDevice));
    int blok = (n + 255) / 256;
    histogramKernel<<<blok > 256 ? 256 : blok, 256>>>(dr.p, n, hist.p, sinirlar.p);
    kontrol(cudaGetLastError());
    cdfKernel<<<1, 256>>>(hist.p, cdf.p, tablo.p, n);
    kontrol(cudaGetLastError());
    donusumKernel<<<blok, 256>>>(dr.p, dgerme.p, desit.p, n, sinirlar.p, tablo.p);
    kontrol(cudaGetLastError());
    karoKernel<<<nx * ny, 256>>>(dr.p, tablolar.p, h, w, kh, kw, nx, limit);
    kontrol(cudaGetLastError());
    filtreKernel<<<blok, 256>>>(dr.p, dg.p, dt.p, dc.p, dor.p, dm.p, tablolar.p,
        h, w, kh, kw, nx, ny);
    kontrol(cudaGetLastError());
    kontrol(cudaDeviceSynchronize());
    Sonuc s;
    s.germe.create(h, w, CV_8UC1);
    s.esit.create(h, w, CV_8UC1);
    s.clahe.create(h, w, CV_8UC1);
    s.ortalama.create(h, w, CV_8UC1);
    s.medyan.create(h, w, CV_8UC1);
    kontrol(cudaMemcpy(s.germe.data, dgerme.p, n, cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(s.esit.data, desit.p, n, cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(s.clahe.data, dc.p, n, cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(s.ortalama.data, dor.p, n, cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(s.medyan.data, dm.p, n, cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(s.hist.data(), hist.p, 256 * sizeof(int), cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(s.cdf.data(), cdf.p, 256 * sizeof(int), cudaMemcpyDeviceToHost));
    kontrol(cudaMemcpy(baslangic, sinirlar.p, sizeof(baslangic), cudaMemcpyDeviceToHost));
    s.alt = baslangic[0];
    s.ust = baslangic[1];
    return s;
}
