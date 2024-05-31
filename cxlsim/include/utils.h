#ifndef __CXL_UTILS_H
#define __CXL_UTILS_H

#include <string>
#include <fstream>
#include "flit.h"
#include "CXLInterface.h"
#include <cstdlib>

#ifndef NDEBUG
    #define CXL_ASSERT(condition) \
        do { \
            if (!(condition)) { \
                std::cerr << "Assertion failed: " << #condition << " in " << __FILE__ \
                          << " at line " << __LINE__ << std::endl; \
                CXL::customFailureHandler(); \
                std::abort(); \
            } \
        } while (0)
#else
    #define CXL_ASSERT(condition) ((void)0)
#endif

namespace CXL
{
    void CXLAssert(bool pred, std::string s, const char *parent_func = __builtin_FUNCTION());
    // void CXLAssert(bool pred, const char* parent_func = __builtin_FUNCTION());
    void customFailureHandler();
    enum round_robin_state
    {
        R,
        r,
        w,
        W
    };

    //! Logger class
    class CXLLog
    {
    public:
        std::ofstream eventlog;
        CXLLog(std::string filename);
        void CXLEventLog(std::string, uint i = -1, uint j = -1);
    };

    CXL_IF::ramulator_req message2ramulator_req(message m);
}
#endif