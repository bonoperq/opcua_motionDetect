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
