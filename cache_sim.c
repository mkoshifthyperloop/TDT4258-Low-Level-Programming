#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

typedef enum { dm, fa } cache_map_t;
typedef enum { uc, sc } cache_org_t;
typedef enum { instruction, data } access_t;

typedef struct {
  uint32_t address;
  access_t accesstype;
} mem_access_t;

typedef struct {
  uint64_t accesses;
  uint64_t hits;
  // You can declare additional statistics if
  // you like, however you are now allowed to
  // remove the accesses or hits
} cache_stat_t;

typedef struct {
    bool valid;
    uint32_t tag;
}cache_block_t;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE

uint32_t cache_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE* ptr_file) {
  char type;
  mem_access_t access;

  if (fscanf(ptr_file, "%c %x\n", &type, &access.address) == 2) {
    if (type != 'I' && type != 'D') {
      printf("Unkown access type\n");
      exit(0);
    }
    access.accesstype = (type == 'I') ? instruction : data;
    return access;
  }

  /* If there are no more entries in the file,
   * return an address 0 that will terminate the infinite loop in main
   */
  access.address = 0;
  return access;
}

bool cache_access_dm(cache_block_t *cache, mem_access_t access, uint32_t address_mask, uint32_t tag_mask){
  uint32_t index = (access.address & address_mask) >> 5;
  uint32_t tag = (access.address & tag_mask) >> 5;
  
  if (cache[index].valid && cache[index].tag == tag){
    return true;
  }
  else{
    cache[index].tag = tag;
    cache[index].valid = true;
    return false;
  }
  

}

bool cache_access_fa(cache_block_t *cache, mem_access_t access, uint32_t tag_mask, uint32_t blocks, uint32_t next_index){
  uint32_t tag = (access.address & tag_mask);

  for (int i = 0; i < blocks; i++){
    if (cache[i].valid && cache[i].tag == tag){
      return true;
    }
  }

  cache[next_index].valid = true;
  cache[next_index].tag = tag;
  return false;
}


void main(int argc, char** argv) {
  // Reset statistics:
  memset(&cache_statistics, 0, sizeof(cache_stat_t));

  /* Read command-line parameters and initialize:
   * cache_size, cache_mapping and cache_org variables
   */
  /* IMPORTANT: *IF* YOU ADD COMMAND LINE PARAMETERS (you really don't need to),
   * MAKE SURE TO ADD THEM IN THE END AND CHOOSE SENSIBLE DEFAULTS SUCH THAT WE
   * CAN RUN THE RESULTING BINARY WITHOUT HAVING TO SUPPLY MORE PARAMETERS THAN
   * SPECIFIED IN THE UNMODIFIED FILE (cache_size, cache_mapping and cache_org)
   */
  if (argc != 4) { /* argc should be 2 for correct execution */
    printf(
        "Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] "
        "[cache organization: uc|sc]\n");
    exit(0);
  } else {
    /* argv[0] is program name, parameters start with argv[1] */

    /* Set cache size */
    cache_size = atoi(argv[1]);
   
    /* Set Cache Mapping */
    if (strcmp(argv[2], "dm") == 0) {
      cache_mapping = dm;
    } else if (strcmp(argv[2], "fa") == 0) {
      cache_mapping = fa;
    } else {
      printf("Unknown cache mapping\n");
      exit(0);
    }

    /* Set Cache Organization */
    if (strcmp(argv[3], "uc") == 0) {
      cache_org = uc;
    } else if (strcmp(argv[3], "sc") == 0) {
      cache_org = sc;
    } else {
      printf("Unknown cache organization\n");
      exit(0);
    }
  }

  uint32_t blocks = cache_size/block_size;

  cache_block_t full_cache[blocks];
  memset(&full_cache, 0, sizeof(full_cache));

  cache_block_t data_cache[(cache_size/2)/block_size];
  memset(&data_cache, 0, sizeof(data_cache));
  cache_block_t instruction_cache[(cache_size/2)/block_size];
  memset(&data_cache, 0, sizeof(data_cache));

  /* Open the file mem_trace.txt to read memory accesses */
  FILE* ptr_file;
  ptr_file = fopen("mem_trace.txt", "r");
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }

  uint32_t dm_adress_mask = ((blocks) - 1) << 5;
  uint32_t dm_tag_mask = 0xffffffff - dm_adress_mask - 63;

  uint32_t fa_tag_mask = 0xffffffff - 63;
  uint32_t fa_next_index_uc = 0;
  uint32_t fa_next_index_data = 0;
  uint32_t fa_next_index_instruction = 0;

  /* Loop until whole trace file has been read */
  mem_access_t access;
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    printf("%d %x\n", access.accesstype, access.address);
    /* Do a cache access */
    // ADD YOUR CODE HERE
    switch (cache_mapping)
    {
    case dm:
      switch (cache_org)
      {
      case uc:
        if(cache_access_dm(full_cache, access, dm_adress_mask, dm_tag_mask)){
          cache_statistics.hits++;
        }
        break;

      case sc:
        switch (access.accesstype)
        {
        case instruction:
          if(cache_access_dm(instruction_cache, access, dm_adress_mask, dm_tag_mask)){
            cache_statistics.hits++;
          }
          break;

        case data:
          if(cache_access_dm(data_cache, access, dm_adress_mask, dm_tag_mask)){
            cache_statistics.hits++;
          }
          break;
        }
        break;
      }
      break;

    case fa:
      switch (cache_org)
      {
      case uc:
        if (cache_access_fa(full_cache, access, fa_tag_mask, blocks, fa_next_index_uc)){
          cache_statistics.hits++;
        }
        else{
          fa_next_index_uc++;
          if (fa_next_index_uc == blocks){
            fa_next_index_uc = 0;
          }
        }
        break;

      case sc:
        switch (access.accesstype)
        {
        case instruction:
          if (cache_access_fa(instruction_cache, access, fa_tag_mask, blocks, fa_next_index_instruction)){
            cache_statistics.hits++;
          }
          else{
            fa_next_index_instruction++;
            if (fa_next_index_instruction == blocks/2){
              fa_next_index_instruction = 0;
            }
          }
          break;

        case data:
          if (cache_access_fa(data_cache, access, fa_tag_mask, blocks, fa_next_index_data)){
            cache_statistics.hits++;
          }
          else{
            fa_next_index_data++;
            if (fa_next_index_data == blocks/2){
              fa_next_index_data = 0;
            }
          }
        }
        break;
      }
      break;
    }
    cache_statistics.accesses++;
  }

  /* Print the statistics */
  // DO NOT CHANGE THE FOLLOWING LINES!
  printf("\nCache Statistics\n");
  printf("-----------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Hit Rate: %.4f\n",
         (double)cache_statistics.hits / cache_statistics.accesses);
  // DO NOT CHANGE UNTIL HERE
  // You can extend the memory statistic printing if you like!

  /* Close the trace file */
  fclose(ptr_file);
}