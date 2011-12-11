
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>

#ifdef WIN32
    #include "XGetopt.h"
#else
    #include <getopt.h>
    #include <unistd.h>
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct imagefile
{
    char* localname;
    char* filename;
    int direntrysector;
    int direntryoffset;
    int sectorInterleave;
    int track;
    int sector;
    int nrSectors;
    int mode;
};

enum
{
    MODE_MIN_TRACK_MASK       = 0x3f,
    MODE_SAVETOEMPTYTRACKS    = 0x40,
    MODE_SAVECLUSTEROPTIMIZED = 0x80
};

enum image_type
{
    IMAGE_D64,
    IMAGE_D64_EXTENDED_SPEED_DOS,
    IMAGE_D64_EXTENDED_DOLPHIN_DOS,
    IMAGE_D71
};

static const int
DIRTRACK               = 18,
DIRENTRIESPERBLOCK     = 8,
DIRENTRYSIZE           = 32,
BLOCKSIZE              = 256,
TRACKLINKOFFSET        = 0,
SECTORLINKOFFSET       = 1,
FILETYPEOFFSET         = 2,
FILETYPE_PRG           = 0x82,
FILETRACKOFFSET        = 3,
FILESECTOROFFSET       = 4,
FILENAMEOFFSET         = 5,
FILENAMEMAXSIZE        = 16,
FILENAMEEMPTYCHAR      = ' ' | 0x80,
FILEBLOCKSLOOFFSET     = 30,
FILEBLOCKSHIOFFSET     = 31,
D64NUMBLOCKS           = 664 + 19,
D64NUMBLOCKS_EXTENDED  = D64NUMBLOCKS + 5 * 17,
D671UMBLOCKS           = D64NUMBLOCKS * 2,
D64SIZE                = D64NUMBLOCKS * BLOCKSIZE,
D64SIZE_EXTENDED       = D64SIZE + 5 * 17 * BLOCKSIZE,
D71SIZE                = D64SIZE * 2,
D64NUMTRACKS           = 35,
D64NUMTRACKS_EXTENDED  = D64NUMTRACKS + 5,
D71NUMTRACKS           = D64NUMTRACKS * 2,
BAM_OFFSET_SPEED_DOS   = 0xac,
BAM_OFFSET_DOLPHIN_DOS = 0xc0;


using namespace std;


void
usage()
{
    printf("Usage: cc1541 -niSsfecwxtu45q image.[d64|d71]\n\n");
    printf("-n diskname   Disk name, default='default'\n");
    printf("-i id         Disk ID, default='lodis'\n");
    printf("-S value      Default sector interleave, default=10\n");
    printf("-s value      Next file sector interleave, after each file\n");
    printf("              the interleave value falls back to the default value set by -S\n");
    printf("-f filename   Use filename as name when writing next file\n");
    printf("-e            Start next file on an empty track\n");
    printf("-r track      Restrict file blocks to the specified track or higher\n");    
    printf("-c            Save next file cluster-optimized (d71 only)\n");
    printf("-w localname  Write local file to disk, if filename is not set then the\n");
    printf("              local name is used. After file written filename is unset\n");
    printf("-x            Don't split files over dirtrack hole (default split files)\n");
    printf("-t            Use dirtrack to also store files (makes -x useless) (default no)\n");
    printf("-u numblocks  When using -t, amount of dir blocks to leave free (default=2)\n");
    printf("-4            Use tracks 35-40 with SPEED DOS BAM formatting\n");
    printf("-5            Use tracks 35-40 with DOLPHIN DOS BAM formatting\n");
    printf("-q            Be quiet\n");
    printf("\n");
    exit(-1);
}

const static int sectors_per_track[] = {
    /*  1-17 */ 21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
    /* 18-24 */ 19,19,19,19,19,19,19,
    /* 25-30 */ 18,18,18,18,18,18,
    /* 31-35 */ 17,17,17,17,17,
                21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
                19,19,19,19,19,19,19,
                18,18,18,18,18,18,
                17,17,17,17,17 };

