#ifndef TOKENRING_CONFIG_H
#define TOKENRING_CONFIG_H

char *ID = NULL;
char *OUT_IP = NULL;
uint16_t LOCAL_PORT = 0;
uint16_t OUT_PORT = 0;
char TOKEN = 0;

void load_arguments(int argc, char **argv);

#endif //TOKENRING_CONFIG_H
