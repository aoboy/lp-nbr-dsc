#ifndef SR_SORT_H
#define SR_SORT_H


#include "./sr-nodes.h"

struct sr_gains *sr_mergesort(struct sr_gains *head);

void sr_insertion_sort(struct sr_gains **head_ref);



#endif //SR_SORT_H