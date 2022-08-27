#include "memlab.h"

int counter=0,var_id=0,actual_size=0,page_table_counter=0;
int remaining_size=0;

void *ptr;
table_entry *ptr1;
stack *ptr_s;

pthread_t gc;
int gc_flag=0,del_num=0;

int createStack(stack *s){
    s->capacity = MAX_STACK_SIZE;
    s->top = -1;
    s->arr = (int *)malloc(MAX_STACK_SIZE*sizeof(int));
    return 1;
}

int stack_status(stack *s){
	if(s->top==-1) return STACK_EMPTY;
	if(s->top==s->capacity-1) return STACK_FULL;
	return STACK_PARTIAL;
}

int push(stack *s, int x){
	if(stack_status(s)==STACK_FULL) return 0;
	s->arr[++s->top]=x;
	printf("%d has been pushed into the stack %p\n",x,s);
	return 1;
}

int pop(stack *s){
	int x = -1;
	if(stack_status(s)==STACK_EMPTY) return x;
	printf("%d has been popped from the stack %p\n",s->arr[s->top],s);
	return s->arr[s->top--];
}

void clear_stack(stack *s){
	while(s->top!=-1){
		pop(s);
	}
	printf("Stack %p has been cleared\n",s);
	return;
}

void print_stack(stack *s){
	printf("stack at %p : \n",s);
	for(int i=s->top;i>=0;i--){
		int offset = (ptr1+s->arr[i])->logical_address;
		int type = (ptr1+s->arr[i])->type;
		if(type==1){
			table_entry *buf=(table_entry *)(ptr1+s->arr[i]);
			while(1){
				int size = buf->size;
				int offset = buf->logical_address;
				for(int i=0;i<size/4;i++) printf("%d ",*((int *)ptr+offset+i));
				if(buf->next==NULL){
					printf("\n");
					break;
				}
				buf=buf->next;
			}
		}
		else if(type==2) printf("%c\n",*((int *)ptr+offset));
		else if(type==3) printf("%d\n",*((int *)ptr+offset));
		else printf("%d\n",*((int *)ptr+offset));
	}
	if(stack_status(s)==STACK_EMPTY) printf("The stack is empty\n");
	return;
}

// stack *select_stack(){
// 	stack *buf=ptr_s;
// 	for(int i=0;i<number_of_stacks;i++){
// 		if(stack_status(buf)==STACK_EMPTY) return buf;
// 		buf--;
// 	}
// 	buf--;
// 	// printf("hi\n");
// 	createStack(buf);
// 	number_of_stacks++;
// 	return buf;
// }
void print_list(list *l){
	while(l!=NULL){
		printf("%d ",l->entry_index);
		l=l->next;
	}
	printf("\n");
	return;
}

int type_to_size(int type){
	if(type==1) return 4;
	if(type==2) return 1;
	if(type==3) return 3;
	return 1;
}

void *createMem(int size){
	counter=0; 
	page_table_counter=0;
	var_id=0; 
	actual_size=size;
	remaining_size=size;
	int book_keeping_size=size;
	ptr = (void *)malloc(size+book_keeping_size);
	if(ptr==NULL){
		printf("Not enough space to allocate %d bytes of memory\n",size+book_keeping_size);
	}
	else{
		printf("%d bytes of memory allocated\n",size);
		printf("%d bytes of book keeping memory allocated\n",book_keeping_size);
	}
	table_entry *buf=(table_entry *)((int *)ptr+size);
	ptr_s=(stack *)((int *)ptr+size-(int)sizeof(stack));
	createStack(ptr_s);
	ptr1=buf;
	for(int i=0;i<PAGE_TABLE_SIZE;i++){
		table_entry e = {-1,i,-1,0,0,NULL};
		memcpy(buf,&e,sizeof(table_entry));
		buf++;
	}
	return ptr;
}

