/* 
 * mm-implicit.c -  Simple allocator based on implicit free lists, 
 *                  first fit placement, and boundary tag coalescing. 
 *
 * Each block has header and footer of the form:
 * 
 *      31                     3  2  1  0 
 *      -----------------------------------
 *     | s  s  s  s  ... s  s  s  0  0  a/f
 *      ----------------------------------- 
 * 
 * where s are the meaningful size bits and a/f is set 
 * iff the block is allocated. The list has the following form:
 *
 * begin                                                          end
 * heap                                                           heap  
 *  -----------------------------------------------------------------   
 * |  pad   | hdr(8:a) | ftr(8:a) | zero or more usr blks | hdr(8:a) |
 *  -----------------------------------------------------------------
 *          |       prologue      |                       | epilogue |
 *          |         block       |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
  /* Team name */
  "CompSci",
  /* First member's full name */
  "Irvin Carbajal",
    //HEAVILY REFERENCED ON THE TEXTBOOK FOR FUNCTIONS
  /* First member's email address */
  "irca8776@colorado.edu",
  /* Second member's full name (leave blank if none) */
  "",
  /* Second member's email address (leave blank if none) */
  ""
};

/////////////////////////////////////////////////////////////////////////////
// Constants and macros
//
// These correspond to the material in Figure 9.43 of the text
// The macros have been turned into C++ inline functions to
// make debugging code easier.
//
/////////////////////////////////////////////////////////////////////////////
#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */

static inline int MAX(int x, int y) {
  return x > y ? x : y;
}

//
// Pack a size and allocated bit into a word
// We mask of the "alloc" field to insure only
// the lower bit is used
//
static inline uint32_t PACK(uint32_t size, int alloc) {
  return ((size) | (alloc & 0x1));
}

//
// Read and write a word at address p
//
static inline uint32_t GET(void *p) { return  *(uint32_t *)p; }
static inline void PUT( void *p, uint32_t val)
{
  *((uint32_t *)p) = val;
}

//
// Read the size and allocated fields from address p
//
static inline uint32_t GET_SIZE( void *p )  { 
  return GET(p) & ~0x7;
}

static inline int GET_ALLOC( void *p  ) {
  return GET(p) & 0x1;
}

//
// Given block ptr bp, compute address of its header and footer
//
static inline void *HDRP(void *bp) {

  return ( (char *)bp) - WSIZE;
}
static inline void *FTRP(void *bp) {
  return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
}

//
// Given block ptr bp, compute address of next and previous blocks
//
static inline void *NEXT_BLKP(void *bp) {
  return  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)));
}

static inline void* PREV_BLKP(void *bp){
  return  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
}

/////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//

static char *heap_listp;  /* pointer to first block */  
static char *endh; 

//
// function prototypes for internal helper routines
//
static void *extend_heap(uint32_t words);
static void place(void *bp, uint32_t asize);
static void *find_fit(uint32_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);

//
// mm_init - Initialize the memory manager 
//
int mm_init(void) 
{
  //
  // You need to provide this
  //
    //creating the initial empty heap
    //if no memory block found, then conditon satisfied
    //mem_sbrk extends the heap by 4 words
    //mem_sbrk returns new start address
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    {
        return -1;
    }
    
    //alignment padding
    //sets first word to zero
    PUT(heap_listp, 0);
    //additional heap space for memory system
    //initializes the prologue block 
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    //initializes the epilogue block
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    //uses multiples of 2 words to maintain aligment 
    heap_listp += (2*WSIZE);
    
    //if condition satisfied, the initialization has failed
    //
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    {
        return -1;
    }
  
    //if it gets here, the initialization has been successful 
  return 0;
}
//the pro/epi blocks eliminate the edge conditions to reduce the marging of error
//makes it faster cause we avoid checking for those rare edge conditions 

