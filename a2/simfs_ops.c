/* This file contains functions that are not part of the visible "interface".
 * They are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

/* Internal helper functions first.
 */

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if ((fp = fopen(filename, mode)) == NULL)
    {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void closefs(FILE *fp)
{
    if (fclose(fp) != 0)
    {
        perror("closefs");
        exit(1);
    }
}

void read(char *filename, fentry *files, fnode *fnodes)
{
    FILE *fp = openfs(filename, "r");

    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0)
    {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0)
    {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }
}
void write(FILE *fp, fentry *files, fnode *fnodes)
{
    if (fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES)
    {
        fprintf(stderr, "Error: write failed on createfile\n");
        closefs(fp);
        exit(1);
    }

    if (fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS)
    {
        fprintf(stderr, "Error: write failed on createfile\n");
        closefs(fp);
        exit(1);
    }
}

/* Find the first file  available of that name and return file size */
int find_filesize(fentry *files, char *newfile)
{
    int num = 0;
    for (int i = 0; i < MAXFILES; i++)
    {
        if (strcmp(files[i].name, newfile) == 0)
        {

            num = files[i].size;
        }
    }
    return num;
}

/* Find the first file block available and return index */
int find_firstblock(fentry *files, char *newfile)
{
    int index = 0;
    for (int i = 0; i < MAXFILES; i++)
    {
        if (strcmp(files[i].name, newfile) == 0)
        {

            index = files[i].firstblock;
        }
    }
    return index;
}
int find_fileindex(fentry *files, char *newfile)
{
    int index = 0;
    for (int i = 0; i < MAXFILES; i++)
    {
        if (strcmp(files[i].name, newfile) == 0)
        {
            index = i;
        }
    }
    return index;
}
/* Find the first fnode blockindex available and return index */
int find_blockindex(fnode *fnodes)
{
    int index = 0;
    for (int i = 1; i < MAXBLOCKS; i++)
    {
        if (fnodes[i].blockindex == -i)
        {
            index = i;
            break;
        }
    }
    return index;
}
int total_fnodes(fnode *fnodes)
{

    int count = 0;
    for (int i = 1; i < MAXBLOCKS; i++)
    {
        if (fnodes[i].blockindex == -i)
        {
            count++;
        }
    }
    return count;
}
int file_fnodes(fnode *fnodes, int block, int count)
{

    if (fnodes[block].blockindex == -1)
    {
        return count;
    }
    else
    {
        return file_fnodes(fnodes, fnodes[block].nextblock, count + 1);
    }
}

int file_exists(char *newfile, fentry *files)
{
    int index = 1;
    for (int i = 0; i < MAXFILES; i++)
    {
        if (strcmp(files[i].name, newfile) == 0)
        {

            index = 0;
        }
    }
    return index;
}
int current_nodes(fnode *fnodes, int count, int block)
{
    if (fnodes[block].nextblock == -1)
    {
        return count;
    }
    else
    {
        return current_nodes(fnodes, count + 1, fnodes[block].nextblock);
    }
}
int currentblock(fentry *files, fnode *fnodes, int max, int blockindex)
{

    for (int i = 0; i < max; i++)
    {
        if (fnodes[blockindex].nextblock == -1)
        {
            int next = find_blockindex(fnodes);
            fnodes[blockindex].nextblock = next;
            fnodes[next].blockindex = next;
            blockindex = next;
        }
        else
        {
            blockindex = fnodes[blockindex].nextblock;
        }
    }
    return blockindex;
}

void delete_f(FILE *fp, fentry *files, fnode *fnodes, char *newfile, int blockindex, int fileindex)
{

    //printf("This start %d\n ", (BLOCKSIZE * blockindex) + (start % BLOCKSIZE));

    //int size = file_fnodes(fnodes, blockindex);
    char zerobuf[BLOCKSIZE] = {0};
    char emptybuf[11] = {'\0'};
    if (find_filesize(files, newfile) == 0)
    {

        strncpy(files[fileindex].name, emptybuf, 11);
        write(fp, files, fnodes);
    }
    else
    {

        int max = (find_filesize(files, newfile) / BLOCKSIZE) + 1;
        strncpy(files[fileindex].name, emptybuf, 11);

        files[fileindex].size = 0;

        files[fileindex].firstblock = -1;

        fseek(fp, (BLOCKSIZE * blockindex), SEEK_SET);
        for (int i = 0; i < max; i++)
        {

            if (fnodes[blockindex].nextblock == -1)
            {
                fwrite(zerobuf, BLOCKSIZE, 1, fp);
                fnodes[blockindex].blockindex = -blockindex;
            }
            else
            {
                fwrite(zerobuf, BLOCKSIZE, 1, fp);
                int temp = blockindex;

                blockindex = fnodes[blockindex].nextblock;
                fnodes[temp].nextblock = -1;
                fnodes[temp].blockindex = -temp;
                fseek(fp, (BLOCKSIZE * blockindex), SEEK_SET);
            }
        }
        fseek(fp, 0, SEEK_SET);
        write(fp, files, fnodes);
    }
}

