#ifndef TOKENRING_CONFIG_H
#define TOKENRING_CONFIG_H

char ID[128];
char OUT_IP[16];
uint16_t LOCAL_PORT = 0;
uint16_t OUT_PORT = 0;
char TOKEN = 0;

void load_arguments(int argc, char **argv);

#endif //TOKENRING_CONFIG_H
