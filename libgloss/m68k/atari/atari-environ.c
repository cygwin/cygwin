/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: BSD-2-Clause
*/

// Should point to a list of global environment variables.
char *__env[1] = { 0 };
char **environ = __env;