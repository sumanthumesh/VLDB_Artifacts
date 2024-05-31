#include <iostream>
#include <thread>
#include <vector>
#include <cstdlib>

const int arraySize = 1024*1024;  // 1MB

void threadFunction(int threadId, int* data) {
    // Access the array in the thread
    for (int i = 0; i < arraySize; ++i) {
        data[i] = threadId;  // Write the thread ID to the array
    }
}

int main() {
    // Create two arrays of size 1MB
    // int array1[arraySize];
    // int array2[arraySize];


    int* array1 = new int[arraySize];
    int* array2 = new int[arraySize];

    std::cout<<"1: "<<array1<<" to "<<array1+arraySize<<"\n";
    std::cout<<"2: "<<array2<<" to "<<array2+arraySize<<"\n";

    // std::cout<<"SIZE:"<<sizeof(*array1)<<"\n";

    // Create two threads, each operating on a separate array
    std::thread thread1(threadFunction, 1, array1);
    std::thread thread2(threadFunction, 2, array2);

    // Wait for threads to finish
    thread1.join();
    thread2.join();

    // Print a few elements from each array for verification
    std::cout << "Array 1: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << array1[i] << " ";
    }
    std::cout << "\n";

    std::cout << "Array 2: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << array2[i] << " ";
    }
    std::cout << "\n";

    return 0;
}
