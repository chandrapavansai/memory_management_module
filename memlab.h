#ifndef MEMLAB_H
#define MEMLAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h> 
#include <pthread.h>

#define max(x,y) ((x)>(y)?(x):(y))
#define min(x,y) ((x)>(y)?(y):(x))

#define MAX_STACK_SIZE 100 
#define STACK_FULL 2
#define STACK_PARTIAL 1
#define STACK_EMPTY 0
#define PAGE_TABLE_SIZE 40
#define MAX_CHUNK_SIZE 128
#define MAX_HOLE_NUMBER 64
#define COMPACTION_HOLE_SIZE 16

typedef struct table_entry{
	int type;
	int id;
	int logical_address;
	int status;
	int size;
	struct table_entry *next;
} table_entry;

typedef struct stack{
    int top;
    int capacity;
    int* arr;
} stack;

typedef struct list{
    int entry_index;
    struct list *next;
} list;

typedef struct hole{
    int offset;
    int size;
} hole;

int type_to_size(int type);
void *createMem(int size);
int createVar(int type);
int createArr(int n);
int assignVar(int id, int val);
int assignArr(int id, int arr[]);
int freeElem(int id);
list *first_fit(int size);

int createStack(stack *s);
int stack_status(stack *s);
int push(stack *s, int x);
int pop(stack *s);
void clear_stack(stack *s);
void print_stack(stack *s);

void print_list(list *l);
#endif