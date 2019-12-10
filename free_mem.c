#include "stdlib.h"
#include "string.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Current bp: %8x\n", sbrk(0));
    printf("Allocation\n");
    int* a1 = (int*)malloc(sizeof(int) * 10000);
    printf("Address of a1: %8x\n", a1);
    printf("Current bp: %8x\n", sbrk(0));
    int* a3 = (int*)malloc(sizeof(int) * 10);
    printf("Address of a3: %8x\n", a3);
    printf("Current bp: %8x\n", sbrk(0));
    free(a1);
    printf("Deallocation\n");
    printf("Current bp: %8x\n", sbrk(0));
    int* a2 = (int*)malloc(sizeof(int) * 10);
    printf("Address of a2: %8x\n", a2);
    printf("Current bp: %8x\n", sbrk(0));
    free(a2);
    printf("Deallocation\n");
    printf("Current bp: %8x\n", sbrk(0));
    int* a4 = (int*)malloc(sizeof(int) * 100);
    printf("Address of a2: %8x\n", a4);
    return 0;
}