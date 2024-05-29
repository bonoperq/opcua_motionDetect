#include "../include/utils.h"

UA_StatusCode read_value(UA_Client *client) {
    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);
    UA_StatusCode status = UA_Client_readValueAttribute(client, UA_NODEID_STRING(1, "motionDetectLog"), &value);
    if (status == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTESTRING])) {
	    
        UA_ByteString *byteString = (UA_ByteString*)value.data;
        /* Open file for writing */
        FILE *file = fopen("./data/get_data.txt","w");
        if (file==NULL) {
            printf("Failed to open file for writing.\n");
            return UA_STATUSCODE_BAD;
        }

        /* Write byteString into the file */
        size_t bytesWritten = fwrite(byteString->data, 1, byteString->length, file);
        if (bytesWritten != byteString->length) {
            printf("Failed to write all data into the file.\n");
            return UA_STATUSCODE_BAD;
        }
        fclose(file);
    }
    return UA_STATUSCODE_GOOD;
}


UA_StatusCode getMultipleImagesFromServer(UA_Client *client, char **imagesName, size_t numImages) {
    // Call the GetMultipleImages method on the server
    UA_Variant input;

    // Prepare an array of UA_String for input
    UA_String *inputStrings = (UA_String *) UA_Array_new(numImages, &UA_TYPES[UA_TYPES_STRING]);
    if (!inputStrings) {
        printf("Failed to allocate memory for input strings.\n");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    // Fill the array with input strings
    for (size_t i = 0; i < numImages; ++i) {
        inputStrings[i] = UA_STRING(imagesName[i]);
    }

    UA_StatusCode status = UA_Variant_setArrayCopy(&input, inputStrings, numImages, &UA_TYPES[UA_TYPES_STRING]);
    free(inputStrings);

    if (status != UA_STATUSCODE_GOOD) {
        printf("Error with input loading. StatusCode : %s\n", UA_StatusCode_name(status));
        return status;
    }
    
    size_t outputSize;

    UA_Variant *output = NULL;

    status = UA_Client_call(client, UA_NODEID_NUMERIC(0, 51022), // directory ID
                                                 UA_NODEID_STRING(1, "GetMultipleImages"),
                                                 1, &input, &outputSize, &output);
    if (status != UA_STATUSCODE_GOOD) {
        printf("Failed to call GetMultipleImages method. StatusCode: %s\n", UA_StatusCode_name(status));
        return status;
    }

    // Check if the output variant contains byte strings
    for (size_t i = 0; i < numImages; ++i) {
        if (!UA_Variant_hasScalarType(&output[i], &UA_TYPES[UA_TYPES_BYTESTRING])) {
            printf("GetImage method did not return a byte string for image %zu.\n", i);
            UA_Variant_clear(output);
            return UA_STATUSCODE_BAD;
        }
    }

    
    // Write each byte string into a separate file
    for (size_t i = 0; i < numImages; ++i) {
        // Extract the byte string from the output variant
        UA_ByteString *byteString = (UA_ByteString*) output[i].data;

        // Open file for writing
        FILE *file = openBinaryImage(imagesName[i]);
        if (file == NULL) {
            printf("Failed to open file for writing for image %zu.\n", i);
            UA_Variant_clear(output);
            return UA_STATUSCODE_BAD;
        }

        // Write byte string into the file
        size_t bytesWritten = fwrite(byteString->data, 1, byteString->length, file);
        if (bytesWritten != byteString->length) {
            printf("Failed to write all data into the file for image %zu.\n", i);
            fclose(file);
            UA_Variant_clear(output);
            return UA_STATUSCODE_BAD;
        }

        fclose(file);
    }

    for (size_t i=0; i<numImages; i++) {
        char command2[100];
        sprintf(command2, "python3 ./src/read_img.py %s",imagesName[i]);
        system(command2);
    }

    UA_Variant_clear(&input);
    UA_Variant_clear(output);
    return UA_STATUSCODE_GOOD;
}

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData);

/* Add new connection to the server */
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &connectionIdentifier);
    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the DataSetReader. */
/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    UA_Server_enableReaderGroup(server, readerGroupIdentifier);

    return retval;
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */
/* Add DataSetReader to the ReaderGroup */
static UA_StatusCode
addDataSetReader(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    /* The following parameters are used to show that the data published by
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the information model */
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);

    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    return retval;
}

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data type.
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

