#include <os.h>
#include "interrupt.h"
#include "threading.h"

#define XSTRING(a) #a
#define STRING(a) XSTRING(a)

#define save_state( s, reglist, c ) do { \
    memcpy((s)->state.reg, reglist, 64); \
    (s)->state.cpsr = *(c); \
    } while (0)

#define bk(a) asm("bkpt #" STRING(a))

typedef struct {
    unsigned reg[16];
    unsigned cpsr;
    unsigned thread_id;
    unsigned sleep;
    void * stack_base;
} cpu_state_t;

enum {
    sp = 13,
    lr = 14,
    pc = 15
};

struct state_link;
typedef struct state_link {
    struct state_link* next;
    struct state_link* prev;
    cpu_state_t state;
} state_link_t;

static state_link_t* volatile state_list;
static volatile unsigned want_out;
static volatile unsigned last_thread_id;
static volatile unsigned number_of_threads;
static volatile unsigned current_thread_id;

static unsigned timer_saved_control, timer_saved_load;

static void thread_run_wrapper(void(*thread_func)(unsigned, void*), unsigned thread_id, void* userdata) __attribute__ ((noreturn));
static unsigned* context_switch(unsigned * reg_list, unsigned * cpsr);
static state_link_t* next_state(state_link_t* thread);
static void thread_cleanup(state_link_t* thread);
static void update_state_deltas();
static state_link_t* get_state_by_thread_id(unsigned id);

int init_threading() {
    /* set up initial stuff */
    current_thread_id = 0; /* main thread is always 0 */
    want_out = 0;
    number_of_threads = 1;
    last_thread_id = 1;

    state_link_t * link = malloc(sizeof(state_link_t));
    if (!link) return -1;
    memset(link, 0, sizeof(state_link_t));

    /* add to linked list*/
   state_list = link;

   /* our state will be automatically filled out by the context switcher */

    /* set up timer */
    volatile unsigned *control = (unsigned*)0x900D0008;
	volatile unsigned *load = (unsigned*)0x900D0000;
	if (is_cx) {
        timer_saved_control = *control;
        timer_saved_load = *load;
        *control = 0; // disable timer
        *control = 0b01100011; // disabled, TimerMode N/A, int, no prescale, 32-bit, One Shot (for the *value > 0 test) -> 32khz
        *control = 0b11100011; // enable timer
        *load = TIMER_LOAD_VALUE;
    }


    /* begin interrupts! */
    init_interrupts(context_switch);

    return 0;
}

void cleanup_threading() {
    end_interrupts();

    /* free up memory */
    state_link_t* next = state_list;
    while (next) {
        state_link_t *this = next;
        next = next->next;
        free(this->state.stack_base);
        free(this);
    }
    state_list = NULL;

    /* restore timers */
    volatile unsigned *control = (unsigned*)0x900D0008;
	volatile unsigned *load = (unsigned*)0x900D0000;
	*control = 0; // disable timer
	*control = timer_saved_control & 0b01111111; // timer still disabled
	*load = timer_saved_load;
	*control = timer_saved_control; // enable timer
}

void wait_threads() {
    unsigned run = 1;
    while (run) {
        set_interrupt_off();
        run = number_of_threads > 1;
        set_interrupt_on();
        __asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) );
    }
}

static void panic() {
    while (1);
}

void run_thread( void(*thread_func)(unsigned, void*), void* userdata ) {
    set_interrupt_off();

    /* allocate structure and stack */
    state_link_t * link = malloc(sizeof(state_link_t));
    void * stack_base = malloc(THREAD_STACK_SIZE);
    if (!link || !stack_base) {
        free(link);
        free(stack_base);
        return;
    }

    memset(link, 0, sizeof(state_link_t));

    /* give it a thread id */
    link->state.thread_id = last_thread_id;
    last_thread_id++;

    /* set initial state */
    link->state.reg[0] = (unsigned)thread_func; /* argument 0 is function ptr */
    link->state.reg[1] = (unsigned)link->state.thread_id; /* argument 1 is id */
    link->state.reg[2] = (unsigned)userdata; /* argument 2 is userdata pointer */
    link->state.reg[sp] = ((unsigned)stack_base) + THREAD_STACK_SIZE; /* stack */
    link->state.reg[lr] = (unsigned)panic; /* panic if the thread_run_wrapper dies */
    link->state.reg[pc] = (unsigned)thread_run_wrapper; /* initial function */

    link->state.cpsr = 0x53;

    /* add to linked list*/
    state_link_t* next = state_list;
    if (!next) {
        state_list = link;
    } else {
        while(next->next) {
            next = next->next;
        }
        next->next = link;
        link->prev = next;
    }
    number_of_threads++;

    set_interrupt_on();
}

