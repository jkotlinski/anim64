#ifndef CBM_DIR_H_
#define CBM_DIR_H_

/* Functions for reading directory. Such functions are also
 * available in standard library - reason to compile them here
 * is to be able to put them in EDIT_CODE segment instead of
 * default CODE segment.
 */

#define DIRENT_SIZE 17
unsigned char __fastcall__ opendir (unsigned char lfn, unsigned char device);
unsigned char __fastcall__ readdir (unsigned char lfn, unsigned char l_dirent[DIRENT_SIZE]);
void __fastcall__ closedir( unsigned char lfn);

#endif  // CBM_DIR_H_
