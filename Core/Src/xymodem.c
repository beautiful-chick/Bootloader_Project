/*
 * Handles the X-Modem, Y-Modem and Y-Modem/G protocols
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This file provides functions to receive X-Modem or Y-Modem(/G) protocols.
 *
 * References:
 *   *-Modem: http://www.techfest.com/hardware/modem/xymodem.htm
 *            XMODEM/YMODEM PROTOCOL REFERENCE, Chuck Forsberg
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "xymodem.h"
#include "crc16.h"
#include "serial.h"
#include "ringbuf.h"
#include "lfs.h"
#include "littlefs_port.h"

#define FLAG_FILE_RECEIVED 1<<0

int g_files_flag = 0;


#define CONFIG_XYMODEM_DEBUG

#ifdef CONFIG_XYMODEM_DEBUG
#define xy_dbg(fmt, args...) printf(fmt, ##args)
#else
#define xy_dbg(fmt, args...) do{} while(0);
#endif

#define min(x, y)  ((x)<(y) ? (x) : (y) )

/* Values magic to the protocol */
#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define BSP 0x08
#define NAK 0x15
#define CAN 0x18

#define CRC_NONE        0   /* No CRC checking */
#define CRC_ADD8        1   /* Add of all data bytes */
#define CRC_CRC16       2   /* CCCIT CRC16 */
#define MAX_CRCS        3

#define SECOND                  1000
#define MAX_RETRIES             20
#define MAX_RETRIES_WITH_CRC    5
#define MAX_CAN_BEFORE_ABORT    5

/* errno.h compatible with linux  */
#define EINVAL          22  /*  Invalid argument */
#define EBADMSG         74  /*  Not a data message */
#define EILSEQ          84  /* Illegal byte sequence */
#define ECONNABORTED    103 /* Software caused connection abort */
#define ETIMEDOUT       110 /*  Connection timed out */
#define EALREADY        114 /* Operation already in progress */

enum proto_state {
    PROTO_STATE_GET_FILENAME = 0,
    PROTO_STATE_NEGOCIATE_CRC,
    PROTO_STATE_RECEIVE_BODY,
    PROTO_STATE_FINISHED_FILE,
    PROTO_STATE_FINISHED_XFER,
};


extern uint8_t             			g_xymodem_rxbuf[1024];
extern lfs_t 						lfs;
extern lfs_file_t 					file;


/**
 * struct xyz_ctxt - context of a x/y modem (g) transfer
 *
 * @mode: protocol (XMODEM, YMODEM or YMODEM/G)
 * @crc_mode: CRC_NONE, CRC_ADD8 or CRC_CRC16
 * @state: protocol state (as in "state machine")
 * @filename : filename transmitted by sender (YMODEM* only)
 * @file_len: length declared by sender (YMODEM* only)
 * @nb_received: number of data bytes received since session open
 *               (this doesn't count resends)
 * @total_SOH: number of SOH frames received (128 bytes chunks)
 * @total_STX: number of STX frames received (1024 bytes chunks)
 * @total_CAN: nubmer of CAN frames received (cancel frames)
 */

typedef signed portBASE_TYPE (* xTransfer_t)( char cOutChar );
typedef signed portBASE_TYPE (* xReceive_t)(xComPortHandle pxPort, signed char *pcRxedChar, int tiemout);


struct xyz_ctxt {
    int                mode;
    int                crc_mode;
    enum proto_state   state;
    xTransfer_t        xPutCharFunc;
    xReceive_t         xGetCharFunc;
    char               filename[128];
    int                file_len;
    int                nb_received;
    int                next_blk;
    int total_SOH, total_STX, total_CAN, total_retries;
};




/**
 * struct xy_block - one unitary block of x/y modem (g) transfer
 *
 * @buf: data buffer
 * @len: length of data buffer (can only be 128 or 1024)
 * @seq: block sequence number (as in X/Y/YG MODEM protocol)
 */
struct xy_block {
    unsigned char buf[1024];
    int len;
    int seq;
};

/*
 * For XMODEM/YMODEM, always try to use the CRC16 versions, called also
 * XMODEM/CRC and YMODEM.
 * Only fallback to additive CRC (8 bits) if sender doesn't cope with CRC16.
 */

static const char invite_filename_hdr[MAX_PROTOS][MAX_CRCS] = {
    { 0, NAK, 'C' },    /* XMODEM */
    { 0, NAK, 'C' },    /* YMODEM */
    { 0, 'G', 'G' },    /* YMODEM-G */
};

