/*
        VertX_CacheTool -
                tool that queries/modifies the cache on a HID VertX V2000
                By Brad Antoniewicz
*/


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define VERSION 0.3


/*
Sample IdentDB Entry
 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
00 26 3F 95 00 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 FE 00 00 00 00 00 00 00

Field 0 - 3 = Card ID
Field 16 = Entry Number
Field 20 = Seems to remain constant (FE)
Field 24 = Enabled(00)/Disababled(01)?
*/

// IdentDB Offset Declarations
#define I_CARDIDL       10
#define I_ENTRYL        28
#define I_CARDIDF       0
#define I_ENTRYF        16
#define I_ENABLEDF      24


/*
Sample Access DB Entry
 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43
01 00 00 00 0F 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 28 55 25 4E FF 0B BD 72 00 00 00 00 00 00 00 00 00 00 00 00
01 00 00 00 0F 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 28 55 25 4E FF 0B BD 72 00 00 00 00 00 00 00 00 00 00 00 00
03 00 00 00 0F 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 24 95 25 4E FF 0B BD 72 00 00 00 00 00 00 00 00 08 00 00 00
05 00 00 00 0F 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C4 6C 26 4E FF 0B BD 72 00 00 00 00 00 00 00 00 08 00 00 00
07 00 00 00 0F 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 60 76 26 4E FF 0B BD 72 00 00 00 00 00 00 00 00 08 00 00 00
09 00 00 00 0F 00 00 00 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C0 9D 26 4E FF 0B BD 72 00 00 00 00 00 00 00 00 20 00 00 00
0B 00 00 00 0F 00 00 00 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 BC A1 26 4E FF 0B BD 72 00 00 00 00 00 00 00 00 20 00 00 00
Field 0 = Entry Number (Matches IdentDB)
Field 4 = Seems to remain constant (0F)
Field 8 = Door Access (Defined in /mnt/flash/config/DoorGroups)
Field 40 = Enabled? 08 = Disabled accounts, 00 and 20? maybe enabled?
*/

// AccessDB Offset Declarations
#define A_ENTRYL        44
#define A_ENTRYF        0
#define A_DOORSF        8
#define A_UNKNF         40
#define MAX_FILESIZE 256

struct rec {
        uint8_t position;
};

struct AccessDB {
        uint8_t record[A_ENTRYL];
};

struct AccessDB accessdb[256];

struct IdentDB {
        uint8_t record[I_ENTRYL];
};
struct IdentDB identdb[256];

void help(char name[]) {
        printf("Options:\n");
        printf("\t-c <ID>\t\tID Value (10 Hex)\n");
        printf("\t-i <IdentDB>\tPath to IdentDB (default: /mnt/flash/config/IdentDB)\n");
        printf("\t-a <AccessDB>\tPath to AccessDB (default: /mnt/flash/config/AccessDB)\n");
        printf("\t-p\t\tDump data from AccessDB and IdentDB\n");
        printf("\t-v\t\tVerbose\n");
//      printf("\t-b\t\tBackup IdentDB and AccessDB\n");
        printf("\t-r\t\tRestart ident and access tasks\n");
        printf("\t-m\t\tMake new AccessDB and IdentDBs but dont modify the originals\n");
        printf("Usage:\n");
        printf("\t %s -p\n",name);
        printf("\t %s -c 0000000000 -b -r \n",name);

}

int write_file(char *file, char *type,int verbose,int entries, int makenew) {
        FILE *file_ptr;
        char append[6] = "-new";
        char newfile[MAX_FILESIZE + strlen(append)];
        int x,y;
        uint8_t endofentry = 0;
        strncpy(newfile,file,MAX_FILESIZE);
        if (makenew)
                strncat(newfile,append,6);

        printf("Saving updated DB to: %s\n",newfile);

        file_ptr = fopen(newfile,"wb");
        if (!file_ptr) {
                printf("ERROR: Could not open %s\n",newfile);
                return -1;
        }
        if (type == "a") {
                fwrite(&accessdb,sizeof(struct AccessDB),entries + 1,file_ptr);
                fwrite(&endofentry,sizeof(uint8_t),1,file_ptr);
        } else if (type == "i") {
                fwrite(&identdb,sizeof(struct IdentDB),entries + 1,file_ptr);
                fwrite(&endofentry,sizeof(uint8_t),1,file_ptr);
        } else {
                printf("ERROR: Unknown Type!\n");
        }

        fclose(file_ptr);


}

