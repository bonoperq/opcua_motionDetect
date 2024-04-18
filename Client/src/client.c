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