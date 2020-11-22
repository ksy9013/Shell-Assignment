//1001352842 Din Chan
//1001757188 Seonyoung Kim

// The MIT License (MIT)
//
// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 4

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define DEFAULT_SIZE 100

int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATS;
int32_t BPB_FATSz32;
int16_t Curr_Cluster;

FILE *fp;

struct __attribute__((__packed__)) DirectoryEntry
{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

//Function to move to the next cluster of the file
int16_t NextLB(uint32_t sector)
{
    uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(fp, FATAddress, SEEK_SET);
    fread(&val, 2, 1, fp);
    return val;
}

//Function to convert cluster to offset number
int LBAToOffset(int32_t sector)
{
    return ((sector - 2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec);
}

//Function to edit filename for comparison reasons
char *editFileName(char *result, char filename[DEFAULT_SIZE])
{
    int j, k = 0;
    for (j = 0; j < 12; j++)
    {
        if (isalpha(filename[j]))
        {
            result[k++] = tolower(filename[j]);
        }
    }
    result[k] = '\0';
    return result;
}

//Funtion to write into the file
void writeFile(char *temp, char *token1, char *token2)
{
    FILE *writtenFile;
    //User specified the new name for the written file
    if (token2 != NULL)
    {
        writtenFile = fopen(token2, "a+");
        fputs(temp, writtenFile);
        fclose(writtenFile);
    }
    else
    {
        writtenFile = fopen(token1, "a+");
        fputs(temp, writtenFile);
        fclose(writtenFile);
    }
}

bool openFile(char token[DEFAULT_SIZE], FILE **fp)
{
    if ((*fp = fopen(token, "r")) == NULL)
        return false;
    return true;
}

void bpb()
{
    fseek(fp, 11, SEEK_SET);
    fread(&BPB_BytsPerSec, 2, 1, fp);
    fseek(fp, 13, SEEK_SET);
    fread(&BPB_SecPerClus, 1, 1, fp);
    fseek(fp, 14, SEEK_SET);
    fread(&BPB_RsvdSecCnt, 2, 1, fp);
    fseek(fp, 16, SEEK_SET);
    fread(&BPB_NumFATS, 1, 1, fp);
    fseek(fp, 36, SEEK_SET);
    fread(&BPB_FATSz32, 4, 1, fp);
}

int32_t findRootDir()
{
    return (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec);
}

int main()
{

    char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);
    bool isClosed = true;

    int32_t RootDirSectors = 0;
    int32_t FirstDataSector = 0;
    int32_t FirstSectorofCluster = 0;

    while (1)
    {
        // Print out the mfs prompt
        printf("mfs> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
            ;

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;

        char *working_str = strdup(cmd_str);

        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Tokenize the input stringswith whitespace used as the delimiter
        while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
               (token_count < MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
            if (strlen(token[token_count]) == 0)
            {
                token[token_count] = NULL;
            }
            token_count++;
        }

        //Handling commands

        if ((strcmp(token[0], "open")) == 0)
        {
            if (isClosed)
            {
                if (token[1] == NULL)
                    printf("command has to be: open <filename>. please try again.\n");
                else
                {
                    if (openFile(token[1], &fp))
                    {
                        bpb(fp, &BPB_BytsPerSec, &BPB_SecPerClus, &BPB_RsvdSecCnt, &BPB_NumFATS, &BPB_FATSz32);
                        fseek(fp, findRootDir(), SEEK_SET);
                        fread(dir, 16, sizeof(struct DirectoryEntry), fp);
                        isClosed = false;
                    }
                    else
                        printf("Error: File system image not found.\n");
                }
            }
            else
                printf("Error: File system image already open.\n");
        }
        else if ((strcmp(token[0], "close")) == 0)
        {
            if (!isClosed)
            {
                fclose(fp);
                isClosed = true;
            }
            else
                printf("Error: File system not open.\n");
        }
        else if ((strcmp(token[0], "bpb")) == 0 && !isClosed)
        {
            printf("BPB_BytsPerSec: dec - %d, hex - %x\n", BPB_BytsPerSec, BPB_BytsPerSec);
            printf("BPB_SecPerClus: dec - %d, hex - %x\n", BPB_SecPerClus, BPB_SecPerClus);
            printf("BPB_RsvdSecCnt: dec - %d, hex - %x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
            printf("BPB_NumFATS: dec - %d, hex - %x\n", BPB_NumFATS, BPB_NumFATS);
            printf("BPB_FATSz32: dec - %d, hex - %x\n", BPB_FATSz32, BPB_FATSz32);
        }
        else if ((strcmp(token[0], "get")) == 0 && !isClosed)
        {
            if (token[1] == NULL)
                printf("command has to be: get <filename>. please try again.\n");
            else
            {
                int i;
                for (i = 0; i < 16; i++)
                {
                    if (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
                    {
                        char filename[11], *editedFileName = malloc(sizeof(11)), *user_input = malloc(sizeof(11));
                        strcpy(filename, dir[i].DIR_Name);

                        editedFileName = editFileName(editedFileName, filename);
                        user_input = editFileName(user_input, token[1]);
                        if (strcmp(editedFileName, user_input) == 0)
                        {
                            int cluster = dir[i].DIR_FirstClusterLow;
                            char temp[BPB_BytsPerSec];
                            while (cluster != -1)
                            {
                                fseek(fp, LBAToOffset(cluster), SEEK_SET);
                                fread(temp, 1, BPB_BytsPerSec, fp);
                                writeFile(temp, token[1], token[2]);
                                cluster = NextLB(cluster);
                            }
                            break;
                        }
                    }
                }
            }
        }

        else if (strcmp(token[0], "ls") == 0 && !isClosed)
        {
            int i;
            for (i = 0; i < 16; i++)
            {
                if (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
                {
                    char filename[11], *editedFileName = malloc(sizeof(12));
                    strcpy(filename, dir[i].DIR_Name);
                    printf("%.11s\n", filename);
                }
            }
        }
        else if (strcmp(token[0], "cd") == 0 && !isClosed)
        {
            if (token[1] == NULL)
                ;
            else if (strcmp(token[1], "..") == 0)
            {

                //If there are no parent directories
                if (dir[1].DIR_FirstClusterLow == 0)
                {
                    fseek(fp, findRootDir(), SEEK_SET);
                    fread(dir, 16, sizeof(struct DirectoryEntry), fp);
                    Curr_Cluster = dir[0].DIR_FirstClusterLow;
                }
                //root directory has parent cluster of 7216
                else if (dir[1].DIR_FirstClusterLow == 7216)
                {
                    printf("You are in root directory can not go further back.\n");
                }
                else
                {
                    //converting parent cluster to offset and set it to current
                    Curr_Cluster = dir[1].DIR_FirstClusterLow;
                    fseek(fp, LBAToOffset(Curr_Cluster), SEEK_SET);
                    fread(dir, 16, sizeof(struct DirectoryEntry), fp);
                }
            }
            else
            {
                bool foundDir = false;
                int i;
                for (i = 0; i < 16; i++)
                {
                    if (dir[i].DIR_Attr == 0x10)
                    {
                        char filename[11], *editedFileName = malloc(sizeof(11)), *user_input = malloc(sizeof(11));
                        strcpy(filename, dir[i].DIR_Name);

                        editedFileName = editFileName(editedFileName, filename);
                        user_input = editFileName(user_input, token[1]);
                        if (strcmp(editedFileName, user_input) == 0)
                        {
                            foundDir = true;
                            int cluster = dir[i].DIR_FirstClusterLow;
                            fseek(fp, LBAToOffset(cluster), SEEK_SET);
                            fread(dir, 16, sizeof(struct DirectoryEntry), fp);
                            Curr_Cluster = dir[0].DIR_FirstClusterLow;
                            break;
                        }
                    }
                }
                if (!foundDir)
                {
                    printf("Couldn't find directory %s\n", token[1]);
                }
            }
        }
        else if (strcmp(token[0], "stat") == 0 && !isClosed)
        {
            int i = 0;
            bool flag = false;
            char filename[11], *editedFileName = malloc(sizeof(11)), *user_input = malloc(sizeof(11));

            if (token[1] == NULL)
                printf("Please use right format.\n");

            else
            {
                for (i = 0; i < 16; i++)
                {
                    if (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
                    {
                        strcpy(filename, dir[i].DIR_Name);
                        user_input = editFileName(user_input, token[1]);
                        editedFileName = editFileName(editedFileName, filename);
                        if (strcmp(user_input, editedFileName) == 0)
                        {
                            printf("%-20s%-15s%-20s\n", "File Attribute", "Size", "Starting Cluster Number");
                            if (dir[i].DIR_Attr == 16)
                            {
                                printf("%-20d%-15d%-20d\n", dir[i].DIR_Attr, 0, dir[i].DIR_FirstClusterLow);
                            }
                            else
                            {
                                printf("%-20d%-15d%-20d\n", dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
                            }

                            flag = true;
                        }
                    }
                }
                if (flag == false)
                {
                    printf("Error: File not found\n");
                }
            }
        }

        else if ((strcmp(token[0], "read")) == 0 && !isClosed)
        {
            if (token[1] == NULL || token[2] == NULL || token[3] == NULL)
                printf("Please use right format\n");
            else
            {
                int i;

                for (i = 0; i < 16; i++)
                {
                    if (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
                    {
                        char filename[11], *editedFileName = malloc(sizeof(11)), *user_input = malloc(sizeof(11));
                        strcpy(filename, dir[i].DIR_Name);
                        int count = 0;
                        editedFileName = editFileName(editedFileName, filename);
                        user_input = editFileName(user_input, token[1]);
                        if (strcmp(editedFileName, user_input) == 0)
                        {
                            int cluster = dir[i].DIR_FirstClusterLow;
                            char temp[BPB_BytsPerSec];
                            // int result[];
                            int result[atoi(token[3])];

                            while (cluster != -1)
                            {
                                fseek(fp, LBAToOffset(cluster), SEEK_SET);
                                fread(temp, 1, BPB_BytsPerSec, fp);
                                int k;
                                for (k = 0; k < BPB_BytsPerSec; k++)
                                {
                                    result[count] = temp[k];
                                    count++;
                                }
                                cluster = NextLB(cluster);
                            }

                            int j;
                            for (j = atoi(token[2]); j < atoi(token[3]); j++)
                            {
                                printf("%d ", result[j]);
                            }
                            printf("\n");
                            break;
                        }
                    }
                }
            }
        }

        else if ((strcmp(token[0], "bpb")) == 0 || (strcmp(token[0], "stat")) == 0 || (strcmp(token[0], "get")) == 0 || (strcmp(token[0], "ls")) == 0 || (strcmp(token[0], "read")) == 0 || (strcmp(token[0], "cd")) == 0 || (strcmp(token[0], "info")) == 0 && isClosed)
        {
            printf("Error: File system image must be opened first.\n");
        }
        else
        {
            printf("Error: Wrong command.\n");
        }

        free(working_root);
    }
    return 0;
}