int update_db(char *type, int entries, int verbose, uint8_t *cardid) {
        uint8_t lastentry;
        int x;
        printf("Adding entry into cache\n");
        if (type == "a") {
                lastentry = accessdb[entries].record[A_ENTRYF];
                if (verbose)
                        printf("Last Entry: %02x\n",lastentry);
                lastentry++;
                if (verbose)
                        printf("Using Entry: %02x\n",lastentry);

                printf("Building AccessDB Entry....\n");
                for (x=0; x < A_ENTRYL; x++) {
                        if (x == A_ENTRYF)
                                accessdb[entries+1].record[x] = lastentry;
                        else if (x == A_DOORSF)
                                // 02 = All doors in my DoorGroups file
                                accessdb[entries+1].record[x] = 02;
                        else if (x == A_UNKNF)
                                accessdb[entries+1].record[x] = 00;
                        else
                                accessdb[entries+1].record[x] = accessdb[entries].record[x];
                        if (verbose)
                                printf("%d.",x);
                }
                if (verbose) {
                        printf("\n");
                        for (x=0; x < A_ENTRYL; x++) {
                                printf("%02x",(unsigned int)accessdb[entries+1].record[x]);
                        }
                        printf("\n");
                }

        } else if (type == "i") {
              lastentry = identdb[entries].record[I_ENTRYF];
              if (verbose)
                        printf("Last Entry: %02x\n",lastentry);
                lastentry++;
                if (verbose)
                        printf("Using Entry: %02x\n",lastentry);

                printf("Building IdentDB Entry....\n");
                for (x=0; x < I_ENTRYL; x++) {
                        if (x == I_ENTRYF)
                                identdb[entries+1].record[x] = lastentry;
                        else if (x >= I_CARDIDF && x <(I_CARDIDF + I_CARDIDL)/2 )
                                identdb[entries+1].record[x] = cardid[x-I_CARDIDF];
                        else
                                identdb[entries+1].record[x] = identdb[entries].record[x];
                        if (verbose)
                                printf("%d.",x);
                }
                if (verbose) {
                        printf("\n");
                        for (x=0; x < I_ENTRYL; x++) {
                                printf("%02x",(unsigned int)identdb[entries+1].record[x]);
                        }
                        printf("\n");
                }

        } else {
                printf("ERROR: Unknown Type!! \n");
        }
}


int parse_db(char *type, int entries, int verbose) {
        int x=0,y=0;

        if (type == "a") {
                printf("Parsing AccessDB\n");
                for (x=0; x <= entries; x++) {
                        printf("\tDB ID: %02x | ",(unsigned int)accessdb[x].record[A_ENTRYF]);
                        printf("Doors: %02x\n",(unsigned int) accessdb[x].record[A_DOORSF]);
                }

        } else if (type == "i") {
                printf("Parsing IdentDB\n");
                for (x=0; x <= entries; x++) {
                        printf("\tDB ID: %02x | ",(unsigned int)identdb[x].record[I_ENTRYF]);
                        printf("Card ID: ");
                        for (y=0; y<I_CARDIDL/2; y++)
                                printf("%02x ",(unsigned int) identdb[x].record[y]);
                        printf(" | Enabled: ");
                        if(identdb[x].record[I_ENABLEDF] == 00)
                                printf("Yes! [%02x]",identdb[x].record[I_ENABLEDF]);
                        else if (identdb[x].record[I_ENABLEDF] == 01)
                                printf("No [%02x]",identdb[x].record[I_ENABLEDF]);
                        else
                                 printf("Unknown [%02x]",identdb[x].record[I_ENABLEDF]);
                        printf("\n");

                }
        } else if (type == "c") {
                printf("Processing Data from AccessDB and IdentDB\n");
                for (x=0; x <= entries; x++) {
                        printf("\tDB ID: %02x | ",(unsigned int)identdb[x].record[I_ENTRYF]);
                        printf("Card ID: ");
                        for (y=0; y<I_CARDIDL; y++)
                                printf("%02x ",(unsigned int) identdb[x].record[y]);

                        if (identdb[x].record[I_ENTRYF] == accessdb[x].record[A_ENTRYF])
                                printf("\t| Doors: %02x | ",(unsigned int) accessdb[x].record[A_DOORSF]);
                        else
                                printf("\n\nAccessDB and IdentDB entry MisMatch!\nSomething is wrong\n");

                        printf("Enabled: ");
                        if(identdb[x].record[I_ENABLEDF] == 00)
                                printf("Yes! [%02x]",identdb[x].record[I_ENABLEDF]);
                        else if (identdb[x].record[I_ENABLEDF] == 01)
                                printf("No [%02x]",identdb[x].record[I_ENABLEDF]);
                        else
                                 printf("Unknown [%02x]",identdb[x].record[I_ENABLEDF]);
                        printf("\n");

                }


        } else {
                printf("ERROR: Unknown Type!! \n");
        }
}


