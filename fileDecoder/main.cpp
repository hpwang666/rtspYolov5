
#include <linux/videodev2.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "video_decode.h"
#include <iostream>
#include <fstream>

using namespace std;

 int decode_proc(context_t& ctx, int argc, char *argv[]);


int
main(int argc, char *argv[])
{
    /* create decoder context. */
    context_t ctx;
    int ret = 0;
    /* save decode iterator number */
    int iterator_num = 0;

    do
    {
        /* Invoke video decode function. */
        ret = decode_proc(ctx, argc, argv);
        iterator_num++;
    } while((ctx.stress_test != iterator_num) && ret == 0);

    /* Report application run status on exit. */
    if (ret)
    {
        cout << "App run failed" << endl;
    }
    else
    {
        cout << "App run was successful" << endl;
    }

    return ret;
}
