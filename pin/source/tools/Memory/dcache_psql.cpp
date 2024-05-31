/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This file contains an ISA-portable cache simulator
 *  data cache hierarchies
 */

#include "pin.H"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdio>

#include "cache.H"
#include "pin_profile.H"
using std::cerr;
using std::endl;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "dcache.out", "specify dcache file name");
KNOB<BOOL> KnobTrackLoads(KNOB_MODE_WRITEONCE, "pintool", "tl", "0", "track individual loads -- increases profiling time");
KNOB<BOOL> KnobTrackStores(KNOB_MODE_WRITEONCE, "pintool", "ts", "0", "track individual stores -- increases profiling time");
KNOB<UINT32> KnobThresholdHit(KNOB_MODE_WRITEONCE, "pintool", "rh", "100", "only report memops with hit count above threshold");
KNOB<UINT32> KnobThresholdMiss(KNOB_MODE_WRITEONCE, "pintool", "rm", "100",
                               "only report memops with miss count above threshold");
KNOB<UINT32> KnobCacheSize(KNOB_MODE_WRITEONCE, "pintool", "c", "32", "cache size in kilobytes");
KNOB<UINT32> KnobLineSize(KNOB_MODE_WRITEONCE, "pintool", "b", "32", "cache block size in bytes");
KNOB<UINT32> KnobAssociativity(KNOB_MODE_WRITEONCE, "pintool", "a", "4", "cache associativity (1 for direct mapped)");

/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool represents a cache simulator.\n"
            "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

// wrap configuation constants into their own name space to avoid name clashes
namespace DL1
{
    const UINT32 max_sets = 128 * KILO;   // cacheSize / (lineSize * associativity);
    const UINT32 max_associativity = 256; // associativity;
    const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

    typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
} // namespace DL1

DL1::CACHE *dl1 = NULL;

typedef enum
{
    COUNTER_MISS = 0,
    COUNTER_HIT = 1,
    COUNTER_NUM
} COUNTER;

typedef COUNTER_ARRAY<UINT64, COUNTER_NUM> COUNTER_HIT_MISS;

// holds the counters with misses and hits
// conceptually this is an array indexed by instruction address
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> profile;

/* ===================================================================== */
// Stuff added by Sumanth

const CHAR *ROI_BEGIN = "__parsec_roi_begin";
const CHAR *ROI_END = "__parsec_roi_end";
const CHAR *BUF_BEGIN = "__parsec_buf_read_begin";
const CHAR *BUF_END = "__parsec_buf_read_end";
const CHAR *BUF_LOCAL_ALLOC = "__parsec_readbufcommon_0";
const CHAR *BUF_LOCAL_READ = "__parsec_readbufcommon_1";
const CHAR *BUF_SHARE_ALLOC = "__parsec_readbufcommon_2";
const CHAR *BUF_SHARE_READ = "__parsec_readbufcommon_3";
const CHAR *BUF_RELEASE = "__parsec_readbufcommon_4";
FILE *trace;
bool isROI = false;

INS global_ins;
UINT32 global_memOp;
// const CHAR * name;

// Print a memory read record
VOID RecordMemRead(VOID *ip, VOID *addr, CHAR *rtn)
{
    // Return if not in ROI
    if (!isROI)
    {
        return;
    }

    // Log memory access in CSV
    fprintf(trace, "%p,R,%p,%s\n", ip, addr, rtn);
}

// Print a memory write record
VOID RecordMemWrite(VOID *ip, VOID *addr, CHAR *rtn)
{
    // Return if not in ROI
    if (!isROI)
    {
        return;
    }

    // Log memory access in CSV
    fprintf(trace, "%p,W,%p,%s\n", ip, addr, rtn);
}

// Set ROI flag
VOID StartROI()
{
    isROI = true;
    fprintf(trace, "startROI\n");
}

// Set ROI flag
VOID StopROI()
{
    isROI = false;
    fprintf(trace, "endROI\n");
}

// Function that will be called before each "RTN", or "function/routine"
VOID Before(CHAR *name) // name is passed from RTN_InsertCall
{
    // if (strcmp(BUF_BEGIN, name) == 0 || strcmp(BUF_END, name) == 0)
    // if (!isROI)
    //     return;
    if (strcmp(ROI_BEGIN, name) == 0 ||
        strcmp(ROI_END, name) == 0 ||
        strcmp(BUF_BEGIN, name) == 0 ||
        strcmp(BUF_END, name) == 0 ||
        strcmp(BUF_LOCAL_ALLOC, name) == 0 ||
        strcmp(BUF_LOCAL_READ, name) == 0 ||
        strcmp(BUF_SHARE_ALLOC, name) == 0 ||
        strcmp(BUF_SHARE_READ, name) == 0 ||
        strcmp(BUF_RELEASE, name) == 0)
        fprintf(trace, "RTN:%s\n", name);
}

