#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dbus-1.0/dbus/dbus.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#define MINUTE 1
#define HOURS 2
#define DOM 3
#define MONTH 4
#define DOW 5
#define false 0
#define true 1
#define bool char

struct JobInfo {
  int id;
  struct tm timeInfo;
  char* command;
  char state;
  char* error;
};
#define JobInfo struct JobInfo

void sendsignal2(DBusConnection* conn2, JobInfo* currentJob, char* line) {
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn =conn2;
   DBusError err;
   int ret;
   dbus_uint32_t serial = 0;

   dbus_error_init(&err);

   ret = dbus_bus_request_name(conn, "crong.signal.source", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Name Error (%s)\n", err.message); 
      dbus_error_free(&err); 
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {/*error*/}
   msg = dbus_message_new_signal("/crong/data/propagation","crong.signal.Type","commandOutput"); // name of the signal
   if (NULL == msg)  { 
      fprintf(stderr, "Message Null\n"); 
      exit(1); 
   }

   dbus_message_iter_init_append(msg, &args);
   dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(currentJob->id));
   dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(currentJob->command));
   dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &line);

   if (!dbus_connection_send(conn, msg, &serial)) {/*error*/}
   dbus_connection_flush(conn);
   dbus_message_unref(msg);
}

void sendsignal(DBusConnection* conn2, JobInfo* currentJob, int secondLeft) {
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn =conn2;
   DBusError err;
   int ret;
   dbus_uint32_t serial = 0;

   // initialise the error value
   dbus_error_init(&err);

   // register our name on the bus, and check for errors
   ret = dbus_bus_request_name(conn, "crong.signal.source", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Name Error (%s)\n", err.message); 
      dbus_error_free(&err); 
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
      /*exit(1);*/
   }

   // create a signal & check for errors 
   msg = dbus_message_new_signal("/crong/data/propagation", // object name of the signal
                                 "crong.signal.Type", // interface name of the signal
                                 "TimeLeft"); // name of the signal
   if (NULL == msg) 
   { 
      fprintf(stderr, "Message Null\n"); 
      exit(1); 
   }
   
   // append arguments onto signal
   dbus_message_iter_init_append(msg, &args);
   dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(currentJob->id));
   dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(currentJob->command));
   dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &secondLeft);

   // send the message and flush the connection
   if (!dbus_connection_send(conn, msg, &serial)) {
      fprintf(stderr, "Out Of Memory!\n"); 
      exit(1);
   }
   dbus_connection_flush(conn);
   
   printf("Signal Sent %d\n",secondLeft);
   
   // free the message and close the connection
   dbus_message_unref(msg);
   /*dbus_connection_close(conn);*/
}

void system_error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void exec_cmd(int *to_me, int *from_me, char *paramArray[]) {
  if (dup2(to_me[0], STDIN_FILENO) == -1)
    system_error("dup2(to_me[0])");
  if (dup2(from_me[1], STDOUT_FILENO) == -1)
    system_error("dup2(from_me[1])");
  if (close(to_me[0]) == -1)
    system_error("close(to_me[0])");
  if (close(to_me[1]) == -1)
    system_error("close(to_me[1])");
  if (close(from_me[0]) == -1)
    system_error("close(from_me[0])");
  if (close(from_me[1]) == -1)
    system_error("close(from_me[1])");
  
  execvp(paramArray[0], paramArray);
  system_error("execvp(paramArray)");
  exit(EXIT_FAILURE);
}

char** getParam(char* command) {
  int counter = 0,space =0,previous =0,i=0;
  while ((command[space] != ' ') && (space != strlen(command))) space++;
  while (space != strlen(command)) {
    counter++;
    previous = space;
    space++;
    while ((command[space] != ' ') && (space != strlen(command))) space++;
  }
  counter++;

  char **paramArray = (char**) malloc(sizeof(char*) *(counter + 2));

  i = 0;
  space =0;
  previous =0;
  while ((command[space] != ' ') && (space != strlen(command))) space++;
  while (space < strlen(command)+1) {
    int paramLenght = space;
    char *param = (char*) malloc((paramLenght + 1)*sizeof(char));
    int j;
    for (j =0; j <  (space - previous); j++)
	    param[j] = command[j + previous];
    param[j] = '\0';
    paramArray[i] = param;
    printf("A command param: %s \n",param);
    i++;
    space++;
    previous = space;
    while ((command[space] != ' ') && (space != strlen(command))) space++;
  }
  paramArray[i] = NULL;
  return paramArray;
}

