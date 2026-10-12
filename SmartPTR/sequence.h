#ifndef SEQUENCE_H
#define SEQUENCE_H

template <class T> class Sequence{
protected:
    virtual void AppendImple(const T& item){
        Append(item);
    }
public:
    virtual T GetFirst() const=0;//абстрактная функция
    virtual T GetLast() const=0;
    virtual T Get(int index) const=0; 
    virtual Sequence<T>*GetSubsequence(int startIndex, int endIndex) const=0;
    virtual int GetLength() const=0;

    virtual Sequence<T>*Append(const T& item)=0;
    virtual Sequence<T>*Prepend(const T& item)=0;
    virtual Sequence<T>*InsertAt(const T& item, int index)=0; 

    virtual ~Sequence(){};

    virtual Sequence<T>* Concat(Sequence<T>* const seq) {

        for (int i = 0; i < seq->GetLength(); i++) {
            Append(seq->Get(i));
        }

        return this;
    }

    virtual Sequence<T>* NewEmpty() const =0;

    virtual Sequence <T>* Map(T (*func)(const T&)) const{
        Sequence <T>* result = NewEmpty();

        for (int i = 0; i < GetLength(); i++) {
            result->AppendImple(func(Get(i)));
        }

        return result;

    }

    virtual Sequence <T>* Where(bool (*func)(const T&)) const{
        Sequence <T>* result = NewEmpty();

        for (int i = 0; i < GetLength(); i++) {
            T item = Get(i);
            if (func(item)){
                result->AppendImple(item);
            }
        }

        return result;

    }

    virtual T Reduce(T init, T (*func)(const T&,const T&)) const{
        T res = init;

        for (int i = 0; i < GetLength(); i++) {
            res = func(res, Get(i));
        }

        return res;

    }



    //Option<T> try GetFirst(bool (*func)(T) = 0);
    //Option<T> GetLast(bool (*func)(T)  = 0); 
};

#endif