/*
This is a  very simple example application...
*/
#include <stdio.h>
#include <stdlib.h>

struct node{
    
  int id;
};

#define NUM_NODES 5

static int Ids[] = {6,1,5,9,3,7};

//static struct node list_nodes [NUM_NODES];

void print_nodes(struct node *lt_ptr){
    int i;
    for(i = 0; i < NUM_NODES; i++){
	printf("Id:%d%s", lt_ptr[i].id, (i== NUM_NODES-1) ?"\n":" ");
    }
}

void nodes_add(struct node *lt_ptr){
  int k;
  
  for(k = 0; k < NUM_NODES; k++){
    lt_ptr[k].id = Ids[k];
  }
}

void swap_nodes(struct node *node1, struct node *node2){
    struct node *tmp;
    
    tmp     = node1;
    node1   = node2;
    node2   = tmp;
}

void buble_sort(struct node *lt_ptr){
  int i, j;
  
  for(i = 0; i < NUM_NODES; i++){
    
      for(j = NUM_NODES - 1; j>i; j--){
	  if(lt_ptr[i].id > lt_ptr[j].id){
	     //swap_nodes(&lt_ptr[i], &lt_ptr[j]);
	      struct node *tmp = &lt_ptr[i];
	      
	      lt_ptr[j]  = lt_ptr[i];
	      lt_ptr[i]  = *tmp;
	  }
      }        
  }  
}

int main(){
  
  struct node *list_nodes = (struct node *)malloc(NUM_NODES*sizeof(struct node));
  if (list_nodes == NULL){
    printf("Error\n");
    exit(1);
  }
  
  nodes_add(list_nodes);
  
  print_nodes(list_nodes);
  
  buble_sort(list_nodes);
  
  print_nodes(list_nodes);
}