int execCommand(JobInfo* jobInfo, DBusConnection* conn) {
  char* command = jobInfo->command;
  int counter;
  int i = 0;
  pid_t pid, pidchild;
  int status;
  int to_cmd[2], from_cmd[2];
  int findStart;
  int j = 0;
  char **paramArray = getParam(command);

  if (pipe(to_cmd) == -1)
    system_error("pipe(to_cmd)");

  if (pipe(from_cmd) == -1)
    system_error("pipe(from_cmd)");

  printf("I am parent PID %d\n", getpid());

  switch (pid = fork()) {
  case (pid_t) -1:
	  system_error("fork");
  case (pid_t) 0:
    printf(" I am child PID=%d\n", getpid());
    exec_cmd(to_cmd, from_cmd, paramArray);
  }

  char bufferW[256];
  if (close(from_cmd[1]) == -1)
    system_error("close(from_cmd[1])");

  if (close(to_cmd[0]) == -1)
    printf("close(to_cmd[0])");

  char bufferR[256];
  ssize_t nr;
  i=0;

  printf("    >>");
  char buffer2[2048];
  int l =0;
  do {
    nr = read(from_cmd[0], bufferR, sizeof bufferR);
    if (nr == -1)
      system_error("read(bufferR)");
    if (nr == 0)
      break;

    for (i = 0; i < nr; i++) {
      if (bufferR[i] == '\n') {
	buffer2[l] = '\0';
	printf("\n    >>%s",buffer2);
	l =0;
        sendsignal2(conn, jobInfo, buffer2);
      }
      else {
	//printf("%c", bufferR[i]);
	buffer2[l++] = bufferR[i];
      }
    }
  } while (nr > 0);
  printf("\n");

  if (close(to_cmd[1]) == -1)
    system_error("close(to_cmd[1])");
  if (close(from_cmd[0]) == -1)
    system_error("close(from_cmd[0])");

  pidchild = wait(NULL);	/* attention adresse ! ! ! */
  printf(" Death of child PID=%d\n", pidchild);
}


int main() {
  char testStr[] = "* * * * * ls -l / --color";
  char tmp[3];
  unsigned int length = strlen(testStr);
  int index =0, tmpI =0;
  bool previous = false, ready = false;
  //JobInfo jobInfo;
  JobInfo jobInfo;
  jobInfo.id =1;
  jobInfo.timeInfo.tm_sec =0;
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
	    jobInfo.timeInfo.tm_min = -1;
	  else
	    jobInfo.timeInfo.tm_min = atoi(tmp);
	  break;
	case HOURS:
	  if (tmp[0] == '*')
	    jobInfo.timeInfo.tm_hour = -1;
	  else
	    jobInfo.timeInfo.tm_hour = atoi(tmp);
	  break;
	case DOM:
	  if (tmp[0] == '*')
	    jobInfo.timeInfo.tm_mday = -1;
	  else
	    jobInfo.timeInfo.tm_mday = atoi(tmp);
	  break;
	case MONTH:
	  if (tmp[0] == '*')
	    jobInfo.timeInfo.tm_mon = -1;
	  else
	    jobInfo.timeInfo.tm_mon = atoi(tmp);
	  break;
	case DOW:
	  if (tmp[0] == '*')
	    jobInfo.timeInfo.tm_wday = -1;
	  else
	    jobInfo.timeInfo.tm_wday = atoi(tmp);
	  ready = true;
	  break;
      }
      previous = false;
      tmpI =0;
    }
  }
  tmp[tmpI++] = '\0';
  //int minuteLeft = (60 - 
  printf("Minute: %d \n",jobInfo.timeInfo.tm_sec);
  printf("Hours: %d \n",jobInfo.timeInfo.tm_hour);
  printf("DOM: %d \n",jobInfo.timeInfo.tm_mday);
  printf("Month: %d \n",jobInfo.timeInfo.tm_mon);
  printf("Dow: %d \n",jobInfo.timeInfo.tm_wday);
  printf("Command: %s \n",jobInfo.command);
  
  int dayPerMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  jobInfo.timeInfo.tm_year = timeinfo->tm_year;
