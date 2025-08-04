/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: BSD-2-Clause
*/

#include <errno.h>
#include <_ansi.h>

extern char *_HeapPtr;
extern char *_HeapBottom;
extern char *_HeapTop;

char *sbrk(int nbytes)
{
	char *newheap = _HeapPtr + nbytes;
	if (newheap > _HeapTop)
	{
		errno = ENOMEM;
		return ((char *)-1);
	}
	char *retptr = _HeapPtr;
	_HeapPtr = newheap;
	return retptr;
}
