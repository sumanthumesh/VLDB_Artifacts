// Program to test out our cache eviction scheme
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define PAGE_SIZE 8192 // In bytes

typedef uint16_t inttype;

void random_assign(inttype *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = rand() % 100;
    }
}

void __parsec_readbufcommon_0()
{
    printf("Breakpoint\n");
}

void buffer_allocate_detected(uint64_t addr)
{
    printf("Evict function called at address 0x%016lx\n", addr);
}

int main()
{
    // Allocate a page of memory
    inttype *page = (inttype*) malloc(PAGE_SIZE);
    __parsec_readbufcommon_0();
    random_assign(page, PAGE_SIZE);
    __parsec_readbufcommon_0();
    // Run a for loop to sum up all the values
    int sum = 0;
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        sum += page[i];
    }
    __parsec_readbufcommon_0();
    printf("Sum: %d\n",sum);

    // Call our cache line evict function
    buffer_allocate_detected((uint64_t)page);
    __parsec_readbufcommon_0();
    // Run a for loop to find the max value
    inttype max = page[0];
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        if (page[i] < max)
            continue;
        max = page[i];
    }
    __parsec_readbufcommon_0();

    printf("Max: %d\n",max);

    return 0;
}