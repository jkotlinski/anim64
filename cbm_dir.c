#include <cbm.h>
#include "cbm_dir.h"

#pragma codeseg("EDITCODE")
#pragma rodataseg("EDITCODE")

unsigned char __fastcall__ opendir (unsigned char lfn, unsigned char device) 
{
    unsigned char status;
    if ((status = cbm_open (lfn, device, CBM_READ, "$")) == 0) {
        if (cbm_k_chkin (lfn) == 0) {
            /* Ignore start address */
            cbm_k_basin();
            cbm_k_basin();
            if (cbm_k_readst()) {
                cbm_close(lfn);
                status = 2;
                cbm_k_clrch();
            } else {
                status = 0;
                cbm_k_clrch();
            }
        }
    }
    return status;
}



unsigned char __fastcall__ readdir (unsigned char lfn, unsigned char ent[DIRENT_SIZE])
{
    unsigned char byte, i;
    unsigned char rv;
    unsigned char is_header;

    rv = 1;
    is_header = 0;

    if (!cbm_k_chkin(lfn)) {
        if (!cbm_k_readst()) {
            /* skip 2 bytes, next basic line pointer */
            cbm_k_basin();
            cbm_k_basin();

            /* File-size */
            cbm_k_basin() | ((cbm_k_basin()) << 8);

            byte = cbm_k_basin();

            /* "B" BLOCKS FREE. */
            if (byte == 'b') {
                /* Read until end, careless callers may call us again */
                while (!cbm_k_readst()) {
                    cbm_k_basin();
                }
                rv = 2; /* EOF */
                goto ret_val;
            }

            /* reverse text shows that this is the directory header */
            if (byte == 0x12) { /* RVS_ON */
                is_header = 1;
            }

            while (byte != '\"') {
                byte = cbm_k_basin();
                /* prevent endless loop */
                if (cbm_k_readst()) {
                    rv = 3;
                    goto ret_val;
                }
            }

            i = 0;
            while ((byte = cbm_k_basin()) != '\"') {
                /* prevent endless loop */
                if (cbm_k_readst()) {
                    rv = 4;
                    goto ret_val;
                }

                if (i < DIRENT_SIZE - 1) {
                    ent[i] = byte;
                    ++i;
                }
            }
            ent[i] = '\0';

            while ((byte = cbm_k_basin()) == ' ') {
                /* prevent endless loop */
                if (cbm_k_readst()) {
                    rv = 5;
                    goto ret_val;
                }
            }

            if (is_header) {
            } else {

                cbm_k_basin();
                cbm_k_basin();

                byte = cbm_k_basin();
            }

            /* read to end of line */
            while (byte != 0) {
                byte = cbm_k_basin();
                /* prevent endless loop */
                if (cbm_k_readst()) {
                    rv = 6;
                    goto ret_val;
                }
            }

            rv = 0;
            goto ret_val;
        }
    }

ret_val:
    cbm_k_clrch();
    return rv;
}


void __fastcall__ closedir( unsigned char lfn)
{
    cbm_close(lfn);
}


