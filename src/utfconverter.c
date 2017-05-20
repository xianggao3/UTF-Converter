/*#include "struct.txt" */
#include "utfconverter.h"
#include <sys/stat.h>

char* filename;
char* outputname;
endianness source;
endianness conversion;
	int numGlyphs;
 	double numSurrogates;
 	double numASCII;

 	int hasPosOutput;
 	int fout;

	static clock_t st_time_r;
	static clock_t en_time_r;
	static struct tms st_cpu_r;
	static struct tms en_cpu_r;
	float sumreaduser;
	float sumreadreal;
	float sumreadsys;

	static clock_t st_time_c;
	static clock_t en_time_c;
	static struct tms st_cpu_c;
	static struct tms en_cpu_c;
	float sumconvertuser;
	float sumconvertreal;
	float sumconvertsys;


	static clock_t st_time_w;
	static clock_t en_time_w;
	static struct tms st_cpu_w;
	static struct tms en_cpu_w;
	float sumwriteuser;
	float sumwritereal;
	float sumwritesys;

	unsigned char msb;
	unsigned char zeroB,oneB,twoB,threeB;
	unsigned int codept;

int main(int argc,char** argv) {
	/* After calling parse_args(), filename and conversion should be set. */
	int fd;
	unsigned char buf[4];
	unsigned char buff[2];
	int rv;
	Glyph* glyph;

	/*verbosity*/
	int vlevel;
	char* abspath;
	char apath [100];
	char hostname[128];
	struct utsname myOS;
	float size;

	struct stat st;
	vlevel = 0;
	hasPosOutput=0;

	sumwriteuser=0;
	sumwritereal=0;
	sumwritesys=0;
	sumreaduser=0;
	sumreadreal=0;
	sumreadsys=0;
	sumconvertuser=0;
	sumconvertreal=0;
	sumconvertsys=0;

	numGlyphs=0;
	numSurrogates=0;
	numASCII=0;
	vlevel=parse_args(argc, argv);

	fd = open(filename, O_RDONLY); 
	rv = 0; 
	buff[0]=0xff;
	buff[1]=0xfe;
	
	glyph = malloc(sizeof(Glyph)); 
	fout= open(outputname, O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	/*start THE CLOCk */
	/* Handle BOM bytes for UTF16 specially. 
         * Read our values into the first and second elements. */
	if((rv = read(fd, &buf[0], 1)) == 1 && (rv = read(fd, &buf[1], 1)) == 1){
		void* memset_return;
		
		buf[0] &= 0xff;
		buf[1] &= 0xff;

		if(hasPosOutput==0){
			if(buf[0] == 0xff && buf[1] == 0xfe){
				/*file is big endian*/
				source = LITTLE; 
				numGlyphs++;
				if(source==conversion){
					write(STDOUT_FILENO,&buf[0],1);
					write(STDOUT_FILENO,&buf[1],1);
				}else{
					write(STDOUT_FILENO,&buf[1],1);
					write(STDOUT_FILENO,&buf[0],1);
				}	

			} else if(buf[0] == 0xfe && buf[1] == 0xff){
				/*file is little endian*/
				source = BIG;
				numGlyphs++;
				if(source==conversion){
					write(STDOUT_FILENO,&buf[0],1);
					write(STDOUT_FILENO,&buf[1],1);
				}else {

					write(STDOUT_FILENO,&buf[1],1);
					write(STDOUT_FILENO,&buf[0],1);
				}

			} else if( buf[0] == 0xef && buf[1]==0xbb  ){ 
				if((rv = read(fd,&buf[2],1))==1){
					source=UTF8;
					numGlyphs++;
					if(LITTLE==conversion){
						write(STDOUT_FILENO,&buff[0],1);
						write(STDOUT_FILENO,&buff[1],1);
					}else {
						write(STDOUT_FILENO,&buff[1],1);
						write(STDOUT_FILENO,&buff[0],1);
					}
					
				buf[1]=0;
				}
			}else {
				/*file has no BOM*/
				free(&glyph->bytes); 
				fprintf(stderr, "File has no BOM.\n");
				quit_converter(NO_FD); 
			}
		}else if (hasPosOutput==1){				/*has positional */

			if(buf[0] == 0xff && buf[1] == 0xfe){
				/*file is big endian*/
				source = LITTLE; 
				numGlyphs++;
				if(source==conversion){
					write(fout,&buf[0],1);
					write(fout,&buf[1],1);
				}else{
					write(fout,&buf[1],1);
					write(fout,&buf[0],1);
				}	

			} else if(buf[0] == 0xfe && buf[1] == 0xff){
				/*file is little endian*/
				source = BIG;
				numGlyphs++;
				if(source==conversion){
					write(fout,&buf[0],1);
					write(fout,&buf[1],1);
				}else {

					write(fout,&buf[1],1);
					write(fout,&buf[0],1);
				}

			} else if( buf[0] == 0xef && buf[1]==0xbb  ){ 
				if((rv = read(fd,&buf[2],1))==1){
					source=UTF8;
					numGlyphs++;
					if(LITTLE==conversion){
						write(fout,&buff[0],1);
						write(fout,&buff[1],1);
					}else {
						write(fout,&buff[1],1);
						write(fout,&buff[0],1);
					}
					
				buf[1]=0;
				}
			}else {
				/*file has no BOM*/
				free(&glyph->bytes); 
				fprintf(stderr, "File has no BOM.\n");
				quit_converter(NO_FD); 
			}
		}
		
		memset_return = memset(glyph, 0, sizeof(Glyph)+1);
		/* Memory write failed, recover from it: */
		if(memset_return == NULL){
			/* tweak write permission on heap memory. */
			/*asm("movl $8, %esi\n\t"
			    "movl $.LC0, %edi\n\t"
			    "movl $0, %eax");*/
			/* Now make the request again. */
			memset(glyph, 0, sizeof(Glyph)+1);
		}
	}else{
			free(&glyph->bytes); 
			fprintf(stderr, "No file found.\n");
			quit_converter(NO_FD); 
	}

	/* Now deal with the rest of the bytes.*/
	st_time_r = times(&st_cpu_r);
	
	while((rv = read(fd, &buf[0], 1)) == 1 ){
		void* memset_return;

		en_time_r= times(&en_cpu_r);
		sumreaduser+=(en_cpu_r.tms_utime - st_cpu_r.tms_utime);
		sumreadreal+=(en_time_r - st_time_r);
		sumreadsys+=(en_cpu_r.tms_stime - st_cpu_r.tms_stime);
		if(source==UTF8){
				msb = (buf[0]&0x80)>>7;

				if (msb == 0){/* 1 byte*/
					codept=buf[0];

					buf[THIRD] = buf[FOURTH] |= 0;

				}else if (buf[0]>=0xf0){
					/*4 bytes*/
					read(fd, &buf[1], 1);
					read(fd, &buf[2], 1);
					read(fd, &buf[3], 1);

					zeroB 	= buf[0]&0x07;
					oneB 	=buf[1]&0x3f;
					twoB	=buf[2]&0x3f;
					threeB	=buf[3]&0x3f;
					codept 	=(zeroB<<18)+(oneB<<12)+(twoB<<6)+threeB;

				}else if (buf[0]>=0xd0){

					read(fd, &buf[1], 1);
					read(fd, &buf[2], 1);
					/*3 bytes*/
					zeroB 	=buf[0]&0x0f;
					oneB 	=buf[1]&0x3f;
					twoB	=buf[2]&0x3f;
					codept 	=(zeroB<<12)+(oneB<<6)+twoB;

				}else if (buf[0]>=0xc0){
					read(fd, &buf[1], 1);
					/*2 bytes*/
					zeroB 	=buf[0]&0x1f;
					oneB 	=buf[1]&0x3f;
					codept 	=(zeroB<<6)+oneB;
				}

			if(source==UTF8&& conversion==LITTLE){
				write_glyph(convert(glyph,conversion));
			}else if (source==UTF8 && conversion==BIG){
				write_glyph(swap_endianness(convert(glyph,conversion)));
			}

		}
		else{
			if((rv = read(fd, &buf[1], 1)) == 1){

				if(source==conversion){
					write_glyph(fill_glyph(glyph, buf, source, &fd));
				}else{
					write_glyph(swap_endianness(fill_glyph(glyph, buf, source, &fd)));
				}
			}
		}
		memset_return = memset(glyph, 0, sizeof(Glyph)+1);
	        /* Memory write failed, recover from it: */
	        if(memset_return == NULL){
		        /* tweak write permission on heap memory. */
		        /*asm("movl $8, %esi\n\t"
		            "movl $.LC0, %edi\n\t"
		            "movl $0, %eax");
		            */
		        /* Now make the request again. */
		        memset(glyph, 0, sizeof(Glyph)+1);
	        }
	}	
	

	if(vlevel>= 1){
		stat(filename,&st); 	
		size = st.st_size;
		size = size*.001;
		fprintf(stderr,"Input file size: %5.2f kb\n", size);
		abspath=realpath(filename,apath);
		fprintf(stderr, "Input file path: %s\n",abspath);
		if(source ==LITTLE){
			fprintf(stderr, "Input file encoding: UTF-16LE\n");
		}else if (source == BIG){
			fprintf(stderr, "Input file encoding: UTF-16BE\n");
		}else {
			fprintf(stderr, "Input file encoding: UTF-8\n");
		}
		if(conversion ==LITTLE){
			fprintf(stderr, "Output encoding: UTF-16LE\n");
		}else if (conversion == BIG){
			fprintf(stderr, "Output encoding: UTF-16BE\n");
		}
		gethostname(hostname, sizeof hostname);
		fprintf(stderr, "Hostmachine: %s\n", hostname);
		uname(&myOS);
		fprintf(stderr, "Operating System: %s\n", myOS.sysname);
	}
	if(vlevel>=2){
		
        fprintf(stderr,"Reading: real=%.1f, user=%.1f, sys=%.1f\n", sumreadreal,sumreaduser,sumreadsys);

        fprintf(stderr,"Converting: real=%.1f, user=%.1f, sys=%.1f\n", sumconvertreal,sumconvertuser,sumconvertsys);
        fprintf(stderr,"Writing: real=%.1f, user=%.1f, sys=%.1f\n", sumwritereal,sumwriteuser,sumwritesys);

		fprintf(stderr, "ASCII: %d%%\nSurrogates: %d%%\nGlyphs: %d\n", (int)((numASCII/numGlyphs)*100), (int)((numSurrogates/numGlyphs)*100), numGlyphs);
	}
	quit_converter(NO_FD);
	return 0;
}

Glyph* 	swap_endianness(Glyph* glyph){
	/* Use XOR to be more efficient with how we swap values. */
	unsigned char temp;

	st_time_c = times(&st_cpu_c);

	temp= glyph->bytes[0];
	glyph->bytes[0] = glyph->bytes[1];
	glyph->bytes[1] = temp;
	if(glyph->surrogate){  /* If a surrogate pair, swap the next two bytes. */
		temp= glyph->bytes[2];
		glyph->bytes[2] = glyph->bytes[3];
		glyph->bytes[3] = temp;
	}

	en_time_c= times(&en_cpu_c);
	sumconvertuser+=(en_cpu_c.tms_utime - st_cpu_c.tms_utime);
	sumconvertreal+=(en_time_c - st_time_c);
	sumconvertsys+=(en_cpu_c.tms_stime - st_cpu_c.tms_stime);

	glyph->end = conversion;
	return glyph;
}

Glyph* fill_glyph(Glyph* glyph,unsigned char data[2],endianness end,int* fd) {
	unsigned int bits;
	unsigned int x;
	glyph->bytes[0] = data[0];
	glyph->bytes[1] = data[1];
	x=0;
	bits = 0;
	x=data[SECOND];
	x=x<<8;
	bits |= (data[FIRST] + x);
	if (source==BIG){
		if ((data[1]+(data[0]<<8))<128){
			numASCII++;
		}
	}else{
		if(bits<128){
			numASCII++;	
		}
	}
	bits-=0x010000;
	/* Check high surrogate pair using its special value range.*/
	if(bits > 0xD800 && bits <0xDBFF ){ 
		if(read(*fd, &data[SECOND], 1) == 1 && read(*fd, &data[FIRST], 1) == 1){
			bits = 0;
			 
			bits = (glyph->bytes[FIRST] + (glyph->bytes[SECOND] << 8)); 
			if(bits >0xDC00  && bits <0xDFFF){ /* Check low surrogate pair.*/
				glyph->surrogate = false;
			} else {
				glyph->surrogate = true;
				numSurrogates++;
				lseek(*fd, OFFSET, SEEK_CUR);
				
			}							
		}
	}
	if(!glyph->surrogate){
		glyph->bytes[THIRD] = glyph->bytes[FOURTH] |= 0;
	} else {
		glyph->bytes[THIRD] = data[FIRST]; 
		glyph->bytes[FOURTH] = data[SECOND];
	}
	glyph->end = end;

	return glyph;
}

void write_glyph(Glyph* glyph) {
	st_time_w = times(&st_cpu_w);

	if(hasPosOutput==0){
		if(glyph->surrogate){
			write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
			
		} else {
			write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
		}
	}else if (hasPosOutput==1){
		if(glyph->surrogate){
			write(fout, glyph->bytes, SURROGATE_SIZE);
			
		} else {
			write(fout, glyph->bytes, NON_SURROGATE_SIZE);
		}
	}


	en_time_w= times(&en_cpu_w);

	sumwriteuser+=(en_cpu_w.tms_utime - st_cpu_w.tms_utime);
	sumwritereal+=(en_time_w - st_time_w);
	sumwritesys+=(en_cpu_w.tms_stime - st_cpu_w.tms_stime);


	numGlyphs++;
}

int parse_args(int argc, char** argv){
	int opt_index, c, vlvl;
	char* endian_convert;
	
	endian_convert = NULL;
	vlvl=0;

	/* If getopt() returns with a valid (its working correctly) 
	 * return code, then process the args! */
	while((c = getopt_long(argc, argv, "vhu:", long_options, &opt_index)) != -1){
		switch(c){ 
			case 'h':
				print_help();
				break;
			case 'u':
				endian_convert = optarg;
				break;
			case 'v':
				vlvl++;
				break;
			default:
				fprintf(stderr, "Unrecognized argument.\n");
				quit_converter(NO_FD);
				break;
		}

	}

	if(optind < argc){
		filename= malloc(sizeof(char)*strlen(argv[optind])+1);
		strcpy(filename, argv[optind]);	
	} else {
		fprintf(stderr, "Filename not given.\n");
		print_help();
	}
	if(optind+1 < argc){
		outputname=malloc(sizeof(char)*strlen(argv[optind +1])+1);
		strcpy(outputname,argv[optind+1]);
		hasPosOutput=1;
	}

	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help();
	}
	if(strcmp(endian_convert, "16LE")==0){ 
		conversion = LITTLE;
	} else if(strcmp(endian_convert, "16BE")==0){
		conversion = BIG;
	} else {
		quit_converter(NO_FD);
	}
	return vlvl;
}

