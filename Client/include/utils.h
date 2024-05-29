#ifndef UTILS_H
#define UTILS_H

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <open62541/config.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
// #include <open62541/plugin/pki_default.h> /* Depends on version used */

// #define CERT_PATH "./data/cert/"
#define CERT_PATH "/media/sf_shareddir/"
#define MAX_LINE_LENGTH 40

UA_ByteString loadFile(const char *const path);
FILE* openBinaryImage(char* fileName);

#endif /* UTILS_H*/
