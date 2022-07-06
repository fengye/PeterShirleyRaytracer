#ifndef _H_LIST_C
#define _H_LIST_C

typedef struct _list {

	void* block;
	size_t block_size;
	size_t item_count;
	
} list_t;

void list_create(list_t* item, uint32_t init_size);
void list_destroy(list_t* item);
size_t list_add_item(list_t* list, void* item);
void list_set_item(list_t* list, void* item);
void* list_get_item(list_t* list, uint32_t i);

#endif //_H_LIST_C