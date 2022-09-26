#include <selectionlist.h>
#include <malloc.h>

typedef struct SelectionListNode {
    void* data;
    struct SelectionListNode* next;
    struct SelectionListNode* prev;
} SelectionListNode;

struct SelectionList {
    SelectionListNode* first;
    SelectionListNode* last;
    SelectionListNode* current;
    int length;
    int index;
};

SelectionList* selection_list_alloc(int elem_size) {
    SelectionList* list = (SelectionList*)malloc(sizeof(SelectionList));
    if (list == NULL) {
        return NULL;
    }

    list->first = NULL;
    list->last = NULL;
    list->current = NULL;
    list->length = 0;
    list->index = -1;
    return list;
}

void selection_list_free(SelectionList* list) {
    selection_list_clear(list);
    free(list);
}

void selection_list_push_back(SelectionList* list, void* item) {
    SelectionListNode* new_last = (SelectionListNode*)malloc(sizeof(SelectionListNode));
    if (new_last == NULL) {
        return;
    }

    new_last->data = item;

    if (list->last != NULL) {
        list->last->next = new_last;
        new_last->prev = list->last;
        list->last = new_last;
    } else if (list->last == NULL) {
        list->first = new_last;
        list->last = new_last;
        list->current = new_last;
        list->index = 0;
    }

    list->length += 1;
}

void selection_list_push_front(SelectionList* list, void* item) {
    SelectionListNode* newFirst = (SelectionListNode*)malloc(sizeof(SelectionListNode));
    if (newFirst == NULL) {
        return;
    }

    newFirst->data = item;

    if (list->first != NULL) {
        list->first->prev = newFirst;
        newFirst->next = list->first;
        list->first = newFirst;
        list->index++;
    } else if (list->first == NULL) {
        list->first = newFirst;
        list->last = newFirst;
        list->current = newFirst;
        list->index = 0;
    }

    list->length += 1;

}

void selection_list_clear(SelectionList* list) {
    if (list->length == 0) {
        return;
    }

    SelectionListNode* currentNode = list->first;
    while (currentNode->next != NULL) {
        SelectionListNode* temp = currentNode;
        currentNode = currentNode->next;
        free(temp);
        list->length--;
    }
    free(currentNode);
    list->length--;

    list->first = NULL;
    list->last = NULL;
    list->current = NULL;
    list->index = 0;
}


int selection_list_index(SelectionList* list) {
    return list->index;
}

int selection_list_length(SelectionList* list) {
    return list->length;
}

void* selection_list_get(SelectionList* list) {
    if (list->length > 0) {
        return list->current->data;
    }
    return NULL;
}

int selection_list_set_index(SelectionList* list, int new_index) {
    if (new_index >= 0 && new_index < list->length) {
        if (new_index < list->index) {
            for (int i = 0; i < list->index - new_index; i++) {
                list->current = list->current->prev;
            }
        } else if (new_index > list->index) {
            for (int i = 0; i < new_index - list->index; i++) {
                list->current = list->current->next;
            }
        }   

        list->index = new_index;
        return 1;
    }

    return 0;
}

int selection_list_can_move_index(SelectionList* list, int offset) {
    return list->index + offset >= 0 && list->index + offset < list->length;
}

int selection_list_try_move_index(SelectionList* list, int offset) {
    if (selection_list_can_move_index(list, offset)) {
        return selection_list_set_index(list, list->index + offset);
    }
    return 0;
}

int selection_list_is_empty(SelectionList* list) {
    return list->length == 0 ? 1 : 0;
}