int read_file(char *file, char *type,int verbose) {
        int OFFSET=0;
        FILE *file_ptr;
        int counter=1,location=0,entry=0,x=0,y=0,z=0;
        struct rec my_record;

        file_ptr = fopen(file,"rb");
        if(!file_ptr) {
                printf("ERROR: Could not open %s\n",file);
                return -1;
        }

        if (type == "a") {
                printf("Reading AccessDB\n");

                if (verbose)
                        printf("AccessDB Raw:\n\t");

                while (!feof(file_ptr) && entry < 255) {
                        if (counter == 1 || counter % A_ENTRYL == 1) {
                                if (verbose)
                                        printf("\n\tEntry %d:",entry);
                                entry++;
                                counter=1;
                        }
                        fread(&my_record,sizeof(struct rec),1,file_ptr);
                        if (verbose)
                                printf("%02x",my_record.position);
                        accessdb[entry - 1].record[counter - 1] = my_record.position;
                        counter++;
                }
                accessdb[entry].record[0] = 0xFF;
                if (verbose)
                        printf("\n");
        } else if (type == "i") {
                printf("Reading IdentDB\n");

                if (verbose)
                        printf("IdentDB Raw:\n\t");

               while (!feof(file_ptr) && entry < 255) {
                        if (counter == 1 || counter % I_ENTRYL == 1) {
                                if (verbose)
                                        printf("\n\tEntry %d:",entry);
                                entry++;
                                counter=1;
                        }
                        fread(&my_record,sizeof(struct rec),1,file_ptr);
                        if (verbose)
                                printf("%02x",my_record.position);
                        identdb[entry - 1].record[counter - 1] = my_record.position;
                        counter++;
                }
                identdb[entry].record[0] = 0xFF;
                if (verbose)
                        printf("\n");

        } else if (type == "c") {
                printf("Custom Type Defined: Pulling\n");

        } else {
                printf("ERROR: Unknown Type!\n");
        }
        fclose(file_ptr);
        return entry - 2;

}


