#include "../include/utils.h"

#define MAX_IMAGE_SIZE 1000000

static UA_StatusCode
customConfig_server(UA_ServerConfig *config) {
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
    UA_AccessControl_default(config, false, NULL, 0, NULL);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:server:Schneider:M262:Application");

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode getImageWithPath(UA_Variant *output, UA_String *inputStr) {
    
    /* Construire le chemin du fichier image */
    char *path = malloc(strlen("./data/images/") + inputStr->length + 1);
    if (!path) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    strcpy(path, "./data/images/");
    strncat(path, (char*)inputStr->data, inputStr->length);
    path[inputStr->length + strlen("./data/images/")] = '\0';

    /* Ouvrir le fichier image */
    FILE *imageFile = fopen(path, "rb");
    free(path); // Libérer la mémoire allouée pour le chemin du fichier
    if (!imageFile) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Lire le contenu du fichier dans un tampon de bytes */
    fseek(imageFile, 0, SEEK_END);
    size_t fileSize = ftell(imageFile);
    fseek(imageFile, 0, SEEK_SET);
    if (fileSize > MAX_IMAGE_SIZE) {
        fclose(imageFile);
        return UA_STATUSCODE_BADINTERNALERROR; // Taille de l'image trop grande
    }
    uint8_t *imageBuffer = (uint8_t*)malloc(fileSize);
    if (!imageBuffer) {
        fclose(imageFile);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    size_t bytesRead = fread(imageBuffer, 1, fileSize, imageFile);
    fclose(imageFile);

    if (bytesRead != fileSize) {
        free(imageBuffer);
        return UA_STATUSCODE_BADINTERNALERROR; // Erreur lors de la lecture de l'image
    }

    /* Créer un tableau de bytes dans le variant de sortie */
    UA_ByteString byteString = {
        .data = imageBuffer,
        .length = fileSize
    };

    // SEG FAULT ICI
    UA_Variant_setScalarCopy(output, &byteString, &UA_TYPES[UA_TYPES_BYTESTRING]);
    
    /* Nettoyer la mémoire allouée pour le tampon de bytes */
    free(imageBuffer);

    /* Informer dans la console que l'image a été récupérée */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Image %.*s was retrieved", (int)inputStr->length, inputStr->data);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getImageMethodCallback(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output) {
    /* Vérifiez s'il y a un argument d'entrée */
    if (inputSize < 1 || !input[0].data) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Récupérer le nom de l'image à partir de l'argument d'entrée */
    UA_String *inputStr = (UA_String*)input[0].data;

    return getImageWithPath(output, inputStr);
}


static void
addGetImageMethod(UA_Server *server, UA_NodeId methodDirectoryNodeId) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("", "Name of the image to get from the method");
    inputArgument.name = UA_STRING("ImageName");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("", "Binary image received");
    outputArgument.name = UA_STRING("ImageValue");
    outputArgument.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes getImageAttr = UA_MethodAttributes_default;
    getImageAttr.description = UA_LOCALIZEDTEXT("","Retrieve a single image from server");
    getImageAttr.displayName = UA_LOCALIZEDTEXT("","GetImage");
    getImageAttr.executable = true;
    getImageAttr.userExecutable = true;

    UA_NodeId newNodeId = UA_NODEID_STRING(1, "GetImage");
    UA_NodeId parentNodeId = methodDirectoryNodeId;
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "GetImage");

    UA_Server_addMethodNode(server, newNodeId, parentNodeId,
                            parentReferenceNodeId, browseName,
                            getImageAttr, &getImageMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

static UA_StatusCode
getMultipleImagesMethodCallback(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionHandle,
                        const UA_NodeId *methodId, void *methodContext,
                        const UA_NodeId *objectId, void *objectContext,
                        size_t inputSize, const UA_Variant *input,
                        size_t outputSize, UA_Variant *output) {
    
    /* Récupérer les noms des images à partir des arguments d'entrée */
    UA_String *inputStrs = (UA_String*)input[0].data;

    UA_StatusCode retVal;

    for (size_t i = 0; i < input->arrayLength; ++i) {
        if (!(inputStrs+i)->data) {
            break;
        }
        retVal = getImageWithPath(output+i, inputStrs+i);
        if (retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }
    }

    /* Informer dans la console que les images ont été récupérées */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Retrieved %zu images", outputSize);

    return UA_STATUSCODE_GOOD;
}

static void
addGetMultipleImagesMethod(UA_Server *server, UA_NodeId methodDirectoryNodeId) {
    UA_Argument inputArguments;
    UA_UInt32 numImage = 5;

    UA_Argument_init(&inputArguments);
    inputArguments.description = UA_LOCALIZEDTEXT("", "Array of image names to get");
    inputArguments.name = UA_STRING("ImageNames");
    inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments.valueRank = 1;

    inputArguments.arrayDimensionsSize=1;
    inputArguments.arrayDimensions = &numImage;
    UA_Argument outputArguments[numImage];

    for (size_t i = 0; i < numImage; ++i) {
        UA_Argument_init(&outputArguments[i]);
        outputArguments[i].description = UA_LOCALIZEDTEXT("en_US", "Single Image in byte");
        outputArguments[i].dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
        outputArguments[i].valueRank = UA_VALUERANK_SCALAR;
        char buffer[10];
        snprintf(buffer, 10, "Image%zu", i + 1);

        outputArguments[i].name = UA_STRING_ALLOC(buffer);
    }

    UA_MethodAttributes getMultipleImagesAttr = UA_MethodAttributes_default;
    UA_MethodAttributes_init(&getMultipleImagesAttr);
    getMultipleImagesAttr.description = UA_LOCALIZEDTEXT("", "Retrieve multiple images from server");
    getMultipleImagesAttr.displayName = UA_LOCALIZEDTEXT("", "GetMultipleImages");
    getMultipleImagesAttr.executable = true;
    getMultipleImagesAttr.userExecutable = true;

    UA_NodeId newNodeId = UA_NODEID_STRING(1, "GetMultipleImages");
    UA_NodeId parentNodeId = methodDirectoryNodeId; // Utiliser l'ID du répertoire "Method"
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "GetMultipleImages");

    UA_Server_addMethodNode(server, newNodeId, parentNodeId,
                            parentReferenceNodeId, browseName,
                            getMultipleImagesAttr, &getMultipleImagesMethodCallback,
                            1, &inputArguments, numImage, outputArguments, NULL, NULL);
}

static UA_NodeId 
addServerData(UA_Server *server) {
    /* 1) Define the variable attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "motionDetectLog");
    UA_ByteString byteString = loadFile("./data/data_server.txt");
    UA_Variant_setScalar(&attr.value, &byteString, &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* 2) Define where the node shall be added with which browsename */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "motionDetectLog");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "motionDetectLog");

    /* 3) Add the node */
    UA_Server_addVariableNode(server, newNodeId, parentNodeId,
                              parentReferenceNodeId, browseName,
                              variableType, attr, NULL, NULL);
    
    free(byteString.data);
    return newNodeId;
}

