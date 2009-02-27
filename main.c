#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MINUTE 1
#define HOURS 2
#define DOM 3
#define MONTH 4
#define DOW 5
#define false 0
#define true 1
#define bool char

struct JobInfo {
  char minute;
  char hours;
  char dom;
  char month;
  char dow;
  char* command;
};
#define JobInfo struct JobInfo

int main() {
  char testStr[] = "10 * 13 14 15 ls -l /home";
  char tmp[3];
  unsigned int length = strlen(testStr);
  int index =0, tmpI =0;
  bool previous = false, ready = false;
  JobInfo jobInfo;
  for (int i =0; i < length;i++) {
    if (((testStr[i] == '*') || ((testStr[i] >= '0') && (testStr[i] <= '9'))) &&  (ready == false)) {
      if (previous == false)
	index++;
      previous = true;
      tmp[tmpI++] = testStr[i];
    }
    else if (ready == true) {
      if (previous == false) {
	previous = true;
	jobInfo.command = malloc((length - i)* sizeof(char));
      }
      jobInfo.command[tmpI++] = testStr[i];
    }
    else if (ready == false) {
      tmp[tmpI++] = '\0';
      switch (index) {
	case MINUTE:
	  if (tmp[0] == '*')
	    jobInfo.minute = -1;
	  else
	    jobInfo.minute = atoi(tmp);
	  break;
	case HOURS:
	  if (tmp[0] == '*')
	    jobInfo.hours = -1;
	  else
	    jobInfo.hours = atoi(tmp);
	  break;
	case DOM:
	  if (tmp[0] == '*')
	    jobInfo.dom = -1;
	  else
	    jobInfo.dom = atoi(tmp);
	  break;
	case MONTH:
	  if (tmp[0] == '*')
	    jobInfo.month = -1;
	  else
	    jobInfo.month = atoi(tmp);
	  break;
	case DOW:
	  if (tmp[0] == '*')
	    jobInfo.dow = -1;
	  else
	    jobInfo.dow = atoi(tmp);
	  ready = true;
	  break;
      }
      previous = false;
      tmpI =0;
    }
  }
  tmp[tmpI++] = '\0';
  printf("Minute: %d \n",jobInfo.minute);
  printf("Hours: %d \n",jobInfo.hours);
  printf("DOM: %d \n",jobInfo.dom);
  printf("Month: %d \n",jobInfo.month);
  printf("Dow: %d \n",jobInfo.dow);
  printf("Command: %s \n",jobInfo.command);
  return 0;
}
