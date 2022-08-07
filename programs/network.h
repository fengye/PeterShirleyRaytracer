#ifndef _H_NETWORK_
#define _H_NETWORK_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mutex.h>
#include <sys/cond.h>

#include "spu_shared.h"

#ifdef _PPU_

extern void get_primary_ip(char* buffer, size_t buflen);

enum NETWORK_CODE
{
	NC_NOP = 0,
	NC_NODEOFFER,
	NC_JOBREQUEST,
	NC_JOBRESULT,
	NC_SHUTDOWN
};

typedef struct _node_result_t
{
    size_t pixel_data_size;
    pixel_data_t* pixel_data;
} node_result_t;

typedef struct _node_sync_t
{
    sys_cond_t cond_var;
    sys_mutex_t mutex;
    sys_mutex_t util_mutex;
} node_sync_t;

extern void node_sync_init(node_sync_t* node_sync);
extern void node_sync_destroy(node_sync_t* node_sync);
extern bool node_sync_wait(node_sync_t* node_sync, u64 timeout);
extern void node_sync_signal(node_sync_t* node_sync);

// Master
extern bool start_listening_server();
extern bool shutdown_listening_server();

/* caller to free serialized_data */
extern bool send_shutdown_request(int conn_fd, bool shutdown);
extern bool recv_processor_offer(int conn_fd, uint8_t* out_ppu_count, uint8_t* out_spu_count);
extern bool send_job_request(int conn_fd, uint32_t job_id, uint8_t* serialized_data, size_t data_size, int img_width, int img_height, int start_y, int end_y);
/* caller to free out_pixel_data */
extern bool recv_job_result(int conn_fd, uint32_t* out_job_id, uint8_t** out_pixel_data, size_t* out_pixel_data_size);
// ~Master

// Slave
extern bool connect_master(const char* master_addr, int* out_conn_fd);
extern bool disconnect_master();

extern bool recv_shutdown_request(int conn_fd, bool* is_shutdown);
extern bool send_processor_offer(int conn_fd, uint8_t ppu_count, uint8_t spu_count);
/* caller to free out_serialized_data */
extern bool recv_job_request(int conn_fd, uint32_t* out_job_id, uint8_t** out_serialized_data, size_t* out_data_size, int* out_img_width, int* out_img_height, int* out_start_y, int* out_end_y);
extern bool send_job_result(int conn_fd, uint32_t job_id, uint8_t* pixel_data, size_t pixel_data_size);
// ~Slave

// Common
extern bool send_request_header(int conn_fd, uint8_t request_code);
extern bool recv_request_header(int conn_fd, uint8_t* out_request_code);
// ~Common

#endif

#endif //_H_NETWORK_