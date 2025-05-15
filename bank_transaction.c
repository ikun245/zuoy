#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "account.h"
#include "visualization.h"

#define NUM_ACCOUNTS 5
#define NUM_TRANSACTIONS 10
#define MAX_TRANSFER_AMOUNT 1000.0
#define MAX_ACCOUNTS 100

// 全局账户数组
Account* accounts[MAX_ACCOUNTS];
int num_accounts = 0;

// 线程函数，执行随机转账
void* perform_random_transfer(void* arg) {
    int thread_id = *(int*)arg;
    
    // 生成随机账户和金额
    int from_idx = rand() % num_accounts;
    int to_idx = rand() % num_accounts;
    
    // 确保不是转给同一个账户
    while (to_idx == from_idx && num_accounts > 1) {
        to_idx = rand() % num_accounts;
    }
    
    // 随机金额
    double amount = ((double)rand() / RAND_MAX) * MAX_TRANSFER_AMOUNT + 1.0;
    
    print_colored("\n[线程 %d] 开始转账: ¥%.2f 从账户 %d 到账户 %d\n",
           MAGENTA, thread_id, amount, accounts[from_idx]->account_id, accounts[to_idx]->account_id);
    
    // 执行转账
    draw_transaction_animation(accounts[from_idx]->account_id, accounts[to_idx]->account_id, amount);
    transfer(accounts[from_idx], accounts[to_idx], amount);
    
    free(arg);
    return NULL;
}

// 运行自动测试 - 使用现有账户
void run_automated_test() {
    clear_screen();
    print_title("银行账户交易系统 - 自动测试");
    
    // 检查是否有足够的账户
    if (num_accounts < 2) {
        print_colored("\n错误: 需要至少两个账户才能运行测试!\n", RED);
        print_colored("请先创建至少两个账户.\n\n", YELLOW);
        return;
    }
    
    print_colored("\n使用现有账户进行测试...\n", YELLOW);
    
    // 显示现有账户
    print_colored("\n==== 现有账户 ====\n", CYAN);
    for (int i = 0; i < num_accounts; i++) {
        print_account_info(accounts[i]);
    }
    
    // 计算初始总资金
    double initial_sum = 0.0;
    for (int i = 0; i < num_accounts; i++) {
        initial_sum += accounts[i]->balance;
    }
    
    // 可视化初始状态
    draw_account_chart(accounts, num_accounts);
    
    print_colored("\n按回车键开始测试...", YELLOW);
    getchar();
    
    // 创建线程进行并发交易
    pthread_t threads[NUM_TRANSACTIONS];
    
    print_colored("\n==== 开始并发交易 ====\n", CYAN);
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        int* thread_id = malloc(sizeof(int));
        *thread_id = i;
        
        if (pthread_create(&threads[i], NULL, perform_random_transfer, thread_id) != 0) {
            perror("创建线程失败");
            free(thread_id);
            exit(EXIT_FAILURE);
        }
        
        // 线程创建间隔
        usleep(500000);  // 500ms
    }
    
    // 等待所有交易完成
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    print_colored("\n==== 所有交易完成 ====\n", GREEN);
    print_colored("\n==== 最终账户余额 ====\n", CYAN);
    for (int i = 0; i < num_accounts; i++) {
        print_account_info(accounts[i]);
    }
    
    // 计算并验证系统总资金
    double final_sum = 0.0;
    for (int i = 0; i < num_accounts; i++) {
        final_sum += accounts[i]->balance;
    }
    
    print_colored("\n系统初始总资金: ¥%.2f\n", WHITE, initial_sum);
    print_colored("系统最终总资金: ¥%.2f\n", WHITE, final_sum);
    
    if (fabs(final_sum - initial_sum) < 0.01) {
        print_colored("验证成功: 系统总资金保持不变\n", GREEN);
    } else {
        print_colored("验证失败: 系统总资金发生变化\n", RED);
    }
    
    // 显示最终图表
    draw_account_chart(accounts, num_accounts);
    
    print_colored("\n按回车键返回主菜单...", YELLOW);
    getchar();
}

