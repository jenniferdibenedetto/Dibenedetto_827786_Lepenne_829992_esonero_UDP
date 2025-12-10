/*
 * protocol.h
 *
 * Shared header file for UDP client and server
 * Contains protocol definitions, data structures, constants and function prototypes
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>

/*
 * ============================================================================
 * PROTOCOL CONSTANTS
 * ============================================================================
 */

#define SERVER_PORT 56700
#define BUFFER_SIZE 512
#define DEFAULT_IP "localhost"

#define STATO_SUCCESSO 0
#define STATO_CITTA_NON_DISPONIBILE 1
#define STATO_RICHIESTA_NON_VALIDA 2

/*
 * ============================================================================
 * PROTOCOL DATA STRUCTURES
 * ============================================================================
 */

// Weather request and response structure
typedef struct {
    char type;         // 't', 'h', 'w', 'p'
    char city[64];     // Nome citta
} weather_request_t;

// Weather response structure
typedef struct {
    unsigned int status; // 0=OK, 1=Err, 2=Inv
    char type;           
    float value;         
} weather_response_t;

/*
 * ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

// Funzioni implementate 
float get_temperatura(void);
float get_umidita(void);
float get_vento(void);
float get_pressione(void);

#endif /* PROTOCOL_H_ */

