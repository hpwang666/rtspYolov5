
#ifndef _RENDERER_H
#define _RENDERER_H

#include "nvdec.h"
void report_metadata(context_t *ctx, v4l2_ctrl_videodec_outputbuf_metadata *metadata);
void query_and_set_capture(context_t * ctx);


#endif