const static int sectors_per_track_extended[] = {
    /*  1-17 */ 21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
    /* 18-24 */ 19,19,19,19,19,19,19,
    /* 25-30 */ 18,18,18,18,18,18,
    /* 31-35 */ 17,17,17,17,17,
    /* 36-40 */ 17,17,17,17,17,
                21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
                19,19,19,19,19,19,19,
                18,18,18,18,18,18,
                17,17,17,17,17,
                17,17,17,17,17 };


static bool quiet = false;

unsigned int
image_size(image_type type)
{
    switch (type) {
        case IMAGE_D64: return D64SIZE;
        case IMAGE_D64_EXTENDED_SPEED_DOS: // fall through
        case IMAGE_D64_EXTENDED_DOLPHIN_DOS: return D64SIZE_EXTENDED;
        case IMAGE_D71: return D71SIZE;
        default: return 0;
    }
}

unsigned int
image_num_tracks(image_type type)
{
    switch (type) {
        case IMAGE_D64: return D64NUMTRACKS;
        case IMAGE_D64_EXTENDED_SPEED_DOS: // fall through
        case IMAGE_D64_EXTENDED_DOLPHIN_DOS: return D64NUMTRACKS_EXTENDED;
        case IMAGE_D71: return D71NUMTRACKS;
        default: return 0;
    }
}

const int*
image_num_sectors_table(image_type type)
{
    return ((type == IMAGE_D64_EXTENDED_SPEED_DOS) || (type == IMAGE_D64_EXTENDED_DOLPHIN_DOS)) ? sectors_per_track_extended : sectors_per_track;
}


int
dirstrcmp(unsigned char* str1, unsigned char* str2)
{
    //cout << "str1: " << str1 << ", str2: " << str2 << endl;

    for (int i = 0; i < FILENAMEMAXSIZE; i++) {
        if ((str1[i] == FILENAMEEMPTYCHAR) || (str1[i] == '\0')) {
            return (str2[i] != FILENAMEEMPTYCHAR) && (str2[i] != '\0');
        }
        if ((str2[i] == FILENAMEEMPTYCHAR) || (str2[i] == '\0')) {
            return (str1[i] != FILENAMEEMPTYCHAR) && (str1[i] != '\0');
        }
        if (str1[i] != str2[i]) {
            return 1;
        }
    }

    return 0;
}

int
linear_sector(image_type type, int track, int sector)
{
    if ((track < 1) || (track > ((type == IMAGE_D64) ? D64NUMTRACKS : (type == IMAGE_D71 ? D71NUMTRACKS : D64NUMTRACKS_EXTENDED)))) {
        fprintf(stderr, "Illegal track %d\n", track);
        exit(-1);
    }

    const int* num_sectors_table = image_num_sectors_table(type);

    int num_sectors = num_sectors_table[track - 1];
    if ((sector < 0) || (sector >= num_sectors)) {
        fprintf(stderr, "Illegal sector %d for track %d (max is %d)\n", sector, track, num_sectors - 1);
        exit(-1);
    }

    int linear_sector = 0;
    for (int i = 0; i < track - 1; i++) {
        linear_sector += num_sectors_table[i];
    }
    linear_sector += sector;

    return linear_sector;
}

bool
is_sector_free(image_type type, unsigned char* image, int track, int sector, int numdirblocks = 0, int dir_sector_interleave = 0)
{
    int bam;
    unsigned char* bitmap;

    if ((type == IMAGE_D71) && (track > D64NUMTRACKS)) {
        // access second side bam
        bam = linear_sector(type, DIRTRACK + D64NUMTRACKS, 0) * BLOCKSIZE;
        bitmap = image + bam + (track - D64NUMTRACKS - 1) * 3;
    } else {
        if (((type == IMAGE_D64_EXTENDED_SPEED_DOS) || (type == IMAGE_D64_EXTENDED_DOLPHIN_DOS)) && (track > D64NUMTRACKS)) {
            track -= D64NUMTRACKS;
            bam = linear_sector(type, DIRTRACK, 0) * BLOCKSIZE + ((type == IMAGE_D64_EXTENDED_SPEED_DOS) ? BAM_OFFSET_SPEED_DOS : BAM_OFFSET_DOLPHIN_DOS);
        } else {
            bam = linear_sector(type, DIRTRACK, 0) * BLOCKSIZE;
        }
        bitmap = image + bam + track * 4 + 1;
    }

    int byte = sector >> 3;
    int bit = sector & 7;

    bool is_not_dir_block = true;
    if ((track == DIRTRACK) && (numdirblocks > 0)) {
        const int* num_sectors_table = image_num_sectors_table(type);

        int dirsector;
        int s = 2;
        for (int i = 0; is_not_dir_block && (i < numdirblocks); i++) {
            switch (i) {
                case 0:
                    dirsector = 0;
                    break;

                case 1:
                    dirsector = 1;
                    break;

                default:
                    dirsector += dir_sector_interleave;
                    if (dirsector >= num_sectors_table[track - 1]) {
                        dirsector = s;
                        s++;
                    }
                    break;
            }
            is_not_dir_block = (sector != dirsector);
        }
    }

     return is_not_dir_block && ((bitmap[byte] & (1 << bit)) != 0);
}

