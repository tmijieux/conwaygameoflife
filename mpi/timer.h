#ifndef CGL_TIMER_H
#define CGL_TIMER_H

typedef struct cgl_timer_ cgl_timer;

#include "../perf/perf.h"
#include "proc.h"

struct cgl_timer_ {
    perf_t p1, p2;
};

void cgl_timer_start(cgl_timer *T);
void cgl_timer_stop(cgl_timer *T);
void cgl_timer_print(cgl_timer *T, cgl_proc *P);

#endif // CGL_TIMER_H
