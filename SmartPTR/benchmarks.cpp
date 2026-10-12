#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <memory>       // std::shared_ptr — только для сравнения, как разрешает ТЗ

#include "benchmarks.h"
#include "sharedPtr.h"
#include "uniquePtr.h"

using namespace std;
using namespace std::chrono;

// ================= ПОДОПЫТНЫЙ ОБЪЕКТ =================
// Отдельный от Probe: здесь не нужны счётчики и печать,
// нужен минимальный объект, чтобы мерить накладные расходы
// самих указателей, а не работу конструктора.

namespace {

struct Payload {
    int value;
    explicit Payload(int v = 0) : value(v) {}
};

// Компилятор с -O2 имеет право выбросить пару new/delete целиком,
// если результат нигде не используется (allocation elision, C++14).
// volatile-переменная заставляет его считать обращение наблюдаемым.
volatile int sink = 0;

// ================= ЗАМЕР ВРЕМЕНИ =================

// На малых N один прогон занимает микросекунды, и результат тонет
// в разрешении таймера. Поэтому повторяем, пока не наберётся
// достаточно работы, и делим на число повторов.
double MeasureMs(void (*body)(long long), long long n) {
    long long reps = 1000000 / n;
    if (reps < 1) { reps = 1; }

    auto start = steady_clock::now();
    for (long long r = 0; r < reps; r++) {
        body(n);
    }
    auto finish = steady_clock::now();

    double totalMs = duration_cast<nanoseconds>(finish - start).count() / 1e6;
    return totalMs / (double)reps;
}

// ================= СЦЕНАРИЙ 1: пропускная способность =================
// Создать и тут же уничтожить N объектов, по одному.
// Память не растёт — меряется чистая стоимость создания/удаления.

void ThroughputRaw(long long n) {
    for (long long i = 0; i < n; i++) {
        Payload* p = new Payload(1);
        sink += p->value;
        delete p;
    }
}

void ThroughputUnique(long long n) {
    for (long long i = 0; i < n; i++) {
        UniquePtr<Payload> p(new Payload(1));
        sink += p->value;
    }
}

void ThroughputShared(long long n) {
    for (long long i = 0; i < n; i++) {
        SharedPtr<Payload> p(new Payload(1));
        sink += p->value;
    }
}

void ThroughputSharedCopy(long long n) {
    for (long long i = 0; i < n; i++) {
        SharedPtr<Payload> p(new Payload(1));
        sink += p->value;
        SharedPtr<Payload> q = p;       // плюс одно копирование на объект
        sink += p->value;
    }
}

void ThroughputStlShared(long long n) {
    for (long long i = 0; i < n; i++) {
        std::shared_ptr<Payload> p(new Payload(1));
        sink += p->value;
    }
}

void ThroughputStlMakeShared(long long n) {
    for (long long i = 0; i < n; i++) {
        auto p = std::make_shared<Payload>(1);   // объект и блок одной аллокацией
        sink += p->value;
    }
}

// ================= СЦЕНАРИЙ 2: массовое хранение =================
// Держим N объектов живыми одновременно. Здесь видно и время
// на массовое создание, и реальный расход памяти.

void BulkRaw(long long n) {
    Payload** arr = new Payload*[n];
    for (long long i = 0; i < n; i++) { arr[i] = new Payload(1); }
    for (long long i = 0; i < n; i++) { delete arr[i]; }
    delete[] arr;
}

void BulkUnique(long long n) {
    UniquePtr<Payload>* arr = new UniquePtr<Payload>[n];
    for (long long i = 0; i < n; i++) { arr[i].Reset(new Payload(1)); }
    delete[] arr;
}

void BulkShared(long long n) {
    SharedPtr<Payload>* arr = new SharedPtr<Payload>[n];
    for (long long i = 0; i < n; i++) { arr[i].Reset(new Payload(1)); }
    delete[] arr;
}

void BulkStlShared(long long n) {
    std::shared_ptr<Payload>* arr = new std::shared_ptr<Payload>[n];
    for (long long i = 0; i < n; i++) { arr[i].reset(new Payload(1)); }
    delete[] arr;
}

// ================= ВЫВОД =================

void PrintHeader(const char* title) {
    cout << "\n" << title << "\n";
    cout << "------------------------------------------------------------------------------------\n";
    cout << setw(12) << "N"
         << setw(12) << "raw"
         << setw(12) << "UniquePtr"
         << setw(12) << "SharedPtr"
         << setw(12) << "Shared+copy"
         << setw(12) << "std::shared"
         << setw(13) << "make_shared"
         << "\n";
    cout << setw(12) << ""
         << setw(12) << "ns/obj"
         << setw(12) << "ns/obj"
         << setw(12) << "ns/obj"
         << setw(12) << "ns/obj"
         << setw(12) << "ns/obj"
         << setw(13) << "ns/obj"
         << "\n";
    cout << "------------------------------------------------------------------------\n";
}

void PrintCell(double ms, long long n) {
    cout << setw(12) << fixed << setprecision(1) << (ms * 1e6 / (double)n);
}

} // namespace

