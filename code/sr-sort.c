
#include "lib/list.h"


#include "./sr-nodes.h"



///=========================================================================/
/* merge the lists.. */
/**
 * @brief merge
 * @param head_one
 * @param head_two
 * @return
 */
static struct sr_nodes*
sr_merge(struct sr_nodes *head_one, struct sr_nodes *head_two) {
    struct sr_nodes *head_three;

    if(head_one == NULL)
        return head_two;

    if(head_two == NULL)
        return head_one;

    if(head_one->slot_gain > head_two->slot_gain) {
        head_three = head_one;
        head_three->next = sr_merge(head_one->next, head_two);
    } else {
        head_three = head_two;
        head_three->next = sr_merge(head_one, head_two->next);
    }

    return head_three;
}

///=========================================================================/
/* preform merge sort on the linked list */
struct sr_nodes *sr_mergesort(struct sr_nodes *head) {
    struct sr_nodes  *head_one;
    struct sr_nodes  *head_two;

    if((head == NULL) || (head->next == NULL))
        return head;

    head_one = head;
    head_two = head->next;
    while((head_two != NULL) && (head_two->next != NULL)) {
        head = head->next;
        head_two = head->next->next;
    }
    head_two = head->next;
    head->next = NULL;

    return sr_merge(sr_mergesort(head_one), sr_mergesort(head_two));
}
///=========================================================================/
///=========================================================================/
///=========================================================================/
/* second attempt */
/**
 * @brief sorted_insert
 * @param head_ref
 * @param new_node
 */
static void 
sr_sorted_insert(struct sr_nodes** head_ref, struct sr_nodes* new_node){
    struct sr_nodes* current;
    //special case for the head end
    if (*head_ref == NULL || (*head_ref)->slot_gain < new_node->slot_gain)
    {
        new_node->next = *head_ref;
        *head_ref = new_node;
    }else{
        //locate the node before the point of insertion
        current = *head_ref;
        while (current->next!=NULL &&
               current->next->slot_gain > new_node->slot_gain)
        {
            current = list_item_next(current); //current->next;
        }
        new_node->next = current->next;
        current->next = new_node;

    }
}
///=========================================================================/
// sort a linked list : insertion sort
/**
 * @brief insertion_sort
 * @param head_ref
 */
void sr_insertion_sort(struct sr_nodes **head_ref)
{
    // Initialize sorted linked list
    struct sr_nodes *sorted = NULL;

    // go through the given linked list and insert every
    // node to sorted
    struct sr_nodes *current_h = *head_ref;
    
    //watchdog_periodic();
    while (current_h != NULL){
      
        // Store next for next iteration
        struct sr_nodes *next_h = list_item_next(current_h);

        // insert current in sorted linked list
        sr_sorted_insert(&sorted, current_h);
	
        // Update current
        current_h = next_h;
    }

    // Update head_ref to point to sorted linked list
    *head_ref = sorted;
}
