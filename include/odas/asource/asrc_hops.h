#ifndef __ODAS_ASOURCE_HOPS
#define __ODAS_ASOURCE_HOPS

/**
 * \file     asource_hops.h
 * \author   François Grondin <francois.grondin2@usherbrooke.ca>
 * \version  2.0
 * \date     2018-03-18
 * \copyright
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>

#include "../amessage/amsg_hops.h"
#include "../general/thread.h"
#include "../source/src_hops.h"

typedef struct asrc_hops_obj {
  src_hops_obj* src_hops;
  amsg_hops_obj* out;
  thread_obj* thread;

} asrc_hops_obj;

asrc_hops_obj* asrc_hops_construct(const src_hops_cfg* src_hops_config,
                                   const msg_hops_cfg* msg_hops_config);

void asrc_hops_destroy(asrc_hops_obj* obj);

void asrc_hops_connect(asrc_hops_obj* obj, amsg_hops_obj* out);

void asrc_hops_disconnect(asrc_hops_obj* obj);

void* asrc_hops_thread(void* ptr);

#endif