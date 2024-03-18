#include "../include/utils.h"

UA_StatusCode customConfig_server(UA_ServerConfig *config) {
    /* Load server certificate and private key and ca certificate */
    UA_ByteString certificate_server = loadFile(CERT_PATH "server/encryption/server.der"); 
    UA_ByteString privateKey_server = loadFile(CERT_PATH "server/encryption/server.key");
    UA_ByteString CA = loadFile(CERT_PATH "ca/ca.der");

    if ( certificate_server.data == NULL || privateKey_server.data == NULL ) {
        printf("Server certificate or privateKey not read\n");
        return UA_STATUSCODE_BAD;
    }

    /* Add client certificate to the trust list for encryption */
    size_t trustListSize = 1;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = CA;
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Add all security policies and trust list, most likely to be for encryption but no authentication */
    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate_server, &privateKey_server,
                                                   trustList, trustListSize,
                                                   issuerList, issuerListSize,
                                                   revocationList, revocationListSize); 
    
    /* Trust list for authentication */
    const char *trustlistFolder = CERT_PATH "client/authentication";
    UA_CertificateVerification_CertFolders(&config->sessionPKI, trustlistFolder, NULL, NULL);
    UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);

    /* Clean up */
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    
    UA_ByteString_clear(&certificate_server);
    UA_ByteString_clear(&privateKey_server);
    
    // Restrict access
    UA_AccessControl_default(config, false, NULL, 0, NULL); /* Déjà utilisé dans la fonction setDefaultWithSecureSecurityPolicies */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:server:Schneider:M262:Application");

    return UA_STATUSCODE_GOOD;
}

UA_NodeId addServerData(UA_Server *server) {
    /* 1) Define the variable attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "byte String");
    UA_ByteString byteString = loadFile("./data/data_server.txt");
    UA_Variant_setScalar(&attr.value, &byteString, &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* 2) Define where the node shall be added with which browsename */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "byteString");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "byte String");

    /* 3) Add the node */
    UA_Server_addVariableNode(server, newNodeId, parentNodeId,
                              parentReferenceNodeId, browseName,
                              variableType, attr, NULL, NULL);
    
    return newNodeId;
}

int main(int argc, char** argv)
{ 
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode retval = customConfig_server(config);

    if (retval != UA_STATUSCODE_GOOD) {
        printf("Error during server configuration\n");
        return EXIT_FAILURE;
    }

    UA_NodeId nodeId = addServerData(server);

    /* Watch the data file and update it */
    struct ThreadArgs targs = {server, nodeId};

    pthread_t thread;
    if (pthread_create(&thread, NULL, watch_file, (void*)&targs) != 0) {
        printf("Error creating thread\n");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Run the server (until ctrl-c interrupt) */
    retval = UA_Server_runUntilInterrupt(server); 

    pthread_cancel(thread);
    pthread_join(thread,NULL);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}