static VOID Routine(RTN rtn, VOID *)
{

    // cerr << "Entered Routine function\n";

    // Get routine name
    const CHAR *name = RTN_Name(rtn).c_str();
    // cerr << "B-1\n";
    // name = "invalid";
    // if(RTN_Valid(rtn))
    // {
    //     name = RTN_Name(rtn).c_str();
    // }
    // cerr << "B0\n";

    RTN_Open(rtn);
    // We pass 'name' as an argument to Before
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Before,
                   IARG_PTR, name,
                   IARG_END);
    RTN_Close(rtn);

    // If routine is invalid, just leave it, it wont have roi begin or end
    //  if(!RTN_Valid(rtn))
    //      return;

    // fprintf(trace,"Routine: %s\n",name);
    // if(RTN_Name(rtn).find(BUF_BEGIN) != std::string::npos || RTN_Name(rtn).find(BUF_END) != std::string::npos)
    //     fprintf(trace,"Routine: %s\n",name);
    if (RTN_Name(rtn).find(ROI_BEGIN) != std::string::npos)
    {
        // Start tracing after ROI begin exec
        // cerr << "B1\n";
        // fprintf(trace,"Routine: %s\n",name);
        RTN_Open(rtn);
        // cerr << "B2\n";
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StartROI, IARG_END);
        // cerr << "B3\n";
        RTN_Close(rtn);
        // cerr << "B4\n";
        // if (isROI) fprintf(trace,"true\n"); else fprintf(trace,"false\n");
        // cerr << "B5\n";
    }
    else if (RTN_Name(rtn).find(ROI_END) != std::string::npos)
    {
        // Stop tracing before ROI end exec
        // fprintf(trace,"Routine: %s\n",name);
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StopROI, IARG_END);
        RTN_Close(rtn);
        // if (isROI) fprintf(trace,"true\n"); else fprintf(trace,"false\n");
    }
}

/* ===================================================================== */

/* ===================================================================== */

VOID LoadMulti(ADDRINT addr, UINT32 size, UINT32 instId, const CHAR *name)
{
    // first level D-cache
    const BOOL dl1Hit = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD);

    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profile[instId][counter]++;
    // INS ins = global_ins;
    if (!dl1Hit)
    {
        // if (!isROI)
        // {
        //     return;
        // }

        // Log memory access in CSV
        // fprintf(trace,"%p,R,%p,%s\n", ins, addr, name);
        fprintf(trace, "R,0x%lx\n", addr);
        // cerr << name << "@@\n";
        // INS_InsertPredicatedCall(
        //         ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
        //         IARG_INST_PTR,
        //         IARG_MEMORYOP_EA, global_memOp,
        //         IARG_ADDRINT, name,
        //         IARG_END);
    }
}

/* ===================================================================== */

VOID StoreMulti(ADDRINT addr, UINT32 size, UINT32 instId, const CHAR *name)
{
    // first level D-cache
    const BOOL dl1Hit = dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE);

    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profile[instId][counter]++;
    // INS ins = global_ins;

    if (!dl1Hit)
    {
        // if (!isROI)
        // {
        //     return;
        // }

        // Log memory access in CSV
        // fprintf(trace,"%p,R,%p,%s\n", ins, addr, name);
        fprintf(trace, "W,0x%lx\n", addr);
        // cerr << name << "$$\n";
        // INS_InsertPredicatedCall(
        //             ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
        //             IARG_INST_PTR,
        //             IARG_MEMORYOP_EA, global_memOp,
        //             IARG_ADDRINT, name,
        //             IARG_END);
    }
}

/* ===================================================================== */

VOID LoadSingle(ADDRINT addr, UINT32 instId, const CHAR *name)
{
    // @todo we may access several cache lines for
    // first level D-cache
    const BOOL dl1Hit = dl1->AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD);

    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profile[instId][counter]++;
    if (!dl1Hit)
    {
        // if (!isROI)
        // {
        //     return;
        // }

        // Log memory access in CSV
        // fprintf(trace,"%p,R,%p,%s\n", ins, addr, name);
        fprintf(trace, "R,0x%lx\n", addr);
        // cerr << name << "$$\n";
        // INS_InsertPredicatedCall(
        //             ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
        //             IARG_INST_PTR,
        //             IARG_MEMORYOP_EA, global_memOp,
        //             IARG_ADDRINT, name,
        //             IARG_END);
    }
}
/* ===================================================================== */

VOID StoreSingle(ADDRINT addr, UINT32 instId, const CHAR *name)
{
    // @todo we may access several cache lines for
    // first level D-cache
    const BOOL dl1Hit = dl1->AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_STORE);

    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profile[instId][counter]++;
    if (!dl1Hit)
    {
        // if (!isROI)
        // {
        //     return;
        // }

        // Log memory access in CSV
        // fprintf(trace,"%p,R,%p,%s\n", ins, addr, name);
        fprintf(trace, "R,0x%lx\n", addr);
        // cerr << name << "$$\n";
        // INS_InsertPredicatedCall(
        //             ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
        //             IARG_INST_PTR,
        //             IARG_MEMORYOP_EA, global_memOp,
        //             IARG_ADDRINT, name,
        //             IARG_END);
    }
}

