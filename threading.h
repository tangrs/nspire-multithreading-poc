#define THREAD_STACK_SIZE 0x80000
#define TIMER_HZ 32000
#define TIMER_LOAD_VALUE 1

int init_threading();
void cleanup_threading();
void wait_threads();
void run_thread(void(*thread_func)(unsigned, void*), void* userdata);
void thread_sleep(unsigned ms);