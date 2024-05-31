#ifndef __CXL_BUF_H
#define __CXL_BUF_H

#include <vector>
#include "flit.h"
#include "utils.h"
#include "CXLParams.h"
#include <iostream>
#include <deque>

namespace CXL
{

    extern int64_t curr_tick;
    extern CXLParams params;

    /*! Buffer between CXL port and tx/rx bus. Responsible for modeling latency*/
    template <typename T>
    class CXLBuf
    {
    private:
        int size_;
        std::deque<T> q_;

    public:
        CXLBuf();
        CXLBuf(int size);
        bool is_buf_full();
        bool is_buf_empty();
        int buf_occupancy();
        void dequeue();
        //T erase(typename std::vector<T>::iterator it);
        void enqueue(T f);
        const T get_head();
        //T get_nth_ele(int n);
        int size();
        bool latency_check(int64_t);
        bool latency_check_flit(int64_t delay);
        //typename std::vector<T>::iterator get_it();
    };

    class CXLRETRYBuf : public CXLBuf<flit>
    {
        // public:
        //     int find_flit(int flit_id);
        //     std::vector<flit> resend_flits(int flit_id);
    };

    template <typename T>
    CXLBuf<T>::CXLBuf() : size_(0) {}

    template <typename T>
    int CXLBuf<T>::size()
    {
        return size_;
    }

    template <typename T>
    CXLBuf<T>::CXLBuf(int size)
    {
        size_ = size;
        //q_.reserve(s ize);
    }

    template <typename T>
    bool CXLBuf<T>::is_buf_full()
    {
        return q_.size() == size_;
    }

    template <typename T>
    bool CXLBuf<T>::is_buf_empty()
    {
        return q_.empty();
    }

    template <typename T>
    int CXLBuf<T>::buf_occupancy()
    {
        return q_.size();
    }
    template <typename T>
    void CXLBuf<T>::dequeue()
    {
        CXL_ASSERT(!is_buf_empty() && "Dequeing from already empty buffer!");
        q_.pop_front();
    }

    // template <typename T>
    // T CXLBuf<T>::erase(typename std::vector<T>::iterator it)
    // {
    //     T f;
    //     CXL_ASSERT(!is_buf_empty() && "Erasing from already empty buffer!");
    //     f = *it;
    //     vec_.erase(it);
    //     return f;
    // }

    template <typename T>
    void CXLBuf<T>::enqueue(T f)
    {


        CXL_ASSERT(!is_buf_full() && "Adding to already full buffer!");
        q_.push_back(f);
    }

    template <typename T>
    const T CXLBuf<T>::get_head()
    {
        CXL_ASSERT(!is_buf_empty() && "Fetching from empty buffer!");
        return q_.front();
    }

    // Returns nth element from head
    // template <typename T>
    // T CXLBuf<T>::get_nth_ele(int n)
    // {
    //     CXL_ASSERT(n < buf_occupancy() && "Fetching from empty buffer!");
    //     return vec_[n];
    // }

    // This is for the CXL tx side packing
    // NOTE: I don't use this anywhere, seems like a good idea to remove this
    template <typename message>
    bool CXLBuf<message>::latency_check(int64_t delay)
    {
        if (curr_tick - get_head().time.tick_created >= delay)
            return true;
        return false;
    }

    // This is for the CXL tx side packing, but checking for a flit
    // NOTE: This is only used in the switch, please do not use it anywhere else
    template <typename flit>
    bool CXLBuf<flit>::latency_check_flit(int64_t delay)
    {
        if(curr_tick - this->get_head().time.time_of_receipt >= delay)
            return true;
        return false;
    }

    //Returns iteratior pointing to head of the queue
    // template <typename T>
    // typename std::vector<T>::iterator CXLBuf<T>::get_it()
    // {
    //     CXL_ASSERT(!is_buf_empty() && "Fetching from empty buffer!");
    //     return vec_.begin();
    // }
}
#endif