int createVar(int type){
	int size1 = type_to_size(type);
	list *chunks = first_fit(size1);
	if(chunks==NULL){
		printf("not enough memory remaining for creation of the variable\n");
		return -1;
	}
	// print_list(chunks);
	(ptr1+(chunks->entry_index))->id = var_id++;
	(ptr1+(chunks->entry_index))->logical_address = counter;
	(ptr1+(chunks->entry_index))->status = 1;
	(ptr1+(chunks->entry_index))->type = type;
	// (ptr1+(chunks->entry_index))->size = 4;
	(ptr1+(chunks->entry_index))->next = NULL;
	counter+=4;
	if(type==1) printf("creating a new int type variable\n");
	else if(type==2) printf("creating a new medium int type variable\n");
	else if(type==3) printf("creating a new char type variable\n");
	else printf("creating a new boolean type variable\n");
	if(type!=1){ 
		printf("incrementing the counter by 4 even though the required size is %d bytes\n",type_to_size(type));
	}
	else printf("incrementing the counter by 4\n");
	push(ptr_s,var_id-1);
	page_table_counter++;
	return var_id-1;
}

int createArr(int n){
	list *chunks = first_fit(4*n);
	if(chunks==NULL){
		printf("not enough memory remaining for creation of the array\n");
		return -1;
	}
	// print_list(chunks);
	while(chunks!=NULL){
		(ptr1+(chunks->entry_index))->id = var_id++;
		(ptr1+(chunks->entry_index))->logical_address = counter;
		(ptr1+(chunks->entry_index))->status = 1;
		(ptr1+(chunks->entry_index))->type = 1;
		// (ptr1+(chunks->entry_index))->size = 4*n;
		if(chunks->next!=NULL){
			(ptr1+(chunks->entry_index))->next = (ptr1+((chunks->next)->entry_index));
		}
		else (ptr1+(chunks->entry_index))->next = NULL;
		chunks = chunks->next;
	}
	printf("creating a new array of size %d\n",n);
	counter+=4*n;
	printf("incrementing the counter by %d\n",4*n);
	page_table_counter++;
	push(ptr_s,var_id-1);
	return var_id-1;
}

int assignVar(int id, int val){
	int offset = (ptr1+id)->logical_address;
	*((int *)ptr+offset)=val;
	return offset;
}

int assignArr(int id, int arr[]){
	int offset = (ptr1+id)->logical_address;
	int size = (ptr1+id)->size;
	for(int i=0;i<size/4;i++){
		*((int *)ptr+offset+i)=arr[i];
	}
	return offset;
}

int freeElem(int id){
	table_entry *buf = (table_entry *)(ptr1+id);
	table_entry *prev = buf;
	while(buf!=NULL){
		int offset = buf->logical_address;
		int size = buf->size;
		buf->status=0;
		prev = buf;
		buf = buf->next;
		prev->next = NULL;
		for(int i=0;i<size/4;i++){
			*((int *)ptr+offset+i)=0;
		}
	}

}

list *first_fit(int size){
	printf("searching the holes for %d bytes of memory\n",size);
	if(size>remaining_size) return NULL;
	int buf=0;
	list *ans = (list *)malloc(sizeof(list));
	list *head=ans;
	list *prev=ans;
	for(int i=0;i<PAGE_TABLE_SIZE;i++){
		if(buf>=size) break;
		if((ptr1+i)->status==1) continue;
		ans->entry_index=i;
		ans->next = (list *)malloc(sizeof(list));
		prev = ans;
		ans = ans->next;
		if((ptr1+i)->size<=0){
			printf("making sure the memory size allocated to the chunk is a multiple of 4 for word alignment\n");
			(ptr1+i)->size=min(size+(4-size%4)%4,MAX_CHUNK_SIZE);
		}
		buf+=((ptr1+i)->size);
	}
	prev->next=NULL;
	return head;
}

int comparator(const void * a, const void * b) {
   return (*(int *)a - *(int *)b);
}

int comparator1(const void * a, const void * b) {
   return (((hole *)a)->offset-((hole *)b)->offset);
}

