#ifndef PROCESS_CONTROL_H
#define PROCESS_CONTROL_H

#include <sys/types.h>
#include <unistd.h>

// 进程状态定义
#define PROCESS_READY 0
#define PROCESS_RUNNING 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3

// 进程控制块 (PCB) 结构
typedef struct PCB {
    int pid;                // 进程ID
    char name[32];          // 进程名称
    int status;             // 进程状态(0-就绪, 1-运行, 2-阻塞, 3-终止)
    int priority;           // 优先级 (值越大优先级越高)
    int arrive_time;        // 到达时间
    int service_time;       // 需要的服务时间 (总时间)
    int remaining_time;     // 剩余执行时间
    int completion_time;    // 完成时间
    int turnaround_time;    // 周转时间
    float weighted_turnaround; // 带权周转时间
    int waiting_time;       // 等待时间
    pid_t unix_pid;         // 真实UNIX进程ID
    struct PCB *next;       // 链表指针
} PCB;

// 进程队列结构
typedef struct {
    PCB *head;
    PCB *tail;
    int count;
} ProcessQueue;

// 进程控制函数
PCB* create_process(char *name, int priority, int service_time);
void terminate_process(PCB *process);
PCB* create_simulated_process(char *name, int priority, int arrive_time, int service_time);

// 队列操作函数
ProcessQueue* create_queue();
void enqueue(ProcessQueue *queue, PCB *process);
PCB* dequeue(ProcessQueue *queue);
PCB* peek_queue(ProcessQueue *queue);
void remove_process(ProcessQueue *queue, int pid);
void destroy_queue(ProcessQueue *queue);

// 进程执行函数
void simulate_process_execution(PCB *process, int time);
void print_process_info(PCB *process);
void print_queue(ProcessQueue *queue);

// 进程统计函数
void calculate_statistics(PCB *process_list, int count);
void visualize_execution_timeline(PCB *process_list, int count, int total_time);

#endif // PROCESS_CONTROL_H