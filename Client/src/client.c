#include "../include/utils.h"


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
    /* Clean up */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
