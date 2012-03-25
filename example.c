#include <os.h>
#include "threading.h"

static void thread(unsigned id, void* userdata __attribute__((unused))) {
    uint16_t *fbptr = SCREEN_BASE_ADDRESS;
    fbptr += (id - 1) * 0x4B00;
    int i;

    for (i=0; i<0x4b00; i++) {
        *fbptr = 0b0000011111100000;
        fbptr++;

        thread_sleep(10*id);
    }
}

int main() {
    memset(SCREEN_BASE_ADDRESS, 0, SCREEN_BYTES_SIZE);
    if (init_threading() != 0) return -1;

    run_thread(thread, NULL);
    run_thread(thread, NULL);
    run_thread(thread, NULL);

    wait_threads(); /* exiting main thread before other threads finish causes undefined behavior */
    cleanup_threading();
    return 0;
}

