#pragma once
#include <stdio.h>
#define RTN_IF_NEG(x) if (x < 0) return x;
#define PRINTR_IF_NEG(x, s) if (x < 0) { fprintf(stdout, "%s", s); return x; }