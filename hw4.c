// Use the psuedo code from Lab7
// Also got help from TA on Friday 3/17/2017 for pointer function
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region heap_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);

//how many ptrs into the heap we have
#define INDEX_SIZE 1000
void* heapindex[INDEX_SIZE];


//grabbing the address and size of the global memory region from proc 
void init_global_range(){
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;

  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    if (strstr(line, "hw4")!=NULL){
      ++counter;
      if (counter==3){
        sscanf(line, "%lx-%lx", &start, &end);
        global_mem.start=(size_t*)start;
        // with a regular address space, our globals spill over into the heap
        global_mem.end=malloc(256);
        free(global_mem.end);
      }
    }
    else if (read_bytes > 0 && counter==3) {
      if(strstr(line,"heap")==NULL) {
        // with a randomized address space, our globals spill over into an unnamed segment directly following the globals
        sscanf(line, "%lx-%lx", &start, &end);
        printf("found an extra segment, ending at %zx\n",end);						
        global_mem.end=(size_t*)end;
      }
      break;
    }
  }
  fclose(mapfile);
}


//marking related operations

int is_marked(size_t* chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(size_t* chunk) {
  (*chunk)|=0x2;
}

void clear_mark(size_t* chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((size_t*)c))& ~(size_t)3 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    printf("Panic, chunk is of zero size.\n");
  }
  if((c+chunk_size(c)) < sbrk(0))
    return ((void*)c+chunk_size(c));
  else 
    return 0;
}
int in_use(void *c) { 
  return (next_chunk(c) && ((*(size_t*)next_chunk(c)) & 1));
}


// index related operations

#define IND_INTERVAL ((sbrk(0) - (void*)(heap_mem.start - 1)) / INDEX_SIZE)
void build_heap_index() {
  // TODO
}

// the actual collection code
void sweep() 
{
  // TODO
  size_t *start = heap_mem.start - 1;
   
  while ( (start!=NULL) && (start < (size_t *)sbrk(0)) )
  {
    size_t *next = (size_t*)next_chunk(start);
    if(is_marked(start))    // if the block is marked
      clear_mark(start);    // then unmark the block
    else if(in_use(start))  // if the block is allocated
      free(start+1);        // free it

    start = next;           // go to next block
  }	

}

//determine if what "looks" like a pointer actually points to a block in the heap
size_t * is_pointer(size_t * ptr) 
{
  // TODO
  size_t *start = heap_mem.start -1;
  size_t *end = heap_mem.end;
  
  // pointer boundaries:
  // if pointer is null, or less than start, or greater than or equal to end, then return null
  if(ptr == NULL || ptr<start || ptr>=end)
    return NULL;
  
  // checking if the start is not null and less than end
  while( (start!=NULL) && (start < end) )
  { 
    int next = (size_t*)next_chunk(start);
    
    // if the pointer is in the chunk
    if ( ptr >= start )
    {
      if( next_chunk(start) > ptr ) 
      {
        if( in_use(start) )
        { 
          return start;   // returns the head of chunk
        }
      }
    }
    start = next;
  }
  
  return NULL; 

}

void walk_region_and_mark(void* start, void* end) 
{
  // TODO
  size_t *startR = (size_t*)start;

  if(startR == NULL)
    return;

  while(startR < (size_t*)end)
  {
    size_t *curr = is_pointer( (size_t*)*startR );
     
    if(curr != NULL)
    {
      if( !is_marked(curr) )
      {
        mark(curr);
        // calculate size of offset to get to next chunk from curr pointer
        int nextOffset = chunk_size(curr)/sizeof(size_t);
        walk_region_and_mark(curr+1, curr+nextOffset);
      }
    }
    startR = startR + 1;
  }

}

// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  heap_mem.start=malloc(512);
  //since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  heap_mem.end=sbrk(0);
  //grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;

  // build the index that makes determining valid ptrs easier
  // implementing this smart function for collecting garbage can get bonus;
  // if you can't figure it out, just comment out this function.
  // walk_region_and_mark and sweep are enough for this project.
  build_heap_index();

  //walk memory regions
  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep();
}