void thread_sleep(unsigned ms) {
    set_interrupt_off();
    ms *= TIMER_HZ/1000;
    state_link_t* current_thread = get_state_by_thread_id(current_thread_id);
    current_thread->state.sleep = ms;
    set_interrupt_on();
    __asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) ); /* wait to be switched out */
}

static unsigned* context_switch(unsigned * reg_list, unsigned * cpsr) {
    /* acknowledge interrupt */
    *IO(0x900A0020, 0x900D000C) = 1;

    /* context switch */
    state_link_t* current_thread = get_state_by_thread_id(current_thread_id);

    /* verify & error check */
    if (!current_thread) {
        if (state_list) {
            current_thread_id = state_list->state.thread_id;
            current_thread = get_state_by_thread_id(current_thread_id);
            if (!current_thread) panic();
        }else{
            /* some weird as shit is going on */
            panic();
        }
    }

    /* finally, we can switch to a new state */
    if (want_out) {
        /* thread has indicated it has finished its work */

        /* take it out of the linked list so it won't be switched in by next_state */
        if (current_thread->prev) current_thread->prev->next = current_thread->next;
            else state_list = current_thread->next;
        if (current_thread->next) current_thread->next->prev = current_thread->prev;

        state_link_t* new_thread = next_state(current_thread);
        thread_cleanup(current_thread);
        current_thread = new_thread;
        want_out = 0;
    }else{
        save_state(current_thread, reg_list, cpsr);
        current_thread = next_state(current_thread);
    }

    *cpsr = current_thread->state.cpsr;
    reg_list = (unsigned*)&current_thread->state.reg;

    /* reload the timer */
    volatile unsigned *load = (unsigned*)0x900D0000;
    *load = TIMER_LOAD_VALUE;
    return reg_list;
}

static void update_state_deltas() {
    state_link_t* next = state_list;
    while (next) {
        if (next->state.sleep > 0) next->state.sleep--;
        next = next->next;
    }
}

static state_link_t* next_state(state_link_t* thread) {
    unsigned check = 1;
    while (1) {
        *IO(0x900A0020, 0x900D000C) = 1; /* acknowledge interrupt */

        state_link_t* next = thread->next;
        unsigned searched;
        /* look for the next available thread to load */
        update_state_deltas();

        for (searched=0;searched<number_of_threads;searched++) {
            if (!next) next = state_list;
            if (!next->state.sleep) {
                check = 0;
                thread = next;
                break;
            }
            next = next->next;
        }
        if (!check) break;
        /* if we reached here, it means all threads are sleeping */
        /* reload the timer and wait for an interrupt */

        volatile unsigned *load = (unsigned*)0x900D0000;
        *load = TIMER_LOAD_VALUE;
        __asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) );
    }

    current_thread_id = thread->state.thread_id;
    return thread;
}

/*static void thread_cleanup_by_id(unsigned id) {
    thread_cleanup(get_state_by_thread_id(id));
}*/

static void thread_cleanup(state_link_t* thread) {
    state_link_t* next, *prev;
    if (!thread) return;

    /* remove from linked list */
    /* next = thread->next;
    prev = thread->prev;
    if (next) next->prev = prev;
    if (prev) prev->next = next; */

    /* clean up memory */
    free(thread->state.stack_base);
    free(thread);

    number_of_threads--;
}

static state_link_t* get_state_by_thread_id(unsigned id) {
    state_link_t* next = state_list;
    while (next) {
        if (next->state.thread_id == id) return next;
        next = next->next;
    }
    return NULL;
}

static void thread_run_wrapper( void(*thread_func)(unsigned, void*), unsigned thread_id, void* userdata ) {
    /* run the thread */
    thread_func(thread_id, userdata);

    /* tell context switcher the current thread has finished */
    set_interrupt_off();
    want_out = 1;
    set_interrupt_on();

    /* wait to be switched out */
    while (1) {
        __asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) );
    }
    __builtin_unreachable();
}