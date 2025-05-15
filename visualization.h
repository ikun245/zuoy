#ifndef VISUALIZATION_H
#define VISUALIZATION_H

// 颜色定义
typedef enum {
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
} Color;

// 可视化函数
void print_colored(const char* format, Color color, ...);
void clear_screen();
void print_title(const char* title);
void print_menu();
void draw_account_chart(Account** accounts, int num_accounts);
void draw_balance_history(Account* account);
void draw_transaction_animation(int from_id, int to_id, double amount);

#endif // VISUALIZATION_H