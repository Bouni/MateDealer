/**
 * Project: MateDealer
 * File name: mdb.c
 * Description:  MultiDropBus methods. 
 *               See www.vending.org/technology/MDB_Version_4-2.pdf for Protocoll information.
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
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include "usart.h"
#include "uplink.h"
#include "mdb.h"

uint8_t mdb_state = MDB_INACTIVE;
uint8_t mdb_poll_reply = MDB_REPLY_ACK;
uint8_t mdb_active_cmd = MDB_IDLE;

uint8_t reset_done = FALSE;

char debug_buffer[24];

extern volatile uint8_t cmd_var[MAX_VAR];

vmcCfg_t vmc = {0,0,0,0};
vmcPrice_t price = {0,0};

cdCfg_t cd = {
    0x01,   // Reader CFG (constant)
    0x01,   // Feature Level [1,2,3]
    0x1978, // Country Code
    0x01,   // Scale Factor
    0x02,   // Decimal Places
    0x05,   // max Response Time
    0x00    // Misc Options
    };
    
mdbSession_t session = {
    {0,0},
    {0,0,0}
};

void mdb_cmd_handler(void) {
            
    switch(mdb_active_cmd) {
    
        case MDB_IDLE:
            // Wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 2) return; 
            // read data from buffer
            uint16_t data = recv_mdb(MDB_USART);
            // if modebit is set and command is in command range for cashless device
            if((data & 0x100) == 0x100 && MDB_RESET <= (data ^ 0x100) && (data ^ 0x100) <= MDB_EXPANSION) {
                //Set command as active command
                mdb_active_cmd = (data ^ 0x100);
                if(!reset_done && mdb_active_cmd != MDB_RESET) {
                    mdb_active_cmd = MDB_IDLE;
                }
            }
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("RX:"));
            itoa(data, debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif
        break;
        
        case MDB_RESET:
            mdb_reset();
        break;
        
        case MDB_SETUP:
            mdb_setup();
        break;
        
        case MDB_POLL:
            mdb_poll();
        break;
        
        case MDB_VEND:
            mdb_vend();
        break;
        
        case MDB_READER:
            mdb_reader();
        break;

        case MDB_REVALUE:
            // Not yet implemented
        break;
        
        case MDB_EXPANSION:
            mdb_expansion();
        break;
    }
}

void mdb_reset(void) {
    
    // Wait for enough data in buffer to proceed reset
    if(buffer_level(MDB_USART,RX) < 2) return; 

    #if DEBUG == 1
    send_str_p(UPLINK_USART, PSTR("RESET\r\n"));
    #endif
   
    uint16_t checksum = recv_mdb(MDB_USART);

    #if DEBUG == 1
    send_str_p(UPLINK_USART, PSTR("RX:"));
    itoa(checksum, debug_buffer, 16);
    send_str(UPLINK_USART, debug_buffer);
    send_str_p(UPLINK_USART, PSTR("\r\n"));
    #endif
    
    // validate checksum
    if(checksum != MDB_RESET) {
        mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_ACK;
        send_str_p(UPLINK_USART,PSTR("Error: invalid checksum for [RESET]\r\n"));
        return;
    }

    // Reset everything
    vmc.feature_level = 0;
    vmc.dispaly_cols = 0;
    vmc.dispaly_rows = 0;
    vmc.dispaly_info = 0;
    price.max = 0;
    price.min = 0;

    // Send ACK
    send_mdb(MDB_USART, 0x100);
    
    #if DEBUG == 1
    send_str_p(UPLINK_USART, PSTR("TX:"));
    itoa(0x100, debug_buffer, 16);
    send_str(UPLINK_USART, debug_buffer);
    send_str_p(UPLINK_USART, PSTR("\r\n"));
    #endif
    
    reset_done = TRUE;
    mdb_state = MDB_INACTIVE;
    mdb_active_cmd = MDB_IDLE;
        mdb_poll_reply = MDB_REPLY_JUST_RESET;
}

void mdb_setup(void) {
    
    static uint16_t checksum = MDB_SETUP;
    static uint8_t state = 0;
    uint8_t data[6] = {0,0,0,0,0,0};
    uint8_t index = 0;
    
    if(state < 2) {
        // Wait for enough data in buffer
        if(buffer_level(MDB_USART,RX) < 12) return; 
                
        // fetch the data from buffer
        for(index = 0; index < 6; index++) {
            data[index] = (uint8_t) recv_mdb(MDB_USART);
        }
        
        #if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("RX:"));
        for(index = 0; index < 6; index++) {
            itoa(data[index], debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR(";"));
        }
        send_str_p(UPLINK_USART, PSTR("\r\n"));
        #endif
                
        // calculate checksum
        checksum += data[0] + data[1] + data[2] + data[3] + data[4];
        checksum = checksum & 0xFF;
        
        // validate checksum
        if(checksum != data[5]) {
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_SETUP;
            send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [SETUP]\r\n"));
            return;  
        }
        state = data[0];
    }       

    // Switch setup state
    switch(state) {
                
        // Stage 1 - config Data
        case 0:
            
        #if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("SETUP STAGE 1\r\n"));
        #endif
        
                    // store VMC configuration data
        vmc.feature_level = data[1];
        vmc.dispaly_cols  = data[2];
        vmc.dispaly_rows  = data[3];
        vmc.dispaly_info  = data[4];
        
        // calculate checksum for own configuration
        checksum = ((cd.reader_cfg + 
                     cd.feature_level +
                    (cd.country_code >> 8) +
                    (cd.country_code & 0xFF) +
                     cd.scale_factor +
                     cd.decimal_places +
                     cd.max_resp_time +
                     cd.misc_options) & 0xFF) | 0x100;

        // Send own config data
        send_mdb(MDB_USART, cd.reader_cfg);
        send_mdb(MDB_USART, cd.feature_level);
        send_mdb(MDB_USART, (cd.country_code >> 8));
        send_mdb(MDB_USART, (cd.country_code & 0xFF));
        send_mdb(MDB_USART, cd.scale_factor);
        send_mdb(MDB_USART, cd.decimal_places);
        send_mdb(MDB_USART, cd.max_resp_time);
        send_mdb(MDB_USART, cd.misc_options);
        send_mdb(MDB_USART, checksum);
        
        #if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("TX:"));

        itoa(cd.reader_cfg, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));
        
        itoa(cd.feature_level, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa((cd.country_code >> 8)), debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa((cd.country_code & 0xFF), debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa(cd.scale_factor, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa(cd.decimal_places, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa(cd.max_resp_time, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa(cd.misc_options, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        itoa(checksum, debug_buffer, 16);
        send_str(UPLINK_USART, debug_buffer);
        send_str_p(UPLINK_USART, PSTR(";"));

        send_str_p(UPLINK_USART, PSTR("\r\n"));
        #endif
        
        state = 2;
        
        // reset checksum for next stage
        checksum = MDB_SETUP;
        return;
            
       break;

        // Stage 2 - price data
        case 1:
        
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("SETUP STAGE 2\r\n"));
            #endif
            
            // store VMC price data
            price.max = (data[1] << 8) | data[2];
            price.min = (data[3] << 8) | data[4];

            // send ACK
            send_mdb(MDB_USART, 0x100);

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("TX:"));
            itoa(0x100, debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif

            // Set MDB State
            mdb_state = MDB_DISABLED;

            state = 0;
            
            checksum = MDB_SETUP;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        break;

        // ACK from VMC for MateDealer cfg data
        case 2:
            
            // Wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 2) return; 
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("SETUP WAIT FOR ACK\r\n"));
            #endif
            
            // Check if VMC sent ACK
            data[0] = recv_mdb(MDB_USART);
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("RX:"));
            itoa(data[0], debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif
            
            /*
             * The following check if VMC answers with ACK to the Setup data we send is not as in the MDB Spec defined.
             * The Sanden Vendo VDI 100-5 send the setup request twice, and ACK with 0x000 first time 
             * (as in the spec!) and 0x001 the second time !? 
             */

            if(data[0] != 0x000 && data[0] != 0x001) {
                state = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [SETUP]"));
                return;    
            }
            
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;

        break;

        // Unknown Subcommand from VMC
        default:
            send_str_p(UPLINK_USART,PSTR("Error: unknown subcommand [SETUP]\r\n"));
            state = 0;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        break;
        
    }
}