void
mark_sector(image_type type, unsigned char* image, int track, int sector, bool free)
{
    if (free != is_sector_free(type, image, track, sector)) {
        int bam;
        unsigned char* bitmap;

        if ((type == IMAGE_D71) && (track > D64NUMTRACKS)) {
            // access second side bam
            bam = linear_sector(type, DIRTRACK + D64NUMTRACKS, 0) * BLOCKSIZE;
            bitmap = image + bam + (track - D64NUMTRACKS - 1) * 3;

            // update number of free sectors on track
            if (free) {
                image[bam + 0xdd + track - D64NUMTRACKS - 1]++;
            } else {
                image[bam + 0xdd + track - D64NUMTRACKS - 1]--;
            }
       } else {
            if (((type == IMAGE_D64_EXTENDED_SPEED_DOS) || (type == IMAGE_D64_EXTENDED_DOLPHIN_DOS)) && (track > D64NUMTRACKS)) {
                track -= D64NUMTRACKS;
                bam = linear_sector(type, DIRTRACK, 0) * BLOCKSIZE + ((type == IMAGE_D64_EXTENDED_SPEED_DOS) ? BAM_OFFSET_SPEED_DOS : BAM_OFFSET_DOLPHIN_DOS);
            } else {
                bam = linear_sector(type, DIRTRACK, 0) * BLOCKSIZE;
            }
            bitmap = image + bam + track * 4 + 1;

            // update number of free sectors on track
            if (free) {
                image[bam + track * 4]++;
            } else {
                image[bam + track * 4]--;
            }
        }

        // update bitmap
        int byte = sector >> 3;
        int bit = sector & 7;

        if (free) {
            bitmap[byte] |= 1 << bit;
        } else {
            bitmap[byte] &= ~(1 << bit);
        }
    }
}

char*
ascii2petscii(char* str)
{
    unsigned char* ascii = (unsigned char *) str;

    while (*ascii != '\0') {
        if ((*ascii >= 'a') && (*ascii <= 'z')) {
            *ascii += 'A' - 'a';
        }

        ascii++;
    }

    return str;
}

void
update_directory(image_type type, unsigned char* image, char* header, char* id)
{
    unsigned int bam = linear_sector(type, DIRTRACK, 0) * BLOCKSIZE;

    image[bam + 0x03] = (type == IMAGE_D71) ? 0x80 : 0x00;

    // Set header and ID
    for (int i = 0; i < 16; i++) {
        if (i < strlen(header))
            image[bam + 0x90 + i] = header[i];
        else
            image[bam + 0x90 + i] = FILENAMEEMPTYCHAR;
    }

    static const char DEFAULT_ID[] = "00 2A";

    for (int i = 0; i < 5; i++)    {
        if (i < strlen(id)) {
            image[bam + 0xa2 + i] = id[i];
        } else {
            image[bam + 0xa2 + i] = DEFAULT_ID[i];
        }
    }
}

