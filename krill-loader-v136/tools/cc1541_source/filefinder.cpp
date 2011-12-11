#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
	#include "XGetopt.h"
#else
	#include <getopt.h>
	#include <unistd.h>
	#define min(a,b) (((a)<(b))?(a):(b))
#endif

struct d64file
{
	char *filename;
	int track;
	int sector;
	int nrSectors;
};

int nrFiles=0;
struct d64file files[100];
char *image;

void usage()
{
	printf("Usage: filefinder image.d64\n\n");
	printf("\n");
	exit(-1);
}

static int sectorsPerTrack[]={ 21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
							   19,19,19,19,19,19,19,
							   18,18,18,18,18,18,
							   17,17,17,17,17 };

int linearSector(int track, int sector)
{
	if ((track<1) || (track>35))
	{
		fprintf(stderr, "Illegal track %d\n", track);
		exit(-1);
	}

	if ((sector<0) || (sector>=sectorsPerTrack[track-1]))
	{
		fprintf(stderr, "Illegal sector %d for track %d (max is %d)\n", sector, track, sectorsPerTrack[track-1]);
		exit(-1);
	}

	int linearSector=0;
	for (int i=0;i<track-1;i++)
		linearSector+=sectorsPerTrack[i];
	linearSector+=sector;

	return linearSector;
}

int main(int argc, char** argv)
{
	optind=opterr=0;
	while(1)
	{
		int i=getopt(argc,argv,"");
		if (i==-1)
			break;
		
		switch(i)
		{
		default:
			usage();
		}
	}

	if (optind!=argc-1)
		usage();
	else
		image=strdup(argv[optind]);

	// Load image
	unsigned char d64image[174848];
	FILE *f=fopen(image, "rb");
	if (f==NULL)
	{
		fprintf(stderr, "File not found '%s'\n", d64image);
		exit(-1);
	}

	printf("%s (%s,%s):\n", image, name,id);
	for (int i=0;i<nrFiles;i++)
	{
		printf("%3d \"%s\" => \"%s\" (SL:%d)", files[i].nrSectors, files[i].localname, files[i].filename, files[i].sectorInterleave);
		int track=files[i].track;
		int sector=files[i].sector;
		int i=0;
		while(track!=0)
		{
			if (i==0)
				printf("\n    ");
			printf("%02d/%02d ",track,sector);
			int offset=linearSector(track,sector)*256;
			track=d64image[offset+0];
			sector=d64image[offset+1];
			i++;
			if (i==10)
				i=0;
		}
		printf("\n");
	}

	return 0;
}
