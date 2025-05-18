#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "process_control.h"
#include "visualization.h"

// 模拟时钟
static int simulation_clock = 0;

// 创建测试进程集
PCB* create_test_processes(int *count) {
    // 预定义测试数据，格式：名称、优先级、到达时间、服务时间
    struct {
        char name[32];
        int priority;
        int arrive_time;
        int service_time;
    } test_data[] = {
        {"进程A", 3, 0, 3},
        {"进程B", 5, 1, 6},
        {"进程C", 1, 3, 4},
        {"进程D", 4, 5, 2},
        {"进程E", 2, 6, 5}
    };
    
    *count = sizeof(test_data) / sizeof(test_data[0]);
    PCB *processes = (PCB*)malloc(sizeof(PCB) * (*count));
    
    for (int i = 0; i < *count; i++) {
        strcpy(processes[i].name, test_data[i].name);
        processes[i].pid = i + 1;
        processes[i].status = PROCESS_READY;
        processes[i].priority = test_data[i].priority;
        processes[i].arrive_time = test_data[i].arrive_time;
        processes[i].service_time = test_data[i].service_time;
        processes[i].remaining_time = test_data[i].service_time;
        processes[i].completion_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].weighted_turnaround = 0.0f;
        processes[i].waiting_time = 0;
        processes[i].unix_pid = -1;  // 模拟进程
        processes[i].next = NULL;
    }
    
    return processes;
}

// 先来先服务 (FCFS) 调度算法
void FCFS_scheduler(PCB *processes, int count) {
    clear_screen();
    print_title("先来先服务 (FCFS) 调度算法模拟");
    
    // 创建就绪队列
    ProcessQueue *ready_queue = create_queue();
    
    // 按到达时间排序进程
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (processes[j].arrive_time < processes[i].arrive_time) {
                PCB temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }
    
    print_colored("初始进程状态:\n", CYAN);
    for (int i = 0; i < count; i++) {
        print_process_info(&processes[i]);
    }
    
    print_colored("\n开始FCFS调度模拟...\n", YELLOW);
    
    // 模拟时钟
    simulation_clock = 0;
    int completed = 0;
    int current_process_idx = -1;
    
    while (completed < count) {
        // 检查新到达的进程
        for (int i = 0; i < count; i++) {
            if (processes[i].arrive_time == simulation_clock && processes[i].status == PROCESS_READY) {
                print_colored("时间 %d: 进程[%d] %s 到达系统\n", 
                             BLUE, simulation_clock, processes[i].pid, processes[i].name);
                enqueue(ready_queue, &processes[i]);
            }
        }
        
        // 如果当前没有正在执行的进程，从队列中取一个
        if (current_process_idx == -1) {
            PCB *next = dequeue(ready_queue);
            if (next != NULL) {
                for (int i = 0; i < count; i++) {
                    if (processes[i].pid == next->pid) {
                        current_process_idx = i;
                        break;
                    }
                }
                
                print_colored("时间 %d: 调度进程[%d] %s 开始执行\n", 
                             GREEN, simulation_clock, processes[current_process_idx].pid, 
                             processes[current_process_idx].name);
                processes[current_process_idx].status = PROCESS_RUNNING;
            }
        }
        
        // 如果有进程在执行，模拟其执行
        if (current_process_idx != -1) {
            // 执行一个时间单位
            processes[current_process_idx].remaining_time--;
            
            print_colored("时间 %d: 进程[%d] %s 正在执行，剩余时间: %d\n", 
                         CYAN, simulation_clock, processes[current_process_idx].pid, 
                         processes[current_process_idx].name, 
                         processes[current_process_idx].remaining_time);
            
            // 检查是否完成
            if (processes[current_process_idx].remaining_time <= 0) {
                processes[current_process_idx].status = PROCESS_TERMINATED;
                processes[current_process_idx].completion_time = simulation_clock + 1;
                
                // 计算统计信息
                processes[current_process_idx].turnaround_time = 
                    processes[current_process_idx].completion_time - 
                    processes[current_process_idx].arrive_time;
                    
                processes[current_process_idx].weighted_turnaround = 
                    (float)processes[current_process_idx].turnaround_time / 
                    processes[current_process_idx].service_time;
                    
                processes[current_process_idx].waiting_time = 
                    processes[current_process_idx].turnaround_time - 
                    processes[current_process_idx].service_time;
                
                print_colored("时间 %d: 进程[%d] %s 执行完成\n", 
                             GREEN, simulation_clock + 1, processes[current_process_idx].pid, 
                             processes[current_process_idx].name);
                
                completed++;
                current_process_idx = -1;
            }
        }
        
        // 时钟前进
        simulation_clock++;
        usleep(500000);  // 放慢显示速度
    }
    
    print_colored("\nFCFS调度完成，所有进程已执行完毕\n", YELLOW);
    
    // 打印统计信息
    calculate_statistics(processes, count);
    
    // 可视化时间线
    visualize_execution_timeline(processes, count, simulation_clock);
    
    // 清理资源
    destroy_queue(ready_queue);
}

