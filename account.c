#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "account.h"
#include "visualization.h"

/**
 * 创建一个新账户
 * @param id 账户ID
 * @param initial_balance 初始余额
 * @return 指向新账户的指针，失败时返回NULL
 */
Account* create_account(int id, double initial_balance) {
    // 分配内存
    Account* new_account = (Account*)malloc(sizeof(Account));
    if (new_account == NULL) {
        perror("创建账户时内存分配失败");
        return NULL;
    }
    
    // 初始化账户数据
    new_account->account_id = id;
    new_account->balance = initial_balance;
    
    // 初始化余额历史记录
    new_account->history_capacity = 20;
    new_account->history_size = 0;
    new_account->history = (double*)malloc(sizeof(double) * new_account->history_capacity);
    
    if (new_account->history == NULL) {
        perror("创建历史记录时内存分配失败");
        free(new_account);
        return NULL;
    }
    
    // 记录初始余额
    record_balance_history(new_account);
    
    // 初始化互斥锁
    if (pthread_mutex_init(&new_account->mutex, NULL) != 0) {
        perror("互斥锁初始化失败");
        free(new_account->history);
        free(new_account);
        return NULL;
    }
    
    print_colored("账户 %d 创建成功，初始余额: ¥%.2f\n", CYAN, id, initial_balance);
    return new_account;
}

/**
 * 释放账户相关资源
 * @param account 要销毁的账户指针
 */
void destroy_account(Account* account) {
    if (account == NULL) return;
    
    // 销毁互斥锁
    pthread_mutex_destroy(&account->mutex);
    
    // 释放历史记录内存
    if (account->history != NULL) {
        free(account->history);
    }
    
    int id = account->account_id; // 保存ID以便在释放后使用
    
    // 释放账户内存
    free(account);
    
    print_colored("账户 %d 已销毁\n", YELLOW, id);
}

/**
 * 记录账户余额历史
 * @param account 目标账户
 */
void record_balance_history(Account* account) {
    if (account == NULL) return;
    
    // 如果需要扩展历史记录数组
    if (account->history_size >= account->history_capacity) {
        int new_capacity = account->history_capacity * 2;
        double* new_history = (double*)realloc(account->history, 
                                             sizeof(double) * new_capacity);
        if (new_history == NULL) {
            return; // 内存分配失败，不记录此次历史
        }
        account->history = new_history;
        account->history_capacity = new_capacity;
    }
    
    // 记录当前余额
    account->history[account->history_size++] = account->balance;
}

/**
 * 向账户存款
 * @param account 目标账户
 * @param amount 存款金额
 * @return 成功返回0，失败返回-1
 */
int deposit(Account* account, double amount) {
    if (account == NULL || amount <= 0) {
        return -1;
    }
    
    // 更新余额
    account->balance += amount;
    
    // 记录历史
    record_balance_history(account);
    
    print_colored("已存入 ¥%.2f 到账户 %d，新余额: ¥%.2f\n", 
           GREEN, amount, account->account_id, account->balance);
    
    return 0;
}

/**
 * 从账户取款
 * @param account 源账户
 * @param amount 取款金额
 * @return 成功返回0，余额不足或其他错误返回-1
 */
int withdraw(Account* account, double amount) {
    if (account == NULL || amount <= 0) {
        return -1;
    }
    
    // 检查余额是否充足
    if (account->balance < amount) {
        print_colored("账户 %d 余额不足: ¥%.2f < ¥%.2f\n", 
               RED, account->account_id, account->balance, amount);
        return -1;
    }
    
    // 更新余额
    account->balance -= amount;
    
    // 记录历史
    record_balance_history(account);
    
    print_colored("已从账户 %d 取出 ¥%.2f，新余额: ¥%.2f\n", 
           YELLOW, account->account_id, amount, account->balance);
    
    return 0;
}

/**
 * 账户间转账
 * @param from 源账户
 * @param to 目标账户
 * @param amount 转账金额
 * @return 成功返回0，失败返回-1
 */
int transfer(Account* from, Account* to, double amount) {
    if (from == NULL || to == NULL || amount <= 0) {
        return -1;
    }
    
    int result = -1;
    
    // 根据账户ID确定锁定顺序，防止死锁
    Account* first = (from->account_id < to->account_id) ? from : to;
    Account* second = (from->account_id < to->account_id) ? to : from;
    
    print_colored("转账请求: ¥%.2f 从账户 %d 到账户 %d\n",
           CYAN, amount, from->account_id, to->account_id);
    
    // 按顺序锁定账户
    print_colored("正在锁定账户 %d\n", BLUE, first->account_id);
    pthread_mutex_lock(&first->mutex);
    print_colored("正在锁定账户 %d\n", BLUE, second->account_id);
    pthread_mutex_lock(&second->mutex);
    
    // 执行转账
    if (withdraw(from, amount) == 0) {
        if (deposit(to, amount) == 0) {
            print_colored("转账成功: ¥%.2f 从账户 %d 到账户 %d\n",
                   GREEN, amount, from->account_id, to->account_id);
            result = 0;
        } else {
            // 如果存款失败，回滚取款操作
            print_colored("存款失败，回滚中...\n", RED);
            deposit(from, amount);
        }
    } else {
        print_colored("转账失败: 账户 %d 余额不足\n", RED, from->account_id);
    }
    
    // 反序解锁
    print_colored("解锁账户 %d\n", BLUE, second->account_id);
    pthread_mutex_unlock(&second->mutex);
    print_colored("解锁账户 %d\n", BLUE, first->account_id);
    pthread_mutex_unlock(&first->mutex);
    
    return result;
}

/**
 * 打印账户信息
 * @param account 要显示的账户
 */
void print_account_info(Account* account) {
    if (account == NULL) return;
    
    print_colored("账户ID: %d, 余额: ¥%.2f\n", 
           WHITE, account->account_id, account->balance);
}