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
