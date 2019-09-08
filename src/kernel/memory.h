#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"
#include "sync.h"

// 内存池标记
enum pool_flags
{
    PF_KERNEL,
    PF_USER
};

#define PG_P_1  1    // present属性位，1表示存在
#define PG_P_0  0    // 表示不存在
#define PG_RW_R 0    // R/W属性位，读/执行
#define PG_RW_W 2    // R/W属性位，读/写/执行
#define PG_US_S 0    // U/S属性位，系统级
#define PG_US_U 4    // U/S属性位，用户级

// 内存池结构
struct pool
{
    struct bitmap pool_bitmap;               // 用于管理内存的位图
    uint32_t phy_addr_start;                 // 所管理内存的起始物理地址
    uint32_t pool_size;                      // 本内存池字节容量
    struct lock lock;
};

// 虚拟地址池，用于管理虚拟地址
struct virtual_addr
{
    struct bitmap vaddr_bitmap;              // 虚拟地址用到的位图结构
    uint32_t vaddr_start;                    // 虚拟地址起始地址
};

// 内存块
struct mem_block
{
    struct list_elem free_elem;
};

// 内存块描述符
struct mem_block_desc
{
    uint32_t block_size;
    uint32_t blocks_per_arena;
    struct list free_list;
};

#define DESC_CNT 7

extern struct pool kernel_pool, user_pool;

void mem_init(void);

void block_desc_init(struct mem_block_desc* desc_array);

uint32_t* pte_ptr(void* vaddr);
uint32_t* pde_ptr(void* vaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(void* vaddr);

#endif // __KERNEL_MEMORY_H