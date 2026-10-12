#include <iostream>

#include "autotests.h"
#include "benchmarks.h"
#include "sharedPtr.h"

using namespace std;

void ShowMenu() {
    cout << "\n===== MENU =====\n";
    cout << "1. Run ALL tests\n";
    cout << "2. Run BENCHMARKS\n";
    cout << "3. WeakPtr DEMO (cycles)\n";
    cout << "4. Toggle VERBOSE\n";
    cout << "0. Exit\n";
    cout << "Choice: ";
}

// ============================================================
// Демонстрация циклической ссылки (доп. задание).
// Класс назван Worker, а не Employee, чтобы не конфликтовать
// с Employee из employee.h: там он объявлен со СЛАБОЙ ссылкой,
// и два разных определения одного имени — нарушение ODR.
// ============================================================
struct Worker {
    string name;
    SharedPtr<Worker> colleague;        // СИЛЬНАЯ ссылка — в этом вся проблема

    explicit Worker(const string& n) : name(n) {
        cout << "  + создан " << n << "\n";
    }
    ~Worker() {
        cout << "  - удалён " << name << "\n";
    }
};

// Показывает, что при двух сильных ссылках объекты не удаляются.
void ShowCycleProblem() {
    cout << "\n--- colleague = SharedPtr (strong) ---\n";
    cout << "входим в блок\n";
    {
        SharedPtr<Worker> ivan(new Worker("Ivan"));
        SharedPtr<Worker> petya(new Worker("Petya"));

        cout << "  до связывания:    Ivan = " << ivan.UseCount()
             << ", Petya = " << petya.UseCount() << "\n";

        ivan->colleague = petya;        // Иван держит Петю
        petya->colleague = ivan;        // Петя держит Ивана

        cout << "  после связывания: Ivan = " << ivan.UseCount()
             << ", Petya = " << petya.UseCount()
             << "   <- каждого держат дважды\n";
    }
    cout << "вышли из блока\n";
    cout << "(строк \"удалён\" выше нет - объекты утекли)\n";
}


// То же самое, но ссылка на коллегу СЛАБАЯ.
// Отличие от Worker ровно в одном слове: WeakPtr вместо SharedPtr.
struct Colleague {
    string name;
    WeakPtr<Colleague> colleague;       // СЛАБАЯ ссылка — цикла не возникает

    explicit Colleague(const string& n) : name(n) {
        cout << "  + создан " << n << "\n";
    }
    ~Colleague() {
        cout << "  - удалён " << name << "\n";
    }
};

// Показывает, что со слабой ссылкой те же объекты удаляются.
void ShowCycleFixed() {
    cout << "\n--- colleague = WeakPtr (weak) ---\n";
    cout << "входим в блок\n";
    {
        SharedPtr<Colleague> ivan(new Colleague("Ivan"));
        SharedPtr<Colleague> petya(new Colleague("Petya"));

        cout << "  до связывания:    Ivan = " << ivan.UseCount()
             << ", Petya = " << petya.UseCount() << "\n";

        ivan->colleague = petya;        // увеличился weak у блока Пети
        petya->colleague = ivan;        // увеличился weak у блока Ивана

        cout << "  после связывания: Ivan = " << ivan.UseCount()
             << ", Petya = " << petya.UseCount()
             << "   <- strong не вырос, вырос weak\n";

        // связь при этом работает: доступ через Lock()
        SharedPtr<Colleague> c = ivan->colleague.Lock();
        if (c.Get()) {
            cout << "  коллега Ивана через Lock(): " << c->name
                 << ", пока держим его - Petya = " << petya.UseCount() << "\n";
        }
    }
    cout << "вышли из блока\n";
    cout << "(обе строки \"удалён\" на месте - утечки нет)\n";
}

int main() {
    cout << "=== SharedPtr project ===\n";

    ShowCycleProblem();
    ShowCycleFixed();

    int choice = -1;

    while (true) {
        ShowMenu();

        if (!(cin >> choice)) {
            cout << "Invalid input. Try again.\n";
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        if (choice == 0) {
            cout << "Exiting...\n";
            break;
        }

        switch (choice) {
            case 1:
                RunAllTests();
                break;

            case 2:
                RunBenchmarks();
                break;

            case 3:
                RunWeakPtrDemo();
                break;

            case 4:
                verbose = !verbose;
                cout << "Verbose: " << (verbose ? "ON\n" : "OFF\n");
                break;

            default:
                cout << "Invalid\n";
        }
    }

    return 0;
}
