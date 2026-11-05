/*
 * Support file for AMDGCN in newlib.
 * Copyright (c) 2026 BayLibre.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

/* Memory allocation in SIMT mode (-msimt).

   In SIMT mode each lane of a wavefront is a separate thread, and any or
   all of the active lanes may call malloc at once, each expecting its own
   allocation.  The heap lock cannot serialize the lanes of one wavefront:
   they execute in lockstep, so the lane that wins the lock cannot go on to
   release it while the other lanes are still spinning, and the wavefront
   deadlocks.

   Instead, the entry points below run the real allocator for one lane at
   a time.  The real allocator is the usual dlmalloc (stdlib/_mallocr.c),
   built here under private names.  The generic copy, and the generic
   wrappers in stdlib, are disabled by MALLOC_PROVIDED, which configure
   defines for SIMT mode (see libm/machine/amdgcn/acinclude.m4).  The heap
   lock (mlock.c) still serializes the wavefronts.  */

#ifdef __SIMT__

/* Build dlmalloc under private names.  */

#undef MALLOC_PROVIDED

#define _malloc_r		__simt_dl_malloc_r
#define _free_r			__simt_dl_free_r
#define _realloc_r		__simt_dl_realloc_r
#define _calloc_r		__simt_dl_calloc_r
#define _memalign_r		__simt_dl_memalign_r
#define _valloc_r		__simt_dl_valloc_r
#define _pvalloc_r		__simt_dl_pvalloc_r
#define _mallinfo_r		__simt_dl_mallinfo_r
#define _mallopt_r		__simt_dl_mallopt_r
#define _malloc_stats_r		__simt_dl_malloc_stats_r
#define _malloc_trim_r		__simt_dl_malloc_trim_r
#define _malloc_usable_size_r	__simt_dl_malloc_usable_size_r

#define DEFINE_MALLOC
#define DEFINE_FREE
#define DEFINE_REALLOC
#define DEFINE_CALLOC
#define DEFINE_MEMALIGN
#define DEFINE_VALLOC
#define DEFINE_PVALLOC
#define DEFINE_MALLINFO
#define DEFINE_MALLOC_STATS
#define DEFINE_MALLOC_USABLE_SIZE
#define DEFINE_MALLOPT

#include "../../stdlib/_mallocr.c"

#undef _malloc_r
#undef _free_r
#undef _realloc_r
#undef _calloc_r
#undef _memalign_r
#undef _valloc_r
#undef _pvalloc_r
#undef _mallinfo_r
#undef _mallopt_r
#undef _malloc_stats_r
#undef _malloc_trim_r
#undef _malloc_usable_size_r

/* _mallocr.c maps these onto the _r names.  */
#undef malloc_stats
#undef malloc_trim
#undef malloc_usable_size

/* Run STMT for each active lane in turn, with only that lane active.  */

#define ONE_LANE_AT_A_TIME(STMT)					\
  do									\
    {									\
      int lane_ = __builtin_gcn_dim_pos (2);				\
      for (int i_ = 0; i_ < 64; i_++)					\
	{								\
	  /* Run STMT in the right lane, but do *not* make this a	\
	     loop exit route or the compiler will unify the exits.  */	\
	  if (lane_ == i_)						\
	    STMT;							\
	  /* Stop when no higher lane is active.  */			\
	  if (!__builtin_gcn_simt_vote_any (lane_ > i_))		\
	    break;							\
	}								\
    }									\
  while (0)

void *
_malloc_r (struct _reent *r, size_t n)
{
  void *ret = NULL;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_malloc_r (r, n));
  return ret;
}

void
_free_r (struct _reent *r, void *p)
{
  ONE_LANE_AT_A_TIME (__simt_dl_free_r (r, p));
}

void *
_realloc_r (struct _reent *r, void *p, size_t n)
{
  void *ret = NULL;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_realloc_r (r, p, n));
  return ret;
}

void *
_calloc_r (struct _reent *r, size_t n, size_t s)
{
  void *ret = NULL;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_calloc_r (r, n, s));
  return ret;
}

void *
_memalign_r (struct _reent *r, size_t a, size_t n)
{
  void *ret = NULL;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_memalign_r (r, a, n));
  return ret;
}

void *
_valloc_r (struct _reent *r, size_t n)
{
  void *ret = NULL;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_valloc_r (r, n));
  return ret;
}

void *
_pvalloc_r (struct _reent *r, size_t n)
{
  void *ret = NULL;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_pvalloc_r (r, n));
  return ret;
}

struct mallinfo
_mallinfo_r (struct _reent *r)
{
  struct mallinfo ret = { 0 };
  ONE_LANE_AT_A_TIME (ret = __simt_dl_mallinfo_r (r));
  return ret;
}

int
_mallopt_r (struct _reent *r, int p, int v)
{
  int ret = 0;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_mallopt_r (r, p, v));
  return ret;
}

void
_malloc_stats_r (struct _reent *r)
{
  ONE_LANE_AT_A_TIME (__simt_dl_malloc_stats_r (r));
}

int
_malloc_trim_r (struct _reent *r, size_t pad)
{
  int ret = 0;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_malloc_trim_r (r, pad));
  return ret;
}

size_t
_malloc_usable_size_r (struct _reent *r, void *p)
{
  size_t ret = 0;
  ONE_LANE_AT_A_TIME (ret = __simt_dl_malloc_usable_size_r (r, p));
  return ret;
}

/* The public entry points, which MALLOC_PROVIDED removes from stdlib.  */

#ifndef _REENT_ONLY
void *
malloc (size_t n)
{
  return _malloc_r (_REENT, n);
}

void
free (void *p)
{
  _free_r (_REENT, p);
}

void
cfree (void *p)
{
  _free_r (_REENT, p);
}

void *
realloc (void *p, size_t n)
{
  return _realloc_r (_REENT, p, n);
}

void *
calloc (size_t n, size_t s)
{
  return _calloc_r (_REENT, n, s);
}

void *
memalign (size_t a, size_t n)
{
  return _memalign_r (_REENT, a, n);
}

void *
valloc (size_t n)
{
  return _valloc_r (_REENT, n);
}

void *
pvalloc (size_t n)
{
  return _pvalloc_r (_REENT, n);
}

struct mallinfo
mallinfo (void)
{
  return _mallinfo_r (_REENT);
}

int
mallopt (int p, int v)
{
  return _mallopt_r (_REENT, p, v);
}

void
malloc_stats (void)
{
  _malloc_stats_r (_REENT);
}

int
malloc_trim (size_t pad)
{
  return _malloc_trim_r (_REENT, pad);
}

size_t
malloc_usable_size (void *p)
{
  return _malloc_usable_size_r (_REENT, p);
}
#endif /* !_REENT_ONLY */

#endif /* __SIMT__ */
