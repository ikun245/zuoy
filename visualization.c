#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "account.h"
#include "visualization.h"

// ANSI 颜色代码
const char* color_codes[] = {
    "\033[0;30m", // BLACK
    "\033[0;31m", // RED
    "\033[0;32m", // GREEN
    "\033[0;33m", // YELLOW
    "\033[0;34m", // BLUE
    "\033[0;35m", // MAGENTA
    "\033[0;36m", // CYAN
    "\033[0;37m"  // WHITE
};

// 重置颜色代码
const char* reset_color = "\033[0m";

/**
 * 使用颜色打印格式化文本
 * @param format 格式化字符串
 * @param color 文本颜色
 * @param ... 变量参数列表
 */
void print_colored(const char* format, Color color, ...) {
    printf("%s", color_codes[color]);
    
    va_list args;
    va_start(args, color);
    vprintf(format, args);
    va_end(args);
    
    printf("%s", reset_color);
}

/**
 * 清空屏幕
 */
void clear_screen() {
    system("clear");
}

/**
 * 打印标题
 * @param title 标题文本
 */
void print_title(const char* title) {
    int len = 0;
    while (title[len] != '\0') len++;
    
    print_colored("\n", WHITE);
    print_colored("┌", CYAN);
    for (int i = 0; i < len + 4; i++) {
        print_colored("─", CYAN);
    }
    print_colored("┐\n", CYAN);
    
    print_colored("│  ", CYAN);
    print_colored("%s", YELLOW, title);
    print_colored("  │\n", CYAN);
    
    print_colored("└", CYAN);
    for (int i = 0; i < len + 4; i++) {
        print_colored("─", CYAN);
    }
    print_colored("┘\n\n", CYAN);
}

/**
 * 打印菜单
 */
void print_menu() {
    print_colored("┌─────────────────────────────┐\n", CYAN);
    print_colored("│       银行系统菜单           │\n", CYAN);
    print_colored("├─────────────────────────────┤\n", CYAN);
    print_colored("│ 1. 创建新账户                │\n", WHITE);
    print_colored("│ 2. 查询账户余额              │\n", WHITE);
    print_colored("│ 3. 存款                     │\n", WHITE);
    print_colored("│ 4. 取款                     │\n", WHITE);
    print_colored("│ 5. 转账                     │\n", WHITE);
    print_colored("│ 6. 显示账户图表              │\n", WHITE);
    print_colored("│ 7. 显示余额历史              │\n", WHITE);
    print_colored("│ 8. 运行自动测试              │\n", WHITE);
    print_colored("│ 0. 退出                     │\n", WHITE);
    print_colored("└─────────────────────────────┘\n", CYAN);
    print_colored("请选择操作: ", YELLOW);
}

/**
 * 绘制账户余额图表
 * @param accounts 账户数组
 * @param num_accounts 账户数量
 */
void draw_account_chart(Account** accounts, int num_accounts) {
    // 找出最大余额，用于缩放
    double max_balance = 1.0; // 防止所有账户余额为0的情况
    for (int i = 0; i < num_accounts; i++) {
        if (accounts[i]->balance > max_balance) {
            max_balance = accounts[i]->balance;
        }
    }
    
    const int MAX_BAR_WIDTH = 50;
    
    print_title("账户余额分布图");
    
    for (int i = 0; i < num_accounts; i++) {
        int bar_width = (int)((accounts[i]->balance / max_balance) * MAX_BAR_WIDTH);
        if (bar_width < 1) bar_width = 1;
        
        // 打印账户信息
        print_colored("账户 %d (¥%.2f): ", WHITE, accounts[i]->account_id, accounts[i]->balance);
        
        // 打印余额条形图
        for (int j = 0; j < bar_width; j++) {
            print_colored("█", GREEN);
        }
        print_colored("\n", WHITE);
    }
    print_colored("\n", WHITE);
}

/**
 * 绘制账户余额历史图表
 * @param account 要显示历史的账户
 */
void draw_balance_history(Account* account) {
    if (account == NULL || account->history_size == 0) {
        print_colored("没有可用的历史数据\n", RED);
        return;
    }
    
    // 找出最大和最小余额，用于缩放
    double max_balance = account->history[0];
    double min_balance = account->history[0];
    
    for (int i = 1; i < account->history_size; i++) {
        if (account->history[i] > max_balance) {
            max_balance = account->history[i];
        }
        if (account->history[i] < min_balance) {
            min_balance = account->history[i];
        }
    }
    
    if (max_balance == min_balance) {
        max_balance += 100; // 防止最大和最小值相等
    }
    
    const int CHART_HEIGHT = 10;
    const int CHART_WIDTH = 60;
    
    char chart[CHART_HEIGHT][CHART_WIDTH];
    
    // 初始化图表
    for (int i = 0; i < CHART_HEIGHT; i++) {
        for (int j = 0; j < CHART_WIDTH; j++) {
            chart[i][j] = ' ';
        }
    }
    
    // 绘制数据点
    for (int i = 0; i < account->history_size && i < CHART_WIDTH; i++) {
        double normalized = (account->history[i] - min_balance) / (max_balance - min_balance);
        int y = CHART_HEIGHT - 1 - (int)(normalized * (CHART_HEIGHT - 1));
        
        if (y < 0) y = 0;
        if (y >= CHART_HEIGHT) y = CHART_HEIGHT - 1;
        
        chart[y][i] = '*';
    }
    
    // 修复后的代码：使用sprintf而不是+运算符拼接字符串
    char title_buffer[100];
    sprintf(title_buffer, "账户 %d 余额历史", account->account_id);
    print_title(title_buffer);
    
    // 打印Y轴标签
    print_colored("¥%.2f ", WHITE, max_balance);
    
    // 打印图表
    for (int i = 0; i < CHART_HEIGHT; i++) {
        if (i > 0) {
            print_colored("      ", WHITE);
        }
        
        for (int j = 0; j < CHART_WIDTH && j < account->history_size; j++) {
            if (chart[i][j] == '*') {
                print_colored("●", CYAN);
            } else {
                print_colored(" ", WHITE);
            }
        }
        print_colored("\n", WHITE);
    }
    
    print_colored("¥%.2f ", WHITE, min_balance);
    for (int j = 0; j < CHART_WIDTH; j++) {
        print_colored("─", WHITE);
    }
    print_colored("\n      ", WHITE);
    print_colored("最早", YELLOW);
    
    for (int j = 0; j < CHART_WIDTH - 10; j++) {
        print_colored(" ", WHITE);
    }
    
    print_colored("最新\n\n", YELLOW);
}

/**
 * 绘制转账动画
 * @param from_id 源账户ID
 * @param to_id 目标账户ID
 * @param amount 转账金额
 */
void draw_transaction_animation(int from_id, int to_id, double amount) {
    print_colored("\n转账进行中: ¥%.2f 从账户 %d -> 账户 %d\n", YELLOW, amount, from_id, to_id);
    
    for (int i = 0; i < 10; i++) {
        print_colored("\r[", WHITE);
        for (int j = 0; j < 20; j++) {
            if (j < i * 2) {
                print_colored("=", GREEN);
            } else {
                print_colored(" ", WHITE);
            }
        }
        print_colored("] %d%%", WHITE, i * 10);
        fflush(stdout);
        usleep(100000); // 100ms 延迟
    }
    
    print_colored("\r[====================] 100%%\n", GREEN);
    print_colored("转账完成!\n\n", GREEN);
}