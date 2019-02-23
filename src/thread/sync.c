#include "sync.h"

// 初始化信号量
void sema_init(struct semaphore* psema, uint8_t value)
{
    psema->value = value;
    list_init(&psema->waiters);
}

// 初始化锁lock
void lock_init(struct lock* plock)
{
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);
}

// void sema_down(struct semaphore* psema)
// {

// }