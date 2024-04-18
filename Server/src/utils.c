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

UA_ByteString loadFileTail(const char *filename) {
    int tail = 30;
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file %s\n", filename);
        return UA_BYTESTRING_NULL;
    }
    fseek(file, -tail, SEEK_END); // Move to 30 bytes before the end of file
    long fileSize = ftell(file);
    if (fileSize < tail) {
        printf("File %s is too small\n", filename);
        fclose(file);
        return UA_BYTESTRING_NULL;
    }
    fseek(file, -tail, SEEK_END); // Move to the correct position again
    UA_Byte *buffer = (UA_Byte *)malloc(tail * sizeof(UA_Byte));
    if (!buffer) {
        printf("Memory allocation failed\n");
        fclose(file);
        return UA_BYTESTRING_NULL;
    }
    size_t bytesRead = fread(buffer, sizeof(UA_Byte), tail, file);
    fclose(file);
    if (bytesRead != tail) {
        printf("Failed to read %d bytes from file %s\n", tail, filename);
        free(buffer);
        return UA_BYTESTRING_NULL;
    }
    UA_ByteString byteString = UA_BYTESTRING_NULL;
    byteString.length = tail;
    byteString.data = buffer;
    return byteString;
}

void *watch_file(void *args) {
    struct ThreadArgs *targs = (struct ThreadArgs *) args;

    int fd = inotify_init();
    if (fd==-1) {
        printf("Error init inotify\n");
        return NULL;
    }

    int wd = inotify_add_watch(fd, "./data/data_server.txt", IN_MODIFY);
    printf("Opening file... wd : %d\n", wd);
    if (wd==-1) {
        printf("Error while watching file\n");
        return NULL;
    }

    while(1) {
        char buf[sizeof(struct inotify_event) + MAX_LINE_LENGTH];
        ssize_t numRead = read(fd, buf, sizeof(buf));
        if (numRead == -1) {
            printf("Error while reading file with inotify\n");
            return NULL;
        }

        UA_ByteString byteString = loadFile("./data/data_server.txt");
        UA_Variant value;
        UA_Variant_setScalar(&value, &byteString, &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Server_writeValue(targs->server, targs->newNodeId, value);
        free(byteString.data);
    }
    return NULL;
}
