
#include "udp_functions.h"
#include "screen_print_c.h"
#include "agent_lib_c.h"


#include <stdlib.h>
#include <stdio.h>

// ----------------------------- IMPORTANT ------------------------------------
//  This library works only in remote mode!!!!
//  Because the library is in C.
// ----------------------------- IMPORTANT ------------------------------------

static struct {
	int socket_id;
	struct sockaddr_in server_addr;
	struct sockaddr_in self_addr;
	struct sockaddr_in client_addr;
	int packet_size;
} socket_config;

uint32_t message_id = 0;
int initialized = 0;
char client_ip_string[INET_ADDRSTRLEN];

void test_lib(){
    printf("The library is loaded correctly\n");
}

#ifndef VOIDSERVER
/*  ------------------------------------ CLIENT FUNCTIONS ------------------------------------  */

// Client agent Init Matlab=================================================================
void client_agent_init_num(unsigned int num_ip[4] , int server_port, int log_on , int log_type) {

    if (num_ip == 0 || (num_ip[0] == 0 && num_ip[1] == 0 && num_ip[2] == 0 && num_ip[3] == 0)) {
        fprintf(stderr, "This library does not support local agent.\n");
        exit(EXIT_FAILURE);
    } else {
        if (log_on == 1) {
            fprintf(stderr, "No log will be generated, log works only in local modality.\n");
        }
        if (initialized == 1) {
            fprintf(stderr, "The system is already initialized.\n");
            exit(EXIT_FAILURE);
        }
        // Init Client
        char ip_str[16];
        sprintf(ip_str, "%d.%d.%d.%d", num_ip[0], num_ip[1], num_ip[2], num_ip[3]);

        memset(&socket_config.server_addr, 0, sizeof(socket_config.server_addr));
        socket_config.server_addr.sin_family = AF_INET;
        socket_config.server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, ip_str, &socket_config.server_addr.sin_addr.s_addr);

        if ((socket_config.socket_id = open_socket(0, &socket_config.server_addr)) == -1) {
            fprintf(stderr, "Socket does not open\n");
            exit(EXIT_FAILURE);
        }
    }
    initialized = 1;
}

// Client agent Init =================================================================
void client_agent_init(const char *server_ip , int server_port, int log_on , int log_type) {

    if (server_ip == 0 || server_ip[0] == '\0') {//LOCAL agent
        fprintf(stderr, "This library does not support local agent.\n");
        exit(EXIT_FAILURE);
    } else {
        if (log_on == 1) {
            fprintf(stderr, "No log will be generated, log works only in local modality.\n");
        }
        if (initialized == 1) {
            fprintf(stderr, "The system is already initialized.\n");
            exit(EXIT_FAILURE);
        }

        memset(&socket_config.server_addr, 0, sizeof(socket_config.server_addr));
        socket_config.server_addr.sin_family = AF_INET;
        socket_config.server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &socket_config.server_addr.sin_addr.s_addr);

        if ((socket_config.socket_id = open_socket(0, &socket_config.server_addr)) == -1) {
            fprintf(stderr, "Socket does not open\n");
            exit(EXIT_FAILURE);
        }
    }
    initialized = 1;
}

// Client send to server =================================================================
int client_send_to_server(uint32_t server_run, uint32_t message_id, input_data_str *scenario_msg) {
    return send_message(socket_config.socket_id, &socket_config.server_addr, server_run, message_id,
                        (char *) scenario_msg, sizeof(input_data_str));
}

// Client receive from server ============================================================
int client_receive_form_server(uint32_t *server_run, uint32_t *message_id, output_data_str *manoeuvre_msg,
                               uint64_t start_time) {
    return receive_message(socket_config.socket_id, &socket_config.server_addr, server_run, message_id,
                           (char *) manoeuvre_msg, sizeof(output_data_str), start_time);
}

// Client agent Compute =================================================================
void client_agent_compute(input_data_str *scenario_msg, output_data_str *manoeuvre_msg) {
    if (socket_config.socket_id == -1) {//LOCAL agent
        fprintf(stderr, "This library does not support local agent.\n");
        exit(EXIT_FAILURE);
    } else {
        uint32_t server_run = 1;
        uint64_t start_time = get_time_ms();
        if (manoeuvre_msg == 0)
            server_run = 0;
        if (send_message(socket_config.socket_id, &socket_config.server_addr, server_run, message_id,
                         (char *) scenario_msg, sizeof(input_data_str)) == -1) {
            fprintf(stderr, "Couldn't send the message with ID %d\n", message_id);
        } else {
            if (receive_message(socket_config.socket_id, &socket_config.server_addr, &server_run, &message_id,
                                (char *) manoeuvre_msg, sizeof(output_data_str), start_time) == -1) {
                fprintf(stderr, "Manoeuvre not calculated\n");
            }
        }
        message_id++;
    }
}

