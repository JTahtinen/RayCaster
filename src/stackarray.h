#pragma once

template <typename T, int cap>
struct StackArray
{
    T data[cap];
    size_t size;
    size_t capacity {cap};

    template <typename T>
    inline bool push(const T& val)
    {
        if (this->size >= this->capacity)
        {
            return false;
        }
        this->data[this->size++] = val;
        return true;
    }

    inline bool pop()
    {
        if (this->size == 0)
        {
            return false;
        }
        --this->size;
        return true;
    }

    inline void clear()
    {
        this->size = 0;
    }

    inline bool empty() const
    {
        bool result = this->size == 0;
        return result;
    }

    inline T& operator[](size_t index)
    {
        T& result = this->data[index];
        return result;
    }


    inline const T& operator[](size_t index) const
    {
        const T& result = this->data[index];
        return result;
    }
};
