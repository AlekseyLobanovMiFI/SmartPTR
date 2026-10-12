#include <iostream>
#include <utility>

#include "autotests.h"
#include "testClasses.h"
#include "sharedPtr.h"
#include "uniquePtr.h"
#include "employee.h"
#include "mutableArraySequence.h"

using namespace std;

// ================= CONFIG =================

bool verbose = false;

// ================= COUNTERS =================

int Probe::aliveCount = 0;
int Base::aliveCount = 0;
int Derived::derivedAliveCount = 0;

// ================= COLORS =================

#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

// ================= STATS =================

int totalChecks = 0;
int failedChecks = 0;

int totalTests = 0;
int failedTests = 0;

bool currentTestFailed = false;

// ================= ASSERT =================
// do/while(0) нужен, чтобы макрос вёл себя как одна инструкция
// и не ломался внутри if без фигурных скобок.

#define ASSERT_EQ(a, b)                                            \
    do {                                                           \
        totalChecks++;                                             \
        if ((a) != (b)) {                                          \
            cout << RED << "  FAIL: " << #a << " != " << #b        \
                 << " (" << (a) << " vs " << (b) << ")"            \
                 << " at line " << __LINE__ << RESET << "\n";      \
            failedChecks++;                                        \
            currentTestFailed = true;                              \
        }                                                          \
    } while (0)

#define ASSERT_TRUE(x)                                             \
    do {                                                           \
        totalChecks++;                                             \
        if (!(x)) {                                                \
            cout << RED << "  FAIL: " << #x                        \
                 << " at line " << __LINE__ << RESET << "\n";      \
            failedChecks++;                                        \
            currentTestFailed = true;                              \
        }                                                          \
    } while (0)

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

// Счётчик владельцев приводим к int, чтобы не ловить warning
// о сравнении знакового с беззнаковым при -Wextra.
#define ASSERT_COUNT(p, n) ASSERT_EQ((int)(p).UseCount(), (n))

// Инвариант, который обязан выполняться в конце каждого теста:
// все подопытные объекты удалены.
#define ASSERT_NO_LEAKS()                                          \
    do {                                                           \
        ASSERT_EQ(Probe::aliveCount, 0);                           \
        ASSERT_EQ(Base::aliveCount, 0);                            \
        ASSERT_EQ(Derived::derivedAliveCount, 0);                  \
    } while (0)

// ================= RUNNER =================

void RunTest(void (*testFunc)(), const string& name) {
    totalTests++;
    currentTestFailed = false;

    cout << YELLOW << "[ RUN      ] " << name << RESET << "\n";

    testFunc();

    if (currentTestFailed) {
        cout << RED << "[  FAILED  ] " << name << RESET << "\n\n";
        failedTests++;
    } else {
        cout << GREEN << "[  PASSED  ] " << name << RESET << "\n\n";
    }
}

// =========================================================
//                  SharedPtr — базовое владение
// =========================================================

// Объект удаляется сам на выходе из области видимости.
static void Test_Shared_DeletedOnScopeExit() {
    {
        SharedPtr<Probe> a(new Probe(1));
        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(a->GetId(), 1);
    }

    ASSERT_EQ(Probe::aliveCount, 0);
    ASSERT_NO_LEAKS();
}

// Пустой указатель создаётся и разрушается без падения.
static void Test_Shared_EmptyPointer() {
    {
        SharedPtr<Probe> empty;
        ASSERT_TRUE(empty.Get() == nullptr);
        ASSERT_COUNT(empty, 0);          // пустой ничем не владеет и ничего не аллоцирует
    }

    ASSERT_NO_LEAKS();
}

// У единственного владельца счётчик равен единице.
static void Test_Shared_UseCountSingleOwner() {
    {
        SharedPtr<Probe> a(new Probe(1));
        ASSERT_COUNT(a, 1);
    }

    ASSERT_NO_LEAKS();
}

// =========================================================
//                  SharedPtr — копирование
// =========================================================

// Копия увеличивает счётчик владельцев.
static void Test_Shared_CopyIncrementsCounter() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> b = a;

        ASSERT_COUNT(a, 2);
        ASSERT_COUNT(b, 2);
        ASSERT_EQ(Probe::aliveCount, 1);   // объект НЕ скопировался
    }

    ASSERT_NO_LEAKS();
}

// Обе копии смотрят на один и тот же объект.
static void Test_Shared_CopiesShareObject() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> b = a;

        ASSERT_TRUE(a.Get() == b.Get());

        b->SetId(42);
        ASSERT_EQ(a->GetId(), 42);          // изменение видно через оба
    }

    ASSERT_NO_LEAKS();
}

// Смерть копии не убивает объект, пока жив хоть один владелец.
static void Test_Shared_CopyDeathKeepsObject() {
    {
        SharedPtr<Probe> a(new Probe(1));

        {
            SharedPtr<Probe> b = a;
            ASSERT_COUNT(a, 2);
        }

        ASSERT_EQ(Probe::aliveCount, 1);    // объект жив
        ASSERT_COUNT(a, 1);                 // владелец снова один
    }

    ASSERT_NO_LEAKS();
}

// Много копий: счётчик растёт и корректно сходится обратно.
static void Test_Shared_ManyCopies() {
    {
        SharedPtr<Probe> a(new Probe(1));

        {
            const int copies = 100;
            SharedPtr<Probe>* pack = new SharedPtr<Probe>[copies];

            for (int i = 0; i < copies; i++) {
                pack[i] = a;
            }

            ASSERT_COUNT(a, copies + 1);
            ASSERT_EQ(Probe::aliveCount, 1);

            delete[] pack;
        }

        ASSERT_COUNT(a, 1);
        ASSERT_EQ(Probe::aliveCount, 1);
    }

    ASSERT_NO_LEAKS();
}

// =========================================================
//                  SharedPtr — присваивание
// =========================================================

