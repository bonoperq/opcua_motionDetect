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

UA_StatusCode getImageFromServer(UA_Client *client, char *imageName) {
    // Call the GetImage method on the server
    UA_Variant input;
    UA_Variant_init(&input);
    UA_String inputString = UA_STRING(imageName);
    UA_Variant_setScalarCopy(&input, &inputString, &UA_TYPES[UA_TYPES_STRING]);
    
    size_t inputSize = 1;
    size_t outputSize;

    UA_Variant *output = NULL;

    UA_StatusCode status = UA_Client_call(client, UA_NODEID_NUMERIC(0, 51022), // directory ID
                                                 UA_NODEID_STRING(1, "GetImage"),
                                                 inputSize, &input, &outputSize, &output);
    if (status != UA_STATUSCODE_GOOD) {
        printf("Failed to call GetImage method. StatusCode: %s\n", UA_StatusCode_name(status));
        return status;
    }

    // Check if the output variant contains byte string
    if (!UA_Variant_hasScalarType(output, &UA_TYPES[UA_TYPES_BYTESTRING])) {
        printf("GetImage method did not return a byte string.\n"); 
        UA_Variant_clear(output);
        return UA_STATUSCODE_BAD;
    }

    // Extract the byte string from the output variant
    UA_ByteString *byteString = (UA_ByteString*)output->data;

    // Open file for writing
    FILE *file = openBinaryImage(imageName);
    if (file == NULL) {
        printf("Failed to open file for writing.\n");
        UA_Variant_clear(output);
        return UA_STATUSCODE_BAD;
    }

    // Write byte string into the file
    size_t bytesWritten = fwrite(byteString->data, 1, byteString->length, file);
    if (bytesWritten != byteString->length) {
        printf("Failed to write all data into the file.\n");
        fclose(file);
        UA_Variant_clear(output);
        return UA_STATUSCODE_BAD;
    }

    fclose(file);
    UA_Variant_clear(&input);
    UA_Variant_clear(output);
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
    UA_CertificateVerification_AcceptAll(&cc->certificateVerification);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:client:Schneider:M262:Application"); // Uri du certificat signé

    UA_ClientConfig_setAuthenticationCert(cc, certificateAuth, privateKeyAuth);

    // Clear allocated data
    free(certificateAuth.data);
    free(privateKeyAuth.data);
    free(privateKey.data);
    free(certificate.data);
    free(CA.data);

    UA_StatusCode retval = UA_Client_connect(client, ip);


    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }


    retval = read_value(client);
    if (retval == UA_STATUSCODE_GOOD) {
        printf("\nFile successfully read\n\n");
    }
    else {
        printf("\nEnable to read file\n\n");
    }
    
    // Test getImage
    char *fileName = "img_23.jpg";
    retval = getImageFromServer(client, fileName);

    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    char command1[100];
    sprintf(command1, "python3 ./src/read_img.py %s",fileName);
    system(command1);

    // Test getMultipleImage
    size_t numFile = 5;
    char **filesName = malloc(numFile*sizeof(fileName));
    filesName[0] = "img_0.jpg";
    filesName[1] = "img_1.jpg";
    filesName[2] = "img_24.jpg";
    filesName[3] = "img_0.jpg";
    filesName[4] = "img_4.jpg";

    retval = getMultipleImagesFromServer(client, filesName, numFile);

    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    for (size_t i=0; i<numFile; i++) {
        char command2[100];
        sprintf(command2, "python3 ./src/read_img.py %s",filesName[i]);
        system(command2);
    }

    /* Clean up */
    free(filesName);
    free(ip);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return UA_STATUSCODE_GOOD;
}
