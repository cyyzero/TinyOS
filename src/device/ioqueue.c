#include "ioqueue.h"
#include "interrupt.h"
#include "global.h"
#include "debug.h"
#include "stdint.h"

// 获取pos下一个下标
#define NEXT_POS(pos) (((pos) + 1) % BUFSIZE)

// 队列初始化
void ioqueue_init(struct ioqueue* ioq)
{
    lock_init(&ioq->lock);
    ioq->producer = ioq->consumer = NULL;
    ioq->head = ioq->tail = 0;
}

// 判断队列是否已满
bool ioq_full(struct ioqueue* ioq)
{
    ASSERT(intr_get_status() == INTR_OFF);
    return NEXT_POS(ioq->head) == ioq->tail;
}

// 判断队列是否已空
bool ioq_empty(struct ioqueue* ioq)
{
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}

// 使当前生产者或消费者在此缓冲区上等待
static void ioq_wait(struct task_struct** waiter)
{
    ASSERT(waiter != NULL && *waiter == NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

// 唤醒waiter
static void ioq_wakeup(struct task_struct** waiter)
{
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

// 消费者从ioq队列中获取一个字符
char ioq_getchar(struct ioqueue* ioq)
{
    ASSERT(intr_get_status() == INTR_OFF);

    while (ioq_empty(ioq))
    {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char byte = ioq->buf[ioq->tail];
    ioq->tail = NEXT_POS(ioq->tail);

    if (ioq->producer != NULL)
    {
        ioq_wakeup(&ioq->producer);
    }

    return byte;
}

// 生产者往ioq队列里写入一个字符byte
void ioq_putchar(struct ioqueue* ioq, char byte)
{
    ASSERT(intr_get_status() == INTR_OFF);

    while (ioq_full(ioq))
    {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }

    ioq->buf[ioq->head] = byte;
    ioq->head = NEXT_POS(ioq->head);

    if (ioq->consumer != NULL)
    {
        ioq_wakeup(&ioq->consumer);
    }
}