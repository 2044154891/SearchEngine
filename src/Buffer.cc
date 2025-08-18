#include <unistd.h>
#include <sys/uio.h> //readv,sendv
#include <errno.h>

#include "Buffer.h"

/*
从fd上读取数据，Poller工作在LT模式
BUffer缓冲区是有大小的 但是从fd上读取数据的时候 却不知道TCp数据的最终大小
从socket读到缓冲区的方法是使用readv先读至buffer_,
Buffer_空间不够会考虑读到栈上的65535个字节大小的空间，然后以append的
方式追加如Buffer_，既考虑了避免系统调用带来开销又不影响数据的接收
*/

ssize_t Buffer::readFd(int fd, int * saveErrno)
{
    char extrabuf[65536] = {0}; //栈上的内存空间 64K

    /*
    struct iovec{
        ptr_t iov_base;
        size_t iov_len
    };*/

    struct iovec vec[2];
    const size_t writable = writableBytes(); //这是buffer底层缓冲区剩余可写空间大小 不一定能完全存储从fd读出的数据

    //第一块缓冲区，指向可写可写空间
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf) ? 2 : 1);
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if( n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if( n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}