#ifndef SELECTION_LIST_IMPLEMENTATION
#define SELECTION_LIST_IMPLEMENTATION
#include <libavcodec/packet.h>

typedef struct SelectionList SelectionList;

SelectionList* selection_list_alloc();
void selection_list_free(SelectionList* list);

void selection_list_push_back(SelectionList* list, void* item);
void selection_list_push_front(SelectionList* list, void* item);
void selection_list_clear(SelectionList* list);

int selection_list_index(SelectionList* list);
int selection_list_length(SelectionList* list);
void* selection_list_get(SelectionList* list);
int selection_list_set_index(SelectionList* list, int index);
int selection_list_can_move_index(SelectionList* list, int offset);
int selection_list_try_move_index(SelectionList* list, int offset);
int selection_list_is_empty(SelectionList* list);
#endif
