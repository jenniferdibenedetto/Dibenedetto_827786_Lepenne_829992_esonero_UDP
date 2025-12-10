/*
 * main.c (Client)
 *
 * UDP Client - Template for Computer Networks assignment
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
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

int main(int argc, char *argv[]) {

	
	//implement client logic
    
    int porta_server = SERVER_PORT;
    char *host_server = DEFAULT_IP;
    char *stringa_input = NULL;
    int i;

    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            i++; if (i < argc) host_server = argv[i];
        }
        else if (strcmp(argv[i], "-p") == 0) {
            i++; if (i < argc) porta_server = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-r") == 0) {
            i++; if (i < argc) stringa_input = argv[i];
        }
    }

    if (stringa_input == NULL) {
        printf("Errore. Uso corretto: %s -r \"tipo citta\" [-s server] [-p porta]\n", argv[0]);
        return -1;
    }

    //Validazione input
    if (strchr(stringa_input, '\t') != NULL) {
        printf("Errore: tabulazioni non ammesse\n");
        return -1;
    }

    char *spazio = strchr(stringa_input, ' ');
    if (spazio == NULL || (spazio - stringa_input) != 1) {
        printf("Errore: formato richiesta invalido (atteso \"t citta\")\n");
        return -1;
    }

    char type_req = stringa_input[0];
    char *city_ptr = spazio + 1;

    if (strlen(city_ptr) > 63) {
        printf("Errore: nome città troppo lungo\n");
        return -1;
    }

    
    
#if defined WIN32
    
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 0;
    }
#endif

    int my_socket;
    
    struct sockaddr_in ind_risposta;
    char buffer_tx[BUFFER_SIZE];
    char buffer_rx[BUFFER_SIZE];

#if defined WIN32
    int lungh_ind;
#else
    socklen_t lungh_ind;
#endif

    //Configure server address 
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      
    hints.ai_socktype = SOCK_DGRAM; 

    char port_str[10];
    sprintf(port_str, "%d", porta_server);

    if (getaddrinfo(host_server, port_str, &hints, &res) != 0) {
        printf("Errore risoluzione DNS server: %s\n", host_server);
        clearwinsock();
        return -1;
    }

    //create UDP socket
    my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (my_socket < 0) {
        printf("Errore creazione socket.\n");
        freeaddrinfo(res);
        clearwinsock();
        return -1;
    }

    //Implement UDP communication 

    //Serializzazione Manuale
    int tx_offset = 0;
    memcpy(buffer_tx + tx_offset, &type_req, sizeof(char));
    tx_offset += sizeof(char);
    strncpy(buffer_tx + tx_offset, city_ptr, 63);
    tx_offset += 64;

    //Invio  
    int sent = sendto(my_socket, buffer_tx, tx_offset, 0, res->ai_addr, res->ai_addrlen);
    if (sent < 0) {
        printf("Errore invio dati.\n");
        closesocket(my_socket);
        freeaddrinfo(res);
        clearwinsock();
        return -1;
    }

    //Ricezione 
    lungh_ind = sizeof(ind_risposta);
    int received = recvfrom(my_socket, buffer_rx, BUFFER_SIZE, 0,
                            (struct sockaddr*)&ind_risposta, &lungh_ind);

    if (received > 0) {
        //Deserializzazione
        weather_response_t resp;
        int rx_offset = 0;

        unsigned int net_status;
        memcpy(&net_status, buffer_rx + rx_offset, sizeof(unsigned int));
        resp.status = ntohl(net_status);
        rx_offset += sizeof(unsigned int);

        memcpy(&resp.type, buffer_rx + rx_offset, sizeof(char));
        rx_offset += sizeof(char);

        unsigned int net_val;
        memcpy(&net_val, buffer_rx + rx_offset, sizeof(unsigned int));
        net_val = ntohl(net_val);
        memcpy(&resp.value, &net_val, sizeof(float));

       
        char final_host[NI_MAXHOST];
        char final_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ind_risposta.sin_addr), final_ip, INET_ADDRSTRLEN);

        if (getnameinfo((struct sockaddr*)&ind_risposta, lungh_ind,
                        final_host, sizeof(final_host), NULL, 0, 0) != 0) {
            strcpy(final_host, final_ip);
        }

        printf("Ricevuto risultato dal server %s (ip %s). ", final_host, final_ip);

        if (resp.status == 0) {
            // Capitalizza la città per output
            char city_cap[64];
            strncpy(city_cap, city_ptr, 63);
            city_cap[63] = '\0';
            if(city_cap[0] >= 'a' && city_cap[0] <= 'z') city_cap[0] = toupper(city_cap[0]);

            switch(resp.type) {
                case 't': printf("%s: Temperatura = %.1f°C\n", city_cap, resp.value); break;
                case 'h': printf("%s: Umidità = %.1f%%\n", city_cap, resp.value); break;
                case 'w': printf("%s: Vento = %.1f km/h\n", city_cap, resp.value); break;
                case 'p': printf("%s: Pressione = %.1f hPa\n", city_cap, resp.value); break;
            }
        } else if (resp.status == 1) {
            printf("Città non disponibile\n");
        } else {
            printf("Richiesta non valida\n");
        }
    }

    //Close socket 
    freeaddrinfo(res);
    closesocket(my_socket);

    printf("Client terminated.\n");
    clearwinsock();
    return 0;
} // main end

