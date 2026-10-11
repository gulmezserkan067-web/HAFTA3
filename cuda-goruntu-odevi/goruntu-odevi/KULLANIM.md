# CUDA görüntü işleme

`calistir.cmd` dosyasını açınca resimler işlenir. `sonuclar` klasöründe hazır çıktı resimleri de bulunur.

- `gpu.cu`: CUDA çekirdekleri ve GPU bellek işlemleri.
- `ortak.h`: elle yazılan CLAHE, 3×3 ortalama ve 5×5 medyan hesapları.
- `main.cpp`: resimleri okuma ve sonuçları kaydetme.
- `resimler`: kullanılan düşük kontrastlı, normal gürültülü ve tuz-biber gürültülü girişler; kaynak ve temiz resim.
- `sonuclar`: beş CUDA çıktısı, 256 satırlık histogram/CDF dosyası ve minimum–maksimum değerleri.

Görüntü işleme hesapları CUDA'da yapılır. OpenCV sadece görüntü okuma, görüntü kaydetme ve görüntü belleği için kullanılır. Hazır histogram, eşitleme, CLAHE veya medyan filtresi çağrısı yoktur. CUDA bloklarında 256 iş parçacığı kullanılır.

CLAHE: 8×8 karo, 2.0 kırpma sınırı ve çift doğrusal ara değer hesabı. Ortalamada 1/9 katsayılı 3×3 çekirdek kullanılır. Medyan için 25 değer elle sıralanıp ortadaki alınır. Filtre sınırlarında en yakın kenar pikseli tekrarlanır. Sabit renkli görüntü germe ve genel histogram eşitlemede aynı bırakılır.

Örnek girişler önceki çalışmadaki `fruits.jpg` görüntüsünden hazırlanmıştır. Düşük kontrastlı görüntü 90–150 aralığına daraltılmıştır. Diğer iki görüntüye standart sapması 18 olan normal gürültü ve yaklaşık %10 tuz-biber gürültüsü eklenmiştir. Rastgelelik tohumu 42'dir.

Kendi resimlerin için aynı boyuttaki üç resmi `dusuk.png`, `gurultu.png`, `tuz_biber.png` adlarıyla bir klasöre koy:

```text
calistir.cmd "giris_klasoru" "cikis_klasoru"
```

`derle.cmd`, bu bilgisayardaki Visual Studio 2022, CUDA 12.6 ve önceki OpenCV kurulumu ile yeniden derler. Hazır exe de bu kurulumu kullanır; OpenCV DLL dosyaları pakete dahil değildir.

Farklı bir bilgisayarda C++17 derleyicisi, CMake, CUDA Toolkit ve OpenCV geliştirme dosyaları gerekir. Uygun derleyici terminalinden:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DOpenCV_DIR="OpenCV_kurulum_yolu" -DCMAKE_CUDA_ARCHITECTURES=native
cmake --build build --config Release
```

Çalıştırırken OpenCV DLL klasörünü PATH'e ekle. `derle.cmd` içindeki yollar bu bilgisayara özeldir. CUDA destekli NVIDIA ekran kartı gerekir; CPU'ya otomatik geçiş yapılmaz. Süre ölçümü bir ısınma çalıştırmasından sonra yapılır; GPU bellek ayırma, hesaplama ve iki yöndeki aktarımları içerir, dosya işlemlerini içermez.

RTX 4050 üzerinde 55 farklı test girdisiyle doğrulandı. Testlerde sabit görüntüler, tek satır/sütun, küçük görüntüler, rastgele değerler ve karo boyutuna tam bölünmeyen boyutlar kullanıldı. Örnek görüntülerde beş CUDA çıktısı da bağımsız kontrol hesaplarıyla piksel düzeyinde aynı çıktı. Test amacıyla kullanılan CPU ve hazır OpenCV uygulamaları bu klasöre dahil değildir.

Başvuru: [OpenCV histogram eşitleme ve CLAHE açıklaması](https://docs.opencv.org/4.x/d5/daf/tutorial_py_histogram_equalization.html), [CUDA C++ programlama kılavuzu](https://docs.nvidia.com/cuda/cuda-c-programming-guide/).