// Присваивание отпускает объект, которым владели раньше.
static void Test_Shared_AssignReleasesOldObject() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> b(new Probe(2));

        ASSERT_EQ(Probe::aliveCount, 2);

        b = a;                              // объект 2 должен умереть здесь

        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(b->GetId(), 1);
        ASSERT_COUNT(a, 2);
    }

    ASSERT_NO_LEAKS();
}

// Самоприсваивание не ломает счётчик и не удаляет объект.
static void Test_Shared_SelfAssignment() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> b = a;

        a = a;

        ASSERT_COUNT(a, 2);
        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(a->GetId(), 1);
    }

    ASSERT_NO_LEAKS();
}

// Присваивание между двумя владельцами одного объекта.
// Наивная реализация здесь обнуляет счётчик и убивает живой объект.
static void Test_Shared_AssignBetweenSharedOwners() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> b = a;

        ASSERT_COUNT(a, 2);

        a = b;                              // оба уже владеют одним объектом

        ASSERT_COUNT(a, 2);
        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(a->GetId(), 1);
    }

    ASSERT_NO_LEAKS();
}

// Присваивание пустого в непустой и наоборот.
static void Test_Shared_AssignEmpty() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> empty;

        a = empty;                          // объект должен умереть

        ASSERT_EQ(Probe::aliveCount, 0);
        ASSERT_TRUE(a.Get() == nullptr);
    }

    {
        SharedPtr<Probe> empty;
        SharedPtr<Probe> a(new Probe(1));

        empty = a;                          // пустой берёт объект

        ASSERT_TRUE(empty.Get() != nullptr);
        ASSERT_COUNT(empty, 2);
    }

    ASSERT_NO_LEAKS();
}

// =========================================================
//                     SharedPtr — Reset
// =========================================================

