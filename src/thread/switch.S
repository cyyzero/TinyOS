[bits 32]
section .text
global switch_to
; void switch_to(struct task_struct* curr, struct task_struct* next)
switch_to:
    ; 栈中此处是返回地址
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20]                      ; 得到参数curr
    mov [eax], esp                           ; 保存栈顶指针esp到 curr->self_kstack

    ; 上面保存环境；下面回复环境

    mov eax, [esp + 24]                      ; 得到参数next
    mov esp, [eax]                           ; 将next->self_kstack给esp

    pop ebp
    pop ebx
    pop edi
    pop esi

    ret