void read_f(FILE *fp, fentry *files, fnode *fnodes, char *newfile, int blockindex, int start, int length)
{

    int temp = start;

    fseek(fp, (BLOCKSIZE * blockindex) + (start % BLOCKSIZE), SEEK_SET);
    //printf("This start %d\n ", (BLOCKSIZE * blockindex) + (start % BLOCKSIZE));
    char block[length];
    for (int i = 0; i < length; i++)
    {

        if (start % BLOCKSIZE == 0 && start > temp)
        {
            if (fnodes[blockindex].nextblock == -1)
            {
                int next = find_blockindex(fnodes);
                fnodes[blockindex].nextblock = next;
                fnodes[next].blockindex = next;
                blockindex = next;
                fseek(fp, (blockindex * BLOCKSIZE), SEEK_SET);

                fread(block, 1, 1, fp);
                fwrite(block, 1, 1, stdout);
                start++;
            }
            else
            {
                blockindex = fnodes[blockindex].nextblock;
                fseek(fp, (blockindex * BLOCKSIZE), SEEK_SET);

                fread(block, 1, 1, fp);
                fwrite(block, 1, 1, stdout);
                start++;
            }
        }
        else
        {
            //printf("Next block %d\n", (blockindex * BLOCKSIZE));
            // fprintf(fp, "%s", &(str[i]));
            fread(block, 1, 1, fp);
            fwrite(block, 1, 1, stdout);
            start++;
        }
    }
}

void alloc(fentry *files, fnode *fnodes, char *newfile)
{
    int filesize = find_filesize(files, newfile);

    if (filesize == 0)
    {
        int index = find_blockindex(fnodes); ///allocate first blockindex
        int fileindex = find_fileindex(files, newfile);
        files[fileindex].firstblock = index;
        fnodes[index].blockindex = index; /// set the fnode blockindex to that index
    }
}
void write_files(FILE *fp, fentry *files, fnode *fnodes, int start, int blockindex, char *str, int length, char *newfile)
{

    int temp = start;

    fseek(fp, (BLOCKSIZE * blockindex) + (start % BLOCKSIZE), SEEK_SET);

    for (int i = 0; i < length; i++)
    {

        if (start % BLOCKSIZE == 0 && start > temp)
        {
            if (fnodes[blockindex].nextblock == -1)
            {
                int next = find_blockindex(fnodes);
                fnodes[blockindex].nextblock = next;
                fnodes[next].blockindex = next;
                blockindex = next;
                fseek(fp, (blockindex * BLOCKSIZE), SEEK_SET);

                fwrite(&str[i], 1, 1, fp);
                start++;
            }
            else
            {
                blockindex = fnodes[blockindex].nextblock;
                fseek(fp, (blockindex * BLOCKSIZE), SEEK_SET);

                fwrite(&str[i], 1, 1, fp);

                start++;
            }
        }
        else
        {

            fwrite(&str[i], 1, 1, fp);

            start++;
        }
    }
    if (temp + length > find_filesize(files, newfile))
    {

        int bytes_used = start % BLOCKSIZE;
        int bytes_to_write = BLOCKSIZE - (bytes_used % BLOCKSIZE);

        char zerobuf[BLOCKSIZE] = {0};
        fseek(fp, (blockindex * BLOCKSIZE) + length + temp, SEEK_SET);

        if (bytes_to_write != 0 && fwrite(zerobuf, bytes_to_write, 1, fp) < 1)
        {
            fprintf(stderr, "Error: write failed on init\n");
            closefs(fp);
            exit(1);
        }
    }
}

void write_f(FILE *fp, fentry *files, fnode *fnodes, int start, int length, char *newfile, char *str)
{
    int max = start / BLOCKSIZE;
    if (find_filesize(files, newfile) == 0)
    {
        alloc(files, fnodes, newfile);
    }
    int block = find_firstblock(files, newfile);

    int blockindex = currentblock(files, fnodes, max, block);

    ///if the file can be written in current fp;
    int start_temp = start;
    int length_temp = length;

    write_files(fp, files, fnodes, start, blockindex, str, length, newfile);

    if (start_temp + length_temp > find_filesize(files, newfile))
    {
        int i = find_fileindex(files, newfile);
        files[i].size = start_temp + length_temp;
    }
}

/* File system operations: creating, deleting, reading, and writing to files.
 */

