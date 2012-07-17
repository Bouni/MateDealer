/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * Project: MateDealer
 * File name: usart.c
 * Description:  USART methods
 *   
 * @author bouni
 * @email bouni@owee.de  
 *   
 * @see The GNU Public License (GPL)
 */

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "usart.h"


/* init the usart buffers */ 
usart_t usart[USART0 + USART1 + USART2 + USART3] = {
#if USART0 == 1
    { { { {}, 0, 0 }, { {}, 0, 0 } }, &UBRR0H, &UBRR0L, &UCSR0B, &UCSR0C },
#endif
#if USART1 == 1
    { { { {}, 0, 0 }, { {}, 0, 0 } }, &UBRR1H, &UBRR1L, &UCSR1B, &UCSR1C },
#endif
#if USART2 == 1
    { { { {}, 0, 0 }, { {}, 0, 0 } }, &UBRR2H, &UBRR2L, &UCSR2B, &UCSR2C },
#endif
#if USART3 == 1
    { { { {}, 0, 0 }, { {}, 0, 0 } }, &UBRR3H, &UBRR3L, &UCSR3B, &UCSR3C }
#endif
};
 
void setup_usart(uint8_t usart_number, uint32_t baudrate, uint8_t framelength, uint8_t parity, uint8_t stopbits) {
 
    /* calculate and set the baudrate */
    uint16_t baud = (F_CPU / 8 / baudrate - 1) / 2; 
    *usart[usart_number].ubrrh = (uint8_t) (baud >> 8);
    *usart[usart_number].ubrrl = (uint8_t) (baud & 0x0ff);
    
    /* activate transmitter, receiver and receiver interrupt */
    *usart[usart_number].ucsrb |= (1 << 3); /* TXEN */
    *usart[usart_number].ucsrb |= (1 << 4); /* RXEN */
    *usart[usart_number].ucsrb |= (1 << 7); /* RXCIE */
    
    /* set framelength bits */
    switch(framelength) {
    case 5:
        *usart[usart_number].ucsrc &= ~(1 << 1); /* UCSZ0 = 0 */
        *usart[usart_number].ucsrc &= ~(1 << 2); /* UCSZ1 = 0 */
        *usart[usart_number].ucsrb &= ~(1 << 2); /* UCSZ2 = 0 */
    break;
    case 6:
        *usart[usart_number].ucsrc |=  (1 << 1); /* UCSZ0 = 1 */
        *usart[usart_number].ucsrc &= ~(1 << 2); /* UCSZ1 = 0 */
        *usart[usart_number].ucsrb &= ~(1 << 2); /* UCSZ2 = 0 */
    break;
    case 7:
        *usart[usart_number].ucsrc &= ~(1 << 1); /* UCSZ0 = 0 */
        *usart[usart_number].ucsrc |=  (1 << 2); /* UCSZ1 = 1 */
        *usart[usart_number].ucsrb &= ~(1 << 2); /* UCSZ2 = 0 */
    break;
    case 8:
        *usart[usart_number].ucsrc |=  (1 << 1); /* UCSZ0 = 1 */
        *usart[usart_number].ucsrc |=  (1 << 2); /* UCSZ1 = 1 */
        *usart[usart_number].ucsrb &= ~(1 << 2); /* UCSZ2 = 0 */
    break;
    case 9:
        *usart[usart_number].ucsrc |=  (1 << 1); /* UCSZ0 = 1 */
        *usart[usart_number].ucsrc |=  (1 << 2); /* UCSZ1 = 1 */
        *usart[usart_number].ucsrb |=  (1 << 2); /* UCSZ2 = 1 */
    break;
    }
    
    /* set bits for parity */
    switch(parity) {
    case 'N': /* None */
        *usart[usart_number].ucsrc &= ~(1 << 4); /* UPM0 = 0 */
        *usart[usart_number].ucsrc &= ~(1 << 5); /* UPM1 = 0 */
    break;
    case 'E': /* Even */
        *usart[usart_number].ucsrc &= ~(1 << 4); /* UPM0 = 0 */
        *usart[usart_number].ucsrc |=  (1 << 5); /* UPM1 = 1 */
    break;
    case 'O': /* Odd */
        *usart[usart_number].ucsrc |=  (1 << 4); /* UPM0 = 1 */
        *usart[usart_number].ucsrc |=  (1 << 5); /* UPM1 = 1 */
    break;
    }
    
    /* set number of stopbits */
    if(stopbits > 1)
        *usart[usart_number].ucsrc |=  (1 << 3); /* USBS = 1 */
    else
        *usart[usart_number].ucsrc &= ~(1 << 3); /* USBS = 0 */
}

uint8_t buffer_level(uint8_t usart_number, uint8_t direction) {
    return usart[usart_number].buffer[direction].write - usart[usart_number].buffer[direction].read;
}

uint8_t read_buffer(uint8_t usart_number, uint8_t direction, uint8_t *data) {
    // buffer empty?
    if (usart[usart_number].buffer[direction].read == usart[usart_number].buffer[direction].write) return 1;
    // read a byte from the buffer
    *data = usart[usart_number].buffer[direction].data[usart[usart_number].buffer[direction].read];
    // update read index
    usart[usart_number].buffer[direction].read = (usart[usart_number].buffer[direction].read + 1) & BUFFERMASK;
    return 0; 
}