// 交互式模式的主函数
int main() {
    // 初始化随机数生成器
    srand(time(NULL));
    
    int choice;
    char buffer[100];
    
    while (1) {
        clear_screen();
        print_title("银行账户交易系统");
        print_menu();
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        
        choice = atoi(buffer);
        
        switch (choice) {
            case 1: {  // 创建新账户
                clear_screen();
                print_title("创建新账户");
                
                if (num_accounts >= MAX_ACCOUNTS) {
                    print_colored("达到最大账户数量限制!\n", RED);
                    break;
                }
                
                int id;
                double initial_balance;
                
                print_colored("请输入账户ID: ", YELLOW);
                scanf("%d", &id);
                getchar(); // 消耗换行符
                
                // 检查ID是否已存在
                int exists = 0;
                for (int i = 0; i < num_accounts; i++) {
                    if (accounts[i]->account_id == id) {
                        exists = 1;
                        break;
                    }
                }
                
                if (exists) {
                    print_colored("错误: 账户ID %d 已存在\n", RED, id);
                    break;
                }
                
                print_colored("请输入初始余额: ", YELLOW);
                scanf("%lf", &initial_balance);
                getchar(); // 消耗换行符
                
                if (initial_balance < 0) {
                    print_colored("错误: 初始余额不能为负数\n", RED);
                    break;
                }
                
                accounts[num_accounts++] = create_account(id, initial_balance);
                print_colored("\n账户创建成功!\n", GREEN);
                break;
            }
            
            case 2: {  // 查询账户余额
                clear_screen();
                print_title("查询账户余额");
                
                if (num_accounts == 0) {
                    print_colored("没有可用账户!\n", RED);
                    break;
                }
                
                print_colored("可用账户:\n", CYAN);
                for (int i = 0; i < num_accounts; i++) {
                    print_account_info(accounts[i]);
                }
                break;
            }
            
            case 3: {  // 存款
                clear_screen();
                print_title("存款");
                
                if (num_accounts == 0) {
                    print_colored("没有可用账户!\n", RED);
                    break;
                }
                
                int id;
                double amount;
                Account* account = NULL;
                
                print_colored("可用账户:\n", CYAN);
                for (int i = 0; i < num_accounts; i++) {
                    print_account_info(accounts[i]);
                }
                
                print_colored("\n请输入账户ID: ", YELLOW);
                scanf("%d", &id);
                getchar(); // 消耗换行符
                
                // 查找账户
                for (int i = 0; i < num_accounts; i++) {
                    if (accounts[i]->account_id == id) {
                        account = accounts[i];
                        break;
                    }
                }
                
                if (account == NULL) {
                    print_colored("账户不存在!\n", RED);
                    break;
                }
                
                print_colored("请输入存款金额: ", YELLOW);
                scanf("%lf", &amount);
                getchar(); // 消耗换行符
                
                pthread_mutex_lock(&account->mutex);
                int result = deposit(account, amount);
                pthread_mutex_unlock(&account->mutex);
                
                if (result == 0) {
                    print_colored("存款成功!\n", GREEN);
                } else {
                    print_colored("存款失败! 请确保输入的金额大于零。\n", RED);
                }
                break;
            }
            
            case 4: {  // 取款
                clear_screen();
                print_title("取款");
                
                if (num_accounts == 0) {
                    print_colored("没有可用账户!\n", RED);
                    break;
                }
                
                int id;
                double amount;
                Account* account = NULL;
                
                print_colored("可用账户:\n", CYAN);
                for (int i = 0; i < num_accounts; i++) {
                    print_account_info(accounts[i]);
                }
                
                print_colored("\n请输入账户ID: ", YELLOW);
                scanf("%d", &id);
                getchar(); // 消耗换行符
                
                // 查找账户
                for (int i = 0; i < num_accounts; i++) {
                    if (accounts[i]->account_id == id) {
                        account = accounts[i];
                        break;
                    }
                }
                
                if (account == NULL) {
                    print_colored("账户不存在!\n", RED);
                    break;
                }
                
                print_colored("请输入取款金额: ", YELLOW);
                scanf("%lf", &amount);
                getchar(); // 消耗换行符
                
                pthread_mutex_lock(&account->mutex);
                int result = withdraw(account, amount);
                pthread_mutex_unlock(&account->mutex);
                
                if (result == 0) {
                    print_colored("取款成功!\n", GREEN);
                } else {
                    print_colored("取款失败! 请确保输入的金额有效且账户余额充足。\n", RED);
                }
                break;
            }
            
            case 5: {  // 转账
                clear_screen();
                print_title("转账");
                
                if (num_accounts < 2) {
                    print_colored("需要至少两个账户才能进行转账!\n", RED);
                    break;
                }
                
                int from_id, to_id;
                double amount;
                Account *from_account = NULL, *to_account = NULL;
                
                print_colored("可用账户:\n", CYAN);
                for (int i = 0; i < num_accounts; i++) {
                    print_account_info(accounts[i]);
                }
                
                print_colored("\n请输入源账户ID: ", YELLOW);
                scanf("%d", &from_id);
                getchar(); // 消耗换行符
                
                print_colored("请输入目标账户ID: ", YELLOW);
                scanf("%d", &to_id);
                getchar(); // 消耗换行符
                
                if (from_id == to_id) {
                    print_colored("不能向同一账户转账!\n", RED);
                    break;
                }
                
                // 查找账户
                for (int i = 0; i < num_accounts; i++) {
                    if (accounts[i]->account_id == from_id) {
                        from_account = accounts[i];
                    }
                    if (accounts[i]->account_id == to_id) {
                        to_account = accounts[i];
                    }
                }
                
                if (from_account == NULL || to_account == NULL) {
                    print_colored("一个或多个账户不存在!\n", RED);
                    break;
                }
                
                print_colored("请输入转账金额: ", YELLOW);
                scanf("%lf", &amount);
                getchar(); // 消耗换行符
                
                draw_transaction_animation(from_id, to_id, amount);
                int result = transfer(from_account, to_account, amount);
                
                if (result == 0) {
                    print_colored("转账成功!\n", GREEN);
                } else {
                    print_colored("转账失败! 请确保输入的金额有效且源账户余额充足。\n", RED);
                }
                break;
            }
            
            case 6: {  // 显示账户图表
                clear_screen();
                print_title("账户余额图表");
                
                if (num_accounts == 0) {
                    print_colored("没有可用账户!\n", RED);
                    break;
                }
                
                draw_account_chart(accounts, num_accounts);
                break;
            }
            
            case 7: {  // 显示余额历史
                clear_screen();
                print_title("账户余额历史");
                
                if (num_accounts == 0) {
                    print_colored("没有可用账户!\n", RED);
                    break;
                }
                
                int id;
                Account* account = NULL;
                
                print_colored("可用账户:\n", CYAN);
                for (int i = 0; i < num_accounts; i++) {
                    print_account_info(accounts[i]);
                }
                
                print_colored("\n请输入账户ID: ", YELLOW);
                scanf("%d", &id);
                getchar(); // 消耗换行符
                
                // 查找账户
                for (int i = 0; i < num_accounts; i++) {
                    if (accounts[i]->account_id == id) {
                        account = accounts[i];
                        break;
                    }
                }
                
                if (account == NULL) {
                    print_colored("账户不存在!\n", RED);
                    break;
                }
                
                draw_balance_history(account);
                break;
            }
            
            case 8:  // 运行自动测试
                run_automated_test();
                break;
                
            case 0:  // 退出
                clear_screen();
                print_title("系统退出");
                print_colored("正在清理资源...\n", YELLOW);
                
                // 清理资源
                for (int i = 0; i < num_accounts; i++) {
                    destroy_account(accounts[i]);
                }
                
                print_colored("感谢使用银行交易系统!\n", GREEN);
                return 0;
                
            default:
                print_colored("无效选择，请重试!\n", RED);
        }
        
        print_colored("\n按回车键继续...", YELLOW);
        getchar();
    }
    
    return 0;
}