/**
 * @file lib/list.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Массивы
 * @version 0.4.2
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
#include	"lib/list.h"

void list_init(list_t* list){
    list->first = nullptr;
    list->count = 0;
}

void list_add(list_t* list, list_item_t* item){
    if (item->list == nullptr){
        if (list->first){
            item->list = list;
            item->next = list->first;
            item->prev = list->first->prev;
            item->prev->next = item;
            item->next->prev = item;
        } else {
            item->list = list;
            item->next = item;
            item->prev = item;
            list->first = item;
        }

        list->count++;
    }
}

void list_remove(list_item_t* item){
    if (item->list->first == item) {
        item->list->first = item->next;
        if (item->list->first == item){
            item->list->first = nullptr;
        }
    }
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->list->count--;
}
