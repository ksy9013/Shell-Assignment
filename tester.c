#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    FILE *fp;

    fp = fopen("fat32.img", "r");

    int16_t BPB_BytsPerSec;
    int8_t BPB_SecPerClus;
    int16_t BPB_RsvdSecCnt;

    fseek(fp, 11, SEEK_SET);
    fread(&BPB_BytsPerSec, 2, 1, fp);

    fseek(fp, 13, SEEK_SET);
    fread(&BPB_SecPerClus, 1, 1, fp);

    fseek(fp, 14, SEEK_SET);
    fread(&BPB_RsvdSecCnt, 2, 1, fp);

    printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
    printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);

    fclose(fp);

    return 0;
}