void
initialize_directory(image_type type, unsigned char* image, char* header, char* id)
{
    unsigned int bam = linear_sector(type, DIRTRACK, 0) * BLOCKSIZE;

    // Clear image
    memset(image, 0, image_size(type));

    // Write initial BAM
    image[bam + 0x00] = DIRTRACK;
    image[bam + 0x01] = 1;
    image[bam + 0x02] = 0x41;
    image[bam + 0x03] = (type == IMAGE_D71) ? 0x80 : 0x00;

    // Mark all sectors unused
    const int* num_sectors_table = image_num_sectors_table(type);
    for (int t = 1; t <= image_num_tracks(type); t++) {
        for (int s = 0; s < num_sectors_table[t - 1]; s++) {
            mark_sector(type, image, t, s, true);
        }
    }

    image[bam + 0xa0] = FILENAMEEMPTYCHAR;
    image[bam + 0xa1] = FILENAMEEMPTYCHAR;

    image[bam + 0xa7] = FILENAMEEMPTYCHAR;
    image[bam + 0xa8] = FILENAMEEMPTYCHAR;
    image[bam + 0xa9] = FILENAMEEMPTYCHAR;
    image[bam + 0xaa] = FILENAMEEMPTYCHAR;

    // Reserve space for BAM
    mark_sector(type, image, DIRTRACK, 0, false);
    if (type == IMAGE_D71) {
        mark_sector(type, image, DIRTRACK + D64NUMTRACKS, 0, false);
    }

    // first dir block
    unsigned int dirblock = linear_sector(type, DIRTRACK, 1) * BLOCKSIZE;
    image[dirblock + SECTORLINKOFFSET] = 255;
    mark_sector(type, image, DIRTRACK, 1, false);

    update_directory(type, image, header, id);
}

void
wipe_file(image_type type, unsigned char* image, unsigned int track, unsigned int sector)
{
    while (track != 0) {
        int block_offset = linear_sector(type, track, sector) * BLOCKSIZE;
        int next_track = image[block_offset + TRACKLINKOFFSET];
        int next_sector = image[block_offset + SECTORLINKOFFSET];
        memset(image + block_offset, 0, BLOCKSIZE);
        mark_sector(type, image, track, sector, true);
        track = next_track;
        sector = next_sector;
    };
}

void
create_dir_entries(image_type type, unsigned char* image, struct imagefile* files, int num_files, int dir_sector_interleave)
{
    int num_overwritten_files = 0;

    for (int i = 0; i < num_files; i++) {
        // find or create slot
        imagefile& file = files[i];

        int dirsector = 1;
        int dirblock;
        int entryOffset;
        bool found = false;
        do {
            dirblock = linear_sector(type, DIRTRACK, dirsector) * BLOCKSIZE;
            for (int j = 0; (!found) && (j < DIRENTRIESPERBLOCK); j++) {
                entryOffset = j * DIRENTRYSIZE;
                // this assumes the dir only holds PRG files
                int filetype = image[dirblock + entryOffset + FILETYPEOFFSET];
                switch (filetype) {
                    case FILETYPE_PRG:
                        if (dirstrcmp(image + dirblock + entryOffset + FILENAMEOFFSET, (unsigned char *) file.filename) == 0) {
                            wipe_file(type, image, image[dirblock + entryOffset + FILETRACKOFFSET], image[dirblock + entryOffset + FILESECTOROFFSET]);
                            num_overwritten_files++;
                            found = true;
                        }
                        break;

                    default:
                        //cout << "default found, sector " << dirsector << ", position 0x" << hex << entryOffset << endl;
                        found = true;
                        break;
                }
            }

            if (found == true) {
                //cout << "found empty slot at sector " << dirsector << ", position 0x" << hex << entryOffset << endl;
            } else {
                if (image[dirblock + TRACKLINKOFFSET] == DIRTRACK) {
                    dirsector = image[dirblock + SECTORLINKOFFSET];
                } else {
                    // allocate new dir block
                    const int* num_sectors_table = image_num_sectors_table(type);
                    int next_sector;
                    for (next_sector = dirsector + dir_sector_interleave; next_sector < dirsector + num_sectors_table[DIRTRACK - 1]; next_sector++) {
                        int findSector = next_sector % num_sectors_table[DIRTRACK - 1];
                        if (is_sector_free(type, image, DIRTRACK, findSector)) {
                            found = true;
                            next_sector = findSector;
                            break;
                        }
                    }
                    if (found == false) {
                        fprintf(stderr, "Dir track full!\n");
                        exit(-1);
                    }

                    //cout << "allocated new dir block at sector " << next_sector << endl;

                    image[dirblock + TRACKLINKOFFSET] = DIRTRACK;
                    image[dirblock + SECTORLINKOFFSET] = next_sector;

                    mark_sector(type, image, DIRTRACK, next_sector, false);

                    // initialize new dir block
                    dirblock = linear_sector(type, DIRTRACK, next_sector) * BLOCKSIZE;

                    memset(image + dirblock, 0, BLOCKSIZE);
                    image[dirblock + TRACKLINKOFFSET] = 0;
                    image[dirblock + SECTORLINKOFFSET] = 255;

                    dirsector = next_sector;
                    found = false;
                }
            }
        } while (found == false);

        // set filetype
        image[dirblock + entryOffset + FILETYPEOFFSET] = FILETYPE_PRG;

        // set filename
        for (unsigned int j = 0; j < FILENAMEMAXSIZE; j++) {
            if (j < strlen(file.filename)) {
                image[dirblock + entryOffset + FILENAMEOFFSET + j] = file.filename[j];
            } else {
                image[dirblock + entryOffset + FILENAMEOFFSET + j] = FILENAMEEMPTYCHAR;
            }
        }

        // set directory entry reference
        file.direntrysector = dirsector;
        file.direntryoffset = entryOffset;

        //cout << "\"" << file.filename << "\": sector " << dirsector << ", position 0x" << hex << entryOffset << endl;
    }

    if (num_overwritten_files > 0) {
        if (quiet == false) {
            cout << num_overwritten_files << " files out of " << num_files << " files are already existing and will be overwritten" << endl;
        }
    }
}

