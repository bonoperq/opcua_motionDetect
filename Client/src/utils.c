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

FILE* openBinaryImage(char *imageName) {

    char *dotPosition = strchr(imageName, '.');
    if (!dotPosition) {
        return NULL;
    }

    size_t rootLength = dotPosition - imageName;

    char *imageRoot = malloc(rootLength + 1);
    if (!imageRoot) {
        return NULL;
    }

    strncpy(imageRoot, imageName, rootLength);
    imageRoot[rootLength] = '\0'; // Terminer la chaîne avec le caractère nul

    // Ouvrir le fichier avec le nom imageRoot
    char filePath[100]; // Assurez-vous que la taille est suffisante pour contenir le chemin complet
    snprintf(filePath, sizeof(filePath), "./data/images/%s.bin", imageRoot);
    FILE *file = fopen(filePath, "wb");

    // Libérer la mémoire allouée pour imageRoot
    free(imageRoot);

    return file;
}