// Client agent Close =================================================================
void client_agent_close() {
    if (socket_config.socket_id == -1) {//LOCAL agent
        fprintf(stderr, "This library does not support local agent.\n");
        exit(EXIT_FAILURE);
    } else {
        char a;
        if (send_message(socket_config.socket_id, &socket_config.server_addr, 0, message_id, &a, 1) == -1) {
            fprintf(stderr, "Couldn't send the message with ID %d\n", message_id);
        }
        close_socket(socket_config.socket_id);
    }
}
/*  ------------------------------------ CLIENT FUNCTIONS ------------------------------------  */
#endif

/*  ------------------------------------ SERVER FUNCTIONS ------------------------------------  */
// Server Init Matlab=================================================================
void server_agent_init_num(unsigned int num_ip[4], int server_port) {
    if (num_ip == 0 || (num_ip[0] == 0 && num_ip[1] == 0 && num_ip[2] == 0 && num_ip[3] == 0)) {
        fprintf(stderr, "The ip parameter is needed.\n");
        exit(EXIT_FAILURE);
    } else {
        if (initialized == 1) {
            fprintf(stderr, "The system is already initialized.\n");
            exit(EXIT_FAILURE);
        }
        // Init Server
        char ip_str[16];
        sprintf(ip_str, "%d.%d.%d.%d", num_ip[0], num_ip[1], num_ip[2], num_ip[3]);

        socklen_t addr_len = sizeof(socket_config.self_addr);
        memset(&socket_config.self_addr, 0, addr_len);
        memset(&socket_config.client_addr, 0, addr_len);
        socket_config.self_addr.sin_family = AF_INET;
        socket_config.self_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, ip_str, &(socket_config.self_addr.sin_addr.s_addr));

        if ((socket_config.socket_id = open_socket(1, &socket_config.self_addr)) == -1) {
            fprintf(stderr, "Socket does not open\n");
            exit(EXIT_FAILURE);
        }

        char *server_start;
        sprintf(server_start, "Server START === (IP: %s, Port: %d)", ip_str, server_port);
        printInputSection(server_start);
    }
    initialized = 1;
}

// Server agent Init =================================================================
void server_agent_init(const char *server_ip, int server_port) {
    if (server_ip == 0 || server_ip[0] == '\0') {
        fprintf(stderr, "The ip parameter is needed.\n");
        exit(EXIT_FAILURE);
    } else {
        if (initialized == 1) {
            fprintf(stderr, "The system is already initialized.\n");
            exit(EXIT_FAILURE);
        }

        socklen_t addr_len = sizeof(socket_config.self_addr);
        memset(&socket_config.self_addr, 0, addr_len);
        memset(&socket_config.client_addr, 0, addr_len);
        socket_config.self_addr.sin_family = AF_INET;
        socket_config.self_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &(socket_config.self_addr.sin_addr.s_addr));

        if ((socket_config.socket_id = open_socket(1, &socket_config.self_addr)) == -1) {
            fprintf(stderr, "Socket does not open\n");
            exit(EXIT_FAILURE);
        }

        char *server_start;
        sprintf(server_start, "Server START === (IP: %s, Port: %d)", server_ip, server_port);
        printInputSection(server_start);
    }
    initialized = 1;
}
/*  ------------------------------------ SERVER FUNCTIONS ------------------------------------  */

// Server get ip of the client===========================================================
char *server_receive_from() {
    inet_ntop(AF_INET, &(socket_config.client_addr.sin_addr.s_addr), client_ip_string, INET_ADDRSTRLEN);
    return client_ip_string;
}

// Server agent Send =================================================================
int server_send_to_client(uint32_t server_run, uint32_t message_id, output_data_str *manoeuvre_msg) {
    return send_message(socket_config.socket_id, &socket_config.client_addr, server_run, message_id,
                        (char *) manoeuvre_msg, sizeof(output_data_str));
}

// Client agent Receive ===============================================================
int server_receive_from_client(uint32_t *server_run, uint32_t *message_id, input_data_str *scenario_msg) {
    return receive_message(socket_config.socket_id, &socket_config.client_addr, server_run, message_id,
                           (char *) scenario_msg, sizeof(input_data_str), 0);
}

void server_agent_close() {
    printInputSection("Server STOPPED");
    close_socket(socket_config.socket_id);
    printTable("Socket closed", 0);
    printTable("Done", 0);
    printLine();
}

/*  ------------------------------------ SERVER FUNCTIONS ------------------------------------  */