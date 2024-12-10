#include<unistd.h>
#include<pthread.h>
#include<stdio.h>

typedef char ALIGN[64];
//Now we need to add the size of the header to the bytes which we wanted to allocate.
union header{
	struct header_t{ 
		size_t size;
		unsigned int is_free;
		union header *next; // creating a linked list as the heap mem may not always be contino
				       // -us. for eg. a calling function might be calling sbrk again.
	}s;
	ALIGN stub;//As the data in the struct header_t will be of varying size , this union is created
		   //so as to make each piece of memory of uniform size.
};

typedef union header header_t;
header_t *head,*tail;

void *Malloc(size_t bytes);
header_t *get_free_block(size_t bytes);

pthread_mutex_t global_malloc_lock;

void *Malloc(size_t bytes){
	size_t total_size;
	void *block;
	header_t *header;
	
	printf("Allocating memory worth %ld\n",bytes);
	
	if(!bytes) // we check if the size is 0
		return NULL;
	
	pthread_mutex_lock(&global_malloc_lock);
	header = get_free_block(bytes);
	if(header){
		header->s.is_free = 0;
		pthread_mutex_unlock(&global_malloc_lock);
		return (void*)(header + 1); //return as free block of memory has already been found
	}
	total_size = sizeof(header_t) + bytes;
	block = sbrk(total_size);
	if (block == (void*) -1) {
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}
	header = block; // as block is a void* we don't need to explicitly type-cast it to header_t*

	header->s.size = bytes;
	header->s.is_free = 0;
	header->s.next = NULL;

	if(!head)
		head = header;
	if(tail)
		tail->s.next = header;

	tail = header;
	pthread_mutex_unlock(&global_malloc_lock);	

	printf("Memory allocated!\n");
	return (void*)(header+1);
}

//traverses the linked lists and sees if there already exist a block of memory marked as free
header_t *get_free_block(size_t bytes){ 
	header_t *curr = head;
	while(curr){
		if(curr->s.is_free && curr->s.size >= bytes)
			return curr;
		curr = curr->s.next;
	}
	
	return NULL;
}

int main(){
	int *a;
	a = (int *)Malloc(100);
	*a =10;
	printf("%p\t%d\n",&a,*a);
}
