# How to use Pintools
* Clone this repo
* The pintool examples are located in `<PIN_DIR>/source/tools/ManualExamples`
* To compile the Manual examples, goto the dir  
`cd source/tools/ManualExamples`
* To compile for 64-bit x86 processors use  
`make all TARGET=intel64`
* Make will generate .so files for each of the pintools in the ManulaExamples directory an place them in `ManualExamples/obj-intel64`.
* To run the pintool by launching an executable use the following syntax  
`<PIN_DIR>/pin -t <PATH_TO_PINTOOL_SO_FILE> -- <EXECUTABLE>`  
You can find a test program in the `<PIN_DIR>/test`. First compile the program using   
`cd test/`  
`make`  
You can generate the address trace by using the following command
`../pin -t ../source/tools/ManualExamples/obj-intel64/roitrace.so -- ./test.out`  
This will generate a roitrace.csv file.

# How to specify the region of interest in your source code
Refer to the example in roi_test.cpp. You will need to define the following two functions in code
```c++
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

```

Add `roi_q = __parsec_roi_begin(roi_s, &roi_i, &roi_j);` before the line where tracing should begin and `roi_q = __parsec_roi_end(roi_s, &roi_i, &roi_j);` after tracing should end.

You will need to declare these variables for this to work
```c++
const char *roi_q;
int roi_i, roi_j;
char roi_s[20] = "chr22:0-5";
```
The reason we need such complicated function definitions for begin and end is to avoid the compiler simply discarding the functions during aggressive optimizations.

# How to attach PIN to an already running process like postgres
The only change needed is in the invoking command. Use the following  
`<PIN_DIR>/pin -t <PATH_TO_PINTOOL_SO_FILE> -pid <PID>`

`/home/jerry/pin/pin -pause_tool 20 -t /home/jerry/pin/source/tools/Memory/obj-intel64/dcache_roi.so -ts -tl -- /home/jerry/pin/test/test.out`