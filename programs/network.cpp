#include <cstring>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/net.h>
#include <net/netdb.h>
#include <sys/thread.h>
#include <sys/mutex.h>
#include <sys/cond.h>

#include "debug.h"
#include "config.h"
#include "network.h"
#include "node_job.h"

static int main_socket_fd = -1;
static int listen_socket_fd = -1;
static sys_ppu_thread_t listening_serv_tid = 0;
static bool listening_serv_run = false;
extern node_sync_t g_slave_on_master_sync;

void get_primary_ip(char* buffer, size_t buflen) 
{
    // assert(buflen >= 16);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    // assert(sock != -1);

    const char* kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    int err = connect(sock, (const struct sockaddr*) &serv, sizeof(serv));
    // assert(err != -1);

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);
    // assert(err != -1);
    (void)err;

    const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
    // assert(p);
    (void)p;

    shutdown(sock, SHUT_RDWR);
}

typedef struct _node_params_t
{
    sys_ppu_thread_t tid; // thread id
    int socket_fd; // socket file descriptor
    char ip_addr[24]; // ip address for debugging
    node_sync_t kickoff_cond_var;
} node_params_t;

static std::vector<node_params_t*> s_slave_nodes;
std::map<uint32_t, node_result_t> g_node_results;
node_sync_t g_node_sync;
extern bool g_master_quit;

// This should read as an opposite to slave_main()
static void node_serv_func(void* args)
{
    node_params_t* param = (node_params_t*)args;

    debug_printf("Node serv socket(START): %d -> %s\n", param->socket_fd, param->ip_addr);

    // fill in the thread id
    sysThreadGetId(&param->tid);

    // wait for kicking off
    node_sync_wait(&param->kickoff_cond_var, 0);

    while(true)
    {
        debug_printf("Node serv socket(LOOP): %d -> %s\n", param->socket_fd, param->ip_addr);

        // tell slave to shutdown or keep running
        bool is_shutdown = g_master_quit; //!require_more_node_jobs();
        debug_printf("Sending %s request\n", is_shutdown ? "SHUTDOWN" : "NOP");
        if (!send_shutdown_request(param->socket_fd, is_shutdown))
            continue;

        if (is_shutdown)
            break;

        // read slave's offer
        uint8_t ppu_count, spu_count;
        debug_printf("Receiving offer from %d\n", param->socket_fd);
        if (!recv_processor_offer(param->socket_fd, &ppu_count, &spu_count))
        {
            debug_printf("Error receiving slave's offer from socket %d\n", param->socket_fd);
            continue;
        }

        debug_printf("Offered: (%d, %d)\n", ppu_count, spu_count);

        // allocate job id
        uint32_t job_id;
        uint8_t* serialized_data;
        size_t serialized_data_size;
        int img_width, img_height;
        int start_y, end_y;
        int32_t job_count;
        debug_printf("Allocating job for %d...\n", param->socket_fd);
        if (!allocate_node_job(ppu_count, spu_count, &job_id, &serialized_data, &serialized_data_size, &img_width, &img_height, &start_y, &end_y))
        {
            debug_printf("No more jobs for current frame!\n");
            // no more jobs to be done, start over node serv proc
            job_count = 0;
        }
        else{
            debug_printf("Allocating job(%d) for %d done\n", job_id, param->socket_fd);
            job_count = ppu_count + spu_count;
        }

        if (!send_job_count(param->socket_fd, job_count))
        {
            debug_printf("Error sending job count to %d\n", param->socket_fd);
            break;
        }

        if (job_count > 0)
        {
            // If job is valid, wait for result
            debug_printf("Job id %d. Scanline Y from %d - %d\n", job_id, start_y, end_y);

            // send job request
            debug_printf("Sending job(%d) request to %d\n", job_id, param->socket_fd);
            if (!send_job_request(param->socket_fd, job_id, serialized_data, serialized_data_size, img_width, img_height, start_y, end_y))
            {
                free(serialized_data);
                debug_printf("Error sending job to slave %d\n", param->socket_fd);
                continue;
            }
            else
            {
                debug_printf("Sending job(%d) request to %d done\n", job_id, param->socket_fd);
                free(serialized_data);    
            }

            sysThreadYield();

            // wait for the result from slave
            node_result_t result;
            uint32_t job_id_returned;
            debug_printf("Receiving job(%d) result from %d...\n", job_id, param->socket_fd);
            if (recv_job_result(param->socket_fd, &job_id_returned, (uint8_t**)(&result.pixel_data), &result.pixel_data_size) && job_id == job_id_returned)
            {
                debug_printf("Receiving job(%d) result: %p(%lld) from %d done!\n", job_id, result.pixel_data, result.pixel_data_size, param->socket_fd);
                debug_printf("Notifying update thread.\n");

                // signal conditional variable
                sysMutexLock(g_node_sync.mutex, 0);
                g_node_results.insert(std::make_pair(job_id, result));
                // put the data into storage before signaling
                sysCondSignal(g_node_sync.cond_var);
                sysMutexUnlock(g_node_sync.mutex);
            }
            else
            {
                debug_printf("Error!\n");
            }
        }
        else
        {
            // If no job needed for this frame, continue loop

            sysThreadYield();
            continue;
        }
    }

    sysThreadExit(0);
}