void mdb_poll(void) {
    
    static uint8_t state = 0;
    uint16_t checksum = 0;
    
    if(state == 0) {
        // Wait for enough data in buffer
        if(buffer_level(MDB_USART,RX) < 2) return; 
        
        #if DEBUG == 1
        send_str_p(UPLINK_USART, PSTR("POLL\r\n"));
        #endif
        
        // validate checksum
        if(recv_mdb(MDB_USART) != MDB_POLL) {
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
            send_str_p(UPLINK_USART,PSTR("Error: Invalid checksum [Poll]\r\n"));
            return;  
        } 
        state = 1;
    } 

    switch(mdb_poll_reply) {
        
        case MDB_REPLY_ACK:
            // send ACK
            send_mdb(MDB_USART, 0x100);
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            state = 0;
        break;

        case MDB_REPLY_JUST_RESET:
            // send JUST RESET
            if(state == 1) {
                send_mdb(MDB_USART, 0x000);
                send_mdb(MDB_USART, 0x100);
                state = 2;
            }
            // wait for the ACK
            else if(state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [JUST RESET]\r\n"));
                    return;    
                }

                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;
            }
        break;
    
        case MDB_REPLY_READER_CFG:
            // not yet implemented
        break;

        case MDB_REPLY_DISPLAY_REQ:
            // not yet implemented                      
        break;

        case MDB_REPLY_BEGIN_SESSION:
            if(session.start.flag && state == 1) {
                send_mdb(MDB_USART, 0x003);
                send_mdb(MDB_USART, (session.start.funds >> 8));
                send_mdb(MDB_USART, (session.start.funds & 0xFF));
                checksum = 0x003 + (session.start.funds >> 8) + (session.start.funds & 0xFF);
                checksum = (checksum & 0xFF) | 0x100;
                send_mdb(MDB_USART, checksum);
                state = 2;
            }
            
            else if(session.start.flag && state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    session.start.flag = 0;
                    session.start.funds = 0;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [START SESSION]\r\n"));
                    return;    
                }
                session.start.flag = 0;
                session.start.funds = 0;
                mdb_state = MDB_SESSION_IDLE;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;
            }
        break;

        case MDB_REPLY_SESSION_CANCEL_REQ:
            if(state == 1) {
                send_mdb(MDB_USART, 0x004);
                send_mdb(MDB_USART, 0x104);
                state = 2;
            }
            else if(state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    session.start.flag = 0;
                    session.start.funds = 0;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [SESSION CANCEL REQ]\r\n"));
                    return;    
                }
                session.start.flag = 0;
                session.start.funds = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;    
            }
        break;
        
        case MDB_REPLY_VEND_APPROVED:
            if(session.result.vend_approved && state == 1) {
                send_mdb(MDB_USART, 0x005);
                send_mdb(MDB_USART, (session.result.vend_amount >> 8));
                send_mdb(MDB_USART, (session.result.vend_amount & 0xFF));
                checksum = 0x005 + (session.result.vend_amount >> 8) + (session.result.vend_amount & 0xFF);
                checksum = (checksum & 0xFF) | 0x100;
                send_mdb(MDB_USART, checksum);
                state = 2;
            }
            else if(session.result.vend_approved && state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    session.result.vend_approved = 0;
                    session.result.vend_amount = 0;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [VEND APPROVE]\r\n"));
                    return;    
                }
                session.result.vend_approved = 0;
                session.result.vend_amount = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;    
            }
        break;
        
        case MDB_REPLY_VEND_DENIED:
            if(session.result.vend_denied && state == 1) {
                send_mdb(MDB_USART, 0x006);
                send_mdb(MDB_USART, 0x106);
                state = 2;
            }
            else if(session.result.vend_denied && state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    session.start.flag = 0;
                    session.start.funds = 0;
                    session.result.vend_denied = 0;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [VEND DENY]\r\n"));
                    return;    
                }
                session.start.flag = 0;
                session.start.funds = 0;
                session.result.vend_denied = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;    
            }
        break;
        
        case MDB_REPLY_END_SESSION:
            if(state == 1) {
                send_mdb(MDB_USART, 0x007);
                send_mdb(MDB_USART, 0x107);
                state = 2;
            }
            else if(state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [END SESSION]\r\n"));
                    return;    
                }
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;    
            }
        break;
        
        case MDB_REPLY_CANCELED:
            if(state == 1) {
                send_mdb(MDB_USART, 0x008);
                send_mdb(MDB_USART, 0x108);
                state = 2;
            }
            else if(state == 2) {
                // wait for enough data in Buffer
                if(buffer_level(MDB_USART,RX) < 2) return; 
                // check if VMC sent ACK
                if(recv_mdb(MDB_USART) != 0x000) {
                    mdb_active_cmd = MDB_IDLE;
                    mdb_poll_reply = MDB_REPLY_ACK;
                    state = 0;
                    send_str_p(UPLINK_USART,PSTR("Error: no ACK received on [REPLY CANCELED]\r\n"));
                    return;    
                }
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                state = 0;
                return;    
            }
        break;
        
        case MDB_REPLY_PERIPHERIAL_ID:
    
        break;
        
        case MDB_REPLY_ERROR:
    
        break;
        
        case MDB_REPLY_CMD_OUT_SEQUENCE:
    
        break;
        
    }
}

