#ifndef __DEMO_THREADS
#define __DEMO_THREADS

#include <odas/odas.h>
#include <time.h>

#include "objects.h"
#include "profiler.h"

void threads_multiple_start(aobjects* aobjs);

void threads_multiple_stop(aobjects* aobjs);

void threads_multiple_join(aobjects* aobjs);

void threads_single_open(objects* objs);

void threads_single_close(objects* objs);

int threads_single_process(objects* objs, profiler* prf);

#endif