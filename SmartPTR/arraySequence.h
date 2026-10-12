#ifndef ARRAY_SEQUENCE_H
#define ARRAY_SEQUENCE_H

#include "sequence.h"
#include "dynamicArray.h"

template <class T> class ArraySequence : public Sequence<T>{
private:

    DynamicArray<T> array;

protected:

    T& operator[](int index){return array[index];}
    void ResizeSelf(int newSize){
        array.Resize(newSize);
    }

public:

    virtual ArraySequence<T>* Instance() = 0;
    
    virtual ArraySequence<T>* CreateNewArray(const T*, int) const =0;


    T GetFirst() const override{
        return array.Get(0);
    }

    T GetLast() const override{
        return array.Get(array.GetSize() - 1);
    }

    T Get(int index) const override{
        return array.Get(index);
    }

    const T& operator[](int index) const{return array[index];}

    ArraySequence<T>* GetSubsequence(int startIndex, int endIndex) const override {
        if (startIndex < 0 || endIndex >= array.GetSize() || startIndex > endIndex) {
            throw std::out_of_range("IndexOutOfRange");
        }

        return CreateNewArray(&array[startIndex], endIndex - startIndex + 1);
    }

    int GetLength() const override{
        return array.GetSize();
    }

    ArraySequence<T>* Append(const T& item) override{
        ArraySequence<T>* result = Instance();
        result->array.Resize(result->array.GetSize()+1);
        result->array.Set(result->array.GetSize()-1, item);
        return result;
    }

    ArraySequence<T>* Prepend(const T& item) override{
        ArraySequence<T>* result = Instance();
        DynamicArray<T>& res_arr = result->array;
        res_arr.Resize(res_arr.GetSize()+1);
        for (int i=res_arr.GetSize()-1; i>0; i--){
            res_arr.Set(i, res_arr.Get(i-1));
        }
        res_arr.Set(0, item);
        return result;

    }

    ArraySequence<T>* InsertAt(const T& item, int index) override{
        if (index < 0 || index > array.GetSize()){
            throw std::out_of_range("IndexOutOfRange");
        }
        ArraySequence<T>* result = Instance();
        DynamicArray<T>& res_arr = result->array;
        res_arr.Resize(res_arr.GetSize()+1);
        for (int i=res_arr.GetSize()-1; i>index; i--){
            res_arr.Set(i, res_arr.Get(i-1));
        }
        res_arr.Set(index, item);
        return result;
    } 

    ArraySequence (const T* items, int count) : array(items, count){};
    ArraySequence () : array(0){};
    ArraySequence (const ArraySequence <T> & arSeq) : array(arSeq.array){};
    ArraySequence (int cnt) : array(cnt){};

};

#endif