#pragma once

class NonCopyable
{
public:
    //禁止
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};