// Reset без аргумента отпускает объект.
static void Test_Shared_ResetToNull() {
    {
        SharedPtr<Probe> a(new Probe(1));
        ASSERT_EQ(Probe::aliveCount, 1);

        a.Reset();

        ASSERT_EQ(Probe::aliveCount, 0);
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}

// Reset с новым объектом: старый удалён, новый взят под управление.
static void Test_Shared_ResetToNewObject() {
    {
        SharedPtr<Probe> a(new Probe(1));

        a.Reset(new Probe(2));

        ASSERT_EQ(Probe::aliveCount, 1);    // ровно один объект, а не два
        ASSERT_EQ(a->GetId(), 2);
        ASSERT_COUNT(a, 1);
    }

    ASSERT_NO_LEAKS();
}

// Reset тем же самым указателем не должен ничего ломать.
static void Test_Shared_ResetSamePointer() {
    {
        Probe* raw = new Probe(1);
        SharedPtr<Probe> a(raw);

        a.Reset(raw);

        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_COUNT(a, 1);
        ASSERT_EQ(a->GetId(), 1);
    }

    ASSERT_NO_LEAKS();
}

// Reset при нескольких владельцах: объект остаётся жить у остальных.
static void Test_Shared_ResetWithMultipleOwners() {
    {
        SharedPtr<Probe> a(new Probe(1));

        {
            SharedPtr<Probe> b = a;
            ASSERT_COUNT(a, 2);

            b.Reset();                      // отпускает только b

            ASSERT_EQ(Probe::aliveCount, 1);
            ASSERT_COUNT(a, 1);
            ASSERT_TRUE(b.Get() == nullptr);
        }

        ASSERT_EQ(a->GetId(), 1);
    }

    ASSERT_NO_LEAKS();
}

// =========================================================
//                  SharedPtr — доступ к объекту
// =========================================================

static void Test_Shared_AccessOperators() {
    {
        SharedPtr<Probe> a(new Probe(7));

        ASSERT_EQ(a->GetId(), 7);           // через стрелку
        ASSERT_EQ((*a).GetId(), 7);         // через звёздочку
        ASSERT_TRUE(a.Get() == &(*a));      // Get даёт тот же адрес

        (*a).SetId(8);                      // ссылка позволяет менять объект
        ASSERT_EQ(a->GetId(), 8);
    }

    ASSERT_NO_LEAKS();
}

// =========================================================
//                  SharedPtr — подтипизация
// =========================================================

// Upcast через конструктор: счётчик у обоих общий.
static void Test_Shared_UpcastSharesCounter() {
    {
        SharedPtr<Derived> d(new Derived());
        SharedPtr<Base> b = d;

        ASSERT_COUNT(d, 2);
        ASSERT_COUNT(b, 2);
        ASSERT_TRUE(d.Get() == b.Get());
        ASSERT_EQ(Derived::derivedAliveCount, 1);
    }

    ASSERT_NO_LEAKS();
}

// Через базовый указатель вызывается метод наследника.
static void Test_Shared_PolymorphicCall() {
    {
        SharedPtr<Derived> d(new Derived());
        SharedPtr<Base> b = d;

        ASSERT_EQ(b->Kind(), 2);            // 2 — это Derived
    }

    ASSERT_NO_LEAKS();
}

// Объект, удаляемый последним владельцем типа Base,
// всё равно должен вызвать деструктор Derived.
static void Test_Shared_DerivedDestructorThroughBase() {
    {
        SharedPtr<Base> b;

        {
            SharedPtr<Derived> d(new Derived());
            b = d;                          // upcast через присваивание
            ASSERT_COUNT(b, 2);
        }

        // d умер, объект держит только b (тип Base)
        ASSERT_EQ(Derived::derivedAliveCount, 1);
        ASSERT_COUNT(b, 1);
    }

    // Удаление произошло через Base* — без virtual ~Base() здесь была бы утечка
    ASSERT_EQ(Derived::derivedAliveCount, 0);
    ASSERT_NO_LEAKS();
}

// =========================================================
//                   SharedPtr — нагрузка
// =========================================================

static void Test_Shared_Stress() {
    const int count = 100000;
    int badPointers = 0;
    int maxAlive = 0;

    for (int i = 0; i < count; i++) {
        SharedPtr<Probe> a(new Probe(i));
        SharedPtr<Probe> b = a;

        // Проверки внутри цикла копим в счётчики, а не в ASSERT —
        // иначе одна ошибка напечатала бы сто тысяч строк.
        if (b.Get() == nullptr || b.Get() != a.Get()) {
            badPointers++;
        }
        if (Probe::aliveCount > maxAlive) {
            maxAlive = Probe::aliveCount;
        }
    }

    ASSERT_EQ(badPointers, 0);
    ASSERT_EQ(maxAlive, 1);             // копия не должна дублировать объект
    ASSERT_EQ(Probe::aliveCount, 0);
    ASSERT_NO_LEAKS();
}

// =========================================================
//                        UniquePtr
// =========================================================

static void Test_Unique_DeletedOnScopeExit() {
    {
        UniquePtr<Probe> a(new Probe(1));
        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(a->GetId(), 1);
    }

    ASSERT_NO_LEAKS();
}

// Перемещение передаёт владение, источник обнуляется, объект не дублируется.
static void Test_Unique_MoveConstructor() {
    {
        UniquePtr<Probe> a(new Probe(1));
        UniquePtr<Probe> b(std::move(a));

        ASSERT_TRUE(a.Get() == nullptr);
        ASSERT_TRUE(b.Get() != nullptr);
        ASSERT_EQ(b->GetId(), 1);
        ASSERT_EQ(Probe::aliveCount, 1);
    }

    ASSERT_NO_LEAKS();
}

// Перемещающее присваивание удаляет старый объект получателя.
static void Test_Unique_MoveAssignment() {
    {
        UniquePtr<Probe> a(new Probe(1));
        UniquePtr<Probe> b(new Probe(2));

        ASSERT_EQ(Probe::aliveCount, 2);

        b = std::move(a);                   // объект 2 умирает здесь

        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(b->GetId(), 1);
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}

// Release снимает владение: объект переживает свой указатель.
static void Test_Unique_Release() {
    Probe* raw = nullptr;

    {
        UniquePtr<Probe> a(new Probe(1));
        raw = a.Release();

        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_EQ(Probe::aliveCount, 1);        // объект жив, владельца нет
    ASSERT_EQ(raw->GetId(), 1);

    delete raw;                             // убираем вручную

    ASSERT_NO_LEAKS();
}

static void Test_Unique_Reset() {
    {
        UniquePtr<Probe> a(new Probe(1));

        a.Reset(new Probe(2));
        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_EQ(a->GetId(), 2);

        a.Reset();
        ASSERT_EQ(Probe::aliveCount, 0);
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}


// =========================================================
//        UniquePtr — массивы (проверка delete[])
// =========================================================
// Во всех тестах ниже доказательство одно и то же: если бы
// вместо delete[] вызвался обычный delete, отработал бы
// деструктор только ПЕРВОГО элемента, и aliveCount не дошёл
// бы до нуля (либо программа упала бы на порче кучи).

// Массив удаляется целиком: вызываются деструкторы всех элементов.
static void Test_Unique_ArrayDeletesAllElements() {
    {
        UniquePtr<Probe[]> arr(new Probe[10]);

        ASSERT_EQ(Probe::aliveCount, 10);

        for (int i = 0; i < 10; i++) {
            arr[i].SetId(i);
        }
        ASSERT_EQ(arr[7].GetId(), 7);
    }

    ASSERT_EQ(Probe::aliveCount, 0);        // при delete осталось бы 9
    ASSERT_NO_LEAKS();
}

// Индексный доступ: чтение, запись и константная версия operator[].
static void Test_Unique_ArrayIndexAccess() {
    {
        UniquePtr<Probe[]> a(new Probe[3]);

        a[0].SetId(10);
        a[1].SetId(11);
        a[2].SetId(12);

        ASSERT_EQ(a[0].GetId(), 10);
        ASSERT_EQ(a[2].GetId(), 12);

        const UniquePtr<Probe[]>& ref = a;  // должна выбраться const-версия
        ASSERT_EQ(ref[1].GetId(), 11);
    }

    ASSERT_NO_LEAKS();
}

// Пустой массивный указатель: delete[] nullptr безопасен.
static void Test_Unique_ArrayEmpty() {
    {
        UniquePtr<Probe[]> empty;
        ASSERT_TRUE(empty.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}

// Перемещение: владение ушло, элементы не продублировались.
static void Test_Unique_ArrayMove() {
    {
        UniquePtr<Probe[]> a(new Probe[5]);
        a[2].SetId(22);

        UniquePtr<Probe[]> b(std::move(a));

        ASSERT_TRUE(a.Get() == nullptr);
        ASSERT_EQ(Probe::aliveCount, 5);    // было бы 10 при копировании
        ASSERT_EQ(b[2].GetId(), 22);
    }

    ASSERT_NO_LEAKS();
}

// Move-присваивание: старый массив получателя удаляется ЦЕЛИКОМ.
static void Test_Unique_ArrayMoveAssignment() {
    {
        UniquePtr<Probe[]> a(new Probe[4]);
        UniquePtr<Probe[]> b(new Probe[6]);

        ASSERT_EQ(Probe::aliveCount, 10);

        b = std::move(a);                   // шесть элементов b умирают разом

        ASSERT_EQ(Probe::aliveCount, 4);    // при delete осталось бы 9
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}

// Reset: замена массива и полное освобождение.
static void Test_Unique_ArrayReset() {
    {
        UniquePtr<Probe[]> a(new Probe[8]);
        ASSERT_EQ(Probe::aliveCount, 8);

        a.Reset(new Probe[3]);              // восемь старых удаляются целиком
        ASSERT_EQ(Probe::aliveCount, 3);

        a.Reset();                          // и три тоже
        ASSERT_EQ(Probe::aliveCount, 0);
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}

// Release снимает владение: массив переживает свой указатель.
static void Test_Unique_ArrayRelease() {
    Probe* raw = nullptr;

    {
        UniquePtr<Probe[]> a(new Probe[5]);
        raw = a.Release();
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_EQ(Probe::aliveCount, 5);        // массив жив, владельца нет
    ASSERT_EQ(raw[0].GetId(), 0);

    delete[] raw;                           // убираем вручную, обязательно со скобками

    ASSERT_NO_LEAKS();
}

// Нагрузка: тысяча массивов по сто элементов — сто тысяч деструкторов.
static void Test_Unique_ArrayStress() {
    const int arrays = 1000;
    const int size = 100;
    int maxAlive = 0;

    for (int i = 0; i < arrays; i++) {
        UniquePtr<Probe[]> a(new Probe[size]);

        if (Probe::aliveCount > maxAlive) {
            maxAlive = Probe::aliveCount;
        }
    }

    ASSERT_EQ(maxAlive, size);              // одновременно жив только один массив
    ASSERT_EQ(Probe::aliveCount, 0);        // все сто тысяч удалились
    ASSERT_NO_LEAKS();
}

// =========================================================
//        SharedPtr — массивы (специализация SharedPtr<T[]>)
// =========================================================

// Разделяемое владение массивом: копия не дублирует элементы,
// а удаляется массив целиком, через delete[] внутри блока.
static void Test_Shared_ArraySharedOwnership() {
    {
        SharedPtr<Probe[]> a(new Probe[10]);
        a[3].SetId(33);

        ASSERT_EQ(Probe::aliveCount, 10);
        ASSERT_COUNT(a, 1);

        {
            SharedPtr<Probe[]> b = a;

            ASSERT_COUNT(a, 2);
            ASSERT_EQ(Probe::aliveCount, 10);   // элементы не продублировались
            ASSERT_EQ(b[3].GetId(), 33);        // смотрят на один массив
            ASSERT_TRUE(a.Get() == b.Get());
        }

        ASSERT_COUNT(a, 1);
        ASSERT_EQ(Probe::aliveCount, 10);       // копия ушла, массив жив
    }

    ASSERT_EQ(Probe::aliveCount, 0);            // при delete осталось бы 9
    ASSERT_NO_LEAKS();
}

// Присваивание отпускает старый массив целиком.
static void Test_Shared_ArrayAssignReleasesOld() {
    {
        SharedPtr<Probe[]> a(new Probe[4]);
        SharedPtr<Probe[]> b(new Probe[6]);

        ASSERT_EQ(Probe::aliveCount, 10);

        b = a;                                  // шесть элементов b умирают разом

        ASSERT_EQ(Probe::aliveCount, 4);
        ASSERT_COUNT(a, 2);
    }

    ASSERT_NO_LEAKS();
}

// Reset и пустое состояние массивной версии.
static void Test_Shared_ArrayResetAndEmpty() {
    {
        SharedPtr<Probe[]> empty;
        ASSERT_TRUE(empty.Get() == nullptr);
        ASSERT_COUNT(empty, 0);
    }

    {
        SharedPtr<Probe[]> a(new Probe[8]);
        ASSERT_EQ(Probe::aliveCount, 8);

        a.Reset(new Probe[3]);
        ASSERT_EQ(Probe::aliveCount, 3);
        ASSERT_COUNT(a, 1);

        a.Reset();
        ASSERT_EQ(Probe::aliveCount, 0);
        ASSERT_TRUE(a.Get() == nullptr);
    }

    ASSERT_NO_LEAKS();
}

// Reset у одного из владельцев не трогает массив, пока есть второй.
static void Test_Shared_ArrayResetWithTwoOwners() {
    {
        SharedPtr<Probe[]> a(new Probe[5]);

        {
            SharedPtr<Probe[]> b = a;
            ASSERT_COUNT(a, 2);

            b.Reset();

            ASSERT_EQ(Probe::aliveCount, 5);    // массив жив у a
            ASSERT_COUNT(a, 1);
            ASSERT_TRUE(b.Get() == nullptr);
        }

        ASSERT_EQ(Probe::aliveCount, 5);
    }

    ASSERT_NO_LEAKS();
}

// =========================================================
//                         WeakPtr
// =========================================================
// Слабая ссылка: наблюдает за объектом, но не владеет им.
// Доказательства во всех тестах ниже строятся на двух числах:
//   UseCount()            — счётчик СИЛЬНЫХ владельцев (strong)
//   Probe/Employee::alive — число живых объектов
// Если бы WeakPtr владел объектом, первое число росло бы, а второе
// не возвращалось бы в ноль.

// Слабая ссылка не увеличивает счётчик владельцев.
static void Test_Weak_DoesNotIncrementCounter() {
    {
        SharedPtr<Probe> a(new Probe(1));
        ASSERT_COUNT(a, 1);

        WeakPtr<Probe> w(a);

        ASSERT_COUNT(a, 1);                 // не 2! вот всё отличие от SharedPtr
        ASSERT_EQ(Probe::aliveCount, 1);

        SharedPtr<Probe> strong = a;        // для сравнения — сильная
        ASSERT_COUNT(a, 2);                 // а эта увеличила
    }

    ASSERT_NO_LEAKS();
}

// Пустая слабая ссылка считается просроченной.
static void Test_Weak_EmptyIsExpired() {
    WeakPtr<Probe> w;

    ASSERT_TRUE(w.Expired());
    ASSERT_TRUE(w.Lock().Get() == nullptr);

    ASSERT_NO_LEAKS();
}

// Пока жив хотя бы один владелец, ссылка не просрочена.
static void Test_Weak_NotExpiredWhileOwnerAlive() {
    {
        SharedPtr<Probe> a(new Probe(1));
        WeakPtr<Probe> w(a);

        ASSERT_FALSE(w.Expired());

        {
            SharedPtr<Probe> b = a;
            ASSERT_COUNT(a, 2);
            ASSERT_FALSE(w.Expired());
        }

        ASSERT_COUNT(a, 1);
        ASSERT_FALSE(w.Expired());          // один владелец ушёл, второй остался
    }

    ASSERT_NO_LEAKS();
}

// Слабая ссылка НЕ продлевает жизнь объекту.
static void Test_Weak_DoesNotPreventDeletion() {
    WeakPtr<Probe> w;

    {
        SharedPtr<Probe> a(new Probe(1));
        w = WeakPtr<Probe>(a);

        ASSERT_EQ(Probe::aliveCount, 1);
        ASSERT_FALSE(w.Expired());
    }                                       // единственный владелец умер

    ASSERT_EQ(Probe::aliveCount, 0);        // объект удалён, хотя w ещё жива
    ASSERT_TRUE(w.Expired());

    ASSERT_NO_LEAKS();
}

// Lock() на живом объекте даёт полноценного владельца.
static void Test_Weak_LockGivesOwner() {
    {
        SharedPtr<Probe> a(new Probe(42));
        WeakPtr<Probe> w(a);

        SharedPtr<Probe> locked = w.Lock();

        ASSERT_TRUE(locked.Get() != nullptr);
        ASSERT_TRUE(locked.Get() == a.Get());    // тот же объект
        ASSERT_EQ(locked->GetId(), 42);
        ASSERT_COUNT(a, 2);                      // Lock поднял счётчик
    }

    ASSERT_NO_LEAKS();
}

// Пока держим результат Lock(), объект жив даже без исходных владельцев.
static void Test_Weak_LockKeepsObjectAlive() {
    WeakPtr<Probe> w;
    {
        SharedPtr<Probe> locked;

        {
            SharedPtr<Probe> a(new Probe(7));
            w = WeakPtr<Probe>(a);
            locked = w.Lock();                   // захватили владение
            ASSERT_COUNT(a, 2);
        }                                        // исходный владелец умер

        ASSERT_EQ(Probe::aliveCount, 1);         // объект ЖИВ — его держит locked
        ASSERT_FALSE(w.Expired());
        ASSERT_COUNT(locked, 1);
        ASSERT_EQ(locked->GetId(), 7);
    }

    ASSERT_EQ(Probe::aliveCount, 0);
    ASSERT_TRUE(w.Expired());
    ASSERT_NO_LEAKS();
}

// Lock() на просроченной ссылке возвращает пустой SharedPtr, а не мусор.
static void Test_Weak_LockOnExpiredReturnsEmpty() {
    WeakPtr<Probe> w;

    {
        SharedPtr<Probe> a(new Probe(1));
        w = WeakPtr<Probe>(a);
    }

    ASSERT_TRUE(w.Expired());

    SharedPtr<Probe> locked = w.Lock();
    ASSERT_TRUE(locked.Get() == nullptr);
    ASSERT_COUNT(locked, 0);

    ASSERT_NO_LEAKS();
}

// Копирование и присваивание слабых ссылок не трогают strong.
static void Test_Weak_CopyAndAssign() {
    {
        SharedPtr<Probe> a(new Probe(1));

        WeakPtr<Probe> w1(a);
        WeakPtr<Probe> w2(w1);               // копирующий конструктор
        WeakPtr<Probe> w3;
        w3 = w1;                             // присваивание

        ASSERT_COUNT(a, 1);                  // три слабых — счётчик владельцев прежний
        ASSERT_FALSE(w2.Expired());
        ASSERT_FALSE(w3.Expired());
        ASSERT_TRUE(w2.Lock().Get() == a.Get());
        ASSERT_TRUE(w3.Lock().Get() == a.Get());
    }

    ASSERT_NO_LEAKS();
}

// Самоприсваивание слабой ссылки не ломает счётчики.
static void Test_Weak_SelfAssignment() {
    {
        SharedPtr<Probe> a(new Probe(1));
        WeakPtr<Probe> w(a);

        w = w;

        ASSERT_FALSE(w.Expired());
        ASSERT_COUNT(a, 1);
        ASSERT_TRUE(w.Lock().Get() == a.Get());
    }

    ASSERT_NO_LEAKS();
}

// Присваивание слабой ссылке другого объекта: старый блок отпускается.
static void Test_Weak_ReassignToAnotherObject() {
    {
        SharedPtr<Probe> a(new Probe(1));
        SharedPtr<Probe> b(new Probe(2));

        WeakPtr<Probe> w(a);
        ASSERT_TRUE(w.Lock().Get() == a.Get());

        w = WeakPtr<Probe>(b);               // переключились на другой объект
        ASSERT_TRUE(w.Lock().Get() == b.Get());
        ASSERT_COUNT(a, 1);
        ASSERT_COUNT(b, 1);
    }

    ASSERT_NO_LEAKS();
}

// Блок переживает объект: после смерти объекта слабая ссылка всё ещё
// может ответить на вопрос «жив ли он», значит счётчики ей доступны.
static void Test_Weak_BlockOutlivesObject() {
    WeakPtr<Probe> w1;
    WeakPtr<Probe> w2;

    {
        SharedPtr<Probe> a(new Probe(1));
        w1 = WeakPtr<Probe>(a);
        w2 = WeakPtr<Probe>(a);
        ASSERT_EQ(Probe::aliveCount, 1);
    }

    ASSERT_EQ(Probe::aliveCount, 0);         // объект удалён сразу
    ASSERT_TRUE(w1.Expired());               // а блок ещё жив — иначе это чтение
    ASSERT_TRUE(w2.Expired());               // было бы обращением к мёртвой памяти

    ASSERT_NO_LEAKS();
}

// Нагрузка: тысяча объектов, у каждого слабая ссылка. Ничего не течёт.
static void Test_Weak_Stress() {
    const int n = 1000;
    int maxAlive = 0;

    for (int i = 0; i < n; i++) {
        SharedPtr<Probe> a(new Probe(i));
        WeakPtr<Probe> w(a);
        SharedPtr<Probe> locked = w.Lock();

        if (Probe::aliveCount > maxAlive) { maxAlive = Probe::aliveCount; }
    }

    ASSERT_EQ(maxAlive, 1);                  // одновременно жив только один
    ASSERT_EQ(Probe::aliveCount, 0);
    ASSERT_NO_LEAKS();
}

// =========================================================
//   Циклические ссылки — задача с Employee
// =========================================================
// Два сотрудника, каждый ссылается на другого как на коллегу.
// Если ссылка сильная, счётчики никогда не дойдут до нуля.

// Проблема: при сильных ссылках объект живёт только потому,
// что его держит другой объект, сам уже недостижимый снаружи.
static void Test_Cycle_SharedKeepsObjectsAlive() {
    {
        SharedPtr<EmployeeCycle> keeper;

        {
            SharedPtr<EmployeeCycle> ivan(new EmployeeCycle("Ivan"));
            SharedPtr<EmployeeCycle> petr(new EmployeeCycle("Petr"));

            ASSERT_COUNT(ivan, 1);
            ASSERT_COUNT(petr, 1);
            ASSERT_EQ(EmployeeCycle::aliveCount, 2);

            ivan->colleague = petr;                  // Иван держит Петра
            petr->colleague = ivan;                  // Пётр держит Ивана

            ASSERT_COUNT(ivan, 2);                   // вот он, лишний владелец
            ASSERT_COUNT(petr, 2);

            keeper = ivan;                           // снаружи держим только Ивана
        }

        // ivan и petr умерли. Снаружи остался владелец только у Ивана,
        // значит Пётр должен был удалиться. Но он жив — его держит Иван,
        // которого держит Пётр. Это и есть утечка по циклу.
        ASSERT_EQ(EmployeeCycle::aliveCount, 2);

        keeper->colleague.Reset();                   // рвём цикл вручную
        ASSERT_EQ(EmployeeCycle::aliveCount, 1);     // Пётр наконец удалён
    }

    ASSERT_EQ(EmployeeCycle::aliveCount, 0);         // keeper умер — Иван тоже
}

// Решение: та же схема, но ссылка на коллегу слабая.
static void Test_Cycle_WeakBreaksTheCycle() {
    {
        SharedPtr<Employee> ivan(new Employee("Ivan"));
        SharedPtr<Employee> petr(new Employee("Petr"));

        ivan->colleague = petr;
        petr->colleague = ivan;

        ASSERT_COUNT(ivan, 1);                       // счётчики НЕ выросли
        ASSERT_COUNT(petr, 1);
        ASSERT_EQ(Employee::aliveCount, 2);

        // связь при этом работает
        SharedPtr<Employee> c = ivan->colleague.Lock();
        ASSERT_TRUE(c.Get() == petr.Get());
        ASSERT_TRUE(c->name == "Petr");
    }

    ASSERT_EQ(Employee::aliveCount, 0);              // оба удалены
    ASSERT_NO_LEAKS();
}

// Коллега умер — слабая ссылка честно сообщает об этом.
static void Test_Cycle_ColleagueOutlivedByWeakRef() {
    {
        SharedPtr<Employee> ivan(new Employee("Ivan"));

        {
            SharedPtr<Employee> petr(new Employee("Petr"));
            ivan->colleague = petr;

            ASSERT_FALSE(ivan->colleague.Expired());
            ASSERT_EQ(Employee::aliveCount, 2);
        }                                            // Пётр уволился

        ASSERT_EQ(Employee::aliveCount, 1);
        ASSERT_TRUE(ivan->colleague.Expired());      // Иван это видит
        ASSERT_TRUE(ivan->colleague.Lock().Get() == nullptr);
    }

    ASSERT_EQ(Employee::aliveCount, 0);
    ASSERT_NO_LEAKS();
}


// =========================================================
//   Демонстрация: циклические ссылки и WeakPtr
// =========================================================
// Отдельный пункт меню. Задача: показать глазами, что при двух
// сильных ссылках объекты не удаляются, а при слабой — удаляются.

void RunWeakPtrDemo() {
    cout << "\n================ WeakPtr: demo ================\n";

    // ---------- 1. СИЛЬНЫЕ ссылки: цикл ----------
    cout << "\n--- 1. colleague is SharedPtr (strong) ---\n";
    {
        SharedPtr<EmployeeCycle> keeper;

        {
            SharedPtr<EmployeeCycle> ivan(new EmployeeCycle("Ivan"));
            SharedPtr<EmployeeCycle> petr(new EmployeeCycle("Petr"));
            cout << "  created two employees, alive = "
                 << EmployeeCycle::aliveCount << "\n";
            cout << "  owners: Ivan = " << ivan.UseCount()
                 << ", Petr = " << petr.UseCount() << "\n";

            ivan->colleague = petr;
            petr->colleague = ivan;
            cout << "  made them colleagues\n";
            cout << "  owners: Ivan = " << ivan.UseCount()
                 << ", Petr = " << petr.UseCount()
                 << "   <- each is held twice now\n";

            keeper = ivan;                 // снаружи держим только Ивана
        }

        cout << "  local pointers died. Only Ivan had an outside owner,\n";
        cout << "  so Petr should be gone. alive = "
             << EmployeeCycle::aliveCount << "   <- but BOTH are alive\n";
        cout << "  this is the leak: each is kept by the other\n";

        keeper->colleague.Reset();         // рвём цикл вручную
        cout << "  broke the cycle by hand, alive = "
             << EmployeeCycle::aliveCount << "\n";
    }
    cout << "  keeper died, alive = " << EmployeeCycle::aliveCount << "\n";

    // ---------- 2. СЛАБАЯ ссылка ----------
    cout << "\n--- 2. colleague is WeakPtr (weak) ---\n";
    {
        SharedPtr<Employee> ivan(new Employee("Ivan"));
        SharedPtr<Employee> petr(new Employee("Petr"));
        cout << "  created two employees, alive = " << Employee::aliveCount << "\n";

        ivan->colleague = petr;
        petr->colleague = ivan;
        cout << "  made them colleagues\n";
        cout << "  owners: Ivan = " << ivan.UseCount()
             << ", Petr = " << petr.UseCount()
             << "   <- counters did NOT grow\n";

        SharedPtr<Employee> c = ivan->colleague.Lock();
        cout << "  Ivan's colleague via Lock(): " << c->name << "\n";
        cout << "  while locked, owners of Petr = " << petr.UseCount()
             << "   <- Lock() took temporary ownership\n";
    }
    cout << "  scope ended, alive = " << Employee::aliveCount
         << "   <- both deleted, no leak\n";

    // ---------- 3. просроченная ссылка ----------
    cout << "\n--- 3. expired weak reference ---\n";
    {
        WeakPtr<Employee> w;
        {
            SharedPtr<Employee> tmp(new Employee("Temp"));
            w = WeakPtr<Employee>(tmp);
            cout << "  object alive, expired = " << w.Expired()
                 << ", Lock() gives "
                 << (w.Lock().Get() ? "object" : "nothing") << "\n";
        }
        cout << "  owner gone,   expired = " << w.Expired()
             << ", Lock() gives "
             << (w.Lock().Get() ? "object" : "nothing") << "\n";
        cout << "  a raw pointer here would be dangling and would lie\n";
    }

    cout << "\n==============================================\n";
}

// =========================================================
//   Контейнер поверх умных указателей (требование ТЗ)
// =========================================================
// Sequence из ЛР2 переиспользуется как есть, инстанцируется
// типом SharedPtr<T>. Проверяем, что указатели корректно
// переживают жизнь внутри чужого контейнера.

// Контейнер владеет объектами: уничтожение контейнера
// освобождает всё, что в нём лежало.
static void Test_Seq_ContainerOwnsObjects() {
    {
        MutableArraySequence<SharedPtr<Probe>> seq;

        for (int i = 0; i < 5; i++) {
            seq.Append(SharedPtr<Probe>(new Probe(i)));
        }

        ASSERT_EQ(seq.GetLength(), 5);
        ASSERT_EQ(Probe::aliveCount, 5);        // пять объектов, не больше
        ASSERT_EQ(seq.Get(0)->GetId(), 0);
        ASSERT_EQ(seq.Get(4)->GetId(), 4);
    }

    ASSERT_EQ(Probe::aliveCount, 0);            // контейнер умер — объекты тоже
    ASSERT_NO_LEAKS();
}

// Рост контейнера (Resize внутри DynamicArray) перекладывает
// указатели в новый буфер и не теряет ни одного объекта.
static void Test_Seq_GrowthKeepsObjects() {
    {
        MutableArraySequence<SharedPtr<Probe>> seq;

        for (int i = 0; i < 100; i++) {         // несколько удвоений ёмкости
            seq.Append(SharedPtr<Probe>(new Probe(i)));
        }

        ASSERT_EQ(seq.GetLength(), 100);
        ASSERT_EQ(Probe::aliveCount, 100);      // ни одного лишнего и ни одного потерянного
        ASSERT_EQ(seq.Get(99)->GetId(), 99);
    }

    ASSERT_NO_LEAKS();
}

// Get возвращает T по значению, то есть копию умного указателя:
// счётчик владельцев временно увеличивается.
static void Test_Seq_GetReturnsCopy() {
    {
        MutableArraySequence<SharedPtr<Probe>> seq;
        seq.Append(SharedPtr<Probe>(new Probe(1)));

        {
            SharedPtr<Probe> taken = seq.Get(0);

            ASSERT_COUNT(taken, 2);             // один в контейнере, один у нас
            ASSERT_EQ(Probe::aliveCount, 1);    // но объект по-прежнему один
        }

        ASSERT_COUNT(seq.Get(0), 2);            // временная копия в самом выражении
        ASSERT_EQ(Probe::aliveCount, 1);
    }

    ASSERT_NO_LEAKS();
}

// Копирование контейнера не дублирует объекты — копируются
// только указатели, а счётчики растут.
static void Test_Seq_CopyDoesNotDuplicateObjects() {
    {
        MutableArraySequence<SharedPtr<Probe>> seq;

        for (int i = 0; i < 3; i++) {
            seq.Append(SharedPtr<Probe>(new Probe(i)));
        }

        {
            MutableArraySequence<SharedPtr<Probe>> copy(seq);

            ASSERT_EQ(copy.GetLength(), 3);
            ASSERT_EQ(Probe::aliveCount, 3);    // объектов всё ещё три, а не шесть
            ASSERT_TRUE(copy.Get(1).Get() == seq.Get(1).Get());
        }

        ASSERT_EQ(Probe::aliveCount, 3);        // копия умерла, оригинал жив
    }

    ASSERT_NO_LEAKS();
}

// Полиморфизм внутри контейнера: храним SharedPtr<Base>,
// кладём туда Derived.
static void Test_Seq_PolymorphicContainer() {
    {
        MutableArraySequence<SharedPtr<Base>> seq;

        seq.Append(SharedPtr<Base>(new Base()));
        seq.Append(SharedPtr<Derived>(new Derived()));   // upcast при добавлении

        ASSERT_EQ(seq.GetLength(), 2);
        ASSERT_EQ(seq.Get(0)->Kind(), 1);
        ASSERT_EQ(seq.Get(1)->Kind(), 2);                // вызвался метод наследника
        ASSERT_EQ(Derived::derivedAliveCount, 1);
    }

    ASSERT_EQ(Derived::derivedAliveCount, 0);            // ~Derived отработал
    ASSERT_NO_LEAKS();
}

// ================= RUN ALL =================

void RunAllTests() {
    totalChecks = 0;
    failedChecks = 0;
    totalTests = 0;
    failedTests = 0;

    cout << "\n=== SharedPtr: ownership ===\n";
    RunTest(Test_Shared_DeletedOnScopeExit,        "DeletedOnScopeExit");
    RunTest(Test_Shared_EmptyPointer,              "EmptyPointer");
    RunTest(Test_Shared_UseCountSingleOwner,       "UseCountSingleOwner");

    cout << "=== SharedPtr: copy ===\n";
    RunTest(Test_Shared_CopyIncrementsCounter,     "CopyIncrementsCounter");
    RunTest(Test_Shared_CopiesShareObject,         "CopiesShareObject");
    RunTest(Test_Shared_CopyDeathKeepsObject,      "CopyDeathKeepsObject");
    RunTest(Test_Shared_ManyCopies,                "ManyCopies");

    cout << "=== SharedPtr: assignment ===\n";
    RunTest(Test_Shared_AssignReleasesOldObject,   "AssignReleasesOldObject");
    RunTest(Test_Shared_SelfAssignment,            "SelfAssignment");
    RunTest(Test_Shared_AssignBetweenSharedOwners, "AssignBetweenSharedOwners");
    RunTest(Test_Shared_AssignEmpty,               "AssignEmpty");

    cout << "=== SharedPtr: reset ===\n";
    RunTest(Test_Shared_ResetToNull,               "ResetToNull");
    RunTest(Test_Shared_ResetToNewObject,          "ResetToNewObject");
    RunTest(Test_Shared_ResetSamePointer,          "ResetSamePointer");
    RunTest(Test_Shared_ResetWithMultipleOwners,   "ResetWithMultipleOwners");

    cout << "=== SharedPtr: access ===\n";
    RunTest(Test_Shared_AccessOperators,           "AccessOperators");

    cout << "=== SharedPtr: subtyping ===\n";
    RunTest(Test_Shared_UpcastSharesCounter,       "UpcastSharesCounter");
    RunTest(Test_Shared_PolymorphicCall,           "PolymorphicCall");
    RunTest(Test_Shared_DerivedDestructorThroughBase, "DerivedDestructorThroughBase");

    cout << "=== SharedPtr: stress ===\n";
    RunTest(Test_Shared_Stress,                    "Stress100k");

    cout << "=== UniquePtr ===\n";
    RunTest(Test_Unique_DeletedOnScopeExit,         "Unique_DeletedOnScopeExit");
    RunTest(Test_Unique_MoveConstructor,            "Unique_MoveConstructor");
    RunTest(Test_Unique_MoveAssignment,             "Unique_MoveAssignment");
    RunTest(Test_Unique_Release,                    "Unique_Release");
    RunTest(Test_Unique_Reset,                      "Unique_Reset");

    cout << "=== UniquePtr: arrays (delete[]) ===\n";
    RunTest(Test_Unique_ArrayDeletesAllElements,    "Array_DeletesAllElements");
    RunTest(Test_Unique_ArrayIndexAccess,           "Array_IndexAccess");
    RunTest(Test_Unique_ArrayEmpty,                 "Array_Empty");
    RunTest(Test_Unique_ArrayMove,                  "Array_Move");
    RunTest(Test_Unique_ArrayMoveAssignment,        "Array_MoveAssignment");
    RunTest(Test_Unique_ArrayReset,                 "Array_Reset");
    RunTest(Test_Unique_ArrayRelease,               "Array_Release");
    RunTest(Test_Unique_ArrayStress,                "Array_Stress100k");

    cout << "=== SharedPtr: arrays ===\n";
    RunTest(Test_Shared_ArraySharedOwnership,       "SharedArray_SharedOwnership");
    RunTest(Test_Shared_ArrayAssignReleasesOld,     "SharedArray_AssignReleasesOld");
    RunTest(Test_Shared_ArrayResetAndEmpty,         "SharedArray_ResetAndEmpty");
    RunTest(Test_Shared_ArrayResetWithTwoOwners,    "SharedArray_ResetWithTwoOwners");

    cout << "=== WeakPtr ===\n";
    RunTest(Test_Weak_DoesNotIncrementCounter,      "Weak_DoesNotIncrementCounter");
    RunTest(Test_Weak_EmptyIsExpired,               "Weak_EmptyIsExpired");
    RunTest(Test_Weak_NotExpiredWhileOwnerAlive,    "Weak_NotExpiredWhileOwnerAlive");
    RunTest(Test_Weak_DoesNotPreventDeletion,       "Weak_DoesNotPreventDeletion");
    RunTest(Test_Weak_LockGivesOwner,               "Weak_LockGivesOwner");
    RunTest(Test_Weak_LockKeepsObjectAlive,         "Weak_LockKeepsObjectAlive");
    RunTest(Test_Weak_LockOnExpiredReturnsEmpty,    "Weak_LockOnExpiredReturnsEmpty");
    RunTest(Test_Weak_CopyAndAssign,                "Weak_CopyAndAssign");
    RunTest(Test_Weak_SelfAssignment,               "Weak_SelfAssignment");
    RunTest(Test_Weak_ReassignToAnotherObject,      "Weak_ReassignToAnotherObject");
    RunTest(Test_Weak_BlockOutlivesObject,          "Weak_BlockOutlivesObject");
    RunTest(Test_Weak_Stress,                       "Weak_Stress1000");

    cout << "=== Cycles: Employee ===\n";
    RunTest(Test_Cycle_SharedKeepsObjectsAlive,     "Cycle_SharedKeepsObjectsAlive");
    RunTest(Test_Cycle_WeakBreaksTheCycle,          "Cycle_WeakBreaksTheCycle");
    RunTest(Test_Cycle_ColleagueOutlivedByWeakRef,  "Cycle_ColleagueOutlivedByWeak");

    cout << "=== Sequence поверх указателей ===\n";
    RunTest(Test_Seq_ContainerOwnsObjects,          "Seq_ContainerOwnsObjects");
    RunTest(Test_Seq_GrowthKeepsObjects,            "Seq_GrowthKeepsObjects");
    RunTest(Test_Seq_GetReturnsCopy,                "Seq_GetReturnsCopy");
    RunTest(Test_Seq_CopyDoesNotDuplicateObjects,   "Seq_CopyDoesNotDuplicate");
    RunTest(Test_Seq_PolymorphicContainer,          "Seq_PolymorphicContainer");

    cout << "============================\n";
    cout << "Tests:   " << totalTests << "\n";
    cout << "Failed:  " << failedTests << "\n";
    cout << "Checks:  " << totalChecks << "\n";

    double percent = 100.0 * (totalTests - failedTests) / totalTests;
    cout << "Success: " << percent << "%\n";
    cout << "============================\n";
}
