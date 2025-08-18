#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stddef.h>


//网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; //初始预留的prependabelk
    static const size_t kInitialSize = 1024; 

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(initialSize + kCheapPrepend)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    //返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readerIndex_; }

    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len; //说明只读取了可读缓冲区的部分数据，len长度
        }
        else //len === readableBytes()
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); //对缓冲区进行复位操作
        return result;
    }


    //把onmessage函数上的buffer数据 转化成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }


    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len); //扩容
        }
    }

    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int * saveErrno);
    //通过fd发送数据
    ssize_t writeFd(int fd, int * saveErrno);
private:
    //vector底层数组首元素的地址 也就是数组的起始地址
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
         /**
         * | kCheapPrepend |xxx| reader | writer |                     // xxx标示reader中已读的部分
         * | kCheapPrepend | reader ｜          len          |
         **/
        if(writableBytes() + prependableBytes() < len + kCheapPrepend) //存储空间不够
        {
            buffer_.resize(writerIndex_ + len);
        }
        else //说明 len <= xxx + write 把reader搬到从xxx开始 使得xxx后面时一段连续的空间
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};