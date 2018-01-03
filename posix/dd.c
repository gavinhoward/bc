#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>


#include "cutils.h"
/* 
	Copyright 2015, C. Graff  "dd" 
	
	This utility is not ready, please do not use it:
		1. conv=block breaks conv=sync and the end of the file is not synced
		2. conv=unblock favors certain block sizes, and the ends of some file are broken
		3. conv=ascii,ibm,ebcdic are all broken for binary files but apparently not text
	..Other than that it is passing all of my tests, perhaps the simplest thing would
	be to just remove conv=ascii,ibm,ebcdic,unblock,block

	TODO: Get rid of strcmp() and put the arguments into a hash table 
	      Replace rotational buffer's for loop with a memcpy() based loop 
	      
*/ 

struct glb { 
	char *opt[2]; // if= of=
	size_t convs[11];
	size_t numbs[7]; // bs= cbs= ibs= obs= skip= seek= count=
} glb;


size_t mltplrs(char *);
void dd_main();
void writeout(char *, char *, char *, int, int); 
size_t convert(size_t);
void swabchar(char *, size_t, size_t, size_t);
void blockdata(char *, char *, size_t, size_t, int);
int main(int argc, char *argv[])
{
	char *tok = NULL;
	char *postok = NULL;

	argc = argc; // argc is not used

	++argv;

	while ( *argv )
	{
		tok = strtok(*argv, "=");
		postok = strtok(NULL, "=");

		if ( strcmp(tok, "if") == 0 )    // done
			glb.opt[0] = postok;
		else if ( strcmp(tok, "of") == 0 )    // done
			glb.opt[1] = postok;
		else if ( strcmp(tok, "ibs") == 0 )   // done
			glb.numbs[0] = mltplrs(postok);
                else if ( strcmp(tok, "obs") == 0 )   // done
			glb.numbs[1] = mltplrs(postok);
		else if ( strcmp(tok, "bs") == 0 )    // done
			glb.numbs[2] = mltplrs(postok);
		else if ( strcmp(tok, "cbs") == 0 )   // done .. part
			glb.numbs[3] = mltplrs(postok);
		else if ( strcmp(tok, "skip") == 0 )  // done
			glb.numbs[4] = atoi(postok);
		else if ( strcmp(tok, "seek") == 0 )  // done
			glb.numbs[5] = atoi(postok);
		else if ( strcmp(tok, "count") == 0 ) // done
			glb.numbs[6] = atoi(postok);
		else if ( strcmp(tok, "conv") == 0 )
		{
			tok = strtok(postok, ",");
	                while ( tok != NULL )
       	         	{
                        	if ( strcmp(tok, "ascii") == 0 )
                                	glb.convs[0] = 1;
                        	else if ( strcmp(tok, "ebcdic") == 0 )
                                	glb.convs[1] = 1;
                    	    	else if ( strcmp(tok, "ibm") == 0 )
                                	glb.convs[2] = 1;
                        	else if ( strcmp(tok, "block") == 0 ) // done
                                	glb.convs[3] = 1;
                        	else if ( strcmp(tok, "unblock") == 0 )
                                	glb.convs[4] = 1;
                        	else if ( strcmp(tok, "lcase") == 0 ) //done
                                	glb.convs[5] = 1;
                        	else if ( strcmp(tok, "ucase") == 0 ) // done
                                	glb.convs[6] = 1;
                        	else if ( strcmp(tok, "swab") == 0 ) // done
                               	 	glb.convs[7] = 1;
                        	else if ( strcmp(tok, "noerror") == 0 )
                                	glb.convs[8] = 1;
                        	else if ( strcmp(tok, "notrunc") == 0 ) // done
                                	glb.convs[9] = 1;
                        	else if ( strcmp(tok, "sync") == 0 ) // done (part)
                                	glb.convs[10] = 1;
                        	tok = strtok(NULL, ",");
                	}
		}
		++argv;
	}

	dd_main();

	return 0; 
}


size_t mltplrs(char *string)
{
	size_t len;
	size_t multi = 1;
	size_t total = 1;
	char *tok;

	tok = strtok(string, "x");

        while ( tok != NULL )
        {
                len = strlen(tok);
                if ( len > 1 && tok[len - 1] == 'b' )
                {
                        tok[len - 1] = '\0';
			multi = 512;
                }
		else if ( len > 1 && tok[len - 1] == 'k' )
                {
                        tok[len - 1] = '\0';
			multi = 1024;
                }
		else if ( len > 1 && tok[len - 1] == 'M' )
               	{ 
                        tok[len - 1] = '\0';
			multi = ( 1024 * 1024 );
                }

		total *= (atoi(tok) * multi);
                tok = strtok(NULL, "x");
        }
	return total;
}