/**
 * **TargetVariables**
 *
 * The SubscribedDataSet option TargetVariables defines a list of Variable mappings between
 * received DataSet fields and target Variables in the Subscriber AddressSpace.
 * The values subscribed from the Publisher are updated in the value field of these variables */
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                           folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields[i].name.data),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newNode);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    retval = UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                           readerConfig.dataSetMetaData.fieldsSize, targetVars);
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    UA_free(targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

/**
 * **DataSetMetaData**
 *
 * The DataSetMetaData describes the content of a DataSet. It provides the information necessary to decode
 * DataSetMessages on the Subscriber side. DataSetMessages received from the Publisher are decoded into
 * DataSet and each field is updated in the Subscriber based on datatype match of TargetVariable fields of Subscriber
 * and PublishedDataSetFields of Publisher */
/* Define MetaData for TargetVariables */
static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name = UA_STRING ("DataSet 1");

    /* Static definition of number of fields size to 4 to create four different
     * targetVariables of distinct datatype
     * Currently the publisher sends only DateTime data type */
    pMetaData->fieldsSize = 4;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* DateTime DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                    &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[0].name =  UA_STRING ("DateTime");
    pMetaData->fields[0].valueRank = -1; /* scalar */

    /* Int32 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[1]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                   &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[1].name =  UA_STRING ("Int32");
    pMetaData->fields[1].valueRank = -1; /* scalar */

    /* Int64 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[2]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId,
                   &pMetaData->fields[2].dataType);
    pMetaData->fields[2].builtInType = UA_NS0ID_INT64;
    pMetaData->fields[2].name =  UA_STRING ("Int64");
    pMetaData->fields[2].valueRank = -1; /* scalar */

    /* Boolean DataType */
    UA_FieldMetaData_init (&pMetaData->fields[3]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                    &pMetaData->fields[3].dataType);
    pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[3].name =  UA_STRING ("BoolToggle");
    pMetaData->fields[3].valueRank = -1; /* scalar */
}

int main(int argc, char *argv[])
{
    if (argc !=2) {
	    printf("Usage: %s <ip>\n", argv[0]);
	    return EXIT_FAILURE;
    }
    char *format = "opc.tcp://%s:4840";
    char* ip;
    if (asprintf(&ip, format, argv[1]) == -1) {
	    printf("Error merging strings");
	    return EXIT_FAILURE;
    }

    UA_ByteString certificateAuth = loadFile(CERT_PATH "client/authentication/uaexpert.der");
    UA_ByteString privateKeyAuth = loadFile(CERT_PATH "client/authentication/uaexpert_key.pem");
    UA_ByteString privateKey = loadFile(CERT_PATH "client/encryption/client.key");
    UA_ByteString certificate = loadFile(CERT_PATH "client/encryption/client.der");


    /* Load CA for authentication */
    UA_ByteString CA = loadFile(CERT_PATH "ca/ca.der");
    if (CA.data == NULL) {
        printf("Error while reading CA certificate\n");
        return EXIT_FAILURE;
    }

    size_t trustListSize = 1;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = CA;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Create a client and connect */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    /* Set securityMode and securityPolicyUri */
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGN;
    cc->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize, 
                                         revocationList, revocationListSize);
    
    // Clear allocated data
    free(CA.data);
    free(privateKey.data);
    free(certificate.data);

    UA_CertificateVerification_AcceptAll(&cc->certificateVerification);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:client:Schneider:M262:Application"); // Uri du certificat signé

    UA_ClientConfig_setAuthenticationCert(cc, certificateAuth, privateKeyAuth);

    // Clear allocated data
    free(certificateAuth.data);
    free(privateKeyAuth.data);

    UA_StatusCode retval = UA_Client_connect(client, ip);
    free(ip);

    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed to connect to the server. StatusCode: %s\n", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    } else {
        printf("Connected to the server.\n");
    }

    while (1) {
        printf("\nChoose an option:\n");
        printf("1. Get data file\n");
        printf("2. Get Images\n");
        printf("3. Disconnect\n");

        int option;
        if (scanf("%d", &option) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }

        switch (option) {
            case 1: {
                retval = read_value(client);
                if (retval == UA_STATUSCODE_GOOD) {
                    printf("\nFile successfully read\n\n");
                }
                else {
                    printf("\nEnable to read file\n\n");
                }
                break;
            }
            case 2: {
                size_t numImages;
                printf("Enter the number of images: ");
                if (scanf("%zu", &numImages) != 1 || numImages < 1) {
                    printf("Invalid input. Please enter a valid number.\n");
                    while (getchar() != '\n'); // Clear input buffer
                    continue;
                }
                char **imageNames = (char **)malloc(numImages * sizeof(char *));
                if (!imageNames) {
                    printf("Failed to allocate memory.\n");
                    break;
                }
                printf("Enter image names separated by space:\n");
                for (size_t i = 0; i < numImages; ++i) {
                    imageNames[i] = (char *)malloc(100 * sizeof(char));
                    if (!imageNames[i]) {
                        printf("Failed to allocate memory.\n");
                        break;
                    }
                    scanf("%99s", imageNames[i]);
                }
                UA_StatusCode status = getMultipleImagesFromServer(client, imageNames, numImages);
                if (status != UA_STATUSCODE_GOOD) {
                    printf("Failed to get multiple images from server. StatusCode: %s\n", UA_StatusCode_name(status));
                }
                for (size_t i = 0; i < numImages; ++i) {
                    free(imageNames[i]);
                }
                free(imageNames);
                break;
            }
            case 3:
                UA_Client_disconnect(client);
                printf("Disconnected from the server.\n");
                UA_Client_delete(client);
                return EXIT_SUCCESS;
            default:
                printf("Invalid option. Please choose a valid option.\n");
        }
    }

    return EXIT_SUCCESS;
}