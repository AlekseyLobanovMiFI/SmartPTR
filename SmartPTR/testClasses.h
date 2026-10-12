#ifndef TEST_CLASSES_H
#define TEST_CLASSES_H

// Подопытные классы для тестов умных указателей.
// Каждый считает свои живые экземпляры — это позволяет проверять
// отсутствие утечек прямо в тестах, не дожидаясь valgrind.

class Probe{
private:
    int id;

public:
    static int aliveCount;

    explicit Probe(int id = 0) : id(id) {
        aliveCount++;
    }

    ~Probe(){
        aliveCount--;
    }

    int GetId() const { return id; }
    void SetId(int newId) { id = newId; }
};


// Иерархия для проверки подтипизации и полиморфного удаления.
// Kind() возвращает разные числа — по нему видно, чей метод вызвался.

class Base{
public:
    static int aliveCount;

    Base(){ aliveCount++; }
    virtual ~Base(){ aliveCount--; }

    virtual int Kind() const { return 1; }
};


class Derived : public Base{
public:
    static int derivedAliveCount;

    Derived(){ derivedAliveCount++; }
    ~Derived() override { derivedAliveCount--; }

    int Kind() const override { return 2; }
};


// Класс вне иерархии Base/Derived — нужен для проверки того,
// что присваивание несвязанных типов не компилируется.
class Unrelated{
public:
    int Kind() const { return 3; }
};

#endif