void
write_files(image_type type, unsigned char* image, struct imagefile* files, int num_files, bool usedirtrack, bool dirtracksplit, int numdirblocks, int dir_sector_interleave)
{
    int track = 1;
    int sector = 0;
    int bytes_to_write = 0;
    int lastTrack = track;
    int lastSector = sector;
    int lastOffset = linear_sector(type, lastTrack, lastSector) * BLOCKSIZE;

    for (int i = 0; i < num_files; i++) {
        imagefile& file = files[i];

        struct stat st;
        stat(files[i].localname, &st);

        int fileSize = st.st_size;

        unsigned char* filedata = new unsigned char[fileSize];
        FILE* f = fopen(files[i].localname, "rb");
        fread(filedata, fileSize, 1, f);
        fclose(f);

        if ((files[i].mode & MODE_MIN_TRACK_MASK) > 0) {
            track = files[i].mode & MODE_MIN_TRACK_MASK;
            if (track > image_num_tracks(type)) {
                printf("invalid minimum track %d for file %s (%s) specified\n", track, files[i].localname, files[i].filename);
                exit(-1);
           }
        }

        if ((files[i].mode & MODE_SAVETOEMPTYTRACKS) != 0) {
        
            //cout << "to empty track: " << file.localname << endl;

            // find first empty track
            bool found = false;
            while (found == false) {
                const int* num_sectors_table = image_num_sectors_table(type);
                for (int s = 0; s < num_sectors_table[track - 1]; s++) {
                    if (!is_sector_free(type, image, track, s, usedirtrack ? numdirblocks : 0, dir_sector_interleave)) {
                        if (files[i].mode & MODE_SAVECLUSTEROPTIMIZED) {
                            if (track >= D64NUMTRACKS) {
                                track = track - D64NUMTRACKS + 1;
                            } else {
                                track += D64NUMTRACKS;
                            }
                        } else {
                            track++;
                        }
                        if ((usedirtrack == false) && ((track == DIRTRACK) || ((type == IMAGE_D71) && (track == D64NUMTRACKS + DIRTRACK)))) { // .d71 track 53 is usually empty except the extra BAM block
                            track++;
                        }
                        if (track > image_num_tracks(type)) {
                            fprintf(stderr, "Disk full!\n");
                            exit(-1);
                        }
                        break;
                    } else {
                        if (s == num_sectors_table[track - 1] - 1) {
                            found = true;
                            sector = 0;
                        }
                    }
                }
            }
        }

        //cout << file.localname << ": " << track << ", " << sector << endl;

        int byteOffset = 0;
        int bytesLeft = fileSize;
        while (bytesLeft > 0) {
            // Find free track & sector, starting from current T/S forward one revolution, then the next track etc... skip dirtrack (unless -t is active)
            // If the file didn't fit before dirtrack then restart on dirtrack + 1 and try again (unless -t is active).
            // If the file didn't fit before track 36/41/71 then the disk is full.

            bool found = false;
            int findSector = 0;

            while (!found) {
                // find spare block on the current track
                const int* num_sectors_table = image_num_sectors_table(type);
                for (int s = sector; s < sector + num_sectors_table[track - 1]; s++) {
                    findSector = s % num_sectors_table[track - 1];
                    if (is_sector_free(type, image, track, findSector, usedirtrack ? numdirblocks : 0, dir_sector_interleave)) {
                        found = true;
                        break;
                    }
                }

                if (found == false) {
                    // find next track
                    sector = (sector + 5 - files[i].sectorInterleave /* some magic to make up for track seek delay */) % num_sectors_table[track - 1];
                    if (files[i].mode & MODE_SAVECLUSTEROPTIMIZED) {
                        if (track >= D64NUMTRACKS) {
                            track = track - D64NUMTRACKS + 1;
                        } else {
                            track += D64NUMTRACKS;
                        }
                    } else {
                        track++;
                    }
                    if ((usedirtrack == false) && ((track == DIRTRACK) || ((type == IMAGE_D71) && (track == D64NUMTRACKS + DIRTRACK)))) { // .d71 track 53 is usually empty except the extra BAM block
                        // Delete old fragments and restart file
                        if (!dirtracksplit) {
                            if (files[i].nrSectors > 0) {
                                int deltrack = files[i].track;
                                int delsector = files[i].sector;
                                while (deltrack != 0) {
                                    mark_sector(type, image, deltrack, delsector, true);
                                    int offset = linear_sector(type, deltrack, delsector) * BLOCKSIZE;
                                    deltrack = image[offset + 0];
                                    delsector = image[offset + 1];
                                    memset(image + offset, 0, BLOCKSIZE);
                                }
                            }

                            bytesLeft = fileSize;
                            byteOffset = 0;
                            files[i].nrSectors = 0;
                        }
                        track = DIRTRACK + 1;
                    }

                    if (track > image_num_tracks(type)) {
                        fprintf(stderr, "Disk full!\n");
                        delete filedata;
                        exit(-1);
                    }
                }
            } // while (found == false)

            sector = findSector;
            int offset = linear_sector(type, track, sector) * BLOCKSIZE;

            if (bytesLeft == fileSize) {
                files[i].track = track;
                files[i].sector = sector;
                lastTrack = track;
                lastSector = sector;
                lastOffset = offset;
            } else {
                image[lastOffset + 0] = track;
                image[lastOffset + 1] = sector;
            }

            // Write sector
            bytes_to_write = min(BLOCKSIZE - 2, bytesLeft);
						memcpy(image + offset + 2, filedata + byteOffset, bytes_to_write);

            bytesLeft -= bytes_to_write;
            byteOffset += bytes_to_write;

            lastTrack = track;
            lastSector = sector;
            lastOffset = offset;

            mark_sector(type, image, track, sector, false);

            sector += files[i].sectorInterleave;
            files[i].nrSectors++;
        }

        delete filedata;

        image[lastOffset + 0] = 0x00;
        image[lastOffset + 1] = bytes_to_write + 1;

        // update directory entry
        int entryOffset = linear_sector(type, DIRTRACK, file.direntrysector) * BLOCKSIZE + file.direntryoffset;
        image[entryOffset + FILETRACKOFFSET] = file.track;
        image[entryOffset + FILESECTOROFFSET] = file.sector;

        image[entryOffset + FILEBLOCKSLOOFFSET] = file.nrSectors & 255;
        image[entryOffset + FILEBLOCKSHIOFFSET] = file.nrSectors >> 8;
    }
}

