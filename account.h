#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <pthread.h>

// 账户结构
typedef struct {
    int account_id;          // 唯一标识符
    double balance;          // 当前余额
    pthread_mutex_t mutex;   // 账户锁
    double* history;         // 余额历史记录
    int history_size;        // 历史记录大小
    int history_capacity;    // 历史记录容量
} Account;

// 交易结构
typedef struct {
    Account* from_account;   // 源账户
    Account* to_account;     // 目标账户
    double amount;           // 转账金额
} Transaction;

// 函数原型
Account* create_account(int id, double initial_balance);
void destroy_account(Account* account);
int deposit(Account* account, double amount);
int withdraw(Account* account, double amount);
int transfer(Account* from, Account* to, double amount);
void print_account_info(Account* account);
void record_balance_history(Account* account);

#endif // ACCOUNT_H