// ================= ПАМЯТЬ =================
// Считаем аналитически: размеры известны точно, и такой расчёт
// не зависит от шума аллокатора и от того, чем меряли.
// Реальный расход = это число плюс служебные заголовки кучи
// (обычно 8-16 байт на каждую аллокацию, зависит от системы).

void PrintMemoryTable() {
    const size_t objSize = sizeof(Payload);

    cout << "\nMemory per one managed object (bytes)\n";
    cout << "------------------------------------------------------------------------\n";
    cout << setw(24) << "variant"
         << setw(12) << "handle"
         << setw(12) << "service"
         << setw(12) << "object"
         << setw(12) << "allocs"
         << "\n";
    cout << "------------------------------------------------------------------------\n";

    cout << setw(24) << "raw pointer"
         << setw(12) << sizeof(Payload*)
         << setw(12) << 0
         << setw(12) << objSize
         << setw(12) << 1 << "\n";

    cout << setw(24) << "UniquePtr"
         << setw(12) << sizeof(UniquePtr<Payload>)
         << setw(12) << 0
         << setw(12) << objSize
         << setw(12) << 1 << "\n";

    cout << setw(24) << "SharedPtr"
         << setw(12) << sizeof(SharedPtr<Payload>)
         << setw(12) << sizeof(ControlBlock<Payload>)
         << setw(12) << objSize
         << setw(12) << 2 << "\n";

    cout << setw(24) << "std::shared_ptr"
         << setw(12) << sizeof(std::shared_ptr<Payload>)
         << setw(12) << "~24"
         << setw(12) << objSize
         << setw(12) << 2 << "\n";

    cout << "------------------------------------------------------------------------\n";
    cout << "handle  - sizeof of the pointer itself (stack or container)\n";
    cout << "service - control block: counter, vptr, owner\n";
    cout << "allocs  - heap allocations per one managed object\n";
}

// ================= ГЛАВНАЯ ФУНКЦИЯ =================

void RunBenchmarks() {
    const long long sizes[] = {10, 100, 1000, 10000, 100000, 1000000, 10000000};
    const int count = sizeof(sizes) / sizeof(sizes[0]);

    ofstream csv("benchmarks.csv");
    csv << "scenario,N,raw_ns,unique_ns,shared_ns,shared_copy_ns,stl_shared_ns,make_shared_ns\n";

    // --- сценарий 1 ---
    PrintHeader("Scenario 1: create + destroy one by one (throughput)");

    for (int i = 0; i < count; i++) {
        long long n = sizes[i];

        double tRaw    = MeasureMs(ThroughputRaw, n);
        double tUnique = MeasureMs(ThroughputUnique, n);
        double tShared = MeasureMs(ThroughputShared, n);
        double tCopy   = MeasureMs(ThroughputSharedCopy, n);
        double tStl    = MeasureMs(ThroughputStlShared, n);
        double tMake   = MeasureMs(ThroughputStlMakeShared, n);

        cout << setw(12) << n;
        PrintCell(tRaw, n);
        PrintCell(tUnique, n);
        PrintCell(tShared, n);
        PrintCell(tCopy, n);
        PrintCell(tStl, n);
        cout << setw(13) << fixed << setprecision(1) << (tMake * 1e6 / (double)n);
        cout << "\n";

        csv << "throughput," << n << ","
            << tRaw * 1e6 / n << "," << tUnique * 1e6 / n << ","
            << tShared * 1e6 / n << "," << tCopy * 1e6 / n << ","
            << tStl * 1e6 / n << "," << tMake * 1e6 / n << "\n";
    }

    // --- сценарий 2 ---
    // Здесь все N объектов живут одновременно, поэтому верхнюю границу
    // приходится опускать: на 10^7 SharedPtr это уже сотни мегабайт.
    PrintHeader("Scenario 2: keep N objects alive at once (bulk)");

    for (int i = 0; i < count - 1; i++) {
        long long n = sizes[i];

        double tRaw    = MeasureMs(BulkRaw, n);
        double tUnique = MeasureMs(BulkUnique, n);
        double tShared = MeasureMs(BulkShared, n);
        double tStl    = MeasureMs(BulkStlShared, n);

        cout << setw(12) << n;
        PrintCell(tRaw, n);
        PrintCell(tUnique, n);
        PrintCell(tShared, n);
        cout << setw(12) << "-";
        PrintCell(tStl, n);
        cout << setw(13) << "-";
        cout << "\n";

        csv << "bulk," << n << ","
            << tRaw * 1e6 / n << "," << tUnique * 1e6 / n << ","
            << tShared * 1e6 / n << ",," << tStl * 1e6 / n << ",\n";
    }

    csv.close();

    PrintMemoryTable();

    cout << "\nResults written to benchmarks.csv\n";
};
