#pragma once


#include <ncurses.h>
#include <string.h>
#include <ctype.h>


#define V_TRESH_LOW 11.0
#define V_TRESH_HI 12.0

#define BAR_LEN 50

typedef struct
{
  unsigned clock, total_t, work_t, prev_total_t, prev_work_t, load;
} core_data;

typedef struct
{
  char procname[512];
  unsigned max_clock, cores_no, ph_cores_no;
  core_data* cores;
} cpu_data;


void bar(char* str, unsigned percent);

void print_basics(cpu_data* cpudata);

void print_load(cpu_data* cpudata, int line);

void print_clock(cpu_data* cpudata, int line);

void print_temp(int temp, unsigned line, unsigned col, const char* label);

void print_v(float, unsigned, unsigned, const char*);

void ncurses_init_color();
