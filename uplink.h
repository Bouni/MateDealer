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
 * File name: uplink.h
 * Description:  Communication methods for interacting with the PC. 
 *   
 * @author bouni
 * @email bouni@owee.de  
 *   
 * @see The GNU Public License (GPL)
 */
 
#ifndef UPLINK_H
#define UPLINK_H

#ifndef F_CPU
#define F_CPU       16000000UL
#endif // F_CPU

#define MAX_CMD_LENGTH      20
#define MAX_VAR             10

typedef struct {
    char* cmd;
    void(*funcptr)(char *arg);
} cmdStruct_t;	
    
void uplink_cmd_handler(void);
void parse_cmd(char *cmd);

void cmd_help(char *arg);
void cmd_info(char *arg);
void cmd_get_mdb_state(char *arg);
void cmd_start_session(char *arg);
void cmd_approve_vend(char *arg);
void cmd_deny_vend(char *arg);
void cmd_cancel_session(char *arg);
/*
void cmd_read(void);
void cmd_write(void);
*/
#endif // UPLINK_H
