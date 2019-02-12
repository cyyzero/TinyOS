#ifndef __KERNEL_INTERUPT_H
#define __KERNEL_INTERUPT_H

typedef void* intr_handler;

void idt_init(void);

#endif // __INTERUPT_H