static void listening_serv_func(void* args)
{
    sys_ppu_thread_t id;
    sys_ppu_thread_stack_t stackinfo;

    sysThreadGetId(&id);
    sysThreadGetStackInformation(&stackinfo);

    debug_printf("Listening server thread\nAddr: %p, Stack size: %d\n",stackinfo.addr,stackinfo.size);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Signal the slave-on-master routine to go
    node_sync_signal(&g_slave_on_master_sync);

    while(listening_serv_run)
    {
        // wait for incoming connection
        debug_printf("Listening for connection... %d\n", listen_socket_fd);
        int conn_fd = accept(listen_socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        if (conn_fd == -1)
        {
            debug_printf("Error acccepting socket! Exiting...\n");
            break;
        }

        debug_printf("Accepted incoming socket %d\n", conn_fd);

        // store slave's connection file descriptor
        node_params_t* node_param = (node_params_t*)malloc(sizeof(node_params_t));
        // find out slave's ip in human readable format
        struct sockaddr_in name;
        socklen_t namelen = sizeof(name);
        if (getpeername(conn_fd, (struct sockaddr*) &name, &namelen) < 0)
        {
            debug_printf("Error getting the name of socket %d\n", conn_fd);
            continue;
        }
        if (!inet_ntop(AF_INET, &name.sin_addr, node_param->ip_addr, 24))
        {
            debug_printf("Error converting ip address of socket %d\n", conn_fd);
            continue;
        }
        debug_printf("Accepted from IP: %s\n", node_param->ip_addr);
        node_param->socket_fd = conn_fd;
        // init kickoff cond var
        node_sync_init(&node_param->kickoff_cond_var);

        s_slave_nodes.push_back(node_param);
        

        // spawn a thread sending job request and waiting for job result, when finish signaling the condition and get notified on main thread
        static int thread_count = 0;
        char thread_name[] = "node_serv_XXXXXX";
        snprintf(thread_name, strlen(thread_name), "node_serv_%d", ++thread_count);

        sys_ppu_thread_t node_serv_tid;
        if (sysThreadCreate(&node_serv_tid, node_serv_func, node_param, 1000, 0x4000, THREAD_JOINABLE, thread_name) < 0)
        {
            debug_printf("Error creating node thread for slave %d\n", conn_fd);
            continue;
        }
    }

    // Join all the slave node connection threads
    debug_printf("Joining all slave node connection threads.\n");
    for(auto* node : s_slave_nodes)
    {
        u64 retval;
        sysThreadJoin(node->tid, &retval);
        // destroy kickoff cond var
        node_sync_destroy(&node->kickoff_cond_var);

        // remember to delete allocated node params
        free(node);
    }
    s_slave_nodes.clear();
    debug_printf("Joining slave node connection threads done.\n");

    sysThreadExit(0);
}

void kickoff_all_nodes()
{
    // FIXME: protect s_slave_nodes!
    for(auto& node : s_slave_nodes)
    {
        node_sync_signal(&node->kickoff_cond_var);
    }
}

bool start_listening_server()
{
    // create socket
    struct sockaddr_in serv_addr;
    listen_socket_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (listen_socket_fd == -1)
        return false;

    // preparation of the socket address, the ip address is symbolic holder of any address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    // bind
    if (bind(listen_socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0)
        return false;

    // listen
    if (listen(listen_socket_fd, LISTENQ) != 0)
        return false;

    // start listener server thread
    listening_serv_run = true;
    char thread_name[] = "listening_serv";
    if (sysThreadCreate(&listening_serv_tid, listening_serv_func, NULL, 1000, 0x4000, THREAD_JOINABLE, thread_name) < 0)
        return false;

    return true;
}


bool shutdown_listening_server()
{
    u64 retval;
    listening_serv_run = false;

    // NOTE: because accept() doesn't have a timeout option, we here to close() the listening socket to tell the listening
    // thread to quit nicely
    debug_printf("Force closing the listening socket: %d\n", listen_socket_fd);
    if (close(listen_socket_fd) != 0)
        return false;

    debug_printf("Waiting for listening server thread %d to exited\n", listening_serv_tid);
    // wait for server thread to join
    if (sysThreadJoin(listening_serv_tid, &retval) < 0)
        return false;

    debug_printf("Listening server thread %d exited with code %llu\n", listening_serv_tid, retval);
    return true;
}

void node_sync_init(node_sync_t* node_sync)
{
    // create mutex
    sys_mutex_attr_t mutex_attr;
    sysMutexAttrInitialize(mutex_attr);
    sysMutexCreate(&node_sync->mutex, &mutex_attr);

    // create conditional variable
    sys_cond_attr cond_attr;
    sysCondAttrInitialize(cond_attr);
    sysCondCreate(&node_sync->cond_var, node_sync->mutex, &cond_attr);
}

void node_sync_destroy(node_sync_t* node_sync)
{
    sysCondDestroy(node_sync->cond_var);
    sysMutexDestroy(node_sync->mutex);
}

bool node_sync_wait(node_sync_t* node_sync, u64 timeout)
{
    sysMutexLock(node_sync->mutex, 0);
    bool succeeded = sysCondWait(node_sync->cond_var, timeout) == 0;
    sysMutexUnlock(node_sync->mutex);
    return succeeded;
}

void node_sync_signal(node_sync_t* node_sync)
{
    sysMutexLock(node_sync->mutex, 0);
    sysCondSignal(node_sync->cond_var);
    sysMutexUnlock(node_sync->mutex);
}

bool send_shutdown_request(int conn_fd, bool shutdown)
{
    uint8_t code = shutdown ? NC_SHUTDOWN : NC_NOP;
    if (send_request_header(conn_fd, code))
    {
        return true;
    }

    return false;
}

bool recv_shutdown_request(int conn_fd, bool* is_shutdown)
{
    uint8_t code = NC_NOP;
    if (recv_request_header(conn_fd, &code))
    {
        if (code == NC_SHUTDOWN)
            *is_shutdown = true;
        else
            *is_shutdown = false;

        return true;
    }

    return false;
}

typedef struct _processor_offer_packet_t
{
    uint8_t ppu_count;
    uint8_t spu_count;
}processor_offer_packet_t;

bool recv_processor_offer(int conn_fd, uint8_t* out_ppu_count, uint8_t* out_spu_count)
{
    uint8_t code = NC_NOP;
    if (recv_request_header(conn_fd, &code) && code == NC_NODEOFFER)
    {
        processor_offer_packet_t processor_offer;
        if (recv(conn_fd, &processor_offer, sizeof(processor_offer_packet_t), 0) == sizeof(processor_offer_packet_t))
        {
            *out_ppu_count = processor_offer.ppu_count;
            *out_spu_count = processor_offer.spu_count;
            return true;
        }
    }

    return false;
}

bool send_processor_offer(int conn_fd, uint8_t ppu_count, uint8_t spu_count)
{
    uint8_t code = NC_NODEOFFER;
    if (send_request_header(conn_fd, code))
    {
        processor_offer_packet_t processor_offer {
            ppu_count,
            spu_count
        };

        if (send(conn_fd, &processor_offer, sizeof(processor_offer_packet_t), 0) == sizeof(processor_offer_packet_t))
        {
            return true;
        }
    }

    return false;
}

typedef struct _job_request_packet_t
{
    uint32_t job_id;
    size_t serialized_data_size;
    int img_width;
    int img_height;
    int start_y;
    int end_y;
} job_request_packet_t;

typedef struct _job_result_packet_t
{
    uint32_t job_id;
    size_t serialized_data_size;
} job_result_packet_t;

bool send_job_count(int conn_fd, int32_t count)
{
    uint8_t code = NC_JOBCOUNT;
    if (send_request_header(conn_fd, code))
    {
        int32_t cnt = count;
        if (send(conn_fd, &cnt, sizeof(cnt), 0) == sizeof(cnt))
        {
            return true;
        }
    }

    return false;
}

bool recv_job_count(int conn_fd, int32_t* out_count)
{
    uint8_t code = NC_JOBCOUNT;
    if (recv_request_header(conn_fd, &code) && code == NC_JOBCOUNT)
    {
        int32_t count;
        if (recv(conn_fd, &count, sizeof(count), 0) == sizeof(count))
        {
            *out_count = count;
            return true;
        }
    }

    return false;
}

bool send_job_request(int conn_fd, uint32_t job_id, uint8_t* serialized_data, size_t data_size, int img_width, int img_height, int start_y, int end_y)
{
    uint8_t code = NC_JOBREQUEST;
    if (send_request_header(conn_fd, code))
    {
        job_request_packet_t job_request {
            job_id,
            data_size,
            img_width,
            img_height,
            start_y,
            end_y
        };

        if (send(conn_fd, &job_request, sizeof(job_request_packet_t), 0) == sizeof(job_request_packet_t))
        {
            if (send(conn_fd, serialized_data, data_size, 0) == (ssize_t)data_size)
            {
                return true;
            }
        }
    }
    return false;
}

bool recv_job_request(int conn_fd, uint32_t* out_job_id, uint8_t** out_serialized_data, size_t* out_data_size, int* out_img_width, int* out_img_height, int* out_start_y, int* out_end_y)
{
    uint8_t code = NC_NOP;
    if (recv_request_header(conn_fd, &code) && code == NC_JOBREQUEST)
    {

        job_request_packet_t job_request;

        if (recv(conn_fd, &job_request, sizeof(job_request_packet_t), 0) == sizeof(job_request_packet_t))
        {
            uint8_t* blob = (uint8_t*)malloc(job_request.serialized_data_size);
            if (recv(conn_fd, blob, job_request.serialized_data_size, 0) == (ssize_t)job_request.serialized_data_size)
            {
                *out_serialized_data = blob;

                *out_job_id = job_request.job_id;
                *out_data_size = job_request.serialized_data_size;
                *out_img_width = job_request.img_width;
                *out_img_height = job_request.img_height;
                *out_start_y = job_request.start_y;
                *out_end_y = job_request.end_y;

                return true;
            }
        }
    }

    return false;
}

static ssize_t send_reliable(int sockfd, const void *buf, size_t len, int flags)
{
    ssize_t remained_len = len;
    uint8_t* remained_buf = (uint8_t*)buf;

    while(remained_len > 0)
    {
        ssize_t sent_bytes = send(sockfd, remained_buf, remained_len, flags);
        if (sent_bytes < 0)
        {
            debug_printf("Sending socket %d error!\n", sockfd);
            return sent_bytes;
        }

        remained_buf += sent_bytes;
        remained_len -= sent_bytes;
    }

    return len;
}

static ssize_t recv_reliable(int sockfd, void *buf, size_t len, int flags)
{
    ssize_t remained_len = len;
    uint8_t* remained_buf = (uint8_t*)buf;

    while(remained_len > 0)
    {
        ssize_t recv_bytes = recv(sockfd, remained_buf, remained_len, flags);
        if (recv_bytes < 0)
        {
            debug_printf("Receiving socket %d error!\n", sockfd);
            return recv_bytes;
        }

        remained_buf += recv_bytes;
        remained_len -= recv_bytes;
    }

    return len;
}

bool send_job_result(int conn_fd, uint32_t job_id, uint8_t* pixel_data, size_t pixel_data_size)
{
    uint8_t code = NC_JOBRESULT;
    if (send_request_header(conn_fd, code))
    {

        job_result_packet_t job_result {
            job_id,
            pixel_data_size
        };

        if (send(conn_fd, &job_result, sizeof(job_result_packet_t), 0) == sizeof(job_result_packet_t))
        {
            if (send_reliable(conn_fd, pixel_data, pixel_data_size, 0) == (ssize_t)pixel_data_size)
            {
                return true;
            }
        }
    }

    return false;
}

bool recv_job_result(int conn_fd, uint32_t* out_job_id, uint8_t** out_pixel_data, size_t* out_pixel_data_size)
{
    uint8_t code = NC_NOP;
    if (recv_request_header(conn_fd, &code) && code == NC_JOBRESULT)
    {
        job_result_packet_t job_result;

        if (recv(conn_fd, &job_result, sizeof(job_result_packet_t), 0) == sizeof(job_result_packet_t))
        {
            uint8_t* blob = (uint8_t*)malloc(job_result.serialized_data_size);
            if (recv_reliable(conn_fd, blob, job_result.serialized_data_size, 0) == (ssize_t)job_result.serialized_data_size)
            {
                *out_pixel_data = blob;

                *out_job_id = job_result.job_id;
                *out_pixel_data_size = job_result.serialized_data_size;
                return true;
            }
        }
    }

    return false;
}

bool connect_master(const char* master_addr, int* out_conn_fd)
{
    struct sockaddr_in serv_addr;
    main_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_socket_fd < 0)
        return false;

    // preparation of the socket address, note the address is a specific ip address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(master_addr);
    serv_addr.sin_port = htons(SERV_PORT);

    // connect to master
    if (connect(main_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        return false;

    *out_conn_fd = main_socket_fd;

    return true;
}

bool disconnect_master()
{
    if (close(main_socket_fd) != 0)
        return false;

    main_socket_fd = -1;
    return true;
}

bool send_request_header(int socket_fd, uint8_t request_code)
{
    uint8_t request_buf[1] = {
        request_code
    };

    const size_t nbytes = sizeof(request_buf);

    if (send(socket_fd, request_buf, nbytes, 0) == nbytes)
    {
        return true;
    }

    return false;
}

bool recv_request_header(int socket_fd, uint8_t* out_request_code)
{
    if (recv(socket_fd, out_request_code, sizeof(uint8_t), 0) == sizeof(uint8_t))
    {
        return true;
    }

    return false;
}
