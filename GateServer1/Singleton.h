#pragma once
#include <mutex>
#include <iostream>

template <typename T>
class Singleton
{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton<T>& operator=(const Singleton<T>&) = delete;
    static std::shared_ptr<T> _instance;
private:
    static std::once_flag _flag;
public:

    // 类内自动inline，多DLL加载时，会重复创建对象
    static std::shared_ptr<T> getInstance();
    void printAddress()
    {
        std::cout << "Singleton address: " << _instance.get() << std::endl;
    }
    ~Singleton()
    {
        std::cout << "~Singleton()" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

template <typename T>
std::once_flag Singleton<T>::_flag;

