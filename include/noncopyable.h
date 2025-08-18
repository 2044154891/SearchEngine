#pragma once // 防止头文件被多次包含
//noncopyable 被继承后 派生类对象可正常构造和析构，但派生类对象不能被复制或赋值
class noncopyable 
{
public:
    // 禁止复制构造函数和赋值运算符
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    // 默认构造函数和析构函数
    noncopyable() = default;
    ~noncopyable() = default;
};