uint8_t write_buffer(uint8_t usart_number, uint8_t direction, uint8_t data) {
    // calc index for the next byte
    uint8_t next = (usart[usart_number].buffer[direction].write + 1) & BUFFERMASK;
    // wait if the buffer is full
    while(usart[usart_number].buffer[direction].read == next) { ; };
    // write the byte to the buffer
    usart[usart_number].buffer[direction].data[usart[usart_number].buffer[direction].write] = data;
    // update write index
    usart[usart_number].buffer[direction].write = next; 
    
    if(direction == TX) {
        // Check if the USART is in 9-Bit mode
        if(*usart[usart_number].ucsrb & 4) {
            // Activate transmit interrupt if the buffer level is even 
            if(!((usart[usart_number].buffer[direction].write - usart[usart_number].buffer[direction].read) & 1))
               *usart[usart_number].ucsrb |= (1 << 5);
        } else {
            // Activate transmit interrupt 
            *usart[usart_number].ucsrb |= (1 << 5);
        }
    }
    return 0;
}

uint8_t send_char(uint8_t usart_number, char c) {
    return write_buffer(usart_number, TX, (uint8_t) c);
}

uint8_t recv_char(uint8_t usart_number, char *c) {
    return read_buffer(usart_number, RX,(uint8_t *)c);
}

uint8_t send_str(uint8_t usart_number, char *str) {
    while(*str) {
        // if an error occurs
        if(send_char(usart_number, *str)) return 1;
        str++;
    }
    return 0;
}
 
uint8_t recv_str(uint8_t usart_number, char *str) {
    char c;
    while(1) {
        if(recv_char(usart_number, &c)) return 1;
        *str = c;
        str++;
        if(c == '\0') return 0;   
    }
}

uint8_t send_str_p(uint8_t usart_number,const char *str) {
    char c;
    while((c = pgm_read_byte(str))) {
        if(send_char(usart_number, c)) return 1;
        str++;
    }
    return 0;
}

uint8_t send_mdb(uint8_t usart_number, uint16_t mdb) {
    return write_buffer(usart_number, TX, (uint8_t)(mdb >> 8)) | write_buffer(usart_number, TX, (uint8_t)(mdb & 0xFF));
}

uint16_t recv_mdb(uint8_t usart_number) {
    uint8_t hb, lb;
    read_buffer(usart_number, RX, &hb);
    read_buffer(usart_number, RX, &lb);
    return ((hb << 8) | lb);
}

#if (USART0 == 1)
ISR(USART0_RX_vect){
    uint8_t data;
    if(UCSR0B & (1 << 2)) {
        data = ((UCSR0B >> 1) & 0x01);
        write_buffer(0, RX, data);
    }
    data = UDR0;
    write_buffer(0, RX, data);
}
#endif

#if (USART1 == 1)
ISR(USART1_RX_vect){
    uint8_t data;
    if(UCSR1B & (1 << 2)) {
        data = ((UCSR1B >> 1) & 0x01);
        write_buffer(1, RX, data);
    }
    data = UDR1;
    write_buffer(1, RX, data);
}
#endif

#if (USART2 == 1)
ISR(USART2_RX_vect){
    uint8_t data;
    if(UCSR2B & (1 << 2)) {
        data = ((UCSR2B >> 1) & 0x01);
        write_buffer(2, RX, data);
    }
    data = UDR2;
    write_buffer(2, RX, data);
}
#endif

#if (USART3 == 1)
ISR(USART3_RX_vect){
    uint8_t data;
    if(UCSR3B & (1 << 2)) {
        data = ((UCSR3B >> 1) & 0x01);
        write_buffer(3, RX, data);
    }
    data = UDR3;
    write_buffer(3, RX, data);
}
#endif

#if (USART0 == 1)
ISR(USART0_UDRE_vect){
    uint8_t data;
    if(buffer_level(0, TX) > 0) {
        /* check if usart is in 9-bit mode */
        if(UCSR0B & (1 << 2)) {
            read_buffer(0, TX, &data);
            if(data & 0x01)
                UCSR0B |= (1<<TXB80);
            else
                UCSR0B &= ~(1<<TXB80);
        }
        read_buffer(0, TX, &data);
        UDR0 = data;
        return;
    }
    UCSR0B &= ~(1<<UDRIE0);
}
#endif

#if (USART1 == 1)
ISR(USART1_UDRE_vect){
    uint8_t data;
    if(buffer_level(1, TX) > 0) {
        /* check if usart is in 9-bit mode */
        if(UCSR1B & (1 << 2)) {
        
            read_buffer(1, TX, &data);
            if(data & 0x01)
                UCSR1B |= (1<<TXB81);
            else
                UCSR1B &= ~(1<<TXB81);
        }
        read_buffer(1, TX, &data);
        UDR1 = data;
        return;
    }
    UCSR1B &= ~(1<<UDRIE1);
}
#endif

#if (USART2 == 1)
ISR(USART2_UDRE_vect){
    uint8_t data;
    if(buffer_level(2, TX) > 0) {
        /* check if usart is in 9-bit mode */
        if(UCSR2B & (1 << 2)) {
            read_buffer(2, TX, &data);
            if(data & 0x01)
                UCSR2B |= (1<<TXB82);
            else
                UCSR2B &= ~(1<<TXB82);
        }
        read_buffer(2, TX, &data);
        UDR2 = data;
        return;
    }
    UCSR2B &= ~(1<<UDRIE2);
}
#endif

#if (USART3 == 1)
ISR(USART3_UDRE_vect){
    uint8_t data;
    if(buffer_level(3, TX) > 0) {
        /* check if usart is in 9-bit mode */
        if(UCSR3B & (1 << 2)) {
            read_buffer(3, TX, &data);
            if(data & 0x01)
                UCSR3B |= (1<<TXB83);
            else
                UCSR3B &= ~(1<<TXB83);
        }
        read_buffer(3, TX, &data);
        UDR3 = data;
        return;
    }
    UCSR3B &= ~(1<<UDRIE3);
}
#endif

