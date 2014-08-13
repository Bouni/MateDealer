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
#endif

#ifndef TRUE
#define TRUE        1
#endif

#ifndef FALSE
#define FALSE       0
#endif

#define DEBUG       2

#define MDB_USART   1

//---------------------------------------------------------------------------
//  MDB STATES
//---------------------------------------------------------------------------
enum MDB_STATES {MDB_INACTIVE,MDB_DISABLED,MDB_ENABLED,MDB_SESSION_IDLE,MDB_VENDING,MDB_REVALUE,MDB_NEGATIVE_VEND};

//---------------------------------------------------------------------------
//  MDB CMDS
//---------------------------------------------------------------------------
enum MDB_CMD {MDB_IDLE,MDB_RESET = 0x10,MDB_SETUP,MDB_POLL,MDB_VEND,MDB_READER,MDB_EXPANSION = 0x17};

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
void mdb_expansion(void);

#endif // MDB_H