// 时间片轮转 (RR) 调度算法
void RR_scheduler(PCB *processes, int count, int time_quantum) {
    clear_screen();
    print_title("时间片轮转 (RR) 调度算法模拟");
    print_colored("时间片大小: %d\n", YELLOW, time_quantum);
    
    // 创建就绪队列
    ProcessQueue *ready_queue = create_queue();
    
    // 按到达时间排序进程
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (processes[j].arrive_time < processes[i].arrive_time) {
                PCB temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }
    
    print_colored("初始进程状态:\n", CYAN);
    for (int i = 0; i < count; i++) {
        print_process_info(&processes[i]);
    }
    
    print_colored("\n开始RR调度模拟...\n", YELLOW);
    
    // 模拟时钟
    simulation_clock = 0;
    int completed = 0;
    int current_process_idx = -1;
    int time_slice_used = 0;
    
    while (completed < count) {
        // 检查新到达的进程
        for (int i = 0; i < count; i++) {
            if (processes[i].arrive_time == simulation_clock && processes[i].status == PROCESS_READY) {
                print_colored("时间 %d: 进程[%d] %s 到达系统\n", 
                             BLUE, simulation_clock, processes[i].pid, processes[i].name);
                enqueue(ready_queue, &processes[i]);
            }
        }
        
        // 如果当前没有进程执行或者时间片用完，调度下一个进程
        if (current_process_idx == -1 || time_slice_used >= time_quantum) {
            // 如果有进程在运行但时间片用完，将其重新加入队列
            if (current_process_idx != -1 && processes[current_process_idx].remaining_time > 0) {
                print_colored("时间 %d: 进程[%d] %s 时间片用完，重新加入队列\n", 
                             YELLOW, simulation_clock, processes[current_process_idx].pid, 
                             processes[current_process_idx].name);
                processes[current_process_idx].status = PROCESS_READY;
                enqueue(ready_queue, &processes[current_process_idx]);
            }
            
            // 重置时间片计数器
            time_slice_used = 0;
            current_process_idx = -1;
            
            // 从队列选取下一个进程
            PCB *next = dequeue(ready_queue);
            if (next != NULL) {
                for (int i = 0; i < count; i++) {
                    if (processes[i].pid == next->pid) {
                        current_process_idx = i;
                        break;
                    }
                }
                
                print_colored("时间 %d: 调度进程[%d] %s 开始执行\n", 
                             GREEN, simulation_clock, processes[current_process_idx].pid, 
                             processes[current_process_idx].name);
                processes[current_process_idx].status = PROCESS_RUNNING;
            }
        }
        
        // 如果有进程在执行，模拟其执行
        if (current_process_idx != -1) {
            // 执行一个时间单位
            processes[current_process_idx].remaining_time--;
            time_slice_used++;
            
            print_colored("时间 %d: 进程[%d] %s 正在执行，剩余时间: %d, 时间片: %d/%d\n", 
                         CYAN, simulation_clock, processes[current_process_idx].pid, 
                         processes[current_process_idx].name, 
                         processes[current_process_idx].remaining_time,
                         time_slice_used, time_quantum);
            
            // 检查是否完成
            if (processes[current_process_idx].remaining_time <= 0) {
                processes[current_process_idx].status = PROCESS_TERMINATED;
                processes[current_process_idx].completion_time = simulation_clock + 1;
                
                // 计算统计信息
                processes[current_process_idx].turnaround_time = 
                    processes[current_process_idx].completion_time - 
                    processes[current_process_idx].arrive_time;
                    
                processes[current_process_idx].weighted_turnaround = 
                    (float)processes[current_process_idx].turnaround_time / 
                    processes[current_process_idx].service_time;
                    
                processes[current_process_idx].waiting_time = 
                    processes[current_process_idx].turnaround_time - 
                    processes[current_process_idx].service_time;
                
                print_colored("时间 %d: 进程[%d] %s 执行完成\n", 
                             GREEN, simulation_clock + 1, processes[current_process_idx].pid, 
                             processes[current_process_idx].name);
                
                completed++;
                current_process_idx = -1;
                time_slice_used = 0;
            }
        }
        
        // 时钟前进
        simulation_clock++;
        usleep(500000);  // 放慢显示速度
    }
    
    print_colored("\nRR调度完成，所有进程已执行完毕\n", YELLOW);
    
    // 打印统计信息
    calculate_statistics(processes, count);
    
    // 可视化时间线
    visualize_execution_timeline(processes, count, simulation_clock);
    
    // 清理资源
    destroy_queue(ready_queue);
}