static const char invite_file_body[MAX_PROTOS][MAX_CRCS] = {
    { 0, NAK, 'C' },    /* XMODEM */
    { 0, NAK, 'C' },    /* YMODEM */
    { 0, 'G', 'G' },    /* YMODEM-G */
};

static const char block_ack[MAX_PROTOS][MAX_CRCS] = {
    { 0, ACK, ACK },    /* XMODEM */
    { 0, ACK, ACK },    /* YMODEM */
    { 0, 0, 0 },        /* YMODEM-G */
};

static const char block_nack[MAX_PROTOS][MAX_CRCS] = {
    { 0, NAK, NAK },    /* XMODEM */
    { 0, NAK, NAK },    /* YMODEM */
    { 0, 0, 0 },        /* YMODEM-G */
};

int xymodem_handle(struct xyz_ctxt *proto);

static int is_xmodem(struct xyz_ctxt *proto)
{
    return proto->mode == PROTO_XMODEM;
}

/*The function is used to read data from a communication interface and store the read data in the buffer `buf`.
 * It is part of the YModem protocol implementation and is typically used to
 * receive data from a serial port or other communication interfaces.
 */

static int xy_gets(struct xyz_ctxt *proto, unsigned char *buf, int len, uint64_t timeout)
{
    int          i;
    signed char  ch;

    for(i=0; i<len; i++)
    {
        if( pdFALSE == proto->xGetCharFunc(NULL, &ch, (TickType_t)timeout) )
        {
            return i;

        }

        buf[i] = (unsigned char)ch;
        printf("0x%02X ", buf[i]);
    }
    printf("\r\n");
    return len;
}

static inline void xy_putc(struct xyz_ctxt *proto, char c)
{
    proto->xPutCharFunc(c);
}

static void xy_block_ack(struct xyz_ctxt *proto)
{
    char c = block_ack[proto->mode][proto->crc_mode];

    if (c)
        xy_putc(proto, c);
}

static void xy_block_nack(struct xyz_ctxt *proto)
{
    char c = block_nack[proto->mode][proto->crc_mode];

    if (c)
        xy_putc(proto, c);

    proto->total_retries++;
}

static int check_crc(unsigned char *buf, int len, int crc, int crc_mode)
{
    unsigned char crc8 = 0;
    uint16_t crc16;
    int i;

    switch (crc_mode) {
    case CRC_ADD8:
        for (i = 0; i < len; i++)
            crc8 += buf[i];
        return crc8 == crc ? 0 : -EBADMSG;
    case CRC_CRC16:
        crc16 = crc16_checksum(buf, len);
        xy_dbg("crc16: received = %x, calculated=%x\n", crc, crc16);
        return crc16 == crc ? 0 : -EBADMSG;
    case CRC_NONE:
        return 0;
    default:
        return -EBADMSG;
    }
}

/**
 * xy_read_block - read a X-Modem or Y-Modem(G) block
 * @proto: protocol control structure
 * @blk: block read
 * @timeout: maximal time to get data
 *
 * This is the pivotal function for block receptions. It attempts to receive one
 * block, ie. one 128 bytes or one 1024 bytes block.  The received data can also
 * be an end of transmission, or a cancel.
 *
 * Returns :
 *  >0           : size of the received block
 *  0            : last block, ie. end of transmission, ie. EOT
 * -EBADMSG      : malformed message (ie. sequence bi-bytes are not
 *                 complementary), or CRC check error
 * -EILSEQ       : block sequence number error wrt previously received block
 * -ETIMEDOUT    : block not received before timeout passed
 * -ECONNABORTED : transfer aborted by sender, ie. CAN
 */