static UA_NodeId 
addDataMonitoring(UA_Server *server) {
    /* 1) Define the variable attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "logTail");
    UA_ByteString byteString = loadFileTail("./data/data_server.txt");
    UA_Variant_setScalar(&attr.value, &byteString, &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* 2) Define where the node shall be added with which browsename */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "logTail");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "logTail");

    /* 3) Add the node */
    UA_Server_addVariableNode(server, newNodeId, parentNodeId,
                              parentReferenceNodeId, browseName,
                              variableType, attr, NULL, NULL);
    free(byteString.data);
    return newNodeId;
}

static UA_NodeId addMethodDirectory(UA_Server *server) {
    /* Définir les attributs du répertoire */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "Method");
    attr.description = UA_LOCALIZEDTEXT("", "Directory for methods");

    /* Spécifier le parent du répertoire (dans ce cas, le dossier racine) */
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    /* Ajouter le répertoire "Method" en tant que sous-répertoire du parent */
    UA_NodeId methodDirectoryNodeId;
    UA_Server_addObjectNode(server, UA_NODEID_NULL, parentNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), // HasOrganizes reference
                            UA_QUALIFIEDNAME(1, "Method"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            attr, NULL, &methodDirectoryNodeId);

    return methodDirectoryNodeId;
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
    UA_NodeId monitoringNodeId;
    addDataMonitoring(server);
    UA_NodeId methodDirectoryNodeId = addMethodDirectory(server);
    addGetImageMethod(server, methodDirectoryNodeId);
    addGetMultipleImagesMethod(server, methodDirectoryNodeId);

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