// 优先级调度算法
void Priority_scheduler(PCB *processes, int count) {
    clear_screen();
    print_title("优先级调度算法模拟");
    
    // 创建就绪队列，这里我们直接管理进程而不是队列
    // 因为优先级调度需要每次选择优先级最高的进程
    
    print_colored("初始进程状态:\n", CYAN);
    for (int i = 0; i < count; i++) {
        print_process_info(&processes[i]);
    }
    
    print_colored("\n开始优先级调度模拟...\n", YELLOW);
    
    // 模拟时钟
    simulation_clock = 0;
    int completed = 0;
    
    while (completed < count) {
        // 检查到达系统的新进程
        for (int i = 0; i < count; i++) {
            if (processes[i].arrive_time == simulation_clock && 
                processes[i].status == PROCESS_READY) {
                print_colored("时间 %d: 进程[%d] %s 到达系统\n", 
                             BLUE, simulation_clock, processes[i].pid, processes[i].name);
            }
        }
        
        // 从就绪进程中选择优先级最高的
        int highest_priority_idx = -1;
        int highest_priority = -1;
        
        for (int i = 0; i < count; i++) {
            if (processes[i].arrive_time <= simulation_clock && 
                processes[i].status == PROCESS_READY && 
                processes[i].priority > highest_priority) {
                highest_priority = processes[i].priority;
                highest_priority_idx = i;
            }
        }
        
        // 如果找到可运行的进程，执行它
        if (highest_priority_idx != -1) {
            print_colored("时间 %d: 调度进程[%d] %s (优先级: %d) 开始执行\n", 
                         GREEN, simulation_clock, processes[highest_priority_idx].pid, 
                         processes[highest_priority_idx].name,
                         processes[highest_priority_idx].priority);
            
            processes[highest_priority_idx].status = PROCESS_RUNNING;
            
            // 非抢占式优先级调度，执行直到完成
            int execution_time = processes[highest_priority_idx].remaining_time;
            processes[highest_priority_idx].remaining_time = 0;
            
            // 显示执行动画
            for (int t = 0; t < execution_time; t++) {
                print_colored("时间 %d: 进程[%d] %s 正在执行，剩余时间: %d\n", 
                             CYAN, simulation_clock + t, processes[highest_priority_idx].pid, 
                             processes[highest_priority_idx].name, execution_time - t);
                usleep(500000);  // 放慢显示速度
            }
            
            // 更新完成时间
            simulation_clock += execution_time;
            processes[highest_priority_idx].status = PROCESS_TERMINATED;
            processes[highest_priority_idx].completion_time = simulation_clock;
            
            // 计算统计信息
            processes[highest_priority_idx].turnaround_time = 
                processes[highest_priority_idx].completion_time - 
                processes[highest_priority_idx].arrive_time;
                
            processes[highest_priority_idx].weighted_turnaround = 
                (float)processes[highest_priority_idx].turnaround_time / 
                processes[highest_priority_idx].service_time;
                
            processes[highest_priority_idx].waiting_time = 
                processes[highest_priority_idx].turnaround_time - 
                processes[highest_priority_idx].service_time;
            
            print_colored("时间 %d: 进程[%d] %s 执行完成\n", 
                         GREEN, simulation_clock, processes[highest_priority_idx].pid, 
                         processes[highest_priority_idx].name);
            
            completed++;
        } else {
            // 没有可运行的进程，时钟前进
            simulation_clock++;
        }
    }
    
    print_colored("\n优先级调度完成，所有进程已执行完毕\n", YELLOW);
    
    // 打印统计信息
    calculate_statistics(processes, count);
    
    // 可视化时间线
    visualize_execution_timeline(processes, count, simulation_clock);
}

