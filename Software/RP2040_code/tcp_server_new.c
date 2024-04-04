/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "tcp_server.h"

TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate server state. memory skill issue\n");
        return NULL;
    }
    DEBUG_printf("Allocated server state\n");
    return state;
}

err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    DEBUG_printf("Server closed successfully\n");
    return err;
}

err_t tcp_server_result(void *arg, int status) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("success\n");
    } else {
        DEBUG_printf("failed %d\n", status);
    }
    //state->complete = true;
    return ERR_OK; //lol
}

err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        // We should get the data back from the client
        state->recv_len = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    /*for(int i=0; i< BUF_SIZE; i++) {
        state->buffer_sent[i] = rand();
    }*/

    state->sent_len = 0;
    DEBUG_printf("Writing %ld bytes to client\n\n", BUF_SIZE);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_begin();
    //DEBUG_printf("Acquired all locks now\n");
    cyw43_arch_lwip_check();
    //DEBUG_printf("We have gotten past the lwip check\n");
    err_t err = tcp_write(tpcb, state->buffer_sent, BUF_SIZE, TCP_WRITE_FLAG_COPY);
    err_t err_o = tcp_output(tpcb);
    //DEBUG_printf("We have gotten past the write statement\n");
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        if (err == ERR_CONN) {
            // no connection
            return tcp_restart(arg);
        } else if (err == ERR_RST) {
            DEBUG_printf("Connection was reset\n");
            return ERR_OK;
        } else {
            return tcp_server_result(arg, -1);
        }
    }
    if (err_o != ERR_OK) {
        DEBUG_printf("Failed to output data %d\n", err_o);
        return tcp_server_result(arg, -1);
    }
    
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        return tcp_server_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_begin();
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    cyw43_arch_lwip_end();

    // Have we have received the whole buffer
    if (state->recv_len == BUF_SIZE) {

        // check it matches
        if (memcmp(state->buffer_sent, state->buffer_recv, BUF_SIZE) != 0) {
            DEBUG_printf("buffer mismatch\n");
            return tcp_server_result(arg, -1);
        }
        DEBUG_printf("tcp_server_recv buffer ok\n");

        // Test complete?
        state->run_count++;
        if (state->run_count >= TEST_ITERATIONS) {
            tcp_server_result(arg, 0);
            return ERR_OK;
        }

        // Send another buffer
        //return tcp_server_send_data(arg, state->client_pcb);
    }
    return ERR_OK;
}

err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    /*DEBUG_printf("No client connected, restarting server\n");
    // If there is no response we will restart the server
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    tcp_server_close(arg);

    if (!tcp_server_open(state, TCP_PORT)) { // Open the server to connections and wait for someone to connect
        printf("\nFailed to open TCP server. Exiting program\n");
        return -1;
    }

    printf("Waiting for a connection...\n");

    tcp_accept(state->server_pcb, tcp_server_accept); // tcp_server_accept will assign all callbacks
                                                      // once the connection is made
    */
    return ERR_OK;
}

err_t tcp_restart(void *arg) {
    DEBUG_printf("No client connected, restarting server\n");
    // If there is no response we will restart the server
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    tcp_server_close(arg);

    if (!tcp_server_open(state, TCP_PORT)) { // Open the server to connections and wait for someone to connect
        printf("\nFailed to open TCP server. Exiting program\n");
        return -1;
    }

    printf("Waiting for a connection...\n\n");

    tcp_accept(state->server_pcb, tcp_server_accept); // tcp_server_accept will assign all callbacks
                                                      // once the connection is made
    return ERR_OK;
}

void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_server_result(arg, err);
        return ERR_VAL;
    }
    DEBUG_printf("Client connected!\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    //tcp_poll(client_pcb, tcp_server_poll, 10);
    tcp_err(client_pcb, tcp_server_err);

    //return tcp_server_send_data(state, client_pcb);
    return ERR_OK;
}

bool tcp_server_open(TCP_SERVER_T *state, uint port) {
    //TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), port);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, &netif_list->ip_addr, port);
    if (err) {
        DEBUG_printf("Error: %u. Failed to bind to port %u\n", err, port);
        return false;
    }    

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("Failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    //tcp_accept(state->server_pcb, tcp_server_accept);
    return true;
}

/*void run_tcp_server_test(void) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return;
    }
    while(!state->complete) {
        sleep_ms(1000);
    }
    free(state);
}*/
