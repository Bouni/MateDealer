/**
 * Project: MateDealer
 * File name: usart.h
 * Description:  USART methods
 *   
 * @author bouni
 * @email bouni@owee.de  
 *   
 */
#ifndef USART_H
#define USART_H

#ifndef F_CPU
#define F_CPU           16000000UL
#endif

/* Activate a USART buffer for appropriate USART */
#define USART0          1
#define USART1          1
#define USART2          0
#define USART3          0

/* set the buffersize for all USART buffers */
#define BUFFERSIZE      128
#define BUFFERMASK      (BUFFERSIZE - 1);

/* Don't change the following two defines */
#define RX              0
#define TX              1

typedef struct _buffer {
    uint8_t data[BUFFERSIZE];
    volatile uint8_t read; 
    volatile uint8_t write;
} buffer_t;

typedef struct _usart {
    buffer_t buffer[2]; 
    volatile uint8_t *ubrrh;    
    volatile uint8_t *ubrrl;
    volatile uint8_t *ucsrb;    
    volatile uint8_t *ucsrc;
} usart_t;


void setup_usart(uint8_t usart_number, uint32_t baudrate, uint8_t framelength, uint8_t parity, uint8_t stopbits);

uint8_t buffer_level(uint8_t usart_number, uint8_t direction);
uint8_t read_buffer(uint8_t usart_number, uint8_t direction, uint8_t *data);
uint8_t write_buffer(uint8_t usart_number, uint8_t direction, uint8_t data);
uint8_t send_char(uint8_t usart_number, char c);
uint8_t recv_char(uint8_t usart_number, char *c);
uint8_t send_str(uint8_t usart_number, char *str);
uint8_t recv_str(uint8_t usart_number, char *str);
uint8_t send_str_p(uint8_t usart_number, const char *str);
uint8_t send_mdb(uint8_t usart_number, uint16_t mdb);
uint16_t recv_mdb(uint8_t usart_number);

#endif
