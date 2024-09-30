#include <stdio.h>
#include <stdlib.h>

#define BOOT2_SIZE 256

typedef unsigned char u8;
typedef unsigned int  u32;
typedef size_t usize;

int main(int argc, char **argv) {
    if (argc < 2) return -1;

    u8 buffer[BOOT2_SIZE] = {0};

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", argv[1]);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    usize fileSize = ftell(file);
    rewind(file);

    fread(buffer, 0, fileSize, file);
    fclose(file);

    u32 crc = 0xFFFFFFFF;
    u32 seed = 0xEDB88320;
    
    // Now do the accual CRC32 calculation.
    for (int i = 0; i < BOOT2_SIZE-4; ++i) {
        u8 byte = buffer[i];
        crc ^= byte;
        for (int j = 7; j >= 0; --j) {
            u32 mask = -(crc & 1);
            crc = (crc >> 1) ^ (seed & mask);
        }
    }
    crc = ~crc;

    printf("0x%x08", crc); 
}
