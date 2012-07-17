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
 * File name: mdb.h
 * Description:  MultiDropBus methods. 
 *               See www.vending.org/technology/MDB_Version_4-2.pdf for Protocoll information.
 *   
 * @author bouni
 * @email bouni@owee.de  
 *   
 * @see The GNU Public License (GPL)
 */
 
#ifndef MDB_H
#define MDB_H

#ifndef F_CPU
#define F_CPU       16000000UL
#endif // F_CPU

#define DEBUG       0

//---------------------------------------------------------------------------
//  MDB STATES
//---------------------------------------------------------------------------
enum MDB_STATES {MDB_INACTIVE,MDB_DISABLED,MDB_ENABLED,MDB_SESSION_IDLE,MDB_VENDING,MDB_REVALUE,MDB_NEGATIVE_VEND};

//---------------------------------------------------------------------------
//  MDB CMDS
//---------------------------------------------------------------------------
enum MDB_CMD {MDB_IDLE,MDB_RESET = 0x10,MDB_SETUP,MDB_POLL,MDB_VEND,MDB_READER};

//---------------------------------------------------------------------------
//  POLL REPLYS
//---------------------------------------------------------------------------
enum POLL_REPLY {	MDB_REPLY_ACK, MDB_REPLY_JUST_RESET, MDB_REPLY_READER_CFG, MDB_REPLY_DISPLAY_REQ, MDB_REPLY_BEGIN_SESSION,
                MDB_REPLY_SESSION_CANCEL_REQ, MDB_REPLY_VEND_APPROVED, MDB_REPLY_VEND_DENIED, MDB_REPLY_END_SESSION,
                MDB_REPLY_CANCELED, MDB_REPLY_PERIPHERIAL_ID, MDB_REPLY_ERROR, MDB_REPLY_CMD_OUT_SEQUENCE 
                };
                
typedef struct {
    uint8_t feature_level;
    uint8_t dispaly_cols;
    uint8_t dispaly_rows;
    uint8_t dispaly_info;
} vmcCfg_t;

typedef struct {
    uint8_t  reader_cfg;
    uint8_t  feature_level;
    uint16_t country_code;
    uint8_t  scale_factor;
    uint8_t  decimal_places;
    uint8_t  max_resp_time;
    uint8_t  misc_options;
} cdCfg_t;

typedef struct {
    uint16_t max;
    uint16_t min;
} vmcPrice_t;


typedef struct {
    uint8_t flag;
    uint16_t funds;
} start_t;

typedef struct {
    uint8_t vend_approved;
    uint8_t vend_denied;
    uint16_t vend_amount;
} result_t;


typedef struct {
    start_t start;
    result_t result;
} mdbSession_t;

uint8_t mdb_state;

void mdb_cmd_handler(void);
void mdb_reset(void);
void mdb_setup(void);
void mdb_poll(void);
void mdb_vend(void);
void mdb_reader(void);

#endif // MDB_H
