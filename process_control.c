#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "process_control.h"
#include "visualization.h"

// 全局变量
static int next_pid = 1;
static int current_time = 0;

// 创建一个新的实际进程
PCB* create_process(char *name, int priority, int service_time) {
    // 分配PCB内存
    PCB *new_process = (PCB*)malloc(sizeof(PCB));
    if (new_process == NULL) {
        perror("内存分配失败");
        return NULL;
    }
    
    // 初始化PCB
    new_process->pid = next_pid++;
    strncpy(new_process->name, name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';
    new_process->status = PROCESS_READY;
    new_process->priority = priority;
    new_process->arrive_time = current_time;
    new_process->service_time = service_time;
    new_process->remaining_time = service_time;
    new_process->completion_time = 0;
    new_process->turnaround_time = 0;
    new_process->weighted_turnaround = 0.0f;
    new_process->waiting_time = 0;
    new_process->next = NULL;
    
    print_colored("\n创建进程: PID=%d, 名称=%s, 优先级=%d, 服务时间=%d\n", 
                  GREEN, new_process->pid, name, priority, service_time);
    
    // 创建实际进程
    pid_t child_pid = fork();
    
    if (child_pid < 0) {
        // 进程创建失败
        print_colored("进程创建失败: %s\n", RED, name);
        free(new_process);
        return NULL;
    } else if (child_pid == 0) {
        // 子进程代码
        print_colored("子进程[%d] 启动, 模拟执行任务: %s\n", CYAN, getpid(), name);
        
        // 模拟进程执行
        int sleep_time = service_time;
        while (sleep_time > 0) {
            print_colored("进程[%d] %s: 剩余执行时间 %d\n", CYAN, getpid(), name, sleep_time);
            sleep(1);  // 模拟执行1秒
            sleep_time--;
        }
        
        print_colored("子进程[%d] %s 执行完成，退出\n", GREEN, getpid(), name);
        exit(0);  // 子进程完成并退出
    } else {
        // 父进程代码
        new_process->unix_pid = child_pid;
        print_colored("父进程: 已创建子进程[%d] 对应PCB PID=%d\n", 
                      YELLOW, child_pid, new_process->pid);
    }
    
    return new_process;
}

// 终止进程并回收资源
void terminate_process(PCB *process) {
    if (process == NULL) return;
    
    print_colored("\n终止进程: PID=%d, 名称=%s\n", YELLOW, process->pid, process->name);
    
    // 等待子进程结束
    if (process->unix_pid > 0) {
        int status;
        print_colored("等待子进程[%d]结束...\n", CYAN, process->unix_pid);
        pid_t result = waitpid(process->unix_pid, &status, 0);
        
        if (result > 0) {
            if (WIFEXITED(status)) {
                print_colored("子进程[%d]正常结束，退出码: %d\n", 
                             GREEN, result, WEXITSTATUS(status));
            } else {
                print_colored("子进程[%d]异常结束\n", RED, result);
            }
        } else {
            perror("waitpid错误");
        }
    }
    
    process->status = PROCESS_TERMINATED;
    print_colored("进程[%d] %s 已终止并回收资源\n", MAGENTA, process->pid, process->name);
}

// 创建模拟进程（仅用于调度算法演示，不创建实际进程）
PCB* create_simulated_process(char *name, int priority, int arrive_time, int service_time) {
    PCB *new_process = (PCB*)malloc(sizeof(PCB));
    if (new_process == NULL) {
        perror("内存分配失败");
        return NULL;
    }
    
    // 初始化PCB
    new_process->pid = next_pid++;
    strncpy(new_process->name, name, sizeof(new_process->name) - 1);
    new_process->name[sizeof(new_process->name) - 1] = '\0';
    new_process->status = PROCESS_READY;
    new_process->priority = priority;
    new_process->arrive_time = arrive_time;
    new_process->service_time = service_time;
    new_process->remaining_time = service_time;
    new_process->completion_time = 0;
    new_process->turnaround_time = 0;
    new_process->weighted_turnaround = 0.0f;
    new_process->waiting_time = 0;
    new_process->unix_pid = -1;  // 模拟进程没有真实UNIX PID
    new_process->next = NULL;
    
    return new_process;
}

// 创建队列
ProcessQueue* create_queue() {
    ProcessQueue *queue = (ProcessQueue*)malloc(sizeof(ProcessQueue));
    if (queue == NULL) {
        perror("队列创建失败");
        return NULL;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    
    return queue;
}

// 将进程添加到队列末尾
void enqueue(ProcessQueue *queue, PCB *process) {
    if (queue == NULL || process == NULL) return;
    
    process->next = NULL;
    
    if (queue->head == NULL) {
        queue->head = process;
        queue->tail = process;
    } else {
        queue->tail->next = process;
        queue->tail = process;
    }
    
    queue->count++;
    print_colored("进程[%d] %s 加入队列\n", BLUE, process->pid, process->name);
}

// 从队列头取出进程
PCB* dequeue(ProcessQueue *queue) {
    if (queue == NULL || queue->head == NULL) return NULL;
    
    PCB *process = queue->head;
    queue->head = process->next;
    
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    
    process->next = NULL;
    queue->count--;
    
    print_colored("进程[%d] %s 离开队列\n", BLUE, process->pid, process->name);
    return process;
}

// 查看队列头部进程但不移除
PCB* peek_queue(ProcessQueue *queue) {
    if (queue == NULL || queue->head == NULL) return NULL;
    return queue->head;
}

// 从队列中移除特定进程
void remove_process(ProcessQueue *queue, int pid) {
    if (queue == NULL || queue->head == NULL) return;
    
    PCB *current = queue->head;
    PCB *previous = NULL;
    
    // 如果是队列头
    if (current->pid == pid) {
        queue->head = current->next;
        if (queue->head == NULL) {
            queue->tail = NULL;
        }
        queue->count--;
        print_colored("进程[%d] %s 从队列中移除\n", BLUE, current->pid, current->name);
        return;
    }
    
    // 搜索队列
    while (current != NULL && current->pid != pid) {
        previous = current;
        current = current->next;
    }
    
    // 如果找到了进程
    if (current != NULL) {
        previous->next = current->next;
        if (current->next == NULL) {
            queue->tail = previous;
        }
        current->next = NULL;
        queue->count--;
        print_colored("进程[%d] %s 从队列中移除\n", BLUE, current->pid, current->name);
    }
}

// 销毁队列
void destroy_queue(ProcessQueue *queue) {
    if (queue == NULL) return;
    
    PCB *current = queue->head;
    PCB *next;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    
    free(queue);
}

// 模拟进程执行特定时间
void simulate_process_execution(PCB *process, int time) {
    if (process == NULL || time <= 0) return;
    
    // 设置为运行状态
    int prev_status = process->status;
    process->status = PROCESS_RUNNING;
    
    // 计算实际执行时间（不超过剩余时间）
    int actual_time = (time < process->remaining_time) ? time : process->remaining_time;
    
    print_colored("执行进程[%d] %s，执行时间: %d\n", 
                 CYAN, process->pid, process->name, actual_time);
    
    // 模拟CPU执行
    for (int i = 0; i < actual_time; i++) {
        print_colored("\r[", WHITE);
        for (int j = 0; j < 20; j++) {
            if (j < (i+1) * 20 / actual_time) {
                print_colored("=", GREEN);
            } else {
                print_colored(" ", WHITE);
            }
        }
        print_colored("] %d%%", WHITE, (i+1) * 100 / actual_time);
        fflush(stdout);
        usleep(200000);  // 减慢速度，便于观察
    }
    print_colored("\n", WHITE);
    
    // 更新进程剩余时间
    process->remaining_time -= actual_time;
    
    // 检查进程是否完成
    if (process->remaining_time <= 0) {
        process->status = PROCESS_TERMINATED;
        print_colored("进程[%d] %s 已完成执行\n", GREEN, process->pid, process->name);
    } else {
        // 恢复之前的状态（通常是就绪状态）
        process->status = prev_status;
        print_colored("进程[%d] %s 暂停执行，剩余时间: %d\n", 
                     YELLOW, process->pid, process->name, process->remaining_time);
    }
}

// 打印进程信息
void print_process_info(PCB *process) {
    if (process == NULL) return;
    
    const char *status_str[] = {"就绪", "运行", "阻塞", "终止"};
    Color status_color[] = {BLUE, GREEN, YELLOW, RED};
    
    print_colored("进程ID: %d, ", WHITE, process->pid);
    print_colored("名称: %s, ", WHITE, process->name);
    print_colored("状态: ", WHITE);
    print_colored("%s, ", status_color[process->status], status_str[process->status]);
    print_colored("优先级: %d, ", WHITE, process->priority);
    print_colored("到达时间: %d, ", WHITE, process->arrive_time);
    print_colored("服务时间: %d, ", WHITE, process->service_time);
    print_colored("剩余时间: %d\n", WHITE, process->remaining_time);
    
    if (process->status == PROCESS_TERMINATED) {
        print_colored("完成时间: %d, ", WHITE, process->completion_time);
        print_colored("周转时间: %d, ", WHITE, process->turnaround_time);
        print_colored("带权周转时间: %.2f, ", WHITE, process->weighted_turnaround);
        print_colored("等待时间: %d\n", WHITE, process->waiting_time);
    }
}

// 打印队列中的所有进程
void print_queue(ProcessQueue *queue) {
    if (queue == NULL) {
        print_colored("队列不存在\n", RED);
        return;
    }
    
    if (queue->head == NULL) {
        print_colored("队列为空\n", YELLOW);
        return;
    }
    
    print_colored("队列中的进程 (总数: %d):\n", CYAN, queue->count);
    print_colored("----------------------------------------------------------\n", WHITE);
    
    PCB *current = queue->head;
    while (current != NULL) {
        print_process_info(current);
        current = current->next;
    }
    
    print_colored("----------------------------------------------------------\n", WHITE);
}

// 计算进程统计信息（周转时间、等待时间等）
void calculate_statistics(PCB *process_list, int count) {
    if (process_list == NULL || count <= 0) return;
    
    float avg_turnaround = 0.0f;
    float avg_weighted_turnaround = 0.0f;
    float avg_waiting = 0.0f;
    
    print_colored("\n进程执行统计信息：\n", CYAN);
    print_colored("-----------------------------------------------------------------\n", WHITE);
    print_colored("| %-3s | %-10s | %-4s | %-4s | %-4s | %-4s | %-4s | %-4s |\n", 
                 WHITE, "PID", "名称", "到达", "服务", "完成", "周转", "带权", "等待");
    print_colored("-----------------------------------------------------------------\n", WHITE);
    
    for (int i = 0; i < count; i++) {
        PCB *p = &process_list[i];
        
        print_colored("| %-3d | %-10s | %-4d | %-4d | %-4d | %-4d | %-4.1f | %-4d |\n", 
                     WHITE, p->pid, p->name, p->arrive_time, p->service_time,
                     p->completion_time, p->turnaround_time, p->weighted_turnaround, p->waiting_time);
        
        avg_turnaround += p->turnaround_time;
        avg_weighted_turnaround += p->weighted_turnaround;
        avg_waiting += p->waiting_time;
    }
    
    print_colored("-----------------------------------------------------------------\n", WHITE);
    
    avg_turnaround /= count;
    avg_weighted_turnaround /= count;
    avg_waiting /= count;
    
    print_colored("平均周转时间: %.2f\n", YELLOW, avg_turnaround);
    print_colored("平均带权周转时间: %.2f\n", YELLOW, avg_weighted_turnaround);
    print_colored("平均等待时间: %.2f\n", YELLOW, avg_waiting);
}

// 可视化进程执行时间线
void visualize_execution_timeline(PCB *process_list, int count, int total_time) {
    if (process_list == NULL || count <= 0 || total_time <= 0) return;
    
    const int WIDTH = 60;  // 总宽度
    const int name_width = 10;  // 进程名宽度
    
    print_colored("\n进程执行时间线 (总时间: %d)：\n", CYAN, total_time);
    
    // 生成时间刻度
    print_colored("%-*s", WHITE, name_width, "时间");
    for (int t = 0; t <= total_time; t += total_time / 10) {
        int pos = t * WIDTH / total_time;
        print_colored("%-*d", WHITE, WIDTH / 10, t);
    }
    print_colored("\n", WHITE);
    
    // 为每个进程绘制时间线
    for (int i = 0; i < count; i++) {
        PCB *p = &process_list[i];
        
        // 打印进程名
        print_colored("%-*s", WHITE, name_width, p->name);
        
        char timeline[WIDTH + 1];
        memset(timeline, '.', WIDTH);
        timeline[WIDTH] = '\0';
        
        // 标记到达时间
        int arrive_pos = p->arrive_time * WIDTH / total_time;
        if (arrive_pos >= 0 && arrive_pos < WIDTH) {
            timeline[arrive_pos] = '>';
        }
        
        // 标记完成时间
        int complete_pos = p->completion_time * WIDTH / total_time;
        if (complete_pos >= 0 && complete_pos < WIDTH) {
            timeline[complete_pos] = '<';
        }
        
        // 标记执行段
        int start_pos = -1;
        int in_execution = 0;
        
        for (int t = 0; t < WIDTH; t++) {
            int time = t * total_time / WIDTH;
            
            // 检查在此时间点进程是否在运行
            // 这部分需要实际调度算法的输出来确定
            // 此处只是一个简化的示例
            if (time >= p->arrive_time && time < p->completion_time) {
                if (!in_execution) {
                    in_execution = 1;
                    start_pos = t;
                }
            } else {
                if (in_execution) {
                    in_execution = 0;
                    for (int j = start_pos; j < t; j++) {
                        timeline[j] = '=';
                    }
                }
            }
        }
        
        // 打印时间线
        for (int t = 0; t < WIDTH; t++) {
            char c = timeline[t];
            switch(c) {
                case '>':
                    print_colored("%c", GREEN, c);
                    break;
                case '<':
                    print_colored("%c", RED, c);
                    break;
                case '=':
                    print_colored("%c", CYAN, c);
                    break;
                default:
                    print_colored("%c", WHITE, c);
            }
        }
        print_colored("\n", WHITE);
    }
}