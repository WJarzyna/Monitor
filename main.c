#include <stdio.h>
#include <time.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "term_out.h"
#include <unistd.h>
#include <fcntl.h>






#define PLOT_BEGIN "file='%s'\n\
set xdata time\n\
set timefmt \"%%H:%%M:%%S\"\n\
set format x \"%%H:%%M:%%S\"\n\
set key left top\n\
set xtics rotate by -30\n\
set origin 0,0\n"
#define PLOT_LOAD "\nset terminal x11 0 noraise title 'Monitor data graphs - Core load'\n\
set size 1,1\n\
\tplot "
#define PLOT_FREQ "\nset terminal x11 1 noraise title 'Monitor data graphs - Frequencies'\nset size 1,0.95\n\tplot "
#define PLOT_TEMP "\nset terminal x11 2 noraise title 'Monitor data graphs - Temperatures'\nset size 1,1\n\tplot file using 1:2 with lines linewidth 2 title 'CPU die'"
#define PLOT_V "\nset terminal x11 3 noraise title 'Monitor data graphs - Voltages'\nset size 1,1\n\tplot file using 1:3 with lines linewidth 2 title 'Battery'"
#define PLOT_SINGLE "file using 1:%d with lines linewidth 2 title 'CPU%d'"





#define TEMP_PATH "/sys/class/hwmon/hwmon4/temp1_input"
#define BATT_PATH "/sys/class/hwmon/hwmon2/in0_input"
#define MAX_FREQ_PATH "/sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_max_freq"


#define LOG_FILE "/tmp/monitor_log"
#define SLOG_FILE "/home/kon/log_"


int colors;
unsigned log_en;










void panic()
{
  fprintf(stderr,"fopen() failed, aborting.\n");
  exit(-1);
}













void get_core_params(cpu_data* cpudata, FILE* log)
{
  static char buf[100];
  static unsigned user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0, gdummy = 0, total_t = 0, work_t = 0;

  FILE* cpuinfo = popen("cat /proc/cpuinfo | grep 'cpu MHz' | awk '{print $NF}'", "r");
      
  FILE* stat = popen("cat /proc/stat | grep cpu[0-9] | awk '{$1=\"\";print $0}'", "r");

  if( cpuinfo == NULL || stat == NULL) panic();

  for(unsigned curr_core = 0; curr_core < cpudata->cores_no; ++curr_core)
    {
      fscanf(cpuinfo, "%s", buf);
      cpudata->cores[curr_core].clock = (unsigned) atof(buf);

      fscanf(stat, "%u %u %u %u %u %u %u %u %u %u", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &gdummy, &gdummy);
      
      total_t = user + nice + system + idle + iowait + irq + softirq + steal;
      work_t = user + nice + system + irq + softirq + steal;
      cpudata->cores[curr_core].load = (( (work_t - cpudata->cores[curr_core].prev_work_t)*100 ) / (total_t - cpudata->cores[curr_core].prev_total_t));
      cpudata->cores[curr_core].prev_total_t = total_t;
      cpudata->cores[curr_core].prev_work_t = work_t;

      if( log_en ) fprintf(log, "%d %d ", cpudata->cores[curr_core].clock, cpudata->cores[curr_core].load);
    }

  pclose(cpuinfo);
  pclose(stat);
}











float get_param(char* addr)
{
  static int param;

  FILE* info = fopen(addr, "r");

  if( info == NULL ) panic();

  fscanf(info, "%d", &param);
  fclose(info);

  return param/1000.0;
}










void run(cpu_data* cpudata)
{
  struct timespec timed = { 0, 1e8};
  char timebuf[16];
  time_t rawtime, old_rawtime;
  int temp = 0;
  float batt_v = 0;
  int rf = 1, sc = 10;
  FILE* log = NULL;
  int dec = 0;
  
  if( log_en )
    {
      log = fopen( LOG_FILE, "a+");
      if( log == NULL )
	{
	  fprintf( stderr, "Unable to open log file. Logging disabled\n");
	  log_en = 0;
	}
    }

  mvprintw(3, 0, "Clock speeds:");
  mvprintw(4+cpudata->cores_no, 0, "Load:");
  
  while( rf )
    {
      temp = (int) get_param( TEMP_PATH );
      batt_v = get_param( BATT_PATH );
      
      print_temp(temp, 2, 0, "CPU die");
      print_v(batt_v, 2, 20, "Battery voltage");
      
      if( log_en )
	{
	  old_rawtime = rawtime;
	  rawtime = time(NULL);

	  strftime(timebuf, 15, "%T", localtime(&rawtime ) );

	  if( sc < 10 )
	    {
	       if( old_rawtime != rawtime ) dec = 0;
	       fprintf(log, "%s.%u %d %2.2f ", timebuf, dec, temp, batt_v);
	       dec += sc;
	    }
	  else fprintf(log, "%s %d %2.2f ", timebuf, temp, batt_v);
	}
      
      get_core_params(cpudata, log);


      print_clock(cpudata, 4);
      print_load(cpudata, 5+cpudata->cores_no);
      
      if( log_en )
	{
	  fprintf(log, "\n");
	  fflush(log);
	}
      
      for( unsigned i = 0; i < sc; ++i)
	{
	  nanosleep(&timed, NULL);
	  switch( getch() )
	    {
	    case 27: rf = 0; i = sc; break;
	    case '+': if( sc > 1 ) --sc; i = sc; break;
	    case '-': if( sc < 100 ) ++sc; i = sc; break;
	    default: break;
	    }
	}
    }
  
  if( log_en ) fclose(log);
}