// Signatures omitted; design as you wish.
void createfile(char *filename, char *newfile)
{
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    read(filename, files, fnodes); /// read the files
                                   ///////// write to file

    if (strcmp(files[MAXFILES - 1].name, "") != 0)
    {

        fprintf(stderr, "Createfile error: Maximum number of files have been created\n");

        exit(1);
    }
    if (file_exists(newfile, files) == 0)
    {
        fprintf(stderr, "Createfile error: No duplicate file names\n");

        exit(1);
    }
    if (strlen(newfile) > 11)
    {
        fprintf(stderr, "Createfile error: newfile must be 11 or less characters\n");

        exit(1);
    }

    FILE *fp = openfs(filename, "r+");
    for (int i = 0; i < MAXFILES; i++)
    {
        if (strcmp(files[i].name, "") == 0)
        {
            strncpy(files[i].name, newfile, 11);
            files[i].name[12] = '\0';
            break;
        }
    }
    write(fp, files, fnodes);

    closefs(fp);
}

void writefile(char *filename, char *newfile, int start, int length)
{
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    read(filename, files, fnodes);
    ///////////////////////////////////////
    //////////////////////////////////////
    if (file_exists(newfile, files) == 0)
    {

        int filesize = find_filesize(files, newfile); ///get the filesize

        int freenodes = total_fnodes(fnodes); ///get the number of freenodes
        int block = find_firstblock(files, newfile);

        int remaining_bytes = BLOCKSIZE - (filesize % BLOCKSIZE); ///remaining bytes to read on fnode block
        int count = 1;
        int freenodebytes = BLOCKSIZE * freenodes;
        if (start % BLOCKSIZE == 0 && filesize > 0)
        {
            int max = current_nodes(fnodes, count, block);
            remaining_bytes = 0;
            freenodebytes = ((freenodebytes + (BLOCKSIZE * max)) - start);
        }
        if (remaining_bytes == 128 && filesize == 0)
        {
            remaining_bytes = 0;
        }
        int total = freenodebytes + remaining_bytes;

        if (filesize == 0 && freenodes == 0)
        {
            fprintf(stderr, "Writefile Error: Not enough fnodes\n");
            exit(1);
        }
        if (total >= length && BLOCKSIZE * MAXBLOCKS > length) ///check if there is enough space to write
        {
            if (filesize >= start)
            {
                if (length == 0)
                {

                    int ch;
                    while ((ch = fgetc(stdin)) != EOF)
                    {
                        if (ch != -1)
                        {
                            fprintf(stderr, "Writefile Error: Lenght must be %d values(s)\n", length);
                            exit(1);
                        }
                    }
                }

                else
                {

                    char str[length + 2];

                    int *count;
                    count = (int *)malloc(sizeof(int));
                    count = 0;
                    while (fgets(&str[*count], length + 2, stdin))
                    {
                        count++;
                    }
                    free(count);
                    //printf("--- in progress: %s ---\n", block)

                    if (strlen(str) != length && length != 6)
                    {
                        fprintf(stderr, "Writefile Error1: Lenght must be %d values(s)\n", length);
                        exit(1);
                    }

                    //printf("--- in progress: %s ---\n", block)

                    FILE *fp = openfs(filename, "r+");

                    write_f(fp, files, fnodes, start, length, newfile, str);
                    fseek(fp, 0, SEEK_SET);
                    write(fp, files, fnodes);
                    closefs(fp);
                }
            }
            else
            {
                fprintf(stderr, "Writefile Error: start exceeds filesize\n");
                exit(1);
            }
        }
        else
        {
            fprintf(stderr, "Writefile Error: Not enough space to write\n");
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "Writefile Error: No file of that name created \n");
        exit(1);
    }
}

void readfile(char *filename, char *newfile, int start, int length)
{
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    read(filename, files, fnodes);

    if (file_exists(newfile, files) == 0)
    {
        int filesize = find_filesize(files, newfile);
        if (filesize >= start && filesize >= start + length)
        {
            FILE *fp = openfs(filename, "r+");
            int max = start / BLOCKSIZE;
            int block = find_firstblock(files, newfile);
            int blockindex = currentblock(files, fnodes, max, block);
            read_f(fp, files, fnodes, newfile, blockindex, start, length);
            closefs(fp);
        }
        else
        {
            fprintf(stderr, "Readfile Error: Index exceeds filesize\n");
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "Readfile Error: No file of that name created or length must be greater than 0\n");
        exit(1);
    }
}
void deletefile(char *filename, char *newfile)
{

    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    read(filename, files, fnodes);
    FILE *fp = openfs(filename, "r+");

    if (file_exists(newfile, files) == 0)
    {

        int block = find_firstblock(files, newfile);
        int index = find_fileindex(files, newfile);

        delete_f(fp, files, fnodes, newfile, block, index);
        closefs(fp);
    }

    else
    {
        fprintf(stderr, "Deletefile Error: No file of that name created\n");
        exit(1);
    }
}
