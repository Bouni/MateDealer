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
 * File name: main.c
 * Description:  Main file of the MateDealer Project
 *   
 * @author bouni
 * @email bouni@owee.de  
 *   
 * @see The GNU Public License (GPL)
 */
 
#ifndef F_CPU
#define F_CPU       16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "usart.h"
#include "mdb.h"
#include "uplink.h"

int main(void) {

    setup_usart(0,38400,8,'N',1);
    setup_usart(1,9600,9,'N',1);
    
    sei();
    
    send_str_p(0,PSTR("MateDealer is up and running\r\n"));
       
    // Main Loop
    while(1) {
        mdb_cmd_handler();
        uplink_cmd_handler();
    }
    
    return 0;
}