static ssize_t xy_read_block(struct xyz_ctxt *proto, struct xy_block *blk, uint64_t timeout)
{
    ssize_t data_len = 0;
    int rc;
    unsigned char hdr = 0, seqs[2]={0}, crcs[2]={0};
    int crc = 0;
    bool hdr_found = 0;

    while (!hdr_found) {
        rc = xy_gets(proto, &hdr, 1, timeout);
        xy_dbg("read 0x%x(%c) -> %d\n", hdr, hdr, rc);
        if (rc <= 0)
            goto timeout;

        switch (hdr) {
        case SOH:
            data_len = 128;
            hdr_found = 1;
            proto->total_SOH++;
            break;
        case STX:
            data_len = 1024;
            hdr_found = 1;
            proto->total_STX++;
            break;
        case CAN:
            rc = -ECONNABORTED;
            if (proto->total_CAN++ > MAX_CAN_BEFORE_ABORT)
                goto out;
            break;
        case EOT:
            rc = 0;
            blk->len = 0;
            goto out;
        default:
            break;
        }
    }

    blk->seq = 0;
    rc = xy_gets(proto, seqs, 2, timeout);
    if (rc < 0)
        goto out;
    blk->seq = seqs[0];
    if (255 - seqs[0] != seqs[1])
        return -EBADMSG;

    rc = xy_gets(proto, blk->buf, data_len, timeout);
    if (rc < 0)
        goto out;
    blk->len = rc;


    switch (proto->crc_mode) {
    case CRC_ADD8:
        rc = xy_gets(proto, crcs, 1, timeout);
        crc = crcs[0];
        break;
    case CRC_CRC16:
        rc = xy_gets(proto, crcs, 2, timeout);
        crc = (crcs[0] << 8) + crcs[1];
        break;
    case CRC_NONE:
        rc = 0;
        break;
    }
    if (rc < 0)
        goto out;

    rc = check_crc(blk->buf, data_len, crc, proto->crc_mode);
    if (rc < 0)
        goto out;
    return data_len;

timeout:
    return -ETIMEDOUT;
out:
    return rc;
}

static int check_blk_seq(struct xyz_ctxt *proto, struct xy_block *blk, int read_rc)
{
    if (blk->seq == ((proto->next_blk - 1) % 256))
        return -EALREADY;
    if (blk->seq != proto->next_blk)
        return -EILSEQ;
    return read_rc;
}

extern char * strsep(char **stringp, const char *delim);

static int parse_first_block(struct xyz_ctxt *proto, struct xy_block *blk)
{
    int filename_len;
    char *str_num;

    filename_len = (int)strlen((char *)blk->buf);
    xy_dbg("Filename length: %d\n", filename_len);
    if (filename_len > blk->len)
        return -EINVAL;
    strncpy(proto->filename, (char *)blk->buf, sizeof(proto->filename));
    xy_dbg("Parsed filename: %s\n", proto->filename);
    str_num = (char *)blk->buf + filename_len + 1;
    printf("String number start: %s\n", str_num);
    strsep(&str_num, " ");
    proto->file_len = (int)strtoul((char *)blk->buf + filename_len + 1, NULL, 10);
    printf("Parsed file length: %d\n", proto->file_len);
    //g_files_flag = 1;
    return 1;
}

static int xy_get_file_header(struct xyz_ctxt *proto)
{
    struct xy_block blk;
    int tries, rc = 0;

    memset(&blk, 0, sizeof(blk));
    proto->state = PROTO_STATE_GET_FILENAME;
    proto->crc_mode = CRC_CRC16;

    for (tries = 0; tries < MAX_RETRIES; tries++)
    {
        xy_putc(proto, invite_filename_hdr[proto->mode][proto->crc_mode]);

        rc = xy_read_block(proto, &blk, 3*SECOND);
        printf("in the xy_get_file_header rc's value is %d \r\n", rc);
        printf("Block buffer (raw): ");
        for (int i = 0; i < blk.len; i++)
        {
        	printf("%02X ", blk.buf[i]);
        }
        printf("\n");
        switch (rc)
        {
        case -ECONNABORTED:
            goto fail;
        case -ETIMEDOUT:
        case -EBADMSG:
            break;
        case -EALREADY:
        default:
            proto->next_blk = 1;
            xy_block_ack(proto);
            proto->state = PROTO_STATE_NEGOCIATE_CRC;
            rc = parse_first_block(proto, &blk);
            return rc;
        }

        if (rc < 0 && tries++ >= MAX_RETRIES_WITH_CRC)
            proto->crc_mode = CRC_ADD8;
    }
    rc = -ETIMEDOUT;
fail:
    proto->total_retries += tries;
    return rc;

}

static int xy_await_header(struct xyz_ctxt *proto)
{
    int rc;

    rc = xy_get_file_header(proto);
    printf("In the xy_await_header rc's value is %d\r\n", rc);
    if (rc < 0)
        return rc;
    proto->state = PROTO_STATE_NEGOCIATE_CRC;
    printf("header received, filename=%s, file length=%d\r\n", proto->filename, proto->file_len);
    lfs_file_open(&lfs, &file, proto->filename, LFS_O_WRONLY | LFS_O_CREAT);
    if ( !proto->filename[0] )
        proto->state = PROTO_STATE_FINISHED_XFER;

    proto->nb_received = 0;
    return rc;
}