struct tables {
	char ascii[256];
	char ebcdic[256];
	char ibm[256];
} tables = {{ 
        '\000', '\001', '\002', '\003', '\234', '\011', '\206', '\177',
        '\227', '\215', '\216', '\013', '\014', '\015', '\016', '\017',
        '\020', '\021', '\022', '\023', '\235', '\205', '\010', '\207',
        '\030', '\031', '\222', '\217', '\034', '\035', '\036', '\037',
        '\200', '\201', '\202', '\203', '\204', '\012', '\027', '\033',
        '\210', '\211', '\212', '\213', '\214', '\005', '\006', '\007',
        '\220', '\221', '\026', '\223', '\224', '\225', '\226', '\004',
        '\230', '\231', '\232', '\233', '\024', '\025', '\236', '\032',
        '\040', '\240', '\241', '\242', '\243', '\244', '\245', '\246',
        '\247', '\250', '\133', '\056', '\074', '\050', '\053', '\041',
        '\046', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\135', '\044', '\052', '\051', '\073', '\136',
        '\055', '\057', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\174', '\054', '\045', '\137', '\076', '\077',
        '\272', '\273', '\274', '\275', '\276', '\277', '\300', '\301',
        '\302', '\140', '\072', '\043', '\100', '\047', '\075', '\042',
        '\303', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\304', '\305', '\306', '\307', '\310', '\311',
        '\312', '\152', '\153', '\154', '\155', '\156', '\157', '\160',
        '\161', '\162', '\313', '\314', '\315', '\316', '\317', '\320',
        '\321', '\176', '\163', '\164', '\165', '\166', '\167', '\170',
        '\171', '\172', '\322', '\323', '\324', '\325', '\326', '\327',
        '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
        '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
        '\173', '\101', '\102', '\103', '\104', '\105', '\106', '\107',
        '\110', '\111', '\350', '\351', '\352', '\353', '\354', '\355',
        '\175', '\112', '\113', '\114', '\115', '\116', '\117', '\120',
        '\121', '\122', '\356', '\357', '\360', '\361', '\362', '\363',
        '\134', '\237', '\123', '\124', '\125', '\126', '\127', '\130',
        '\131', '\132', '\364', '\365', '\366', '\367', '\370', '\371',
        '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
        '\070', '\071', '\372', '\373', '\374', '\375', '\376', '\377' 
},{ 
        '\000', '\001', '\002', '\003', '\067', '\055', '\056', '\057',
        '\026', '\005', '\045', '\013', '\014', '\015', '\016', '\017',
        '\020', '\021', '\022', '\023', '\074', '\075', '\062', '\046',
        '\030', '\031', '\077', '\047', '\034', '\035', '\036', '\037',
        '\100', '\117', '\177', '\173', '\133', '\154', '\120', '\175',
        '\115', '\135', '\134', '\116', '\153', '\140', '\113', '\141',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\172', '\136', '\114', '\176', '\156', '\157',
        '\174', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
        '\310', '\311', '\321', '\322', '\323', '\324', '\325', '\326',
        '\327', '\330', '\331', '\342', '\343', '\344', '\345', '\346',
        '\347', '\350', '\351', '\112', '\340', '\132', '\137', '\155',
        '\171', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
        '\210', '\211', '\221', '\222', '\223', '\224', '\225', '\226',
        '\227', '\230', '\231', '\242', '\243', '\244', '\245', '\246',
        '\247', '\250', '\251', '\300', '\152', '\320', '\241', '\007',
        '\040', '\041', '\042', '\043', '\044', '\025', '\006', '\027',
        '\050', '\051', '\052', '\053', '\054', '\011', '\012', '\033',
        '\060', '\061', '\032', '\063', '\064', '\065', '\066', '\010',
        '\070', '\071', '\072', '\073', '\004', '\024', '\076', '\341',
        '\101', '\102', '\103', '\104', '\105', '\106', '\107', '\110',
        '\111', '\121', '\122', '\123', '\124', '\125', '\126', '\127',
        '\130', '\131', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\160', '\161', '\162', '\163', '\164', '\165',
        '\166', '\167', '\170', '\200', '\212', '\213', '\214', '\215',
        '\216', '\217', '\220', '\232', '\233', '\234', '\235', '\236',
        '\237', '\240', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
        '\312', '\313', '\314', '\315', '\316', '\317', '\332', '\333',
        '\334', '\335', '\336', '\337', '\352', '\353', '\354', '\355',
        '\356', '\357', '\372', '\373', '\374', '\375', '\376', '\377' 
},{ 
        '\000', '\001', '\002', '\003', '\067', '\055', '\056', '\057',
        '\026', '\005', '\045', '\013', '\014', '\015', '\016', '\017',
        '\020', '\021', '\022', '\023', '\074', '\075', '\062', '\046',
        '\030', '\031', '\077', '\047', '\034', '\035', '\036', '\037',
        '\100', '\132', '\177', '\173', '\133', '\154', '\120', '\175',
        '\115', '\135', '\134', '\116', '\153', '\140', '\113', '\141',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\172', '\136', '\114', '\176', '\156', '\157',
        '\174', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
        '\310', '\311', '\321', '\322', '\323', '\324', '\325', '\326',
        '\327', '\330', '\331', '\342', '\343', '\344', '\345', '\346',
        '\347', '\350', '\351', '\255', '\340', '\275', '\137', '\155',
        '\171', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
        '\210', '\211', '\221', '\222', '\223', '\224', '\225', '\226',
        '\227', '\230', '\231', '\242', '\243', '\244', '\245', '\246',
        '\247', '\250', '\251', '\300', '\117', '\320', '\241', '\007',
        '\040', '\041', '\042', '\043', '\044', '\025', '\006', '\027',
        '\050', '\051', '\052', '\053', '\054', '\011', '\012', '\033',
        '\060', '\061', '\032', '\063', '\064', '\065', '\066', '\010',
        '\070', '\071', '\072', '\073', '\004', '\024', '\076', '\341',
        '\101', '\102', '\103', '\104', '\105', '\106', '\107', '\110',
        '\111', '\121', '\122', '\123', '\124', '\125', '\126', '\127',
        '\130', '\131', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\160', '\161', '\162', '\163', '\164', '\165',
        '\166', '\167', '\170', '\200', '\212', '\213', '\214', '\215',
        '\216', '\217', '\220', '\232', '\233', '\234', '\235', '\236',
        '\237', '\240', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
        '\312', '\313', '\314', '\315', '\316', '\317', '\332', '\333',
        '\334', '\335', '\336', '\337', '\352', '\353', '\354', '\355',
        '\356', '\357', '\372', '\373', '\374', '\375', '\376', '\377' 
	
}}; 