int cpu_detect(cpu_data* cpudata)
{
  unsigned clock;
  
  FILE *proc = popen("cat /proc/cpuinfo | grep 'processor\\|model name\\|cpu cores' | tail -n 3 |  awk -F': ' '{print $2}'", "r");
  if( proc == NULL ) return -1;
  if( fscanf(proc, "%u\n%511[^\n]%u", &(cpudata->cores_no), cpudata->procname, &(cpudata->ph_cores_no) ) < 1) return -1;
  if( pclose(proc) ) return -1;
  ++cpudata->cores_no;
  
  FILE *max_freq = fopen(MAX_FREQ_PATH, "r");
  if( max_freq == NULL ) return -1;
  if( fscanf(max_freq, "%u", &clock) < 1) return -1;
  if( fclose(max_freq) ) return -1;
  cpudata->max_clock = clock/1000;

  cpudata->cores = calloc(cpudata->cores_no, sizeof(core_data));
  if( cpudata->cores == NULL ) return -1;
  
  return 0;
}




















void create_plot(cpu_data* cpudata)
{
  FILE *plotcmd = fopen("plot", "w");
  
  fprintf(plotcmd, PLOT_BEGIN, LOG_FILE);

  fprintf(plotcmd, PLOT_FREQ);
  for(unsigned curr_core = 0; curr_core < cpudata->cores_no; ++curr_core)
    {
      fprintf(plotcmd, PLOT_SINGLE, 4+curr_core*2, curr_core);
      if( curr_core < (cpudata->cores_no - 1) ) fprintf(plotcmd, " ,\\\n\t\t" );
    }
  
  fprintf(plotcmd, PLOT_LOAD);
  for(unsigned curr_core = 0; curr_core < cpudata->cores_no; ++curr_core)
    {
      fprintf(plotcmd, PLOT_SINGLE, 5+curr_core*2, curr_core);
      if( curr_core < (cpudata->cores_no - 1) ) fprintf(plotcmd, " ,\\\n\t\t" );
    }

  fprintf(plotcmd, PLOT_TEMP);
  fprintf(plotcmd, PLOT_V);
  fclose(plotcmd);
}











void get_screen_res( char* out )
{
  FILE* res = popen("xrandr | grep '*' | awk '{print $1}'", "r");

  if( res == NULL )
    {
      fprintf( stderr, "Failed to get screen geometry, defaulting to 640x480\n");
      sprintf( out, "640x480" );
      return;
    }
  
  fscanf(res, "%s", out);
  pclose( res );
}














void plot_log( cpu_data* cpudata )
{
  char res[16];
  char plotcmd[256];
  
  create_plot(cpudata);
  get_screen_res(res);
  
  sprintf(plotcmd, "gnuplot -geometry %s -p plot", res);
  system(plotcmd);
  
  remove("plot");  
}








void save_log( void )
{
  char lfilename[256];
  char timebuf[64];
  char buf[1024];
  
  time_t rawtime = time(NULL);
  strftime( timebuf, 63, "%F %T", localtime(&rawtime ) );
  sprintf( lfilename, "%s%s", SLOG_FILE, timebuf);
  
  int slog = open( lfilename, O_RDWR);
  if( slog == -1 ) fprintf( stderr, "Unable to open log save file. Log unsaved\n");
  else
    {
      FILE* log = fopen( LOG_FILE, "r");
      if( log == NULL ) fprintf( stderr, "Unable to open temporary log file. Log unsaved\n");
      else
	{
	  while( fgets( buf, 1023, log ) != NULL ) write( slog, buf, strlen( buf ) );

	  fclose( log );
	  close( slog );
 
	}
      
    }

}














int main(int argc, char** argv)
{
  cpu_data cpudata;
  unsigned log_save = 0;

  switch( argc )
    {
    case 1: log_en = 0; log_save = 0; break;
    case 2: log_en = 1; log_save = 0; break;
    case 3: log_en = 1; log_save = 1; break;
    default: fprintf( stderr, "Too many arguments.\n"); exit(-2);
    }
  
  ncurses_init_color();
 
  if( cpu_detect(&cpudata) )
  {
    fprintf(stderr,"CPU detection failed, aborting.\n");
    exit(-1);
  }

  print_basics(&cpudata);
  run(&cpudata);

  if( log_en )
    {
      plot_log( &cpudata );

      if( log_save ) save_log();
      remove( LOG_FILE );
    }
  
  free(cpudata.cores);
  
  endwin();

  return 0;
}
