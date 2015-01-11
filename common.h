/*
 * ResCoin - Multiple type Resource Scheduling of VMs in pursuit of fairness 
 * Copyright (c) 2014, Coperd <lhcwhu@gmail.com>
 * All rights reserved.
 *
 * Author: Coperd <lhcwhu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __COMMON_H
#define __COMMON_H

int strtol_i(char const *s, char **end_ptr, int base, int *result);
int strtol_ui(char const *s, char **end_ptr, int base, unsigned int *result);
int strtol_l(char const *s, char **end_ptr, int base, long *result);
int strtol_ul(char const *s, char **end_ptr, int base, unsigned long *result);
int strtol_ll(char const *s, char **end_ptr, int base, long long *result);
int strtol_ull(char const *s, char **end_ptr, int base, unsigned long long *result);
int strtod_d(char const *s, char **end_ptr, double *result);

#endif
