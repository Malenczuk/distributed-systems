#include <stdio.h>
#include <stdlib.h>
#include <popt.h>
#include <netinet/in.h>
#include "config.h"

void load_arguments(int argc, char **argv) {
    char *id = NULL;
    char *out_ip = NULL;
    int local_port = 0;
    int out_port = 0;
    int token = 0;

    poptContext pc;
    struct poptOption po[] = {
            {"ID", 'I', POPT_ARG_STRING, &id, 0, "", ""},
            {"ip", 'i', POPT_ARG_STRING, &out_ip, 0, "", ""},
            {"port", 'p', POPT_ARG_INT, &out_port, 0, "", ""},
            {"local-port", 'l', POPT_ARG_INT, &local_port, 0, "", ""},
            {"token", 't', POPT_ARG_NONE, &token, 0, "", ""},
            POPT_AUTOHELP
            POPT_TABLEEND
    };

    // pc is the context for all popt-related functions
    pc = poptGetContext(NULL, argc, (const char **) argv, po, 0);
    poptSetOtherOptionHelp(pc, "[ARG...]");
    if (argc < 2) {
        poptPrintUsage(pc, stderr, 0);
        exit(1);
    }

    // process options and handle each val returned
    int val;
    while ((val = poptGetNextOpt(pc)) >= 0) {
        printf("poptGetNextOpt returned val %d\n", val);
    }

    // poptGetNextOpt returns -1 when the final argument has been parsed
    // otherwise an error occured
    if (val != -1) {
        // handle error
        switch (val) {
            case POPT_ERROR_NOARG:
                printf("Argument missing for an option\n");
                exit(1);
            case POPT_ERROR_BADOPT:
                printf("Option's argument could not be parsed\n");
                exit(1);
            case POPT_ERROR_BADNUMBER:
            case POPT_ERROR_OVERFLOW:
                printf("Option could not be converted to number\n");
                exit(1);
            default:
                printf("Unknown error in option processing\n");
                exit(1);
        }
    }

    // Handle ARG... part of commandline
    while (poptPeekArg(pc) != NULL) {
        char *arg = (char *) poptGetArg(pc);
        printf("poptGetArg returned arg: \"%s\"\n", arg);
    }

    // Handle required arguments
    int err = 0;
    if ((id == NULL) & (err |= id == NULL))
        printf("Enter ID [-I|--ID]\n");
    if ((out_ip == NULL) & (err |= out_ip == NULL))
        printf("Enter IP [-i|--ip]\n");
    if ((out_port == 0) & (err |= out_port == 0))
        printf("Enter PORT [-p|--port]\n");
    if ((local_port == 0) & (err |= local_port == 0))
        printf("Enter LOCAL PORT [-l|--local-port]\n");
    if (err) {
        poptPrintUsage(pc, stderr, 0);
        exit(1);
    }

    ID = id;
    OUT_IP = out_ip;
    LOCAL_PORT = local_port;
    OUT_PORT = out_port;
    TOKEN = token;
}