// 短作业优先调度算法
void SJF_scheduler(PCB *processes, int count) {
    clear_screen();
    print_title("短作业优先 (SJF) 调度算法模拟");
    
    print_colored("初始进程状态:\n", CYAN);
    for (int i = 0; i < count; i++) {
        print_process_info(&processes[i]);
    }
    
    print_colored("\n开始SJF调度模拟...\n", YELLOW);
    
    // 模拟时钟
    simulation_clock = 0;
    int completed = 0;
    
    while (completed < count) {
        // 检查到达系统的新进程
        for (int i = 0; i < count; i++) {
            if (processes[i].arrive_time == simulation_clock && 
                processes[i].status == PROCESS_READY) {
                print_colored("时间 %d: 进程[%d] %s 到达系统\n", 
                             BLUE, simulation_clock, processes[i].pid, processes[i].name);
            }
        }
        
        // 从就绪进程中选择服务时间最短的
        int shortest_job_idx = -1;
        int shortest_time = 999999;
        
        for (int i = 0; i < count; i++) {
            if (processes[i].arrive_time <= simulation_clock && 
                processes[i].status == PROCESS_READY && 
                processes[i].service_time < shortest_time) {
                shortest_time = processes[i].service_time;
                shortest_job_idx = i;
            }
        }
        
        // 如果找到可运行的进程，执行它
        if (shortest_job_idx != -1) {
            print_colored("时间 %d: 调度进程[%d] %s (服务时间: %d) 开始执行\n", 
                         GREEN, simulation_clock, processes[shortest_job_idx].pid, 
                         processes[shortest_job_idx].name,
                         processes[shortest_job_idx].service_time);
            
            processes[shortest_job_idx].status = PROCESS_RUNNING;
            
            // 非抢占式SJF，执行直到完成
            int execution_time = processes[shortest_job_idx].remaining_time;
            processes[shortest_job_idx].remaining_time = 0;
            
            // 显示执行动画
            for (int t = 0; t < execution_time; t++) {
                print_colored("时间 %d: 进程[%d] %s 正在执行，剩余时间: %d\n", 
                             CYAN, simulation_clock + t, processes[shortest_job_idx].pid, 
                             processes[shortest_job_idx].name, execution_time - t);
                usleep(500000);  // 放慢显示速度
            }
            
            // 更新完成时间
            simulation_clock += execution_time;
            processes[shortest_job_idx].status = PROCESS_TERMINATED;
            processes[shortest_job_idx].completion_time = simulation_clock;
            
            // 计算统计信息
            processes[shortest_job_idx].turnaround_time = 
                processes[shortest_job_idx].completion_time - 
                processes[shortest_job_idx].arrive_time;
                
            processes[shortest_job_idx].weighted_turnaround = 
                (float)processes[shortest_job_idx].turnaround_time / 
                processes[shortest_job_idx].service_time;
                
            processes[shortest_job_idx].waiting_time = 
                processes[shortest_job_idx].turnaround_time - 
                processes[shortest_job_idx].service_time;
            
            print_colored("时间 %d: 进程[%d] %s 执行完成\n", 
                         GREEN, simulation_clock, processes[shortest_job_idx].pid, 
                         processes[shortest_job_idx].name);
            
            completed++;
        } else {
            // 没有可运行的进程，时钟前进
            simulation_clock++;
        }
    }
    
    print_colored("\nSJF调度完成，所有进程已执行完毕\n", YELLOW);
    
    // 打印统计信息
    calculate_statistics(processes, count);
    
    // 可视化时间线
    visualize_execution_timeline(processes, count, simulation_clock);
}

