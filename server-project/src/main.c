/*
 * main.c (Server)
 *
 * UDP Server - Template for Computer Networks assignment
 */

#if defined WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}


float get_temperatura() { return ((float)rand() / (float)(RAND_MAX)) * 50.0 - 10.0; }
float get_umidita() { return ((float)rand() / (float)(RAND_MAX)) * 80.0 + 20.0; }
float get_vento() { return ((float)rand() / (float)(RAND_MAX)) * 100.0; }
float get_pressione() { return ((float)rand() / (float)(RAND_MAX)) * 100.0 + 950.0; }

int main(int argc, char *argv[]) {

    // Variabili per la logica server
    int porta_server = SERVER_PORT;
    int i;

   
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            i++;
            if (i < argc) porta_server = atoi(argv[i]);
        }
    }

    srand((unsigned int)time(NULL));

#if defined WIN32
    // Initialize Winsock 
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 0;
    }
#endif

    int my_socket;
    struct sockaddr_in ind_server;
    struct sockaddr_in ind_client;

   
    char buffer_rx[BUFFER_SIZE];
    char buffer_tx[BUFFER_SIZE];

#if defined WIN32
    int lungh_client;
#else
    socklen_t lungh_client;
#endif

    //Create UDP socket 
    my_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (my_socket < 0) {
        printf("Errore creazione socket.\n");
        clearwinsock();
        return -1;
    }

    //Configure server address 
    memset(&ind_server, 0, sizeof(ind_server));
    ind_server.sin_family = AF_INET;
    ind_server.sin_port = htons(porta_server);
    ind_server.sin_addr.s_addr = htonl(INADDR_ANY);

    //Bind socket 
    if (bind(my_socket, (struct sockaddr*)&ind_server, sizeof(ind_server)) < 0) {
        printf("Errore bind.\n");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    printf("Server UDP in ascolto sulla porta %d\n", porta_server);

    //Implement UDP datagram reception loop 
    while (1) {
        lungh_client = sizeof(ind_client);

        
        int byte_ricevuti = recvfrom(my_socket, buffer_rx, BUFFER_SIZE, 0,
                                     (struct sockaddr*)&ind_client, &lungh_client);

        if (byte_ricevuti < 0) continue;

        
        char host_client[NI_MAXHOST];
        char ip_client[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(ind_client.sin_addr), ip_client, INET_ADDRSTRLEN);

        if (getnameinfo((struct sockaddr*)&ind_client, lungh_client,
                        host_client, sizeof(host_client),
                        NULL, 0, 0) != 0) {
            strcpy(host_client, ip_client); 
        }

        //Deserializzazione 
        weather_request_t req;
        int offset = 0;

        if (byte_ricevuti >= (int)(sizeof(char) + 1)) {
            memcpy(&req.type, buffer_rx + offset, sizeof(char));
            offset += sizeof(char);

            memset(req.city, 0, 64);
            int len_rimanente = byte_ricevuti - offset;
            if(len_rimanente > 63) len_rimanente = 63;
            memcpy(req.city, buffer_rx + offset, len_rimanente);
            req.city[63] = '\0';
        } else {
            req.type = 0; 
        }

       
        printf("Richiesta ricevuta da %s (ip %s): type='%c', city='%s'\n",
               host_client, ip_client, req.type, req.city);

       
        weather_response_t res;
        res.type = req.type;
        res.status = STATO_SUCCESSO;
        res.value = 0.0f;

        // Validazione stringa città 
        int caratteri_validi = 1;
        for(int k=0; req.city[k] != '\0'; k++) {
            if(!isalpha(req.city[k]) && req.city[k] != ' ') {
                caratteri_validi = 0; break;
            }
        }

        if (strchr("thwp", req.type) == NULL || !caratteri_validi) {
            res.status = STATO_RICHIESTA_NON_VALIDA;
        } else {
            
            char *citta_note[] = {"bari", "roma", "milano", "napoli", "torino",
                                  "palermo", "genova", "bologna", "firenze", "venezia"};
            int trovata = 0;
            char city_lower[64];
            strncpy(city_lower, req.city, 64);
            for(int k=0; city_lower[k]; k++) city_lower[k] = tolower(city_lower[k]);

            for(int k=0; k<10; k++) {
                if(strcmp(city_lower, citta_note[k]) == 0) {
                    trovata = 1; break;
                }
            }

            if (!trovata) {
                res.status = STATO_CITTA_NON_DISPONIBILE;
            } else {
                switch(req.type) {
                    case 't': res.value = get_temperatura(); break;
                    case 'h': res.value = get_umidita(); break;
                    case 'w': res.value = get_vento(); break;
                    case 'p': res.value = get_pressione(); break;
                }
            }
        }

        
        int tx_offset = 0;

        unsigned int net_status = htonl(res.status);
        memcpy(buffer_tx + tx_offset, &net_status, sizeof(unsigned int));
        tx_offset += sizeof(unsigned int);

        memcpy(buffer_tx + tx_offset, &res.type, sizeof(char));
        tx_offset += sizeof(char);

        unsigned int net_val_int;
        memcpy(&net_val_int, &res.value, sizeof(float));
        net_val_int = htonl(net_val_int); 
        memcpy(buffer_tx + tx_offset, &net_val_int, sizeof(unsigned int));
        tx_offset += sizeof(unsigned int);

        
        sendto(my_socket, buffer_tx, tx_offset, 0, (struct sockaddr*)&ind_client, lungh_client);
    }

    printf("Server terminated.\n");
    closesocket(my_socket);
    clearwinsock();
    return 0;
} // main end

