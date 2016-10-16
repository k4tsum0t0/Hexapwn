/**

	zynos rom-0 config password retriever
	http://piotrbania.com


	Sample LZS unpacker for rom-0 files (those files are used to
	store your valuable information by the router). This proggy
	only focuses on retrieving the password from the rom-0 file.


	heavily based on:
	 - http://git.kopf-tisch.de/?p=zyxel-revert;a=summary
	 - https://github.com/OmerMor/SciStudio/tree/master/scistudio

**/



#include <stdio.h>
#include <stdlib.h>
#include <memory.h>



typedef unsigned short U16;
typedef unsigned long U32;
typedef unsigned char UCHAR;

typedef struct _lzs_struct
{
	UCHAR	*Src;
	UCHAR	*Dest;
	UCHAR	*DestNew;
	U32		SrcPos;
} lzs_s;


#ifdef _MSC_VER
#pragma pack(push, 1)
typedef struct romfile_struct {
	U16 version;
	U16 size;
	U16 offset;
	char name[14];
} rom_s;
#pragma pack(pop)
#endif

#ifdef __GNUC__
typedef struct __attribute__((packed)) romfile_struct {
	U16 version;
	U16 size;
	U16 offset;
	char name[14];
} rom_s;

#define __FUNCTION__ __func__
#define _snprintf snprintf
#endif



U16 htons(U16 x)
{
	UCHAR *s = (UCHAR*)&x;
	return (U16)(s[0] << 8 | s[1]);
}

U32 GetBits(lzs_s *Lzs, int NumOfBits)
{
	U32		Out = 0;
	int		BytePos, BitPos;

	if(NumOfBits > 0)
	{
		BytePos		=	Lzs->SrcPos / 8;
		BitPos		=	Lzs->SrcPos % 8;

		Out			=	(Lzs->Src[BytePos] << 16) | (Lzs->Src[BytePos + 1] << 8) | Lzs->Src[BytePos + 2];
		Out			=	(Out >> (24 - NumOfBits - BitPos)) & ((1L << NumOfBits) - 1);
		Lzs->SrcPos	+=	NumOfBits;
	}

	return Out;
}

int GetLen(lzs_s *Lzs)
{

	int Bits;
	int Length=2;

	do
	{
		Bits	=	GetBits(Lzs, 2);
		Length	+=	Bits;
	}
	while((Bits == 3) && (Length < 8));

	if (Length == 8)
	{
		do
		{
			Bits	=	GetBits(Lzs, 4);
			Length	+=	Bits;
		}
		while(Bits == 15);
	}

	return Length;
}


int LzsUnpack(lzs_s *Lzs)
{
	int			Tag, Offset, Len;
	UCHAR		*d, *Dict;

	d			=	Lzs->Dest;

	// unpacking loop
	while (1)
	{

		Tag		=	GetBits(Lzs, 1);

		if (Tag == 0)
		{
			// uncompressed byte
			*d++	=	(UCHAR)GetBits(Lzs, 8);
			continue;
		}


		Tag		=	GetBits(Lzs, 1);
		Offset	=	GetBits(Lzs, (Tag == 1) ? 7:11);

		if ((Tag == 1) && (Offset == 0))
		{
			// end of stream?
			
			break;
		}


		Dict	=	&d[-Offset];
	
		if (Dict < Lzs->Dest)
		{
			printf("%s: underflow error, offset=0x%08x tag=0x%08x\r\n",
				__FUNCTION__,
				Offset,
				Tag);
			break;
		}


		Len		=	GetLen(Lzs);		
		while (Len--)
			*d++	=	*Dict++;
	} 

	Lzs->DestNew		=	d;
	return 0;
}

int LzsUnpackFile(char *FileName)
{
	int			i;
	FILE		*SrcFile, *DestFile;
	UCHAR		*Src, *Dest, *Base, *d, *s;
	long		FileSize;

	lzs_s		Lzs;
	rom_s		Roms;

	char		DestFilePath[256];

	memset(&Lzs, 0, sizeof(lzs_s));


	SrcFile		=	fopen(FileName, "rb");
	if (!SrcFile)
	{
		printf("%s: unable to open file \"%s\"\r\n", __FUNCTION__, FileName);
		return -1;
	}


	fseek(SrcFile, 0, SEEK_END);
	FileSize = ftell(SrcFile);
	fseek(SrcFile, 0, SEEK_SET);
	

	// warning: this can overflow
#define DEST_SIZE	FileSize*50
	Src		=	(UCHAR*)malloc(FileSize);
	Dest	=	(UCHAR*)malloc(DEST_SIZE);

	if (!Src || !Dest)
	{
		printf("%s: unable to allocate memory, FileSize = 0x%08x\r\n", __FUNCTION__, FileSize);
		fclose(SrcFile);
		return -1;
	}

	d		=	Dest;
	s		=	Src;

	memset(Dest, 0, DEST_SIZE);
	fread(Src, 1, FileSize, SrcFile);
	fclose(SrcFile);



#define BASE_OFFSET		0x2000
#define PASS_OFFSET		0x14

	i				=	0;
	Base			=	s + BASE_OFFSET;


	while (1)
	{

		memcpy(&Roms, Base, sizeof(rom_s));
		Roms.size	=	htons(Roms.size);
		Roms.offset	=	htons(Roms.offset);

		if ((Base > (Src + FileSize)) || (Roms.name[0] == 0))
		{
			
			break;
		}

		


		if (strcmp((char*)&Roms.name, "autoexec.net") == 0)
		{
			Lzs.Dest	=	d;
			Lzs.Src		=	s + BASE_OFFSET + Roms.offset + 0xC + 4;
			Lzs.SrcPos	=	0;
			LzsUnpack(&Lzs);

			printf("admin password : %s\r\n", (Lzs.Dest + PASS_OFFSET));
		}
		else
		{
			
		}

		Base		+=	sizeof(rom_s);
	}


	_snprintf(DestFilePath, sizeof(DestFilePath) - 1, "%s.dat", FileName);

	DestFile	=	fopen(DestFilePath, "wb");
	if (DestFile)
	{
		fwrite(Dest, ((UCHAR*)Lzs.DestNew - (UCHAR*)Dest), 1, DestFile);
		fclose(DestFile);

		
	}
	else
	{
		printf("%s: error unable to dump unpacked data to \"%s\"!\r\n", __FUNCTION__, DestFilePath);
	}

	free(Dest);
	free(Src);
	return 0;
}



int main(int argc, char *argv[])
{

	


	if (argc < 2)
	{
		
		return -1;
	}

	
	LzsUnpackFile(argv[1]);



	return 0;
}



