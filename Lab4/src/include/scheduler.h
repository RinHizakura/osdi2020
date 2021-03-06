#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#define THREAD_CPU_CONTEXT 0 	// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__
#define NR_TASKS 64 
#define THREAD_SIZE  4096

#define TASK_RUNNING	0
#define TASK_ZOMBIE     1

#define current get_current()

extern int uart_read_lock;
extern struct task_struct *task[NR_TASKS];

struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long sp;
    unsigned long pc;
};

struct signal_struct{
	int pending;
	int block;
};

struct task_struct{
	struct cpu_context cpu_context;
	struct signal_struct signal;

	int pid;
	long state;
	long priority;
	long counter;
	long preempt_lock;
	unsigned long stack;
	
};

extern void switch_to(struct task_struct* prev, struct task_struct* next);
extern struct task_struct* get_current();
extern void init_idle_task(struct task_struct* task);

extern void schedule(void);
extern void context_switch(struct task_struct* next);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void schedule_tail(void);
extern void timer_tick();
extern void exit_process();
extern int  num_runnable_tasks();

#define IDLE_TASK { {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	{0,0}, \
	0,0,1,0,0,0} 

#endif
#endif /*_SCHEDULER_H */
