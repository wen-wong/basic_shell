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
    struct node* temp = (struct node*) malloc(sizeof(node));
    temp -> pid = pid;
    if (*head == NULL)
    {
        *head = temp;
        (*head) -> next = NULL;
    } else {
        struct node* ptr = *head;

        while(ptr -> next != NULL) ptr = ptr -> next;

        ptr -> next = temp;
        temp -> next = NULL;
    }
}

void print_all(struct node **head)
{
    struct node* ptr = *head;
    while(ptr != NULL)
    {
        printf("Job: %d\n", ptr -> pid);
        ptr = ptr -> next;
    }
}

int main(void)
{
    struct node *head;
    add(1, &head);
    add(3, &head);
    add(5, &head);
    print_all(&head);
}