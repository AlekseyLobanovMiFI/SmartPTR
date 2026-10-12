#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H

template <class T>
struct UniversalDelete{
    void operator()(T* ptr) const{
        delete ptr;
    }

};

template <class T>
struct UniversalDelete<T[]>{
    void operator()(T* ptr) const{
        delete[] ptr;
    }

};


template <typename T>
class UniquePtr {
private:
    T* ptr;

public:
    template <class U> friend class UniquePtr;

    template <class U>
    UniquePtr(UniquePtr<U>&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }


    explicit UniquePtr(T* p = nullptr) : ptr(p) {}
    ~UniquePtr() { UniversalDelete<T>()(ptr); }

    UniquePtr(const UniquePtr<T>&) = delete;
    UniquePtr<T>& operator=(const UniquePtr<T>&) = delete;

    UniquePtr(UniquePtr<T>&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    UniquePtr<T>& operator=(UniquePtr<T>&& other) noexcept {
        if (this != &other) {
            UniversalDelete<T>()(ptr);
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    T* Get() const { return ptr; }

    T* Release() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void Reset(T* p = nullptr) {
        if (ptr != p) {
            UniversalDelete<T>()(ptr);
            ptr = p;
        }
    }
};

template <typename T>
class UniquePtr<T[]> {
private:
    T* ptr;

public:
    explicit UniquePtr(T* p = nullptr) : ptr(p) {}
    ~UniquePtr() { UniversalDelete<T[]>()(ptr); }

    UniquePtr(const UniquePtr<T[]>&) = delete;
    UniquePtr<T[]>& operator=(const UniquePtr<T[]>&) = delete;

    UniquePtr(UniquePtr<T[]>&& other) noexcept : ptr(other.ptr){
        other.ptr = nullptr;
    }

    UniquePtr<T[]>& operator=(UniquePtr<T[]>&& other) noexcept {
        if (this != &other) {
            UniversalDelete<T[]>()(ptr);
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    T& operator[](size_t ind){
        return ptr[ind];
    }
    const T& operator[](size_t ind) const{
        return ptr[ind];
    }

    T* Get() const { return ptr; }

    T* Release() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void Reset(T* p = nullptr) {
        if (ptr != p) {
            UniversalDelete<T[]>()(ptr);
            ptr = p;
        }
    }
};

#endif