static void xy_finish_file(struct xyz_ctxt *proto)
{
    proto->state = PROTO_STATE_FINISHED_FILE;
}

int xymodem_handle(struct xyz_ctxt *proto)
{
    struct xy_block blk;
    int rc = 0, xfer_max, len = 0, again = 1, remain;
    int crc_tries = 0, same_blk_retries = 0;
    char invite;
    char fpath[64];

    __NOP();

    while (again) {
        __NOP();
        switch (proto->state) {
            case PROTO_STATE_GET_FILENAME:
                crc_tries = 0;
                __NOP();
                rc = xy_await_header(proto);

                if (rc < 0)
                    goto out;
                continue;
            case PROTO_STATE_FINISHED_FILE:
            	g_files_flag = 1;
            	proto->state = PROTO_STATE_FINISHED_XFER;

                xy_putc(proto, ACK);

                continue;
            case PROTO_STATE_FINISHED_XFER:
                again = 0;
                rc = 0;
                goto out;
            case PROTO_STATE_NEGOCIATE_CRC:
                invite = invite_file_body[proto->mode][proto->crc_mode];
                proto->next_blk = 1;
                if (crc_tries++ > MAX_RETRIES_WITH_CRC)
                    proto->crc_mode = CRC_ADD8;

                xy_putc(proto, invite);


                /* Fall through */
            case PROTO_STATE_RECEIVE_BODY:
                rc = xy_read_block(proto, &blk, 3*SECOND);
                if (rc > 0) {
                    rc = check_blk_seq(proto, &blk, rc);
                    proto->state = PROTO_STATE_RECEIVE_BODY;
                }
                break;
        }

        if (proto->state != PROTO_STATE_RECEIVE_BODY)
            continue;

        switch (rc) {
            case -ECONNABORTED:
                goto out;
            case -ETIMEDOUT:
                if (proto->mode == PROTO_YMODEM_G)
                    goto out;
                xy_block_nack(proto);
                break;
            case -EBADMSG:
            case -EILSEQ:
                if (proto->mode == PROTO_YMODEM_G)
                    goto out;
                xy_block_nack(proto);
                break;
            case -EALREADY:
                xy_block_ack(proto);
                break;
            case 0:
                xy_finish_file(proto);
                break;
            default:
                remain = proto->file_len - proto->nb_received;
                if (is_xmodem(proto))
                    xfer_max = blk.len;
                else
                    xfer_max = min(blk.len, remain);

                blk.buf[xfer_max] = '\0';
                printf(">>>File contains %d bytes: %s\n", xfer_max, blk.buf);
                lfs_file_write(&lfs, &file, blk.buf, xfer_max);
                lfs_sync(&lfs);
                //rc = write(proto->fd, blk.buf, xfer_max);
                proto->next_blk = ((blk.seq + 1) % 256);
                proto->nb_received += rc;
                len += rc;
                xy_block_ack(proto);
                break;
        }
        if (rc < 0)
            same_blk_retries++;
        else
            same_blk_retries = 0;
        if (same_blk_retries > MAX_RETRIES)
            goto out;
        if (g_files_flag)
        {
        	printf("File transfer completed.\n");
            break;
        }
    }
out:
    return rc;
}

static int xymodem_open(struct xyz_ctxt *proto, int mode)
{
    if( !proto )
        return -1;

    memset(proto, 0, sizeof(*proto));

    proto->mode = mode;
    proto->crc_mode = CRC_CRC16;

    proto->xGetCharFunc = xSerialGetChar;
    proto->xPutCharFunc = xSerialPutChar;

    if (is_xmodem(proto)) {
        proto->state = PROTO_STATE_NEGOCIATE_CRC;
    } else {
        proto->state = PROTO_STATE_GET_FILENAME;
    }

    return 0;
}

static void xymodem_close(struct xyz_ctxt *proto)
{
    printf("\nxyModem - %d(SOH)/%d(STX)/%d(CAN) packets, %d retries\n",
           proto->total_SOH, proto->total_STX,
           proto->total_CAN, proto->total_retries);
}

int do_load_ymodem(void)
{
    struct xyz_ctxt proto;
    int rc = 0;

    xymodem_open(&proto, PROTO_YMODEM);
    printf("Open the ymodem protocol okay\r\n");

    g_files_flag = 0;



        do {
            rc = xymodem_handle(&proto);
        } while (rc > 0);


    printf(" the firmware file upload over.\r\n");

    xymodem_close(&proto);

    return rc < 0 ? rc : 0;
}