/* ===================================================================== */

VOID LoadMultiFast(ADDRINT addr, UINT32 size) { dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_LOAD); }

/* ===================================================================== */

VOID StoreMultiFast(ADDRINT addr, UINT32 size) { dl1->Access(addr, size, CACHE_BASE::ACCESS_TYPE_STORE); }

/* ===================================================================== */

VOID LoadSingleFast(ADDRINT addr) { dl1->AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD); }

/* ===================================================================== */

VOID StoreSingleFast(ADDRINT addr) { dl1->AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_STORE); }

/* ===================================================================== */

VOID Instruction(INS ins, void *v)
{
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    const CHAR *name;
    if (RTN_Valid(INS_Rtn(ins)))
    {
        name = RTN_Name(INS_Rtn(ins)).c_str();
    }
    else
    {
        name = "invalid";
    }

    global_ins = ins;

    // Instrument each memory operand. If the operand is both read and written
    // it will be processed twice.
    // Iterating over memory operands ensures that instructions on IA-32 with
    // two read operands (such as SCAS and CMPS) are correctly handled.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        const UINT32 size = INS_MemoryOperandSize(ins, memOp);
        const BOOL single = (size <= 4);

        global_memOp = memOp;

        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            if (KnobTrackLoads)
            {
                // map sparse INS addresses to dense IDs
                const ADDRINT iaddr = INS_Address(ins);
                const UINT32 instId = profile.Map(iaddr);
                if (single)
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)LoadSingle, IARG_MEMORYOP_EA, memOp, IARG_UINT32,
                                             instId, IARG_ADDRINT, name, IARG_END);
                }
                else
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)LoadMulti, IARG_MEMORYOP_EA, memOp, IARG_UINT32, size,
                                             IARG_UINT32, instId, IARG_ADDRINT, name, IARG_END);
                }
            }
            else
            {
                if (single)
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)LoadSingleFast, IARG_MEMORYOP_EA, memOp, IARG_END);
                }
                else
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)LoadMultiFast, IARG_MEMORYOP_EA, memOp, IARG_UINT32,
                                             size, IARG_END);
                }
            }
        }

        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            if (KnobTrackStores)
            {
                const ADDRINT iaddr = INS_Address(ins);
                const UINT32 instId = profile.Map(iaddr);

                if (single)
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)StoreSingle, IARG_MEMORYOP_EA, memOp, IARG_UINT32,
                                             instId, IARG_ADDRINT, name, IARG_END);
                }
                else
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)StoreMulti, IARG_MEMORYOP_EA, memOp, IARG_UINT32, size,
                                             IARG_UINT32, instId, IARG_ADDRINT, name, IARG_END);
                }
            }
            else
            {
                if (single)
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)StoreSingleFast, IARG_MEMORYOP_EA, memOp, IARG_END);
                }
                else
                {
                    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)StoreMultiFast, IARG_MEMORYOP_EA, memOp, IARG_UINT32,
                                             size, IARG_END);
                }
            }
        }
    }
}

/* ===================================================================== */

VOID Fini(int code, VOID *v)
{
    std::ofstream out(KnobOutputFile.Value().c_str());

    // print D-cache profile
    // @todo what does this print

    out << "PIN:MEMLATENCIES 1.0. 0x0\n";

    out << "#\n"
           "# DCACHE stats\n"
           "#\n";

    out << dl1->StatsLong("# ", CACHE_BASE::CACHE_TYPE_DCACHE);

    if (KnobTrackLoads || KnobTrackStores)
    {
        out << "#\n"
               "# LOAD stats\n"
               "#\n";

        out << profile.StringLong();
    }
    out.close();
}

/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    dl1 = new DL1::CACHE("L1 Data Cache", KnobCacheSize.Value() * KILO, KnobLineSize.Value(), KnobAssociativity.Value());

    profile.SetKeyName("iaddr          ");
    profile.SetCounterName("dcache:miss        dcache:hit");

    COUNTER_HIT_MISS threshold;

    threshold[COUNTER_HIT] = KnobThresholdHit.Value();
    threshold[COUNTER_MISS] = KnobThresholdMiss.Value();

    profile.SetThreshold(threshold);

    RTN_AddInstrumentFunction(Routine, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Open trace file and write header
    char filename[100];
    sprintf(filename,"roitrace_%d.csv",PIN_GetPid());
    trace = fopen(filename, "w");
    fprintf(trace, "pc,rw,addr,rtn\n");

    // Never returns

    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