size_t convert(size_t c)
{
	int ascii = glb.convs[0];
	int ebcdic = glb.convs[1];
	int ibm = glb.convs[2];
	int lcase = glb.convs[5];
	int ucase = glb.convs[6];

	if ( ucase == 1 )
		c = toupper(c);
	if ( lcase == 1 )
		c = tolower(c); 
	if ( ascii ) 
		c = tables.ascii[c];
	if ( ebcdic ) 
		c = tables.ebcdic[c];
	if ( ibm ) 
		c = tables.ibm[c];
	return c; 
}


void swabchar(char *buf, size_t i, size_t n, size_t ibs)
{
	static int z = 0;
	static int cache = 0;

	if ( z == 0 )
        {
                cache = buf[i];
                buf[i] = buf[i + 1];
                buf[i + 1] = cache;
                if ( i == (n - 1) && n != ibs )
               		buf[i] = cache;
                ++z;
        }
        else
                ++z;

        if ( z == 2 )
                z = 0;
}


void blockdata(char *buf, char *rotary, size_t b, size_t i, int output)
{
	static size_t inaline = 0; 
        size_t obs = glb.numbs[1];
        size_t cbs = glb.numbs[3]; 


	if ( buf[i] == '\n' )
	{
       		while ( inaline < cbs )
        	{
			rotary[b] = ' ';
			++b;
			++inaline;
			if ( b == obs)
			{
        			write(output, rotary, obs);
        			b = 0;
			}
        	}
        	inaline = 0;
	}
	else
        	++inaline;

	if ( inaline <= cbs && inaline != 0 )
	{
        	rotary[b] = buf[i];
        	++b;
	} 
}