// 演示进程创建与撤销
void demo_process_creation() {
    clear_screen();
    print_title("进程创建与撤销演示");
    
    print_colored("创建几个示例进程...\n", YELLOW);
    
    PCB *p1 = create_process("进程1", 5, 3);
    PCB *p2 = create_process("进程2", 3, 2);
    PCB *p3 = create_process("进程3", 7, 4);
    
    print_colored("\n进程创建完成，等待子进程执行...\n", YELLOW);
    sleep(2);
    
    print_colored("\n开始回收进程资源...\n", YELLOW);
    terminate_process(p1);
    terminate_process(p2);
    terminate_process(p3);
    
    print_colored("\n所有进程已撤销\n", GREEN);
    
    // 释放资源
    free(p1);
    free(p2);
    free(p3);
}

// 主菜单
void show_scheduler_menu() {
    int choice = 0;
    char buffer[100];
    
    while (1) {
        clear_screen();
        print_title("操作系统进程管理模拟系统");
        
        print_colored("┌─────────────────────────────┐\n", CYAN);
        print_colored("│       主菜单                │\n", CYAN);
        print_colored("├─────────────────────────────┤\n", CYAN);
        print_colored("│ 1. 演示进程创建与撤销        │\n", WHITE);
        print_colored("│ 2. 先来先服务(FCFS)调度算法  │\n", WHITE);
        print_colored("│ 3. 时间片轮转(RR)调度算法    │\n", WHITE);
        print_colored("│ 4. 优先级调度算法           │\n", WHITE);
        print_colored("│ 5. 短作业优先(SJF)调度算法   │\n", WHITE);
        print_colored("│ 6. 银行账户管理系统         │\n", WHITE);
        print_colored("│ 0. 退出                    │\n", WHITE);
        print_colored("└─────────────────────────────┘\n", CYAN);
        print_colored("请选择操作: ", YELLOW);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        
        choice = atoi(buffer);
        
        switch (choice) {
            case 1:
                demo_process_creation();
                break;
            case 2: {
                int count;
                PCB *processes = create_test_processes(&count);
                FCFS_scheduler(processes, count);
                free(processes);
                break;
            }
            case 3: {
                int count;
                PCB *processes = create_test_processes(&count);
                print_colored("请输入时间片大小: ", YELLOW);
                int quantum = 2; // 默认值
                if (fgets(buffer, sizeof(buffer), stdin)) {
                    quantum = atoi(buffer);
                }
                if (quantum < 1) quantum = 1;
                RR_scheduler(processes, count, quantum);
                free(processes);
                break;
            }
            case 4: {
                int count;
                PCB *processes = create_test_processes(&count);
                Priority_scheduler(processes, count);
                free(processes);
                break;
            }
            case 5: {
                int count;
                PCB *processes = create_test_processes(&count);
                SJF_scheduler(processes, count);
                free(processes);
                break;
            }
            case 6:
                // 运行原来的银行账户系统
                system("./bank_system");
                break;
            case 0:
                print_colored("谢谢使用，再见!\n", GREEN);
                return;
            default:
                print_colored("无效选择，请重试!\n", RED);
        }
        
        print_colored("\n按回车键继续...", YELLOW);
        getchar();
    }
}

int main() {
    show_scheduler_menu();
    return 0;
}