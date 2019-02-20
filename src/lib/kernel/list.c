#include "list.h"
#include "interrupt.h"

// 初始化双向链表
void list_init(struct list* list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

// 将elem元素插入到before元素前
void list_insert_before(struct list_elem* before, struct list_elem* elem)
{
    enum intr_status old_status = intr_disable();

    elem->prev = before->prev;
    elem->next = before;
    before->prev->next = elem;
    before->prev = elem;

    intr_set_status(old_status);
}

// 添加元素到列表队首
void list_push(struct list* list, struct list_elem* elem)
{
    list_insert_before(list->head.next, elem);
}

// 追加元素到列表尾部
void list_append(struct list* list, struct list_elem* elem)
{
    list_insert_before(&list->tail, elem);
}

// 使元素pelem脱离链表
void list_remove(struct list_elem* elem)
{
    enum intr_status old_status = intr_disable();

    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    intr_set_status(old_status);
}

// 将链表的第一个元素弹出并返回，类似栈的pop操作
struct list_elem* list_pop(struct list* list)
{
    struct list_elem* elem = list->head.next;

    list_remove(elem);

    return elem;
}

// 从链表中查找obj_elem，成功时返回true，否则返回false
bool elem_find(struct list* list, struct list_elem* obj_elem)
{
    struct list_elem* elem = list->head.next;
    while (elem != &list->tail)
    {
        if (elem == obj_elem)
            return true;
        elem = elem->next;
    }
    return false;
}

// 在list中寻找符合func条件的第一个节点
struct list_elem* list_traversal(struct list* list, function func, int arg)
{
    struct list_elem* elem = list->head.next;
    if (list_empty(list))
    {
        return NULL;
    }

    while (elem != &list->tail)
    {
        if (func(elem, arg))
            return elem;
        elem = elem->next;
    }
    return NULL;
}

uint32_t list_len(struct list* list)
{
    struct list_elem* elem = list->head.next;
    uint32_t length = 0;
    while (elem != &list->tail)
    {
        ++length;
        elem = elem->next;
    }
    return length;
}

bool list_empty(struct list* list)
{
    return (list->head.next == &list->tail);
}
