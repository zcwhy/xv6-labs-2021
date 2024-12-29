#include "kernel/types.h"
#include "user.h"

#define STACK_SIZE  8192
#define MAX_THREAD  10
#define K_THREAD    1

struct context{
    uint64 ra;
    uint64 sp;

    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;

    uint64 a0;
};

enum threadstate { UNUSED, USED, RUNNABLE, RUNNING, ZOMBIE };

struct vcpu {
    int vid;                // kernel thread id
    struct context context; // sched context
}vcpu[K_THREAD];

struct tcb {
    enum threadstate state;
    int tid;                 // user thread id
    struct context context;
    void (*fn)(void *);
    void *fn_arg;
    char *stack[STACK_SIZE];
} tcb_table[MAX_THREAD];

extern void thread_switch(uint64, uint64);

struct vcpu* mythread(void);

void scheduler() {
    struct tcb* t;
    struct vcpu *thread = mythread(); 
    for(;;) {
        for(t = tcb_table; t < tcb_table + MAX_THREAD; t++) {
            if(t->state == RUNNABLE) {
                t->state = RUNNING;
                printf("[scheduler] a0 addr is %p\n", t->context.a0);
                thread_switch((uint64)&thread->context, (uint64)&t->context);
            }
        }
    }
}

void schedinit() {
    for(int i = 0; i < K_THREAD; i++) {
        create_thread(&vcpu[i].vid, scheduler, 0);
    }
}

void start(void *context) {
    printf("[start] context addr is %p\n", context);
    struct tcb *ctx = (struct tcb*)context;
    printf("before thread routine invoke\n");
    printf("thread routine arg is %d\n", *(int *)ctx->fn_arg);
    (ctx->fn)(ctx->fn_arg);
    printf("after thread routine invoke\n");
    exit(0);
}

struct tcb* allocate_thread() {
    struct tcb* t;
    for(t = tcb_table; t < tcb_table + MAX_THREAD; t++) {
        if(t->state == UNUSED) {
            t->state = USED;
            return t;
        }
    }
    return 0;
}

void pthread_create(int *tidaddr, void (*routine)(void *), void *arg) {
    struct tcb* t = allocate_thread();
    t->fn = routine;
    t->fn_arg = arg;
    printf("thread routine arg is %d\n", *(int *)arg);
    // init state
    t->context.ra = (uint64)start;
    t->context.sp = (uint64)t->stack + STACK_SIZE;
    t->context.a0 = (uint64)t;
    printf("[pthread_create] tcb addr is %p\n", t->context.a0);

    t->state = RUNNABLE;
}

struct vcpu* mythread() {
    int tid = gettid();

    struct vcpu* p;
    for(p = vcpu; p < vcpu + K_THREAD; p++) {
        if(p->vid == tid) {
            return p;
        }
    }
    return 0;
}
