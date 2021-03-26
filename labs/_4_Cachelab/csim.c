#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define max_line_sz 60
#define max_addr_sz 17

typedef struct Cacheblock{
    struct Cacheblock *prev;
    struct Cacheblock *next;
    long tag;
    char valid;
}Cacheblock;

void unix_error(char *msg);/*Unix-style error*/
FILE *Fopen(const char *pathname, const char *mode);
//char *Fgets(char *str, int n, FILE *stream);
Cacheblock *initCache(int S,int E);
Cacheblock *findhead(Cacheblock *setptr,int E);
Cacheblock *findtail(Cacheblock *setptr,int E);

int main(int argc , char *argv[])
{
    assert((argc == 9) || (argc == 10));

    int i,j,s,S,E,b,hits=0,misses=0,evictions=0,vflag = 0;
    long addr,tagset,tag,set;
    char line[max_line_sz];
    char saddr[max_addr_sz];
    char sop;
    FILE *trace;
    Cacheblock *Cacheptr,*setptr,*oldhead,*oldtail;


    
    if (argc == 10){
        if (!(strcmp(argv[1],"-h"))){
	printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
        exit(10);
	}
        if (!(strcmp(argv[1],"-v"))){
	vflag = 1;
	}
    }	
    s = atoi(argv[argc-7]);//read the arguments
    S = 1 << s;
    E = atoi(argv[argc-5]);
    b = atoi(argv[argc-3]);
    trace = Fopen(argv[argc-1],"r");
    
    Cacheptr = initCache(S,E);
    
    if (vflag == 0){
        while (fgets(line,max_line_sz,trace) != NULL){//fgets will append \0 after the last character
    	    
	    //read L,S,M, from the line, read the address
	    if (line[0] != ' ') continue;//ignore op I
	    printf("%s",line);
	    sop = line[1];
	    for (j = 0,i = 3;line[i] != ',';i++,j++){//i=3 as address starts at 3
		    saddr[j] = line[i];
	    }
	    saddr[j] = '\0';
	    addr = strtol(saddr,NULL,16);
	    tagset = addr >> b;
	    tag = tagset >> s;
	    set = (~(~0>> s << s))&tagset;
	    setptr = Cacheptr + set * E;
	   // printf("op:%c\nsaddr:%s\naddr:%lx\ntagset:%lx\ntag:%lx\nset:%lx\nsetptr:%lx\n",sop,saddr,addr,tagset,tag,set,setptr);   
	    
	    //check hits/misses
	    for (i = 0;i < E;i++){
	        if (((setptr + i)->valid== 1) && ((setptr + i)-> tag == tag)) break;
	    }
	    //misses,need to add or evict
	    if (i == E){
    		misses++;
				
	        for (i = 0;i < E;i++){
		    //add to cache directly,set Cacheblock accordingly,(old head and new head)
	            if ((setptr + i)->valid == 0){
			if (i != 0){//when the queue is not empty
		    	oldhead = findhead(setptr,E);
			oldhead->prev = (setptr+i);
			(setptr+i)->next = oldhead;
			}
			//else  (setptr+i)->next = NULL;

			(setptr+i)->prev = NULL;
			(setptr+i)->tag = tag;
			(setptr+i)->valid = 1;
			break;
		    }
	        }
		//if all ocupied ,implement evictions
		if (i == E) {
		evictions++;
		oldhead = findhead(setptr,E);
		oldtail = findtail(setptr,E);//sgementfault when put this in "if" below,Really worring,yet this is so hard to find out,the lesson is i have to plan it well before typing,or will waste much more time on debugging,and try to get things out of the scope if possible may be a good habit?
		if (E != 1){

			oldhead->prev = oldtail;
			oldtail->prev->next = NULL;
			oldtail->prev = NULL;
			oldtail->next = oldhead;
			}
		oldtail->tag = tag;
		}
	    }
	    else {
		hits++;
		oldhead = findhead(setptr,E);
		oldtail = findtail(setptr,E);
		if ((setptr+i)!=oldhead && (setptr+i)!=oldtail){
			(setptr+i)->prev->next = (setptr+i)->next;
			(setptr+i)->next->prev = (setptr+i)->prev;
			(setptr+i)->next = oldhead;
			(setptr+i)->prev = NULL;
			oldhead->prev = (setptr+i);
		}
		else if ((setptr+i) == oldhead){;}
		else if ((setptr+i) == oldtail){
			(setptr+i)->prev->next = (setptr+i)->next;
			(setptr+i)->next = oldhead;
			(setptr+i)->prev = NULL;
			oldhead->prev = (setptr+i);
				
		}
	    }
	    if (sop == 'M') hits++;
	    printf("hits:%d,misses:%d,evictions:%d\n",hits,misses,evictions);
        }
	
    }

    /*printf("s=%d,E=%d,b=%d\n",s,E,b);
    Fgets(line,max_line_sz,trace);
    printf("t=\n %s",line);
    Fgets(line,max_line_sz,trace);
    printf("t=\n %s",line);
    */
    printSummary(hits, misses, evictions);
    fclose(trace);
    free(Cacheptr);
    return 0;
}

void unix_error(char *msg){

    fprintf(stderr,"%s: %s\n",msg,strerror(errno));
    exit(0);
}

FILE *Fopen(const char *pathname, const char *mode){
    FILE *file;

    if((file=fopen(pathname,mode)) == NULL){
    	unix_error("fopen error");
    }
    return file;
}


/*char *Fgets(char *str, int n, FILE *stream){
       
    if(fgets(str, n ,stream) == NULL){
    	unix_error("fgets error");
    }
    return str; 
}*/


struct Cacheblock *initCache(int S,int E){
    Cacheblock *head;

    head = calloc(S*E,sizeof(Cacheblock));

    return head;
}


Cacheblock *findhead(Cacheblock *setptr,int E){
    for (int i = 0;i < E;i++){
        if (((setptr + i)->prev == NULL) ) return (setptr+i);
    }
    return NULL;
}

Cacheblock *findtail(Cacheblock *setptr,int E){
    for (int i = 0;i < E;i++){
        if (((setptr + i)->next == NULL) ) return (setptr+i);
    }
    return NULL;
}

