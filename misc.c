// misc.c
/* miscellaneous */

#include "defs.h"

#ifdef WIN32
#include "windows.h"
#else
#ifndef _SYS_TIME_H
#include "sys/time.h"
#endif
#endif

// returns the time in milliseconds truncated to a four byte integer
int GetTime(void)
{
#ifdef WIN32
	return GetTickCount();
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
#endif
}

/*
 *  I copied the InputWaiting() function as a whole
 *  from the 'vice' chess engine by BlueFeverSoft
 */
 
// returns non-zero when there is input in stdin waiting
// http://home.arcor.de/dreamlike/chess/
int InputWaiting()
{
#ifndef WIN32
  struct timeval tv;

  fd_set readfds;

  FD_ZERO (&readfds);
  FD_SET (fileno(stdin), &readfds);
  tv.tv_sec=0; tv.tv_usec=0;
  select(16, &readfds, 0, 0, &tv);

  return (FD_ISSET(fileno(stdin), &readfds));
#else
   static int init = 0, pipe;
   static HANDLE inh;
   DWORD dw;

   if (!init) {
     init = 1;
     inh = GetStdHandle(STD_INPUT_HANDLE);
     pipe = !GetConsoleMode(inh, &dw);
     if (!pipe) {
        SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
        FlushConsoleInputBuffer(inh);
      }
    }
    if (pipe) {
      if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
      return dw;
    } else {
      GetNumberOfConsoleInputEvents(inh, &dw);
      return dw <= 1 ? 0 : dw;
	}
#endif
}

char intStr[128];

// checks if input is in stdin
// if so it stops the search and stores the input in the global string 'intStr'
void CheckInput(searchinfo_t *sinfo)
{
	int i;

	if(InputWaiting()){
		sinfo->stop = true;

		for(i = 0; i < 128; i++){
			if(read(0, &intStr[i], 1) <= 0) break;
			if(intStr[i] == '\n'){
				intStr[i] = '\0';
				break;
			}
		}

		if(strstr(intStr, "quit") != NULL) sinfo->quit = true;
	}
}