void dd_main()
{ 
	char *buf;
	char *rotary;

	char *cbsbuf;


	int input = STDIN_FILENO;
	int output = STDOUT_FILENO; 

	size_t ibs = glb.numbs[0];
	size_t obs = glb.numbs[1]; 
	
	size_t cbs = glb.numbs[3];
	
	size_t bs = 512; 
	

	size_t skip = glb.numbs[4];
	size_t seek = glb.numbs[5]; 


	/* 1 Check for source file */
	if ( glb.opt[0] != NULL )
		input = open(glb.opt[0], O_RDONLY); 

	/* 2 Check for dest file */ 
	if ( glb.opt[1] != NULL ) 
		output = open(glb.opt[1], O_CREAT|O_RDWR, S_IRUSR|S_IWUSR); 

	/* 3 Check that source and dest were valid */
	if (input == -1 || output == -1 )
		cutilerror("Unable to open file streams\n", 1);

	/* 4 Determine the input and output buffer size using ibs and obs or bs */
	if ( glb.numbs[2] != 0 ) 
		ibs = obs = bs = glb.numbs[0] = glb.numbs[1] = glb.numbs[2];
	if ( ibs == 0 )
		glb.numbs[0] = ibs = bs;
	if ( obs == 0 )
		glb.numbs[1] = obs = bs; 

	/* 5 Create a buffer that corresponds to the maximum ibs= and obs=  */ 
	if (!(buf = malloc((sizeof(char) * ibs + 15 ))))
		cutilerror("Insufficient memory\n", 1); // ibs= 
	if (!(rotary = malloc((sizeof(char) * obs + 15))))
                cutilerror("Insufficient memory\n", 1); // obs= 

	if (!(cbsbuf = malloc((sizeof(char) * cbs + 15))))
                cutilerror("Insufficient memory\n", 1); // conversion temporary buff

	/* 6 "skip" to user specified coordinates on input file */ 
	if ( skip != 0 ) // skip=
		if ( lseek(input, (off_t)skip * ibs, SEEK_SET) == -1 )
			cutilerror("Bad lseek()\n", 1);

	/* 7 "seek" to user specified coordinates on output file  */ 
	if ( seek != 0 ) // seek= 
		if ( lseek(output, (off_t)seek * obs, SEEK_SET) == -1 )
			cutilerror("Bad lseek()\n", 1); 

	/* 8 Perform the actual data transfer */ 
	writeout(buf, rotary, cbsbuf, input, output);

	/* 9 Truncate file in case of seek=, unless conv=notrunc was specified */ 
	if ( glb.convs[9] != 1 && output != STDOUT_FILENO) // conv=notrunc
		ftruncate(output, lseek(output, 0 , SEEK_CUR));


	/* 10 Close all file descriptors, and free memory*/
	if (input != STDIN_FILENO )
		close(input); 
	if (output != STDOUT_FILENO )
		close(output);
//	if (buf) 
//		free(buf);
//	if (rotary)
//		free(rotary);
}


void writeout(char *buf, char *rotary, char *cbsbuf, int input, int output)
{
	static size_t i, b, n; 
	size_t end = glb.numbs[6];
	size_t ibs = glb.numbs[0];
	size_t obs = glb.numbs[1];
	size_t cbs = glb.numbs[3]; 
	size_t count = 0;

	int sync = glb.convs[10];
	int block = glb.convs[3];
	int unblock = glb.convs[4];
	int ascii = glb.convs[0];
        int ebcdic = glb.convs[1];
        int ibm = glb.convs[2];
        int lcase = glb.convs[5];
        int ucase = glb.convs[6];
	int conversion;

	int swab = glb.convs[7]; 
	char padchar = '\0';

	size_t q = 0; 
	size_t a = 0; 

	if (block)
		padchar = ' ';

	/* This needs to be replaced, it simply delays an inevitable speed loss */
	conversion = (ascii + ebcdic + ibm + lcase + ucase);

	if ( cbs == 0 )
		block = 0;


	while ((n = read(input, buf, ibs)) > 0)
        { 
		if ( end != 0 && count == end) 
       	               	return; 

               	++count;
                i = 0; 

		while ( sync && n < ibs ) 
			buf[(++n) - 1] = padchar; 

                while ( i < n )
                { 
			/* conv filters */
			if (conversion)
				buf[i] = convert(buf[i]);

			if (swab)
				swabchar(buf, i, n, ibs); 

			if (block)
				blockdata(buf, rotary, b, i, output); 

			else if (unblock)
			{
				cbsbuf[q] = buf[i];
				++q;
				if ( q >= cbs || (i >= (n - 1) && n != ibs ) || i == n ) 
				{ 
					// replace this with a scan for continuos ' '
					// in the loop above 
		
					while ( cbsbuf[q - 1] == ' ')
						q--;
				
                	                cbsbuf[q] = '\n';
					++q;
					a = 0;
					while ( a < q )
					{
						if ( b == obs )
			                        {
                        			        write(output, rotary, b);
                             				b = 0;
                        			}
						rotary[b] = cbsbuf[a];
						++b;
						++a;
					}
					a = 0;
        	                        q = 0;
	                        } 
			}
			/* default */ 
			else
				rotary[(++b) - 1] = buf[i];

		
			if ( b == obs )
			{
				write(output, rotary, b);
				b = 0;
			} 
			++i;
                }
        }
	/* handle partial blocks (needs improved)*/
	if ( b != 0 )
		write(output, rotary, b); 

}