//   
  if (jobInfo.timeInfo.tm_hour == -1) {
    if ((timeinfo->tm_min < jobInfo.timeInfo.tm_min) || (jobInfo.timeInfo.tm_min == -1)) {
      jobInfo.timeInfo.tm_hour =timeinfo->tm_hour;
    }
    else {
      if (timeinfo->tm_hour != 23) {
	jobInfo.timeInfo.tm_hour =timeinfo->tm_hour +1;
      }
      else {
	jobInfo.timeInfo.tm_hour = 0;
      }
    }
  }
  
  if (jobInfo.timeInfo.tm_min == -1) {
    if (jobInfo.timeInfo.tm_hour != timeinfo->tm_hour) {
      jobInfo.timeInfo.tm_min = 0;
    }
    else {
      if (timeinfo->tm_min != 59) {
	jobInfo.timeInfo.tm_min = timeinfo->tm_min +1;
      }
      else {
	jobInfo.timeInfo.tm_min = 0;
      }
    }
  }
  
  if (jobInfo.timeInfo.tm_mday == -1) {
    if ((timeinfo->tm_hour < jobInfo.timeInfo.tm_hour) 
      || ((timeinfo->tm_hour == jobInfo.timeInfo.tm_hour) && (timeinfo->tm_min < jobInfo.timeInfo.tm_min))) {
      jobInfo.timeInfo.tm_mday = timeinfo->tm_mday;
    }
    else {
      if (timeinfo->tm_mday != dayPerMonth[timeinfo->tm_mon]) {
	jobInfo.timeInfo.tm_mday =timeinfo->tm_mday +1;
      }
      else {
	jobInfo.timeInfo.tm_mday = 1;
      }
    }
  }
  
  if (jobInfo.timeInfo.tm_mon == -1) {
    if ((timeinfo->tm_mday < jobInfo.timeInfo.tm_mday) 
      || ((timeinfo->tm_mday == jobInfo.timeInfo.tm_mday) && (timeinfo->tm_hour < jobInfo.timeInfo.tm_hour))
      || ((timeinfo->tm_mday == jobInfo.timeInfo.tm_mday) && (timeinfo->tm_hour == jobInfo.timeInfo.tm_hour) && (timeinfo->tm_min < jobInfo.timeInfo.tm_min))) {
      jobInfo.timeInfo.tm_mon =timeinfo->tm_mon;
    }
    else {
      if (timeinfo->tm_mon != 11) {
	jobInfo.timeInfo.tm_mon =timeinfo->tm_mon +1;
      }
      else {
	jobInfo.timeInfo.tm_mon = 0;
      }
    }
  }
  
  
  printf("Minute left: %d \n",jobInfo.timeInfo.tm_min);
  printf("Hours left: %d \n",jobInfo.timeInfo.tm_hour);
  printf("DOM left: %d \n",jobInfo.timeInfo.tm_mday);
  printf("Month left: %d \n",jobInfo.timeInfo.tm_mon);
  /*printf("Dow left: %d \n",jobInfo.timeInfo.tm_wday);*/
  printf ( "The current date/time is: %s", asctime(timeinfo) );
  printf ( "Will execute on: %s", asctime(&(jobInfo.timeInfo)) );
  time_t jobTime = mktime(&(jobInfo.timeInfo));
  
  if (jobTime <= time(NULL)) {
    jobInfo.timeInfo.tm_year++;
    jobTime = mktime(&(jobInfo.timeInfo));
  }
  
  int diffTime = difftime(jobTime,time(NULL));
  
  
  printf ( "second left: %d\n", diffTime);
  
  /*Dbus part*/
  DBusError err;
  DBusConnection* conn;
  // initialise the error value
  dbus_error_init(&err);

  // connect to the DBUS system bus, and check for errors
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) { 
    fprintf(stderr, "Connection Error (%s)\n", err.message); 
    dbus_error_free(&err); 
  }
  if (NULL == conn) { 
    exit(1); 
  }

  
  int i = diffTime;
  while (--i != 0) {
    sendsignal(conn, &jobInfo, i);
    sleep(1);
  }
  //sleep(diffTime);
  printf ( "exec '%s'\n",jobInfo.command);
  execCommand(&jobInfo, conn);
  return 0;
}