//
// extend_heap - Extend heap with free block and return its block pointer
//
static void *extend_heap(uint32_t words) 
{
  //
  // You need to provide this
  //
    
    //
    char *bp;
    size_t size;
    
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    //fails to extend when out of memory 
    //mem_sbrk returns -1 
    if((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }
    
    //initializes header of free block at even alignment 
    PUT(HDRP(bp), PACK (size, 0));
    //initializes footer of free block at even alignement 
    PUT(FTRP(bp), PACK (size, 0));
    //sets new header for the epilogue
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
    
    //coalesce of the prev was free
    return coalesce(bp);
    
}


//
// Practice problem 9.8
//
// find_fit - Find a fit for a block with asize bytes 
//
static void *find_fit(uint32_t asize)
{
    
    void *bp;
    
    if (endh == NULL)
    {
        //sets end of heap at prologue block
        endh = heap_listp;
    }
    
    //goes through the block pointers from start to end of list searching for 
    //a fit for a block of size asize. 
    //improves because if its not endh == NULL, its doesnt reset bp to heap_listp
    for(bp = endh; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if(!GET_ALLOC(HDRP(bp)) && ( GET_SIZE(HDRP(bp)) >= asize))
        {
            endh = bp;
            return bp;
        }
    }
    
    //returns null if not fit was found
    return NULL;
    
}

// 
// mm_free - Free a block 
//
void mm_free(void *bp)
{
  //
  // You need to provide this
  //
    
    //gets the size of header of block 
    size_t size = GET_SIZE(HDRP(bp));

    //frees and initializes the header and footer of block 
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    //merges it to any free blocks 
    coalesce(bp);
}

//
// coalesce - boundary tag coalescing. Return ptr to coalesced block
//
static void *coalesce(void *bp) 
{
    
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    //prev and next are allocated 
    if(prev_alloc && next_alloc)
    {  
        return bp;
    }
    
    //prev is allocated and next is free
    else if(prev_alloc && !next_alloc) 
    { 
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } 
    
    //prev is free and next is allocated
    else if(!prev_alloc && next_alloc) 
    { 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    //both prev and next are free
    else 
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    endh = bp;
  return bp;
}

//
// mm_malloc - Allocate a block with at least size bytes of payload 
//
void *mm_malloc(uint32_t size) 
{
  //
  // You need to provide this
  //
    
    size_t asize; //adjusted block size
    size_t extendsize; //amount to extend heap if no fit
    char *bp;

    //Ignore 0 mallocs
    if(size == 0) 
    { 
        return NULL; 
    }
    //adjust block size to include overhead and alignment requirements
    //asize doubles b/c we need a block of min size of 16
    //16: 8 for alignment requirement and 8 for overhead of header and footer
    if(size <= DSIZE)
    {
        asize = 2*DSIZE;
    }
    //adds overhead bytes and rounds up to multiples of 8 
    else
    {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
    
    //finds a fit for the adjusted block
    //if found, it places it and returns the address of block
    if((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    //if no fit found, the heap is extended and places the the block 
    //then returns the address of block 
    extendsize = MAX(asize, CHUNKSIZE);
    //will return NULL if ran out of memory
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
    {
            return NULL;
    }
    place(bp, asize);

    return bp;
} 

//
//
// Practice problem 9.9
//
// place - Place block of asize bytes at start of free block bp 
//         and split if remainder would be at least minimum block size
//
static void place(void *bp, uint32_t asize)
{
    //gets the size of free block 
    size_t csize = GET_SIZE(HDRP(bp));
    
    //compares the size of free block and the requested block
    //if size of remainder equals or exceeds 16, minimal b size
    if((csize - asize) >= (2*DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        //splits and allocates in next block
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    
    //no split needed so just places the requested block at beginning of free block
    else 
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


//
// mm_realloc -- implemented for you
//
void *mm_realloc(void *ptr, uint32_t size)
{
  void *newp;
  uint32_t copySize;

  newp = mm_malloc(size);
  //if pointer is NULL then no memory
  if (newp == NULL) {
    printf("ERROR: mm_malloc failed in mm_realloc\n");
    exit(1);
  }
 //copies the data of original to the new block
  copySize = GET_SIZE(HDRP(ptr));
  //ajusts size if larger
  if (size < copySize) {
    copySize = size;
  }
  memcpy(newp, ptr, copySize);
    
  //frees the oringinal block and returns the new pointer 
  mm_free(ptr);
  return newp;
}

//
// mm_checkheap - Check the heap for consistency 
//
void mm_checkheap(int verbose) 
{
  //
  // This provided implementation assumes you're using the structure
  // of the sample solution in the text. If not, omit this code
  // and provide your own mm_checkheap
  //
  void *bp = heap_listp;
  
  if (verbose) {
    printf("Heap (%p):\n", heap_listp);
  }

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))) {
	printf("Bad prologue header\n");
  }
  checkblock(heap_listp);

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (verbose)  {
      printblock(bp);
    }
    checkblock(bp);
  }
     
  if (verbose) {
    printblock(bp);
  }

  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))) {
    printf("Bad epilogue header\n");
  }
}

static void printblock(void *bp) 
{
  uint32_t hsize, halloc, fsize, falloc;

  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));  
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));  
    
  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%d:%c] footer: [%d:%c]\n",
	 bp, 
	 (int) hsize, (halloc ? 'a' : 'f'), 
	 (int) fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
  if ((uintptr_t)bp % 8) {
    printf("Error: %p is not doubleword aligned\n", bp);
  }
  if (GET(HDRP(bp)) != GET(FTRP(bp))) {
    printf("Error: header does not match footer\n");
  }
}

