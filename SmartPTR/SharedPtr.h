#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include "UniquePtr.h"
#include <cstddef>

// Не шаблон! Через эту базу SharedPtr<Base> и SharedPtr<Derived> работают с одним и тем же блоком и одним счётчиком.
struct ControlBlockBase {
    size_t strong;
    size_t weak;

    ControlBlockBase() : strong(1), weak(0) {}
    virtual void DestroyObject() = 0;
    virtual ~ControlBlockBase() {}
};

// А здесь уже известен настоящий тип объекта, и UniquePtr
// сам знает, каким способом его удалять.
template <class U>
struct ControlBlock : ControlBlockBase {
    UniquePtr<U> owner;
    explicit ControlBlock(U* p) : owner(p) {}
    void DestroyObject() override { owner.Reset(); }
};

// Для массивов: U* — это указатель на ЭЛЕМЕНТ, а не на массив.
template <class U>
struct ControlBlock<U[]> : ControlBlockBase {
    UniquePtr<U[]> owner;
    explicit ControlBlock(U* p) : owner(p) {}
    void DestroyObject() override { owner.Reset(); }
};

template <typename T> class WeakPtr;

template <typename T>
class SharedPtr {
private:
    T* ptr;
    ControlBlockBase* ctrl;

    void Release() {
        if (ctrl == nullptr) {return;}
        --(ctrl->strong);
        if (ctrl->strong == 0) {
            ctrl->DestroyObject();
            if (ctrl->weak == 0){
                delete ctrl;
            }
        }
        ptr = nullptr;
        ctrl = nullptr;
    }

public:
    template <class U> friend class SharedPtr;
    template <class U> friend class WeakPtr;

    SharedPtr() : ptr(nullptr), ctrl(nullptr) {}

    template <class U>
    explicit SharedPtr(U* p) : ptr(p), ctrl(p ? new ControlBlock<U>(p) : nullptr) {}

    SharedPtr(const SharedPtr<T>& other) : ptr(other.ptr), ctrl(other.ctrl) {
        if (ctrl) { ++(ctrl->strong); }
    }

    explicit SharedPtr(const WeakPtr<T>& w);

    template <class U>
    SharedPtr(const SharedPtr<U>& other) : ptr(other.ptr), ctrl(other.ctrl) {
        if (ctrl) { ++(ctrl->strong); }
    }

    ~SharedPtr() { Release(); }

    SharedPtr<T>& operator=(const SharedPtr<T>& other) {
        if (this == &other) { return *this; }

        ControlBlockBase* tmp = other.ctrl;
        if (tmp) { ++(tmp->strong); }   // сначала захватить, потом отпустить
        Release();
        ptr = other.ptr;
        ctrl = tmp;

        return *this;
    }

    template <class U>
    void Reset(U* p) {
        if (ptr == p) { return; }
        Release();
        ptr = p;
        ctrl = p ? new ControlBlock<U>(p) : nullptr;
    }

    void Reset() {
        if (!ptr) { return; }
        Release();
        ptr = nullptr;
        ctrl = nullptr;
    }

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    T* Get() const { return ptr; }

    size_t UseCount() const { return ctrl ? ctrl->strong : 0; }
};


// ---------------------------------------------------------
// Специализация для массивов.
// Отличий от основной версии два: блок создаётся как
// ControlBlock<T[]> (внутри него UniquePtr<T[]>, который знает
// про delete[]), и доступ идёт через operator[].
// ---------------------------------------------------------
template <typename T>
class SharedPtr<T[]> {
private:
    T* ptr;
    ControlBlockBase* ctrl;

    void Release() {
        if (ctrl == nullptr) {return;}
        --(ctrl->strong);
        if (ctrl->strong == 0) {
            ctrl->DestroyObject();
            if (ctrl->weak == 0){
                delete ctrl;
            }
        }
        ptr = nullptr;
        ctrl = nullptr;
    }

public:
    SharedPtr() : ptr(nullptr), ctrl(nullptr) {}

    explicit SharedPtr(T* p) : ptr(p), ctrl(p ? new ControlBlock<T[]>(p) : nullptr) {}

    SharedPtr(const SharedPtr<T[]>& other) : ptr(other.ptr), ctrl(other.ctrl) {
        if (ctrl) { ++(ctrl->strong); }
    }

    // Конвертирующего конструктора здесь намеренно нет:
    // массив Derived НЕ является массивом Base (разный sizeof)
    ~SharedPtr() { Release(); }

    SharedPtr<T[]>& operator=(const SharedPtr<T[]>& other) {
        if (this == &other) { return *this; }

        ControlBlockBase* tmp = other.ctrl;
        if (tmp) { ++(tmp->strong); }
        Release();
        ptr = other.ptr;
        ctrl = tmp;

        return *this;
    }
    
    template <class U>
    void Reset(U* p) {
        if (ptr == p) { return; }
        Release();
        ptr = p;
        ctrl = p ? new ControlBlock<U[]>(p) : nullptr;
    }

    void Reset() {
        if (!ptr) { return; }
        Release();
        ptr = nullptr;
        ctrl = nullptr;
    }

    T& operator[](size_t ind) { return ptr[ind]; }
    const T& operator[](size_t ind) const { return ptr[ind]; }

    T* Get() const { return ptr; }

    size_t UseCount() const { return ctrl ? ctrl->strong : 0; }
};


template <typename T>
class WeakPtr {
private:
    T* ptr;
    ControlBlockBase* ctrl;

    void Release() {
        if (ctrl == nullptr){return;}
        --(ctrl->weak);
        if (ctrl->strong == 0 && ctrl->weak == 0){
            delete ctrl;
        }
        ptr = nullptr;
        ctrl = nullptr;
    }

public:
    template <class U> friend class SharedPtr;   // чтобы Lock() мог отдать SharedPtr

    WeakPtr() : ptr(nullptr), ctrl(nullptr) {}
    WeakPtr(const SharedPtr<T>& other) : ptr(other.ptr), ctrl(other.ctrl){
        if (ctrl){
            ++(ctrl->weak);
        }
    }
    WeakPtr(const WeakPtr& other) : ptr(other.ptr), ctrl(other.ctrl){
        if (ctrl){
            ++(ctrl->weak);
        }
    }
    ~WeakPtr(){
        Release();
    }

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other){return *this;}

        ControlBlockBase* tmp = other.ctrl;
        if (tmp) {++(tmp->weak);}
        Release();
        ptr = other.ptr;
        ctrl = tmp;

        return *this;
    }

    bool Expired() const{
        if (!ctrl || ctrl->strong == 0){
            return true;
        }
        return false;
    }

    SharedPtr<T> Lock() const{
        return SharedPtr<T>(*this);
    }
};

template <typename T>
SharedPtr<T>::SharedPtr(const WeakPtr<T>& w) : ptr(nullptr), ctrl(nullptr) {
    // Проверяем блок ИСТОЧНИКА (w.ctrl), а не свой: свой только что обнулён
    // списком инициализации. Если объект уже умер (strong == 0) — остаёмся пустыми,
    // и Lock() честно вернёт пустой SharedPtr.
    if (w.ctrl && w.ctrl->strong > 0) {
        ptr = w.ptr;
        ctrl = w.ctrl;
        ++(ctrl->strong);
    }
}

#endif