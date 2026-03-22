#ifndef SINGLETON_H
#define SINGLETON_H


template<typename T>
class Singleton
{
public:
    static T& instant(){
        static T s;
        return s;
    }

    Singleton(T&&) = delete;
    Singleton(const T&) = delete;
    void operator= (const T&) = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

#endif // SINGLETON_H
