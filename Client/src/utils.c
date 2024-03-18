#include "../include/utils.h"

UA_ByteString loadFile(const char *const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        printf("Cannot open file %s\n", path);
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}

UA_StatusCode read_value(UA_Client *client) {
    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);
    UA_StatusCode status = UA_Client_readValueAttribute(client, UA_NODEID_STRING(1, "byteString"), &value);
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