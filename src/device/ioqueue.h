#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H

#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define BUFSIZE 64

// 环形队列
struct ioqueue
{
    struct lock lock;                        // 锁
    struct task_struct* producer;            // 生产者
    struct task_struct* consumer;            // 消费者
    char buf[BUFSIZE];                       // 队列
    int32_t head;                            // 队首index
    int32_t tail;                            // 对尾index
};

void ioqueue_init(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);

#endif // __DEVICE_IOQUEUE_H