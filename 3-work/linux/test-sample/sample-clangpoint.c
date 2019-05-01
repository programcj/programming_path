#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union _CLangPointer {
    void* vptr;
    char* vstr;

#if __WORDSIZE == 32
    int vinteger;
    unsigned int vuint;
#elif __WORDSIZE == 64
    long vinteger;
    unsigned long vuint;
#endif

} CLangPointer;

struct test {
    int v1;
    int v[]; //为0
};

typedef void* CPoint; //个人觉得应该这样表示指针 CPoint

//指针即整数,它的值即地址
int main(int argc, char const* argv[])
{
    {
        const char* buff = "hello";
        void* ptr = NULL;
        CLangPointer dptr;

        ptr = buff;

        printf("ptr=%s\n", ptr); //ptr "hello"

        dptr.vptr = ptr;

        printf("dptr=%s\n", dptr.vptr); //dptr.vptr "hello"

        memcpy(&dptr, &ptr, sizeof(ptr));
        printf("dptr=%s\n", dptr.vptr); //dptr.vptr "hello"

        ptr = dptr.vptr;
        printf("dptr=%s\n", dptr.vptr); //dptr.vptr "hello"

        ptr = *(long*)&dptr; //gcc not compile (long)dptr
        printf("-ptr=%s\n", ptr); //dptr.vptr "hello"
    }

    {
        //             0   8     16
        char* a[] = { "1", "2", "3" };
        CPoint* ptr = a; //void **ptr;
        void* ptrv = a;

        printf("ptr[1]=[%p]%s\n", &ptr[1], ptr[1]); // ptr[1] = "2"

        printf("ptrv[8]=[%p]%s\n", (char *)(ptrv + 8), *(void **)(ptrv + 8));
    }
    {
        printf("sizeof=%d\n", sizeof(struct test));
    }
    return 0;
}