void mdb_vend(void) {
    
    static uint8_t data[6] = {0,0,0,0,0,0};
    static uint8_t state = 0;
    uint8_t checksum = MDB_VEND;
    char buffer[40];
    
    // wait for the subcommand 
    if(state == 0) {
        // wait for enough data in buffer
        if(buffer_level(MDB_USART,RX) < 2) return;   
        
        // fetch the subommand from Buffer
        data[0] = recv_mdb(MDB_USART);
        state = 1;
    }
    
    // switch through subcommands
    switch(data[0]) {   
        // vend request 
        case 0:
            // wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 10) return;     

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("VEND REQUEST\r\n"));
            #endif
            
            // fetch the data from buffer
            for(uint8_t i=1; i < 6; i++) {
                data[i] = (uint8_t) recv_mdb(MDB_USART);
            }
            
            // calculate checksum
            checksum += data[0] + data[1] + data[2] + data[3] + data[4];
            checksum &= 0xFF;
            
            // validate checksum
            if(checksum != data[5]) {
                state = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_VEND;
                send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [VEND]\r\n"));
                return;  
            }
            
            sprintf(buffer, "vend-request %d %d\r\n", (data[1] + data[2]), (data[3] + data[4]));
            send_str(0,buffer);  
                
            // send ACK
            send_mdb(MDB_USART, 0x100);
            state = 0;
            mdb_state = MDB_VENDING;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;            
        break;
        
        // vend cancel
        case 1:  
            // wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 2) return;     

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("VEND Cancel\r\n"));
            #endif
            
            // fetch the data from buffer
            data[1] = (uint8_t) recv_mdb(MDB_USART);
            
            // calculate checksum
            checksum += data[0];
            checksum &= 0xFF;
            
            // validate checksum
            if(checksum != data[1]) {
                state = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_VEND;
                send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [VEND]\r\n"));
                return;  
            }
            
            send_str_p(UPLINK_USART,PSTR("vend-cancel\r\n"));  
                
            // send ACK
            send_mdb(MDB_USART, 0x100);
            state = 0;
            mdb_state = MDB_SESSION_IDLE;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_VEND_DENIED;
            return;
        break; 
        
        // vend success
        case 2:  
            // wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 6) return;     

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("VEND SUCCESS\r\n"));
            #endif
            
            // fetch the data from buffer
            for(uint8_t i=1; i < 4; i++) {
                data[i] = (uint8_t) recv_mdb(MDB_USART);
            }
            
            // calculate checksum
            checksum += data[0] + data[1] + data[2];
            checksum &= 0xFF;
            
            // validate checksum
            if(checksum != data[3]) {
                state = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_VEND;
                send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [VEND]\r\n"));
                return;  
            }
            
            sprintf(buffer, "vend-success %d\r\n", (data[1] + data[2]));
            send_str(0,buffer);  
                
            // send ACK
            send_mdb(MDB_USART, 0x100);
            state = 0;
            mdb_state = MDB_SESSION_IDLE;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        break; 
        
        // vend failure
        case 3:  
            // wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 2) return;     

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("VEND FAILURE\r\n"));
            #endif
            
            // fetch the data from buffer
            data[1] = (uint8_t) recv_mdb(MDB_USART);
            
            // calculate checksum
            checksum += data[0];
            checksum &= 0xFF;
            
            // validate checksum
            if(checksum != data[1]) {
                state = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_VEND;
                send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [VEND]\r\n"));
                return;  
            }
            
            send_str_p(UPLINK_USART,PSTR("vend-failure\r\n"));  
                
            // send ACK
            send_mdb(MDB_USART, 0x100);
            state = 0;
            mdb_state = MDB_ENABLED;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        break; 
        
        // session complete
        case 4:  
            // wait for enough data in buffer
            if(buffer_level(MDB_USART,RX) < 2) return;     

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("VEND SESSION COMPLETE\r\n"));
            #endif
            
            // fetch the data from buffer
            data[1] = (uint8_t) recv_mdb(MDB_USART);
            
            // calculate checksum
            checksum += data[0];
            checksum &= 0xFF;
            
            // validate checksum
            if(checksum != data[1]) {
                state = 0;
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_VEND;
                send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [VEND]\r\n"));
                return;  
            }
            
            send_str_p(UPLINK_USART,PSTR("session-complete\r\n"));  
                
            // send ACK
            send_mdb(MDB_USART, 0x100);
            state = 0;
            mdb_state = MDB_ENABLED;
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        break; 
    }
}

