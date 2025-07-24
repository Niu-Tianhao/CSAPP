#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

typedef struct cache_line
{
    int valid;    
    int tag;       
    int timestamp;
} Cache_line;

typedef struct cache_
{
    int S;
    int E;
    int B;
    Cache_line **line;
} Cache;

int hit_count = 0, miss_count = 0, eviction_count = 0; 
int verbose = 0;                                   
char t[1000];
Cache *cache = NULL;

void Init_Cache(int s, int E, int b)
{
    int S = 1 << s;
    int B = 1 << b;
    cache = (Cache *)malloc(sizeof(Cache));
    cache->S = S;
    cache->E = E;
    cache->B = B;
    cache->line = (Cache_line **)malloc(sizeof(Cache_line *) * S);
    for (int i = 0; i < S; i++)
    {
        cache->line[i] = (Cache_line *)malloc(sizeof(Cache_line) * E);
        for (int j = 0; j < E; j++)
        {
            cache->line[i][j].valid = 0; 
            cache->line[i][j].tag = -1;
            cache->line[i][j].timestamp = 0;
        }
    }
}

void free_Cache()
{
    int S = cache->S;
    for (int i = 0; i < S; i++)
    {
        free(cache->line[i]);
    }
    free(cache->line);
    free(cache);
}

int get_index(int set_index, int tag)
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->line[set_index][i].valid && cache->line[set_index][i].tag == tag)
            return i;
    }
    return -1;
}
int find_LRU(int set_index)
{
    int max_index = 0;
    int max_stamp = 0;
    for(int i = 0; i < cache->E; i++){
        if(cache->line[set_index][i].timestamp > max_stamp){
            max_stamp = cache->line[set_index][i].timestamp;
            max_index = i;
        }
    }
    return max_index;
}
int is_full(int set_index)
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->line[set_index][i].valid == 0)
            return i;
    }
    return -1;
}
void update(int i, int set_index, int tag){
    cache->line[set_index][i].valid=1;
    cache->line[set_index][i].tag = tag;
    for(int k = 0; k < cache->E; k++)
        if(cache->line[set_index][k].valid==1)
            cache->line[set_index][k].timestamp++;
    cache->line[set_index][i].timestamp = 0;
}
void update_info(int tag, int set_index)
{
    int index = get_index(set_index, tag);
    if (index == -1)
    {
        miss_count++;
        if (verbose)
            printf("miss ");
        int i = is_full(set_index);
        if(i==-1){
            eviction_count++;
            if(verbose) printf("eviction");
            i = find_LRU(set_index);
        }
        update(i,set_index,tag);
    }
    else{
        hit_count++;
        if(verbose)
            printf("hit");
        update(index,set_index,tag);    
    }
}
void get_trace(int s, int E, int b)
{
    FILE *pFile;
    pFile = fopen(t, "r");
    if (pFile == NULL)
    {
        exit(-1);
    }
    char identifier;
    unsigned address;
    int size;
    // Reading lines like " M 20,1" or "L 19,3"
    while (fscanf(pFile, " %c %x,%d", &identifier, &address, &size) > 0) 
    {
        int tag = address >> (s + b);
        int set_index = (address >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s));
        switch (identifier)
        {
        case 'M': 
            update_info(tag, set_index);
            update_info(tag, set_index);
            break;
        case 'L':
            update_info(tag, set_index);
            break;
        case 'S':
            update_info(tag, set_index);
            break;
        }
    }
    fclose(pFile);
}

void print_help()
{
    printf("** A Cache Simulator by Deconx\n");
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n");
    printf("-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n");
    printf("-b <num>   Number of block offset bits.\n");
    printf("-t <file>  Trace file.\n\n\n");
    printf("Examples:\n");
    printf("linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}
int main(int argc, char *argv[])
{
    char opt;
    int s, E, b;
    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:")))
    {
        switch (opt)
        {
        case 'h':
            print_help();
            exit(0);
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            strcpy(t, optarg);
            break;
        default:
            print_help();
            exit(-1);
        }
    }
    Init_Cache(s, E, b);
    get_trace(s, E, b);
    free_Cache();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}