void
print_file_allocation(image_type type, unsigned char* image, struct imagefile* files, int num_files)
{
    for (int i = 0; i < num_files; i++) {
        printf("%3d \"%s\" => \"%s\" (SL:%d)", files[i].nrSectors, files[i].localname, files[i].filename, files[i].sectorInterleave);
        int track = files[i].track;
        int sector = files[i].sector;
        int j = 0;
        while (track != 0) {
            if (j == 0) {
                printf("\n    ");
            }
            printf("%02d/%02d ", track, sector);
            int offset = linear_sector(type, track, sector) * 256;
            track = image[offset + 0];
            sector = image[offset + 1];
            j++;
            if (j == 10) {
                j = 0;
            }
        }
        printf("\n");
    }
}

void
print_bam(image_type type, unsigned char* image)
{
    const int* num_sectors_table = image_num_sectors_table(type);
    int sectorsFree = 0;
    int sectorsFreeOnDirTrack = 0;
    int sectorsOccupied = 0;
    int sectorsOccupiedOnDirTrack = 0;

    int max_track = ((type == IMAGE_D64_EXTENDED_SPEED_DOS) || (type == IMAGE_D64_EXTENDED_DOLPHIN_DOS)) ? D64NUMTRACKS_EXTENDED : D64NUMTRACKS;

    for (int t = 1; t <= max_track; t++) {

        printf("%2d: ", t);
        for (int s = 0; s < num_sectors_table[t - 1]; s++) {
            if (is_sector_free(type, image, t, s)) {
                printf("0");
                if (t != DIRTRACK) {
                    sectorsFree++;
                } else {
                    sectorsFreeOnDirTrack++;
                }
            } else {
                printf("1");
                if (t != DIRTRACK) {
                    sectorsOccupied++;
                } else {
                    sectorsOccupiedOnDirTrack++;
                }
            }
        }

        if (type == IMAGE_D71) {
            for (int i = num_sectors_table[t - 1]; i < 23; i++) {
                printf(" ");
            }

            printf("%2d: ", t + D64NUMTRACKS);
            for (int s = 0; s < num_sectors_table[t + D64NUMTRACKS - 1]; s++) {
                if (is_sector_free(type, image, t + D64NUMTRACKS, s)) {
                    printf("0");
                    if ((t + D64NUMTRACKS) != DIRTRACK) {
                        sectorsFree++;
                    } else {
                        // track 53 is usually empty except the extra BAM block
                        sectorsFreeOnDirTrack++;
                    }
                } else {
                    printf("1");
                    sectorsOccupied++;
                   }
            }
        }

        printf("\n");
    }
    printf("%3d (%d) BLOCKS FREE (out of %d (%d) BLOCKS)\n", sectorsFree, sectorsFree + sectorsFreeOnDirTrack,
                                                             sectorsFree + sectorsOccupied, sectorsFree + sectorsFreeOnDirTrack + sectorsOccupied + sectorsOccupiedOnDirTrack);
}

