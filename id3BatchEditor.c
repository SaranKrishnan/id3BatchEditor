/*
   Copyrights Saran Krishnan <k.saran.k@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU
   General Public License along with this
   program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct tID3Header
{
    char tag[3];
    char version[2];
    char flags;
    char size[4];
}tID3Header;

typedef struct tFrameHeader
{
    char frName[4];
    char size[4];
    char flags[2];
}tFrameHeader;



char *mp3FileNames[10];
char *removeText[10];

int FileIndex = 0;
int TextIndex = 0;



char gNULL[100];

/* This function removes the sentence text from the ID3
 * header */

void RefineText (char * text, unsigned int id3FrameSize)
{
    char *pch;

    pch = (char *) memchr (text, removeText[0][0], id3FrameSize);

    if (pch != NULL)
    {
        if (strncmp ((text + (pch-text)), removeText[0], strlen(removeText[0])) == 0)
        {
            memset (text + (pch-text), 0, strlen(removeText[0]));

            if (memcmp (text, gNULL, id3FrameSize) == 0)
            {
                strncpy (pch, "Unknown", 8);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    tID3Header ID3Header;
    tFrameHeader FrameHeader;
    unsigned int id3TagSize;
    unsigned int id3FrameSize;
    unsigned int fileDes;
    unsigned int TmpSize = 0;

    char *buf = NULL; 
    char *tmpbuf = NULL;

    /* For holding the all the content of
    tag. Only from this buf, the individual frame data will
    be paresed. */

    int command = 0;
    while((command = getopt(argc, argv, "f:r:")) != -1)
    {

        switch(command){
            case 'f':
                optind--;
                for( ;optind < argc && *argv[optind] != '-'; optind++)
                {
                    mp3FileNames[FileIndex] = (char *)malloc(sizeof(char) * 
                            strlen(argv[optind]) + 1);

                    strncpy (mp3FileNames[FileIndex], argv[optind],
                            strlen(argv[optind]));

                    FileIndex++;
                }
                break;
            case 'r':
                optind--;
                for( ;optind < argc && *argv[optind] != '-'; optind++)
                {
                    if (TextIndex != 0)
                    {
                        printf ("Currently one string can be removed \r\n");
                        break;
                    }

                    removeText[TextIndex] = (char *)malloc(sizeof(char) * 
                            strlen(argv[optind]) + 1);

                    strncpy (removeText[TextIndex], argv[optind],
                            strlen(argv[optind]));

                    TextIndex++;
                }
                break;
        } 
    }

    FileIndex--;
    TextIndex--;

    /* The last Index won't point to anything. That is the
     * reason to decrement by one */




    while (FileIndex >= 0)
    {
        fileDes = open(mp3FileNames[FileIndex], O_RDONLY);


        if (fileDes == 0)
        perror("Unable to open file \r\n");
        /* Check for 0 */

        read (fileDes, &ID3Header, sizeof(tID3Header));
        /* Check Return value */

        if (strncmp (ID3Header.tag, "ID3", 3) != 0)
        {
            printf ("This MP3 doesn't support ID3v2\r\n");
            close(fileDes);
            argc--;
            continue;
        }

        /* For now ignoring the version and flags */


        id3TagSize = (ID3Header.size[3] & 0xFF) | ((ID3Header.size[2] & 0xFF) <<
                7 ) | ((ID3Header.size[1] & 0xFF) << 14 ) | ((ID3Header.size[0] 
                        & 0xFF) << 21 );

#if 0
        printf ("Version %d %d\r\n", ID3Header.version[0], ID3Header.version[1]);
        printf ("Flag %d\r\n", ID3Header.flags);
#endif

        /* Allocate memory for buf to hold the data */

        buf = (char *) malloc (sizeof(char) * id3TagSize);
        /* Check for NULL */

        read (fileDes, buf, id3TagSize);
        /* Check Error value */

        close (fileDes);

        /* The Opened file can be closed. Later it will be
         * opened in Write Mode */ 

        memcpy (&FrameHeader, buf, sizeof(tFrameHeader));
        tmpbuf = buf + sizeof(tFrameHeader);
        TmpSize = TmpSize + 10;


        do
        {
            id3FrameSize = (FrameHeader.size[3] & 0xFF) | ((FrameHeader.size[2] & 0xFF) <<
                    8 ) | ((FrameHeader.size[1] & 0xFF) << 16 ) | ((FrameHeader.size[0] 
                            & 0xFF) << 24 );

            /* Remove unwanted text */

            if ((strncmp (&FrameHeader, "TIT2", 4)
            == 0) || (strncmp (&FrameHeader, "TALB", 4) == 0) ||
            (strncmp (&FrameHeader, "TPE1", 4) == 0))
            {
                /* Removes the unwanted field the mp3 File */
                RefineText (tmpbuf, id3FrameSize);
            }

            tmpbuf += id3FrameSize;
            TmpSize += id3FrameSize;

            memcpy (&FrameHeader, tmpbuf, sizeof(tFrameHeader));
            tmpbuf += sizeof(tFrameHeader);
            TmpSize += 10;

            if (FrameHeader.frName[0] < 'A' ||
                    FrameHeader.frName[1] > 'Z')
                break;
        }while (TmpSize < id3TagSize);

        fileDes = open(mp3FileNames[FileIndex], O_WRONLY);
        
        if (fileDes == 0)
        perror("Unable to open file \r\n");

        printf ("Processing File %s\r\n",mp3FileNames[FileIndex]);

        write (fileDes, &ID3Header, sizeof(tID3Header));
        write (fileDes, buf, id3TagSize);

        memset (&ID3Header, 0, sizeof(tID3Header));
        memset (&FrameHeader, 0, sizeof(tFrameHeader));

        free(buf);
        buf = NULL;
        tmpbuf = NULL;
        close (fileDes);
        TmpSize = 0;
        free(mp3FileNames[FileIndex]);
        FileIndex--;
    }

    return 0;
}
