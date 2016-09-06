#ifndef MYCBM_H_
#define MYCBM_H_

/* Functions for reading directory. Such functions are also
 * available in standard library - reason to compile them here
 * is to be able to put them in EDIT_CODE segment instead of
 * default CODE segment.
 */

#define DIRENT_SIZE 17
unsigned char __fastcall__ opendir (unsigned char lfn, unsigned char device);
unsigned char __fastcall__ readdir (unsigned char lfn, unsigned char l_dirent[DIRENT_SIZE]);
void __fastcall__ closedir( unsigned char lfn);

unsigned int mycbm_load (const char* name, unsigned char device, void* data);
/* Loads file "name" from given device to given address or to the load
 * address of the file if "data" is the null pointer (like load"name",8,1
 * in BASIC).
 * Returns number of bytes that where loaded if loading was successful
 * otherwise 0. "_oserror" contains an errorcode then (see table below).
 */

unsigned char mycbm_save (const char* name, unsigned char device,
                        const void* data, unsigned int size);
/* Saves "size" bytes starting at "data" to a file.
 * Returns 0 if saving was successful, otherwise an errorcode (see table
 * below).
 */

unsigned char __fastcall__ mycbm_open (unsigned char lfn,
                                     unsigned char device,
                                     unsigned char sec_addr,
                                     const char* name);
/* Opens a file. Works just like the BASIC command.
 * Returns 0 if opening was successful, otherwise an errorcode (see table
 * below).
 */

void __fastcall__ mycbm_close (unsigned char lfn);
/* Closes a file */

int __fastcall__ mycbm_read (unsigned char lfn, void* buffer, unsigned int size);
/* Reads up to "size" bytes from a file to "buffer".
 * Returns the number of actually read bytes, 0 if there are no bytes left
 * (EOF) or -1 in case of an error. _oserror contains an errorcode then (see
 * table below).
 */

int __fastcall__ mycbm_write (unsigned char lfn, void* buffer, unsigned int size);
/* Writes up to "size" bytes from "buffer" to a file.
 * Returns the number of actually written bytes or -1 in case of an error.
 * _oserror contains an errorcode then (see above table).
 */

#endif  // MYCBM_H_