int
main(int argc, char* argv[])
{
    image_type type = IMAGE_D64;
    struct imagefile files[144] = { 0 };
    int nrFiles = 0;
    char* imagepath = NULL;
    char* header = (char *) "default";
    char* id     = (char *) "lodis";
    bool dirtracksplit = true;
    bool usedirtrack = false;
    int numtracks = D64NUMTRACKS;

    int defaultSectorInterleave = 10;
    int sectorInterleave = 10;
    int dir_sector_interleave = 3;
    int numdirblocks = 2;
    char* filename = NULL;
    bool clear = true;
    bool set_header = false;

    optind = opterr = 1;
    
    while (true) {
        int i = getopt(argc, argv, "n:i:S:s:f:er:cw:xtu:45q");
        if (i == -1) {
            break;
        }

        switch (i) {
            case 'n':
                header = strdup(optarg);
                set_header = true;
                break;

            case 'i':
                id = strdup(optarg);
                set_header = true;
                break;

            case 'S':
                defaultSectorInterleave = atoi(optarg);
                sectorInterleave = defaultSectorInterleave;
                break;

            case 's':
                sectorInterleave = atoi(optarg);
                break;

            case 'f':
                filename = strdup(optarg);
                break;

            case 'e':
                files[nrFiles].mode |= MODE_SAVETOEMPTYTRACKS;
                break;

            case 'r':
                i = atoi(optarg);
                if ((i < 1) || ((i & MODE_MIN_TRACK_MASK) != i)) {
                    printf("invalid minimum track %d for file %s (%s) specified\n",
                           i, files[nrFiles].localname ? files[nrFiles].localname : "", files[nrFiles].filename ? files[nrFiles].filename : "");
                    exit(-1);
                }
                files[nrFiles].mode |= i;
                break;

            case 'c':
                files[nrFiles].mode |= MODE_SAVECLUSTEROPTIMIZED;
                break;

            case 'w':
                struct stat st;
                if (stat(optarg, &st) == 0)    {
                    files[nrFiles].localname = strdup(optarg);

                    if (filename == NULL) {
                        files[nrFiles].filename = ascii2petscii(strdup(files[nrFiles].localname));
                    } else {
                        files[nrFiles].filename = filename;
                    }

                    files[nrFiles].sectorInterleave = sectorInterleave;
                    files[nrFiles].nrSectors = 0;

                    nrFiles++;
                } else {
                    fprintf(stderr, "File '%s' not found, skipping...\n", optarg);
                }

                filename = NULL;
                sectorInterleave = defaultSectorInterleave;
                break;

            case 'x':
                dirtracksplit = false;
                break;

            case 't':
                usedirtrack = true;
                break;

            case 'u':
                numdirblocks = atoi(optarg);
                break;

            case '4':
                type = IMAGE_D64_EXTENDED_SPEED_DOS;
                break;

            case '5':
                type = IMAGE_D64_EXTENDED_DOLPHIN_DOS;
                break;

            case 'q':
                quiet = true;
                break;

            default:
                usage();
        }
    }

    if (optind != argc - 1) {
        usage();
    } else {
        imagepath = strdup(argv[optind]);
    }

    if ((strlen(imagepath) >= 4) && !strcmp(imagepath + strlen(imagepath) - 4, ".d71")) {
        if ((type == IMAGE_D64_EXTENDED_SPEED_DOS) || (type == IMAGE_D64_EXTENDED_DOLPHIN_DOS)) {
            printf("extended .d71 images are not supported\n");
            exit(-1);
        }
        type = IMAGE_D71;
    }
    
    unsigned int imagesize = image_size(type);
    unsigned char* image = new unsigned char[imagesize];
    FILE* f = fopen(imagepath, "rb");
    if (f == NULL) {
        initialize_directory(type, image, header, id);
    } else {
        size_t read_size = fread(image, 1, imagesize, f);
        fclose(f);
        if (read_size != imagesize) {
            if (((type == IMAGE_D64_EXTENDED_SPEED_DOS) || (type == IMAGE_D64_EXTENDED_DOLPHIN_DOS)) && (read_size == D64SIZE)) {
                // Clear extra tracks
              memset(image + image_size(IMAGE_D64), 0, image_size(type) - image_size(IMAGE_D64));

              // Mark all extra sectors unused
              const int* num_sectors_table = image_num_sectors_table(type);
                for (int t = D64NUMTRACKS + 1; t <= image_num_tracks(type); t++) {
                    for (int s = 0; s < num_sectors_table[t - 1]; s++) {
                        mark_sector(type, image, t, s, true);
                    }
                }
            } else {
                printf("wrong filesize: expected to read %d bytes, but read %d bytes\n", imagesize, (int) read_size);
                exit(-1);
            }
        }
        if (set_header) {
            update_directory(type, image, header, id);
        }
    }

    // Create directory entries
    create_dir_entries(type, image, files, nrFiles, dir_sector_interleave);

    // Write files and mark sectors in BAM
    write_files(type, image, files, nrFiles, usedirtrack, dirtracksplit, numdirblocks, dir_sector_interleave);

    if (quiet == false) {
        printf("%s (%s,%s):\n", imagepath, header, id);
        print_file_allocation(type, image, files, nrFiles);

        print_bam(type, image);
    }

    // Save image
    f = fopen(imagepath, "wb");
    fwrite(image, imagesize, 1, f);
    fclose(f);

    delete[] image;

    return 0;
}