void mdb_reader(void) {
    
    uint8_t data[2] = {0,0};
    uint8_t index = 0;
    
    // wait for enough data in buffer
    if(buffer_level(MDB_USART,RX) < 4) return;     

    // fetch the data from buffer
    for(index = 0; index < 2; index++) {
        data[index] = recv_mdb(MDB_USART);
    }
    
    // switch through subcommands
    switch(data[0]) {
        // reader disable
        case 0:
            if(data[1] != 0x14) {
                send_str_p(UPLINK_USART,PSTR("Error: checksum error [READER]\r\n"));     
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                return;
            }
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("READER DISABLE\r\n"));
            #endif
            
            // send ACK
            send_mdb(MDB_USART, 0x100);
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            mdb_state = MDB_DISABLED;
        break;
        
        // reader enable
        case 1:
            if(data[1] != 0x15) {
                send_str_p(UPLINK_USART,PSTR("Error: checksum error [READER]\r\n"));        
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                return;
            }
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("READER ENABLE\r\n"));
            #endif
            
            // send ACK
            send_mdb(MDB_USART, 0x100);
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            mdb_state = MDB_ENABLED;
        break;

        // reader cancel
        case 2:
            if(data[1] != 0x16) {
                send_str_p(UPLINK_USART,PSTR("Error: checksum error [READER]\r\n"));        
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                return;
            }
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("READER CANCEL\r\n"));
            #endif
            
            // send ACK
            send_mdb(MDB_USART, 0x100);
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_CANCELED;
            mdb_state = MDB_ENABLED;
        break;

        // unknown subcommand
        default:
            send_str_p(UPLINK_USART,PSTR("Error: unknown subcommand [READER]\r\n"));
            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            return;
        break;
    }
}

