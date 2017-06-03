/*
	Chad Coates
	ECE 373
	Homework #3
	April 29, 2017

	This is the reader/writer for the acme_pci driver
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct ring_info{
	int head;
	int tail;
	uint32_t icr;
	uint32_t rh;
	uint32_t rl;
	uint32_t len;
};

int acme_error(int);
int reader(int,char *);
int writer(int,uint32_t,char *);

int main(int argc, char *argv[]){
	char *file="/dev/ece_led";
	int fd=open(file,O_RDWR);

	if(fd<0){
		fprintf(stderr,"open failed: %s\n",file);
		exit(1);
	}

	for(;;){
		reader(fd,file);
		sleep(2);
	}
	
/*
	switch(argc){
		case 1:
			reader(fd,file);
			break;
		case 2:
			val=atoi(argv[argc-1]);
		
			switch(val){
				case 1:
					getchar();
					close(fd);
					return 0;
				default: break;		 
			}
			fprintf(stderr,"Setting blink rate to: %d blinks/sec\n",val);
			return writer(fd,val,file);
		default: return acme_error(fd);		 
	}
//*/
	close(fd);
	exit(0);
}

int acme_error(int fd){
	fprintf(stderr,"Error, cowardly refusing to proceed!\n");
	close(fd);
	exit(1);
}

int reader(int fd,char *file){
//	uint32_t val;
	struct ring_info info;
	if(read(fd,&info,sizeof(struct ring_info))<0){
		fprintf(stderr,"read failed!: %s\n",file);
		exit(1);
	}

	fprintf(stderr, "rh: (0x%x), rl: (0x%x), len: %i\n", info.rh,info.rl,info.len);
	fprintf(stderr, "icr: (0x%x), Head: %i, Tail: %i\n",info.icr,info.head,info.tail);

	return 0;
}

/*
int writer(int fd,uint32_t val,char *file){
	if(write(fd,&val,sizeof(int))<0){
		fprintf(stderr,"write failure!: %s\n",file);
		exit(1);
	}
	fprintf(stderr,"write success!: %s\n",file);
	return 0;
}
//*/

