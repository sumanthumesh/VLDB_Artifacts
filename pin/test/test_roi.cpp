#include <iostream>
#include <cstring>


const char *__parsec_roi_begin(const char *s, int *beg, int *end);
const char *__parsec_roi_end(const char *s, int *beg, int *end);

const char *__parsec_roi_begin(const char *s, int *beg, int *end)
{
    const char *colon = strrchr(s, ':');
    if (colon == NULL) {
        *beg = 0; *end = 0x7fffffff;
        return s + strlen(s);
    }
    return NULL;
}

const char *__parsec_roi_end(const char *s, int *beg, int *end)
{
    const char *colon = strrchr(s, ':');
    if (colon == NULL) {
        *beg = 0; *end = 0x77ffffff;
        return s + strlen(s);
    }
    return NULL;
}


void func1()
{
    std::cout<<"Function 1" << std::endl;
}

void func2()
{
    std::cout<<"Function 2\n";
}

void func3()
{
    std::cout<<"Function 3\n";
}

void func4()
{
    std::cout<<"Function 4\n";
}

// function to check if a given number is prime
bool isPrime(int n)
{
    // since 0 and 1 is not prime return false.
    if (n == 1 || n == 0)
        return false;
 
    // Run a loop from 2 to n-1
    for (int i = 2; i < n; i++) {
        // if the number is divisible by i, then n is not a
        // prime number.
        if (n % i == 0)
            return false;
    }
    // otherwise, n is prime number.
    return true;
}

int main()
{
    const char *roi_q;
    int roi_i, roi_j;
    char roi_s[20] = "chr22:0-5";

    func1();
    roi_q = __parsec_roi_begin(roi_s, &roi_i, &roi_j);
    func2();
    
    int N = 100;
 
    // check for every number from 1 to N
    for (int i = 1; i <= N; i++) {
        // check if current number is prime
        if (isPrime(i))
            std::cout << i << " ";
    }

    std::cout<<"\n";

    func3();
    // func4();
    roi_q = __parsec_roi_end(roi_s, &roi_i, &roi_j);
    func4();
    return 0;
}