void print_help(void) {
	printf("%s", USAGE); 	
	quit_converter(NO_FD);
}

void quit_converter(int fd) {
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD)
		close(fd);
	exit(0);
}

Glyph* convert(Glyph* glyph, endianness end){
	unsigned int highv,lowv;

	st_time_c = times(&st_cpu_c);

	if(codept>=0x10000){
		codept = codept-0x10000;
		highv=codept>>10;
		lowv=codept& 0x3FF;
		highv=highv+0xd800;
		lowv=lowv+0xdc00;
		glyph->surrogate=true;
		glyph->bytes[1]=(highv&0xff00)>>8;
		glyph->bytes[0]=(highv&0x00ff);
		glyph->bytes[3]=(lowv&0xff00)>>8;
		glyph->bytes[2]=(lowv&0x00ff);
		numSurrogates++;

	}else{
		glyph->surrogate=false;
		glyph->bytes[0]=codept;
		glyph->bytes[1]=(codept>>8);
	}
	if( codept<128){
		numASCII++;
	}
	
	if (end ==LITTLE){
		glyph->end = LITTLE;
		
	}else if (end == BIG){
		glyph->end = BIG;
		
	}else{
		quit_converter(NO_FD);
	}

	en_time_c= times(&en_cpu_c);
	sumconvertuser+=(en_cpu_c.tms_utime - st_cpu_c.tms_utime);
	sumconvertreal+=(en_time_c - st_time_c);
	sumconvertsys+=(en_cpu_c.tms_stime - st_cpu_c.tms_stime);

	return glyph;
}
