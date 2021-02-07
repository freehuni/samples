#include <iostream>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>

using namespace std;

void timer_handler(int);

struct itimerval timervalue;

struct sigaction newSignal;
struct sigaction oldSignal;

void timer_sig_handler(int);

void (*timer_func_handler_pntr)(void);


int start_timer(int mSec, void (*timer_func_handler)(void))
{
  timer_func_handler_pntr = timer_func_handler;

  timervalue.it_interval.tv_sec = mSec / 1000;
  timervalue.it_interval.tv_usec = (mSec % 1000) * 1000;
  timervalue.it_value.tv_sec = mSec / 1000;
  timervalue.it_value.tv_usec = (mSec % 1000) * 1000;
  if(setitimer(ITIMER_REAL, &timervalue, NULL))
  {
    printf("\nsetitimer() error\n");
    return(1);
  }

  newSignal.sa_handler = &timer_sig_handler;
  newSignal.sa_flags = SA_NOMASK;
  if(sigaction(SIGALRM, &newSignal, &oldSignal))
  {
    printf("\nsigaction() error\n");
    return(1);
  }

  return(0);
}


void timer_sig_handler(int arg)
{
  timer_func_handler_pntr();
}

void onTimer()
{
    static int idx = 0;
    printf("onTimer:%d\n", idx++);
}

void stop_timer(void)
{
  timervalue.it_interval.tv_sec = 0;
  timervalue.it_interval.tv_usec = 0;
  timervalue.it_value.tv_sec = 0;
  timervalue.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &timervalue, NULL);

  sigaction(SIGALRM, &oldSignal, NULL);
}

int main()
{
    cout << "Hello World!" << endl;

    start_timer(1000,onTimer);

    while(1)
    {
        usleep(1000);
    }
    return 0;
}

