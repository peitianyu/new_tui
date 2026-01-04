/* tcc_bt.h - TCC兼容的Windows信号backtrace */
#ifndef TCC_BT_H
#define TCC_BT_H

#include <stdio.h>
#include <windows.h>
#include <signal.h>

/* ==================== TCC兼容的内联汇编 ==================== */
#ifdef __TINYC__
/* TCC的内联汇编语法 */
#define BT_GET_EBP(ebp) asm("movl %%ebp, %0" : "=r"(ebp))
#else
/* MSVC的内联汇编 */
#define BT_GET_EBP(ebp) __asm mov ebp, ebp
#endif

/* ==================== 简单栈遍历 ==================== */
static void bt_print_simple(int sig) {
    void* frames[20];
    int count = 0;
    
    /* 手动遍历栈帧（不依赖内联汇编） */
    void** current_frame;
    
#ifdef __TINYC__
    /* TCC方式获取当前帧指针 */
    asm("movl %%ebp, %0" : "=r"(current_frame));
#else
    /* 备用方法：通过局部变量近似 */
    int dummy;
    current_frame = (void**)&dummy - 2; /* 近似EBP位置 */
#endif
    
    printf("\n╔══════════════════════════════════╗\n");
    
    if(sig > 0) {
        const char* sig_name = "Unknown";
        switch(sig) {
            case SIGSEGV: sig_name = "Segmentation Fault"; break;
            case SIGFPE:  sig_name = "Floating Point Error"; break;
            case SIGILL:  sig_name = "Illegal Instruction"; break;
            case SIGABRT: sig_name = "Abort"; break;
        }
        printf("║ Signal: %-26s ║\n", sig_name);
        printf("╠══════════════════════════════════╣\n");
    }
    
    printf("║         Stack Trace              ║\n");
    printf("╠══════════════════════════════════╣\n");
    
    /* 遍历栈帧 */
    for(int i = 0; i < 15; i++) {
        if(!current_frame) break;
        
        /* 返回地址在EBP+4的位置 */
        void* ret_addr = current_frame[1];
        if(!ret_addr) break;
        
        frames[count++] = ret_addr;
        printf("║ #%02d: 0x%-24p ║\n", i, ret_addr);
        
        /* 上一级EBP在当前EBP指向的位置 */
        void* next_frame = current_frame[0];
        if(!next_frame || next_frame == current_frame) break;
        current_frame = next_frame;
    }
    
    if(count == 0) {
        printf("║ (No stack frames)               ║\n");
    }
    
    printf("╚══════════════════════════════════╝\n");
    
    /* 额外信息 */
    printf("\nProgram: %s\n", __FILE__);
    printf("PID: %lu\n", GetCurrentProcessId());
    printf("\n");
}

/* ==================== 异常处理器 ==================== */
static LONG WINAPI bt_exception_handler(PEXCEPTION_POINTERS ep) {
    int sig = 0;
    
    switch(ep->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            sig = SIGSEGV;
            printf("\n=== ACCESS VIOLATION ===\n");
            if(ep->ExceptionRecord->NumberParameters >= 2) {
                printf("Address: 0x%p\n", 
                    (void*)ep->ExceptionRecord->ExceptionInformation[1]);
                printf("Type: %s\n",
                    ep->ExceptionRecord->ExceptionInformation[0] ? 
                    "Write violation" : "Read violation");
            }
            break;
            
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            sig = SIGFPE;
            printf("\n=== DIVIDE BY ZERO ===\n");
            break;
            
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            sig = SIGILL;
            printf("\n=== ILLEGAL INSTRUCTION ===\n");
            break;
            
        default:
            printf("\n=== UNHANDLED EXCEPTION ===\n");
            printf("Code: 0x%08X\n", ep->ExceptionRecord->ExceptionCode);
            break;
    }
    
    /* 打印栈回溯 */
    bt_print_simple(sig);
    
    /* 等待按键 */
    printf("Press Enter to exit...");
    fflush(stdout);
    getchar();
    
    return EXCEPTION_EXECUTE_HANDLER;
}

/* ==================== 信号处理器 ==================== */
static void bt_signal_handler(int sig) {
    bt_print_simple(sig);
    
    /* 恢复默认并重新触发 */
    signal(sig, SIG_DFL);
    raise(sig);
}

/* ==================== 用户API宏 ==================== */
/* 安装所有处理器 */
#define BT_INSTALL() do { \
    SetUnhandledExceptionFilter(bt_exception_handler); \
    signal(SIGSEGV, bt_signal_handler); \
    signal(SIGFPE, bt_signal_handler); \
    signal(SIGILL, bt_signal_handler); \
    signal(SIGABRT, bt_signal_handler); \
    signal(SIGTERM, bt_signal_handler); \
} while(0)

/* 手动打印栈 */
#define BT() bt_print_simple(0)

/* 带消息打印栈 */
#define BT_MSG(msg) do { \
    printf("\n[%s]\n", msg); \
    bt_print_simple(0); \
} while(0)

#endif /* TCC_BT_H */