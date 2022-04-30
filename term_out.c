#include "term_out.h"

extern int colors;










void bar(char* str, unsigned percent)
{
  for(unsigned i = 0; i < BAR_LEN; ++i) str[i] = (i < (percent/2) ? '|' : ' ');
  str[BAR_LEN] = '\0';
}







void print_basics(cpu_data* cpudata)
{
  char brandstr[256];
  int brandcolor = 0;
  
  sscanf(cpudata->procname, "%s", brandstr);

  for(unsigned i = 0; i < strlen(brandstr); ++i)
    {
      if( !isalpha(brandstr[i]) ) brandstr[i] = 0;
    }

  if( !strcmp( brandstr, "AMD") ) brandcolor = COLOR_PAIR(COLOR_RED);
  else if( !strcmp( brandstr, "Intel") ) brandcolor = COLOR_PAIR(COLOR_BLUE);
  else brandcolor = COLOR_PAIR(COLOR_WHITE);

  mvprintw( 0, 0, "CPU: ");
  if( colors ) attron( brandcolor );
  printw( "%s\n", cpudata->procname);
  if( colors ) attroff( brandcolor );
  printw( "%u cores, %u threads, %d MHz maximum base clock\n", cpudata->ph_cores_no, cpudata->cores_no, cpudata->max_clock);
}







void print_clock(cpu_data* cpudata, int line)
{
  int color;
  unsigned cscale = 0;

  for(unsigned curr_core = 0; curr_core < cpudata->cores_no; ++curr_core)
    {
      cscale = cpudata->cores[curr_core].clock*100 / cpudata->max_clock;
      
      if( cscale > 100 ) color = COLOR_PAIR(COLOR_RED);
      else if( cscale >= 50 && cscale <= 100 ) color = COLOR_PAIR(COLOR_GREEN);
      else color = COLOR_PAIR(COLOR_WHITE);
      
      if(colors) attron(color);
      mvprintw(line+curr_core, 0, "CPU%u: %u MHz   ", curr_core, cpudata->cores[curr_core].clock);
      if(colors) attroff(color);
    }
}





void print_load(cpu_data* cpudata, int line)
{
  int color;
  static char barbuf[BAR_LEN];

  for(unsigned curr_core = 0; curr_core < cpudata->cores_no; ++curr_core)
    {
      if(cpudata->cores[curr_core].load < 50) color = COLOR_PAIR(COLOR_GREEN);
      else if(cpudata->cores[curr_core].load >= 50 && cpudata->cores[curr_core].load < 70) color = COLOR_PAIR(COLOR_YELLOW);
      else color = COLOR_PAIR(COLOR_RED);
  
      bar(barbuf, cpudata->cores[curr_core].load);
      
      if(colors) attron(color);
      mvprintw(line+curr_core, 0 ,"CPU%u: %u %%\t[%s] ", curr_core, cpudata->cores[curr_core].load, barbuf);
      if(colors) attroff(color);
    }
}	  








void print_temp(int temp, unsigned line, unsigned col, const char* label)
{
  int color;
  if(temp < 0) color = COLOR_PAIR(COLOR_BLUE);
  else if(temp >= 50 && temp < 70) color = COLOR_PAIR(COLOR_YELLOW);
  else if(temp >= 70) color = COLOR_PAIR(COLOR_RED);
  else color = COLOR_PAIR(COLOR_WHITE);

  if(colors) attron(color);
  mvprintw(line, col, "%s: %d °C    ", label, temp);
  if(colors) attroff(color);
}






void print_v(float v, unsigned line, unsigned col, const char* label)
{
  int color;
  if(v < V_TRESH_LOW) color = COLOR_PAIR(COLOR_RED);
  else if(v >= V_TRESH_LOW && v < V_TRESH_HI) color = COLOR_PAIR(COLOR_YELLOW);
  else if(v >= V_TRESH_HI) color = COLOR_PAIR(COLOR_GREEN);

  if(colors) attron(color);
  mvprintw(line, col, "%s: %2.2f V    ", label, v);
  if(colors) attroff(color);
}






void ncurses_init_color()
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);
  
  if(has_colors() == FALSE)
    {
      fprintf(stderr, "monitor.x: Your terminal does not support colors. Goin' ol' school black&white.\n");
      colors = 0;
    }
  else
    {
      colors = 1;
      start_color();

      if(can_change_color() == TRUE)
	{
	  init_color(COLOR_GREEN, 0, 1000, 0);
	  init_color(COLOR_YELLOW, 1000, 800, 0);
	  init_color(COLOR_RED, 1000, 0, 0);
	}
  
      init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
      init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
      init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
      init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
      init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    }

}
