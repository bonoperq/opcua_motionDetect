#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <open62541/config.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/server.h>

#define MAX_LINE_LENGTH 40
#define CERT_PATH "./data/cert/"

struct ThreadArgs {
    UA_Server *server;
    UA_NodeId newNodeId;
};

UA_ByteString loadFile(const char *const path);
UA_ByteString loadFileTail(const char *filename);
void *watch_file(void *args);

#endif /* UTILS_H*/