#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H

#include "global.h"

#define DEFAULT_PRIO 31
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000

struct task_struct;

void process_execute(void* filename, const char* name);
void start_process(void* filename);
void process_activate(struct task_struct* p_thread);
void page_dir_activate(struct task_struct* p_thread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);

#endif // __USERPROG_PROCESS_H
