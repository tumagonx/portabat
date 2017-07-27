#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned __int64 QWORD;
#define BELOW_NORMAL_PRIORITY_CLASS 0x4000
#define ABOVE_NORMAL_PRIORITY_CLASS 0x8000

EXTERN_C VOID CDECL PrintShortDateAndTime(SYSTEMTIME *st)
{
  char cBuf[160];
  if (GetDateFormat(LOCALE_USER_DEFAULT, 0, st, NULL, cBuf, sizeof cBuf))
    {
      CharToOem(cBuf, cBuf);
      printf("%s ", cBuf);
    }

  if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, st, NULL, cBuf, sizeof cBuf))
    {
      CharToOem(cBuf, cBuf);
      fputs(cBuf, stdout);
    }
}
EXTERN_C VOID CDECL PrintAbsoluteTime(QWORD qwTime)
{
  double dblMilliseconds;
  DWORD dwSeconds;
  DWORD dwMinutes;
  DWORD dwHours;
  DWORD dwDays;
  DWORD dwWeeks;

  dwWeeks = (DWORD)(qwTime / 6048000000000); qwTime %= 6048000000000;
  dwDays = (DWORD)(qwTime / 864000000000); qwTime %= 864000000000;
  dwHours = (DWORD)(qwTime / 36000000000); qwTime %= 36000000000;
  dwMinutes = (DWORD)(qwTime / 600000000); qwTime %= 600000000;
  dwSeconds = (DWORD)(qwTime / 10000000); qwTime %= 10000000;
  dblMilliseconds = (double)qwTime / 10000;

  if (dwWeeks)
    printf("%u weeks, ", dwWeeks);

  if (dwDays)
    printf("%u days, ", dwDays);

  printf("%u:%.2u:%.2u %f",
	 dwHours, dwMinutes, dwSeconds, dblMilliseconds);

}
#define win_perror

#ifdef _DLL
#pragma comment(lib, "minwcrt.lib")
#endif

int
main(int argc, char **argv)
{
  DWORD dwPID;
  HANDLE hProcess;
  DWORD dwPriority;
  FILETIME ftCreationTime;
  FILETIME ftLocalCreationTime;
  FILETIME ftDummy;
  FILETIME ftKernelTime;
  FILETIME ftUserTime;
  SYSTEMTIME stCreationTime;

  if (argc < 2)
    {
      puts("Usage:\r\n"
	   "SETPRIO pid [priorityclass]\r\n"
	   "\n"
	   "Prints current priority class if priorityclass parameter is empty. To set\r\n"
	   "priority class, use one of the following:\r\n"
	   "\n"
	   "H High priority class. Specify this class for a process that performs time-\r\n"
	   "  critical tasks that must be executed immediately. The threads of the process\r\n"
	   "  preempt the threads of normal or idle priority class processes. An example is\r\n"
	   "  Windows Task List, which must respond quickly when called by the user,\r\n"
	   "  regardless of the load on the operating system.\r\n"
	   "\n"
	   "  Use extreme care when using the high-priority class, because a high-priority\r\n"
	   "  class application can use nearly all available CPU time.\r\n"
	   "\n"
	   "I Idle priority class. Specify this class for a process whose threads run only\r\n"
	   "  when the system is idle. The threads of the process are preempted by the\r\n"
	   "  threads of any process running in a higher priority class. An example is a\r\n"
	   "  screen saver.\r\n"
	   "  The idle-priority class is inherited by child processes.\r\n"
	   "\n"
	   "N Normal priority class. Specify this class for a process with no special\r\n"
	   "  scheduling needs.\r\n"
	   "\n"
	   "R Realtime priority class. Specify this class for a process that has the\r\n"
	   "  highest possible priority. The threads of the process preempt the threads of\r\n"
	   "  all other processes, including operating system processes performing\r\n"
	   "  important tasks. For example, a real-time process that executes for more than\r\n"
	   "  a very brief interval can cause disk caches not to flush or cause the mouse\r\n"
	   "  to be unresponsive.\r\n"
	   "\n"
	   "A Priority above normal but below high.\r\n"
	   "\n"
	   "B Priority below normal but above high.\r\n"
	   "\n"
	   "The A or B options requires Windows 2000, Windows XP, Windows\r\n"
	   "Server 2003 or later versions.");

      return 0;
    }

  dwPID = strtoul(argv[1], NULL, 0);

  if (argv[2])
    {
      switch (argv[2][0] | 0x20)
	{
      	case 'h': dwPriority = HIGH_PRIORITY_CLASS; break;
      	case 'i': dwPriority = IDLE_PRIORITY_CLASS; break;
	case 'b': dwPriority = BELOW_NORMAL_PRIORITY_CLASS; break;
      	case 'n': dwPriority = NORMAL_PRIORITY_CLASS; break;
	case 'a': dwPriority = ABOVE_NORMAL_PRIORITY_CLASS; break;
      	case 'r': dwPriority = REALTIME_PRIORITY_CLASS; break;
	default:
	  fprintf(stderr, "Unsupported priority class - %c.\n",
		  argv[2][0] | 0x20);
	  return -1;
	}

      hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwPID);

      if (hProcess == NULL)
	{
	  win_perror("OpenProcess");
	  return 1;
	}

      if (!SetPriorityClass(hProcess, dwPriority))
	{
	  win_perror("SetPriorityClass");
	  return 2;
   	}
    }
  else
    {
      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);

      if(hProcess == NULL)
	{
	  win_perror("OpenProcess");
	  return 1;
	}

      if ((dwPriority = GetPriorityClass(hProcess)) == 0)
	{
	  win_perror("GetPriorityClass");
	  return 2;
   	}

      printf("Priority class for pid %i: ", dwPID);

      switch (dwPriority)
	{
      	case HIGH_PRIORITY_CLASS:
	  puts("High");
	  break;
      	case IDLE_PRIORITY_CLASS:
	  puts("Idle");
	  break;
	case BELOW_NORMAL_PRIORITY_CLASS:
	  puts("Below normal");
	  break;
      	case NORMAL_PRIORITY_CLASS:
	  puts("Normal");
	  break;
	case ABOVE_NORMAL_PRIORITY_CLASS:
	  puts("Above normal");
	  break;
      	case REALTIME_PRIORITY_CLASS:
	  puts("Realtime");
	  break;
	default:
	  printf("%i\n", dwPriority);
	}

      if (!GetProcessTimes(hProcess, &ftCreationTime, &ftDummy, &ftKernelTime,
			   &ftUserTime))
	{
	  if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	    return 0;

	  win_perror("Error getting process times");
	  return 1;
	}

      if (*(QWORD*)&ftCreationTime == 0)
	return 0;

      FileTimeToLocalFileTime(&ftCreationTime, &ftLocalCreationTime);
      FileTimeToSystemTime(&ftLocalCreationTime, &stCreationTime);

      fputs("Start time: ", stdout);
      PrintShortDateAndTime(&stCreationTime);
      fputc('\n', stdout);

      if (*(QWORD*)&ftUserTime == 0)
	return 0;

      fputs("Kernel time: ", stdout);
      PrintAbsoluteTime(*(QWORD*)&ftKernelTime);
      fputs("\nUser time: ", stdout);
      PrintAbsoluteTime(*(QWORD*)&ftUserTime);
      fputc('\n', stdout);

      return 0;
    }
}
