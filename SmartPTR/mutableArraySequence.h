#ifndef MUTABLE_ARRAY_SEQUENCE_H
#define MUTABLE_ARRAY_SEQUENCE_H

#include "arraySequence.h"

template<class T> class MutableArraySequence : public ArraySequence<T>{
private:

public:
    virtual MutableArraySequence<T>* Instance() override { 
        return this; 
    }

    virtual MutableArraySequence<T>* CreateNewArray(const T* items, int cnt) const override{
        return new MutableArraySequence<T>(items, cnt);
    }

    virtual Sequence<T>* NewEmpty() const override {
        return new MutableArraySequence<T>();
    }

    MutableArraySequence (const T* items, int count) : ArraySequence<T>(items, count){};
    MutableArraySequence () : ArraySequence<T>(){};
    MutableArraySequence (const MutableArraySequence & seq) : ArraySequence<T>(seq){};
    MutableArraySequence (int cnt) : ArraySequence<T>(cnt){};

};

#endif