void mdb_expansion(void) {  

    static uint8_t state = 0;
    static uint8_t data[34];

    static uint8_t checksum = MDB_EXPANSION;

    switch(state) {
        case 0: // Request ID
            if(buffer_level(MDB_USART,RX) < 2) return;
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("EXPANSION\r\n"));
            #endif

            data[0] = (uint8_t) recv_mdb(MDB_USART);
            checksum += data[0];
    
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("RX:"));
            itoa(data[0], debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif
            
            state++;

        break;

        case 1: // Manufacturer code
            if(buffer_level(MDB_USART,RX) < 6) return;

            for(uint8_t i=1; i <=3; i++) {
                data[i] = (uint8_t) recv_mdb(MDB_USART);
                checksum =+ data[i];
                
                #if DEBUG == 1
                send_str_p(UPLINK_USART, PSTR("RX:"));
                itoa(data[i], debug_buffer, 16);
                send_str(UPLINK_USART, debug_buffer);
                send_str_p(UPLINK_USART, PSTR("\r\n"));
                #endif
            }
            
            state++;

        break;

        case 2: // Serial Number
            if(buffer_level(MDB_USART,RX) < 24) return;

            for(uint8_t i=4; i <=15; i++) {
                data[i] = (uint8_t) recv_mdb(MDB_USART);
                checksum =+ data[i];
                
                #if DEBUG == 1
                send_str_p(UPLINK_USART, PSTR("RX:"));
                itoa(data[i], debug_buffer, 16);
                send_str(UPLINK_USART, debug_buffer);
                send_str_p(UPLINK_USART, PSTR("\r\n"));
                #endif
            }
            
            state++;

        break;

        case 3: // Model Number
            if(buffer_level(MDB_USART,RX) < 24) return;

            for(uint8_t i=16; i <=27; i++) {
                data[i] = (uint8_t) recv_mdb(MDB_USART);
                checksum =+ data[i];

                #if DEBUG == 1
                send_str_p(UPLINK_USART, PSTR("RX:"));
                itoa(data[i], debug_buffer, 16);
                send_str(UPLINK_USART, debug_buffer);
                send_str_p(UPLINK_USART, PSTR("\r\n"));
                #endif
            }

            state++;

        break;
        
        case 4: // Software Version
            if(buffer_level(MDB_USART,RX) < 4) return;

            for(uint8_t i=28; i <=29; i++) {
                data[i] = (uint8_t) recv_mdb(MDB_USART);
                checksum =+ data[i];

                #if DEBUG == 1
                send_str_p(UPLINK_USART, PSTR("RX:"));
                itoa(data[i], debug_buffer, 16);
                send_str(UPLINK_USART, debug_buffer);
                send_str_p(UPLINK_USART, PSTR("\r\n"));
                #endif
            }

            state++;

        break;
        
        case 5: // Checksum
            if(buffer_level(MDB_USART,RX) < 2) return;   
            
            data[30] = (uint8_t) recv_mdb(MDB_USART);

            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("RX:"));
            itoa(data[30], debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif
            
            // validate checksum
            if(checksum != data[30]) {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_EXPANSION;
                state = 0;
                send_str_p(UPLINK_USART,PSTR("Error: invalid checksum [EXPANSION]\r\n"));
                return;  
            }

            state++;

        break;

        case 6: // Answer with the same data to fool the VMC
        
            checksum = 0;

            data[0] = 0x09; // Peripherial ID response

            for(uint8_t i=0; i <=29; i++) {
                send_mdb(MDB_USART,data[i]);
                checksum += data[i];        
            
                #if DEBUG == 1
                send_str_p(UPLINK_USART, PSTR("TX:"));
                itoa(data[i], debug_buffer, 16);
                send_str(UPLINK_USART, debug_buffer);
                send_str_p(UPLINK_USART, PSTR("\r\n"));
                #endif
            }

            send_mdb(MDB_USART,checksum);
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("TX:"));
            itoa(checksum, debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif

            state++;

        break;     

        case 7: // Wait for ACK
            
            if(buffer_level(MDB_USART,RX) < 2) return; 
            
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("EXPANSION WAIT FOR ACK\r\n"));
            #endif
            
            data[0] = recv_mdb(MDB_USART);
                
            #if DEBUG == 1
            send_str_p(UPLINK_USART, PSTR("RX:"));
            itoa(data[0], debug_buffer, 16);
            send_str(UPLINK_USART, debug_buffer);
            send_str_p(UPLINK_USART, PSTR("\r\n"));
            #endif
            
            if(data[0] != 0x100) {
                mdb_active_cmd = MDB_IDLE;
                mdb_poll_reply = MDB_REPLY_ACK;
                checksum = MDB_EXPANSION;
                state = 0;
                send_str_p(UPLINK_USART,PSTR("Error: VMC sent no ACK [EXPANSION]\r\n"));
                return;  
            }

            mdb_active_cmd = MDB_IDLE;
            mdb_poll_reply = MDB_REPLY_ACK;
            checksum = MDB_EXPANSION;
            state = 0;
            send_str_p(UPLINK_USART,PSTR("[EXPANSION] suceed!\r\n"));
            
        break;
    }

}