void copy_data(int size, int offset, int offset1){
	for(int i=0;i<size;i++){
		*((int *)ptr+offset1+i) = *((int *)ptr+offset+i);
	}
	return;
}

void compaction(hole h[], int h_num){
	printf("compaction : \n");
	printf("holes are (offset, size): \n");
	for(int i=0;i<h_num;i++) printf("%d    %d\n",h[i].offset,h[i].size);
	int hole_size=0,j=0;
	table_entry *buf = (table_entry *)ptr1;
	for(int i=0;i<PAGE_TABLE_SIZE && j<h_num;){
		if(buf->status==1){
			if(buf->logical_address<h[j].offset) continue;
			else{
				if(buf->size>=h[j].size){
					copy_data(h[j].size,buf->logical_address,h[j].offset);
					// buf->logical_address=
					h[j].size=0;
					h[j].offset=-1;
					j++;
				}
				else{
					copy_data(buf->size,buf->logical_address,h[j].offset);
					h[j].size-=buf->size;
					h[j].offset+=buf->size;
					i++; buf++;
				}
			}
		}
		else continue;
	}
}


void gc_run(){
	gc_flag=1;
	usleep(5000);
	return;
}

void *gc_runner(void *arg){
	time_t start,end;
	time(&start);
	while(1){
		if(gc_flag==1){
			int del_arr[del_num];
			int size=del_num,hole_size=0;
			printf("starting the marking phase\n");
			while(del_num--){
				int x=pop(ptr_s);
				printf("entry in the page table with id %d has been marked\n",x);
				del_arr[size-del_num-1]=x;
			}
			qsort(del_arr,size,sizeof(int),comparator);
			int j=0;
			printf("staring the sweep phase\n");
			hole h[MAX_HOLE_NUMBER];
			int h_num=0;
			for(int i=0;i<PAGE_TABLE_SIZE;i++){
				if((ptr1+i)->id == del_arr[j]){
					printf("freeing the memory occupied by the variable with id %d\n",del_arr[j]);
					freeElem(del_arr[j]);
					hole_size=max(hole_size,(ptr1+i)->size);
					if((ptr1+i)->size>COMPACTION_HOLE_SIZE){
						if(h_num<MAX_HOLE_NUMBER){ 
							h[h_num].offset = (ptr1+i)->logical_address;
							h[h_num].size = (ptr1+i)->size;
						}
					}
					j++;
				}
				if(j==size) break;
			}
			qsort(h,h_num,sizeof(hole),comparator1);
			if(hole_size>=COMPACTION_HOLE_SIZE) compaction(h,h_num);
			time(&start);
			del_num=0;
			gc_flag=0;
		}
		else{
			while(gc_flag==0){
				time(&end);
				if(end-start>=0.1){
					gc_flag=1;
					break;
				}
			}
		}
	}
}


void gc_initialize(){
	pthread_create(&gc, NULL, gc_runner, NULL);
    // pthread_join(gc, NULL);
	return;
}

int func(int a, int b){
	del_num=0;
	int x = createArr(8);
	int arr[8]={1,2,3,4,5,6,7,8};
	int offset = assignArr(x,arr);
	del_num++;
	print_stack(ptr_s);
	gc_run();
	return 1;
}


int main(){
	createMem(20000*sizeof(char));
	gc_initialize();
	int del_num1=0;
	int x=createVar(1);
	del_num1++;
	int offset=assignVar(x,6);
	printf("%d \n",*((int *)ptr+offset));
	x = createArr(6);
	del_num1++;
	int arr[6]={1,2,3,4,5,6};
	offset = assignArr(x,arr);
	for(int i=0;i<6;i++) printf("%d ",*((int *)ptr+offset+i));
	printf("\n");
	func(2,3);
	print_stack(ptr_s);
	clear_stack(ptr_s);
	print_stack(ptr_s);
	del_num=del_num1;
	gc_run();
	sleep(2);
	return 0;
}