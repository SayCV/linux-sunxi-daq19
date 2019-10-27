
#ifndef __CAR_REVERSE_MISC_H__
#define __CAR_REVERSE_MISC_H__

#include "car_reverse.h"

struct buffer_pool *
		alloc_buffer_pool(struct device *dev, int depth, int buf_size);
void free_buffer_pool(struct device *dev, struct buffer_pool *bp);
void rest_buffer_pool(struct device *dev, struct buffer_pool *bp);
void dump_buffer_pool(struct device *dev, struct buffer_pool *bp);

int preview_output_start(struct preview_params *params);
int preview_output_stop(void);
void preview_update(struct buffer_node *frame);

int video_source_connect(struct preview_params *params);
int video_source_disconnect(struct preview_params *params);
//int video_source_streamon(void);
int video_source_streamon(int input_sync);
//int video_source_streamoff(void);
int video_source_streamoff(int input_sync);


//struct buffer_node *video_source_dequeue_buffer(void);
//void video_source_queue_buffer(struct buffer_node *node);

struct buffer_node *vfe_video_source_dequeue_buffer(void);
struct buffer_node *tvd_video_source_dequeue_buffer(void);

void video_source_queue_buffer(struct buffer_node *node,int input_type);

//void vfe_video_source_queue_buffer(struct buffer_node *node);
//void tvd_video_source_queue_buffer(struct buffer_node *node);



#endif
