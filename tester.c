#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

struct __attribute__((__packed__)) DirectoryEntry{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];




int main() {
    FILE *fp;

    fp = fopen("fat32.img", "r");

    int16_t BPB_BytsPerSec;
    int8_t BPB_SecPerClus;
    int16_t BPB_RsvdSecCnt;
    int8_t BPB_NumFATs;
    int32_t BPB_FATSz32;

    fseek(fp, 11, SEEK_SET);
    fread(&BPB_BytsPerSec, 2, 1, fp);

    fseek(fp, 13, SEEK_SET);
    fread(&BPB_SecPerClus, 1, 1, fp);

    fseek(fp, 14, SEEK_SET);
    fread(&BPB_RsvdSecCnt, 2, 1, fp);

    printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
    printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);

    fseek(fp, 0x100400, SEEK_SET);
    fread(dir, 16, sizeof( struct DirectoryEntry),fp);

    int i;
    for(i = 0; i< 16; i++)
    {
        if(dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
            printf("FileName: %s %d %d\n", dir[i].DIR_Name, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
    }

    fclose(fp);

    return 0;
}