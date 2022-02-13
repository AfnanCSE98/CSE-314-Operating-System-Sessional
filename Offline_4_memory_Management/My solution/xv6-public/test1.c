#include "types.h"
#include "stat.h"
#include "user.h"

void try1()
{
    int sz = 4096 * 17;
    char *a = (char *)malloc(sz);

    for (int i = 0; i < sz - 1; i++)
    {
        char ch = (i % 26) + 'a';
        a[i] = ch;
    }
    sleep(250);
    int flag = 1;

    for (int i = 0; i < sz - 1; i++)
    {
        if (a[i] - 'a' != (i % 26))
        {
            printf(2, "Index i = %d , Failed\n", i);
            flag = 0;
            break;
        }
    }
    if (!flag)
    {
        printf(2, "Failed!!!\n");
    }
    else
    {
        printf(2, "Success!!!!!!\n");
    }
    free((void *)a);
}

int main(int argc, char *argv[])
{
    try1();
    exit();
}