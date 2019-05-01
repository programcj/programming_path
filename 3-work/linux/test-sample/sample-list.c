#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "../public_tools/list_queue.h"

struct test
{
    struct list_head list;
    int v;
};

int main(int argc, char **argv)
{
    struct list_queue list;
    struct test *v;
    int i = 0;

    list_queue_init(&list);

    for (i = 0; i < 100; i++)
    {
        v = malloc(sizeof(struct test));
        v->v = i;
        list_queue_append_tail(&list, &v->list);
    }

    {
        struct list_head *item = NULL;
        int ret = 0;

        int value = 0;
        sem_getvalue(&list.sem, &value);
        printf("sem value=%d\n", value);

        while (1)
        {
            ret = list_queue_wait(&list, 100);
            //ret=list_queue_timedwait(&list, 1);
            printf("ret=%d  ", ret);
            fflush(stdout);
            if (ret != 0)
                break;
        }

        sem_getvalue(&list.sem, &value);
        printf("sem value=%d\n", value);
        printf("errno=%d \n", errno);
        printf("errs=%s\n", strerror(errno));

        while ((item = list_queue_pop(&list)) != NULL)
        {
            v = list_entry(item, struct test, list);
            printf("v=%d ", v->v);
            free(v);
        }
    }

    list_queue_release(&list);

    return 0;
}