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

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define FILENAME_SIZE 100

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

bool openFile(char token[FILENAME_SIZE], FILE **fp ){
  if ((*fp = fopen(token, "r")) == NULL) return false;
  return true;
}

void bpb(FILE *fp, int16_t *BPB_BytsPerSec, int8_t *BPB_SecPerClus, int16_t *BPB_RsvdSecCnt, int8_t *BPB_NumFATS, int32_t *BPB_FATSz32){

  fseek(fp, 11, SEEK_SET);
  fread(BPB_BytsPerSec, 2, 1, fp);
  printf("BPB_BytsPerSec: dec - %d, hex - %x\n", *BPB_BytsPerSec, *BPB_BytsPerSec);

  fseek(fp, 13, SEEK_SET);
  fread(BPB_SecPerClus, 1, 1, fp);
  printf("BPB_SecPerClus: dec - %d, hex - %x\n", *BPB_SecPerClus, *BPB_SecPerClus);

  fseek(fp, 14, SEEK_SET);
  fread(BPB_RsvdSecCnt, 2, 1, fp);
  printf("BPB_RsvdSecCnt: dec - %d, hex - %x\n", *BPB_RsvdSecCnt, *BPB_RsvdSecCnt);

  fseek(fp, 16, SEEK_SET);
  fread(BPB_NumFATS, 1, 1, fp);
  printf("BPB_NumFATS: dec - %d, hex - %x\n", *BPB_NumFATS, *BPB_NumFATS);

  fseek(fp, 36, SEEK_SET);
  fread(BPB_FATSz32, 4, 1, fp);
  printf("BPB_FATSz32: dec - %d, hex - %x\n", *BPB_FATSz32, *BPB_FATSz32);
}

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  bool isClosed = true;
  FILE *fp;

  char BS_OEMName[8];
  int16_t BPB_BytsPerSec;
  int8_t BPB_SecPerClus;
  int16_t BPB_RsvdSecCnt;
  int8_t BPB_NumFATS;
  int16_t BPB_RootEntCnt;
  char BS_VolLab[11];
  int32_t BPB_FATSz32;
  int32_t BPB_RootClus;

  int32_t RootDirSectors = 0;
  int32_t FirstDataSector = 0;
  int32_t FirstSectorofCluster = 0;

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    //Handling commands

    if ((strcmp(token[0], "open")) == 0){
      if (isClosed){
        if (token[1] == NULL) printf("command has to be: open <filename>. please try again.\n");
        else {
          if (openFile(token[1], &fp)){
            isClosed = false;
          }
          else printf("Error: File system image not found.\n");
        }
      }
      else printf("Error: File system image already open.\n");
    }
    else if ((strcmp(token[0], "close")) == 0){
      if (!isClosed){
        fclose(fp);
        isClosed = true;
      }
      else printf("Error: File system not open.\n");
    }
    else if ((strcmp(token[0], "bpb")) == 0 && !isClosed){
      bpb(fp, &BPB_BytsPerSec, &BPB_SecPerClus, &BPB_RsvdSecCnt, &BPB_NumFATS, &BPB_FATSz32);
    }
    else if ((strcmp(token[0], "test")) == 0 && !isClosed){
      printf("[TEST] BPB_NumFATS = %d\n", BPB_NumFATS);
    }

    else if(strcmp(token[0], "ls") == 0 && !isClosed)
    {
        fseek(fp, 0x100400, SEEK_SET);
        fread(dir, 16, sizeof( struct DirectoryEntry),fp);
        int i;
        for(i = 0; i< 16; i++)
        {
          if(dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
              printf("FileName: %s\n", dir[i].DIR_Name);
        }    
    }

    else if ((strcmp(token[0], "bpb")) == 0 
    || (strcmp(token[0], "stat")) == 0 
    || (strcmp(token[0], "get")) == 0 
    || (strcmp(token[0], "ls")) == 0 
    || (strcmp(token[0], "read")) == 0
    || (strcmp(token[0], "cd")) == 0
    || (strcmp(token[0], "info")) == 0
    && isClosed)
    {
      printf("Error: File system image must be opened first.\n");
    }
    else {printf("Error: Wrong command.\n");}

    free( working_root );

  }
  return 0;
}