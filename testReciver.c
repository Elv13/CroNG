#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void manageQueue(DBusMessageIter args, DBusMessage* msg) {
  int jobId;
  char* command;
  int second;
  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Missiing ID\n");
    return;
  }
  else if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
    dbus_message_iter_get_basic(&args, &jobId);
  }
  else {
    fprintf(stderr, "Wrong data type (ID)\n");
    return;
  }

  dbus_message_iter_next(&args);
  /*if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Missiing ID\n");
    return;
  }*/
  if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
    dbus_message_iter_get_basic(&args, &command);
  }
  else {
    fprintf(stderr, "Wrong data type (Command)\n");
    return;
  }
  
  dbus_message_iter_next(&args);
  /*if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Missing ID\n");
    return;
  }
  else */
  if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
    dbus_message_iter_get_basic(&args, &second);
  }
  else {
    fprintf(stderr, "Wrong data type (seconfLeft)\n");
    return;
  }
  

  printf("Queuing %d: %s Time left: %d\n", jobId, command, second);
}

void manageExec(DBusMessageIter args, DBusMessage* msg) {
  int jobId;
  char* command;
  char* text;

  if (!dbus_message_iter_init(msg, &args)) {
    fprintf(stderr, "Missiing ID\n");
    return;
  }
  else if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
    dbus_message_iter_get_basic(&args, &jobId);
  }
  else {
    fprintf(stderr, "Wrong data type (ID)\n");
    return;
  }
  dbus_message_iter_next(&args);
  if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
    dbus_message_iter_get_basic(&args, &command);
  }
  else {
    fprintf(stderr, "Wrong data type (command)\n");
    return;
  }
  dbus_message_iter_next(&args);
  if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
    dbus_message_iter_get_basic(&args, &text);
  }
  else {
    fprintf(stderr, "Wrong data type (textt)\n");
    return;
  }
  printf("Exec %d: %s Text: %s\n", jobId, command, text);
}

void receive() {
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   int ret;
   dbus_error_init(&err);
   
   // connect to the bus and check for errors
   conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err); 
   }
   if (NULL == conn)
      exit(1);
   
   ret = dbus_bus_request_name(conn, "crong.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err); 
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
      exit(1);

   // add a rule for which messages we want to see
   dbus_bus_add_match(conn, "type='signal',interface='crong.signal.Type'", &err); // see signals from the given interface
   dbus_connection_flush(conn);
   if (dbus_error_is_set(&err)) { 
      fprintf(stderr, "Match Error (%s)\n", err.message);
      exit(1); 
   }
   printf("Match rule sent\n");

   while (true) {
      dbus_connection_read_write(conn, 0);
      msg = dbus_connection_pop_message(conn);

      if (NULL == msg) { 
         sleep(1);
         continue;
      }

      if (dbus_message_is_signal(msg, "crong.signal.Type", "TimeLeft")) {
         manageQueue(args,msg);
      }
      else if (dbus_message_is_signal(msg, "crong.signal.Type", "commandOutput")) {
         manageExec(args,msg);
      }
      dbus_message_unref(msg);
   }
   dbus_connection_close(conn);
}

int main(int argc, char** argv) {
  receive();
  return 0;
}
