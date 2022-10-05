#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct node
{
    int pid;
    struct node* next;
} node;

void add(int pid, struct node **head)
{
    struct node *temp = malloc(sizeof(node));
    temp -> pid = pid;
    temp -> next = NULL;
    if (*head == NULL)
    {
        *head = temp;
    } else {
        struct node* ptr = *head;

        while(ptr -> next != NULL) ptr = ptr -> next;

        ptr -> next = temp;
    }
    temp = NULL;
    free(temp);
}

void print_all(struct node **head)
{
    struct node *ptr = *head;
    if (ptr == NULL) return;
    while((ptr) != NULL)
    {
        printf("Job: %d\n", ptr -> pid);
        ptr = ptr -> next;
    }
}

void remove_node(int index, struct node **head)
{
    struct node* ptr;
    struct node* prv = *head;

    if (index == 1)
    {
        ptr = *head;
        if ((*head) -> next != NULL) 
        {
            *head = (*head) -> next;
        } else {
            *head = NULL;
        }
    
    } else {
        for (int i = 1; i < index - 1; i++)
        {
            prv = prv -> next;
        }
        if (prv -> next -> next == NULL)
        {
            prv -> next = NULL;
        } else {
            ptr = prv -> next;
            prv -> next = prv -> next -> next;
        }
    }
    free(ptr);
}

int main(void)
{
    struct node *head = NULL;
    add(1, &head);
    add(3, &head);
    add(5, &head);
    print_all(&head);
    remove_node(1, &head);
    remove_node(1, &head);
    remove_node(1, &head);
    free(head);
}