int main(int argc, char *argv[]) {
        int c=0, x=0, y=0, l=0, ch=0, a_entries=0,i_entries=0,d=0;
        uint8_t card_val[I_CARDIDL];
        char accessDB_file[MAX_FILESIZE] = "/mnt/flash/config/AccessDB";
        char identDB_file[MAX_FILESIZE] = "/mnt/flash/config/IdentDB";
        char tmp[3];
        char buff[I_CARDIDL];
        int makenew=0,parse=0,verbose=0, restart=0,backup=0;
        char backuptag[5] = "-bak";
        char backupfile[MAX_FILESIZE + strlen(backuptag)];


        printf("HID VertX V2000 IdentDB/AccessDB Tool v%1.1f\n",VERSION);
        printf("By brad a.\n");
        printf("---------------------------------\n");

        if (argc < 2 ) {
                help(argv[0]);
                return 0;
        }

        for ( x = 0; x < argc; x++) {

                switch( (int)argv[x][0]) {
                        case '-':
                                l = strlen(argv[x]);
                                for ( y = 1; y < l; ++y) {
                                        ch = (int)argv[x][y];
                                        switch(ch) {
                                                case 'c':
                                                        if (strlen(argv[x+1]) == I_CARDIDL) {
                                                                for(c=0;c<I_CARDIDL;c+=2) {
                                                                        tmp[0] = argv[x+1][c];
                                                                        tmp[1] = argv[x+1][c+1];
                                                                        card_val[d] = strtol(tmp,NULL,16);
                                                                        d++;
                                                                }

                                                                printf("Using Card Value: ");
                                                                for (c=0;c<I_CARDIDL/2;c++)
                                                                        printf(" %02x", card_val[c]);
                                                                printf("\n");
                                                                
                                                        } else {
                                                                printf("Value provided for CardID is not the correct size.\n");
                                                                return -1;
                                                        }

                                                        break;
                                                case 'i':
                                                        if (strlen(argv[x+1]) < MAX_FILESIZE)
                                                                strncpy(identDB_file, argv[x+1],MAX_FILESIZE);
                                                        break;
                                                case 'a':
                                                        if (strlen(argv[x+1]) < MAX_FILESIZE)
                                                                strncpy(accessDB_file, argv[x+1], MAX_FILESIZE);
                                                        break;
                                                case 'p':
                                                        parse=1;
                                                        break;
                                                case 'v':
                                                        verbose=1;
                                                        break;
                                                case 'm':
                                                        makenew=1;
                                                        break;
                                                case 'b':
                                                        backup=1;
                                                        break;
                                                case 'r':
                                                        restart=1;
                                                        break;
                                                default:
                                                        help(argv[0]);
                                                        return 1;
                                        }
                                }
                        break;
                }
        }
        if ((makenew && backup) || (makenew && restart)) {
                printf("Cannot use -m and -b or -m and -r together\n");
                return -1;
        }
        printf("AccessDB Location: %s\n",accessDB_file);
        printf("IdentDB Location: %s\n",identDB_file);

        if (parse) {
                a_entries=read_file(accessDB_file,"a",verbose);
                i_entries=read_file(identDB_file,"i",verbose);

                if (verbose) {
                        printf("AccessDB Entries: %d\n",a_entries);
                        printf("IdentDB Enries: %d\n",i_entries);
                }
                if (i_entries == a_entries) {
                        if (verbose) {
                                parse_db("a",a_entries,verbose);
                                parse_db("i",i_entries,verbose);
                        }
                        parse_db("c",a_entries,verbose);
                } else if (a_entries != -1) {
                         parse_db("a",a_entries,verbose);
                } else if (i_entries != -1) {
                         parse_db("i",i_entries,verbose);
                }
        } else {

                a_entries=read_file(accessDB_file,"a",verbose);
                i_entries=read_file(identDB_file,"i",verbose);

                if (i_entries == a_entries && i_entries != -1 && a_entries != -1) {
                        update_db("a", a_entries,verbose, card_val);
                        update_db("i", i_entries,verbose, card_val);
                        if (backup) {
                                strncpy(backupfile,accessDB_file,MAX_FILESIZE);
                                strncat(backupfile,backuptag,strlen(backuptag));
                                printf("Backing up %s to %s\n",accessDB_file,backupfile);
                                system("ls");
                                strncpy(backupfile,identDB_file,MAX_FILESIZE);
                                strncat(backupfile,backuptag,strlen(backuptag));
                                printf("Backing up %s to %s\n",identDB_file,backupfile);
                                system("ls");
                        }
                        write_file(accessDB_file,"a",verbose,a_entries+1,makenew);
                        write_file(identDB_file,"i", verbose, i_entries+1,makenew);
                        if (restart) {
                                printf("Restarting /etc/init.d/access\n");
                                system("/etc/init.d/access restart");
                                printf("Restarting /etc/init.d/ident\n");
                                system("/etc/init.d/ident restart");
                        }

                } else {
                        printf("AccessDB and IdentDB Checks Failed!\n");
                        printf("Quitting\n");
                        return -1;
                }
        }




  return 0;
}

