#include <stdio.h>
#include "pin.H"
#include <string>
#include <cstring>

const CHAR *ROI_BEGIN = "__parsec_roi_begin";
const CHAR *ROI_END = "__parsec_roi_end";
const CHAR *BUF_BEGIN = "__parsec_buf_read_begin";
const CHAR *BUF_END = "__parsec_buf_read_end";

FILE *trace;
bool are_we_in_ROI = false;
bool mainCalled = false;

// Print a memory read record
VOID RecordMemRead(VOID *ip, VOID *addr, const CHAR *rtn)
{
    // Return if not in ROI
    if (!are_we_in_ROI)
    {
        return;
    }
    // Get routine name
    // const CHAR * name = RTN_Name(rtn).c_str();
    // if (strcmp(rtn, BUF_BEGIN) == 0 || strcmp(rtn, BUF_END) == 0)
    //     fprintf(trace, "Routine: %s\n", rtn);
    // Log memory access in CSV
    fprintf(trace, "R,%p\n", addr);
}

// Print a memory write record
VOID RecordMemWrite(VOID *ip, VOID *addr, CHAR *rtn)
{
    // Return if not in ROI
    if (!are_we_in_ROI)
    {
        return;
    }
    // if (strcmp(rtn, BUF_BEGIN) == 0 || strcmp(rtn, BUF_END) == 0)
    //     fprintf(trace, "Routine: %s\n", rtn);
    // Log memory access in CSV
    fprintf(trace, "W,%p\n", addr);
}

// Set ROI flag
VOID StartROI()
{
    are_we_in_ROI = true;
    fprintf(trace, "startROI\n");
}

// Set ROI flag
VOID StopROI()
{
    are_we_in_ROI = false;
    fprintf(trace, "endROI\n");
}
// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        // Get routine name if valid
        const CHAR *name = "invalid";
        if (RTN_Valid(INS_Rtn(ins)))
        {
            name = RTN_Name(INS_Rtn(ins)).c_str();
        }

        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_ADDRINT, name,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_ADDRINT, name,
                IARG_END);
        }
    }
}

// Pin calls this function every time a new rtn is executed
// VOID Routine(RTN rtn, VOID *v)
// {
//     // Get routine name
//     const CHAR * name = RTN_Name(rtn).c_str();

//     fprintf(trace,"Routine, %s\n",name);

//     if(strcmp(name,ROI_BEGIN) == 0) {
//         // Start tracing after ROI begin exec
//         RTN_Open(rtn);
//         RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StartROI, IARG_END);
//         RTN_Close(rtn);
//     } else if (strcmp(name,ROI_END) == 0) {
//         // Stop tracing before ROI end exec
//         RTN_Open(rtn);
//         RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)StopROI, IARG_END);
//         RTN_Close(rtn);
//     }
// }

// Function that will be called before each "RTN", or "function/routine"
VOID Before(CHAR *name) // name is passed from RTN_InsertCall
{
    // if (strcmp(BUF_BEGIN, name) == 0 || strcmp(BUF_END, name) == 0)
    if(!are_we_in_ROI)
        return;
    fprintf(trace,"RTN:%s\n", name);
}

static VOID Routine(RTN rtn, VOID *)
{

    // Get routine name
    const CHAR *name = RTN_Name(rtn).c_str();

    // if(RTN_Name(rtn).find(BUF_BEGIN) != std::string::npos || RTN_Name(rtn).find(BUF_END) != std::string::npos)
    //     fprintf(trace,"Routine: %s\n",name);

    RTN_Open(rtn);
    // We pass 'name' as an argument to Before
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Before,
                   IARG_PTR, name,
                   IARG_END);
    RTN_Close(rtn);

    // fprintf(trace,"Routine: %s\n",name);
    if (RTN_Name(rtn).find(ROI_BEGIN) != std::string::npos)
    {
        // Start tracing after ROI begin exec
        fprintf(trace, "Routine: %s\n", name);
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StartROI, IARG_END);
        RTN_Close(rtn);
        if (are_we_in_ROI)
            fprintf(trace, "true\n");
        else
            fprintf(trace, "false\n");
    }
    else if (RTN_Name(rtn).find(ROI_END) != std::string::npos)
    {
        // Stop tracing before ROI end exec
        fprintf(trace, "Routine: %s\n", name);
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StopROI, IARG_END);
        RTN_Close(rtn);
        if (are_we_in_ROI)
            fprintf(trace, "true\n");
        else
            fprintf(trace, "false\n");
    }
}

// Pin calls this function at the end
VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of memory addresses\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // Usage
    if (PIN_Init(argc, argv))
        return Usage();

    // Open trace file and write header
    trace = fopen("roitrace.csv", "w");
    fprintf(trace, "pc,rw,addr,rtn\n");

    // Add instrument functions
    RTN_AddInstrumentFunction(Routine, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}