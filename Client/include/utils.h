#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <open62541/config.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/securitypolicy_default.h>
// #include <open62541/plugin/pki_default.h> /* Depends on version used */

// #define CERT_PATH "./data/cert/"
#define CERT_PATH "/media/sf_shareddir/"
#define MAX_LINE_LENGTH 40

UA_ByteString loadFile(const char *const path);
UA_StatusCode read_value(UA_Client *client);

#endif /* UTILS_H*/
