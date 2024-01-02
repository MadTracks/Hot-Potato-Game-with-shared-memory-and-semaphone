#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <getopt.h>
#include <time.h>

typedef struct sharedm{
    char shmfifolist[1024][1024];
    int shmfifoused[1024];
    int potato_id[1024];
    int potato_cooldown[1024];
    int potato_original_size[1024];
}sharedm;

void SIGINT_handler(int signa);

int main(int argc, char ** argv){

    int i=0,potato=0;
    int parser = 0;
    int parser_number=0;
    int fd1=0,shmfd1=0;
    int inputcheck[4]={0,0,0,0};
    char sharedmemory_name[1000];
    char fifo_name[1000];
    char semaphore_name[1000];
    char printformat[10000];
    sem_t * shm_semaphore;

    while((parser = getopt(argc, argv, "b:s:f:m:")) != -1)   
    {  
        switch(parser)  
        {  
            case 'b':
            {
                if(inputcheck[0]>=1){
                    fprintf(stderr,"Same input twice.\n");
                    return -1;
                }        
                potato=0;
                int length=0;
                while(optarg[length]!='\0'){
                    length++;
                }
                i=length-1; 
                int multiplier=1; 
                while(i>=0){
                    if(optarg[i]>='0' && optarg[i]<='9'){
                        potato+=(int)(optarg[i] - '0') * multiplier;
                        multiplier *= 10;
                        i--;
                    }
                    else{
                        fprintf(stderr,"Invalid Value");
                        return -1;
                    }
                }        
                inputcheck[0]++;
                break;
            }
            case 's':
            {
                if(inputcheck[1]>=1){
                    fprintf(stderr,"Same input twice.\n");
                    return -1;
                }

                sprintf(sharedmemory_name,"%s",optarg);
                inputcheck[1]++;
                break;
            }
            case 'f':
            {
                if(inputcheck[2]>=1){
                    fprintf(stderr,"Same input twice.\n");
                    return -1;
                }
                sprintf(fifo_name,"%s",optarg);
                inputcheck[2]++;
                break;
            }  
            case 'm':
            {
                if(inputcheck[3]>=1){
                    fprintf(stderr,"Same input twice.\n");
                    return -1;
                }

                sprintf(semaphore_name,"%s",optarg);

                inputcheck[3]++;
                break;
            }
            default:
            {
                fprintf(stderr,"The command line arguments are missing/invalid.\n");
                return -1;
            }
            break;
        }
        parser_number++;  
    }
    if(parser_number!=4){
        fprintf(stderr,"Too many/less arguments.\n");
        return -1;
    }

    srand(time(NULL));
    signal(SIGINT,SIGINT_handler);

    fd1=open(fifo_name,O_RDWR);
    if(fd1<0){
        perror("Open error");
        exit(EXIT_FAILURE);
    }

    shm_semaphore=sem_open(semaphore_name,O_CREAT,0666,1);

    off_t currentline=0;
    char str[1024][1024];
    char c;
    int k=0;
    int t=0;
    do{
        lseek(fd1,currentline,SEEK_SET);
        do{
            read(fd1,&c,1);
            str[t][k]=c;
            k++;
        }
        while(c!='\n');
        str[t][k-1]='\0';
        t++;
        k=0;        
        currentline=lseek(fd1,0,SEEK_CUR);
    }
    while(currentline!=lseek(fd1,0,SEEK_END));

    shmfd1=shm_open(sharedmemory_name,O_RDWR | O_CREAT,0666);
    //write(1,"HERE\n",strlen("HERE\n"));
    sharedm * shared_memory;
    ftruncate(shmfd1,sizeof(sharedm));


    
    shared_memory = mmap(NULL,sizeof(sharedm),PROT_READ | PROT_WRITE,MAP_SHARED,shmfd1,0);
    
    sem_wait(shm_semaphore);

    for(i=0;i<t;i++){        
        if(strcmp(shared_memory->shmfifolist[i],str[i])!=0){
            write(1,shared_memory->shmfifolist[i],strlen(shared_memory->shmfifolist[i]));
            strcpy(shared_memory->shmfifolist[i],str[i]);
            shared_memory->shmfifoused[i]=0;
            //sprintf(printformat,"New Shared Memory %d element: %s - %d\n",i,shared_memory->shmfifolist[i],shared_memory->shmfifoused[i]);
            //write(1,printformat,strlen(printformat));
                 
        }
        else{
            //sprintf(printformat,"Checked Shared Memory %d element: %s - %d\n",i,shared_memory->shmfifolist[i],shared_memory->shmfifoused[i]);
            //write(1,printformat,strlen(printformat));
        }
    }
    //write(1,"HERE2\n",strlen("HERE2\n"));

    int curr_fifo;
    int z=0;
    for(i=0;i<t;i++){
        if(shared_memory->shmfifoused[i]==0){
            curr_fifo=i;
            if(potato==0){
                shared_memory->shmfifoused[curr_fifo]=1;
            }
            else{
                shared_memory->shmfifoused[curr_fifo]=2;
            }
            z=1;
            break;
        }
    }
    sem_post(shm_semaphore);
    if(z==0){
        fprintf(stderr,"All fifos are taken.\n");
        exit(EXIT_FAILURE);
    }    
    sprintf(printformat,"pid=%d current fifo= %s\n",getpid(),shared_memory->shmfifolist[curr_fifo]);
    write(1,printformat,strlen(printformat));

    int fifofd1;
    mkfifo(shared_memory->shmfifolist[curr_fifo],0666);
    int random_num;
    int curr_potato=0;
    int switch_num=0;
    int fifofd2;
    sem_wait(shm_semaphore);
    if(potato!=0){
        for(i=0;i<t;i++){
            if(shared_memory->potato_id[i]==0){
                break;
            }
        }
        shared_memory->potato_id[i]=getpid();
        shared_memory->potato_cooldown[i]=potato;
        shared_memory->potato_original_size[i]=potato;
        curr_potato=shared_memory->potato_id[i];
        switch_num=0;        
    }
    sem_post(shm_semaphore);
    int is_potato_left=1;
    do{
        if(curr_potato!=0){
            //write(1,"HERE-WRITE1\n",strlen("HERE-WRITE1\n"));
            is_potato_left=0;
            for(i=0;i<t;i++){
                if(shared_memory->potato_cooldown[i]>0){
                    is_potato_left=1;
                    break;                    
                }
            }
            if(is_potato_left==0){
                break;
            }
            /*for(i=0;i<t;i++){
                sprintf(printformat,"Potato id=%d Cooldown=%d\n",shared_memory->potato_id[i],shared_memory->potato_cooldown[i]);
                write(1,printformat,strlen(printformat));
            }*/
            do{
                random_num=rand()%t;
            }
            while(shared_memory->shmfifoused[random_num]!=1);
            //write(1,"HERE-WRITE2\n",strlen("HERE-WRITE2\n"));
            fifofd2=open(shared_memory->shmfifolist[random_num],O_WRONLY);
            sem_wait(shm_semaphore);
            write(fifofd2,&curr_potato,sizeof(curr_potato));
            shared_memory->shmfifoused[curr_fifo]=1;
            for(i=0;i<t;i++){
                if(shared_memory->potato_id[i]==curr_potato){
                    switch_num=shared_memory->potato_original_size[i]-shared_memory->potato_cooldown[i]+1;
                }
            }
            sprintf(printformat,"pid=%d sending potato number %d to %s; this is switch number %d\n",getpid(),curr_potato,shared_memory->shmfifolist[random_num],switch_num);
            write(1,printformat,strlen(printformat));
            curr_potato=0;
            sem_post(shm_semaphore);            
            close(fifofd2);
            //write(1,"HERE-WRITE3\n",strlen("HERE-WRITE3\n"));
        }
        else{
            //write(1,"HERE-READ1\n",strlen("HERE-READ1\n"));
            fifofd1=open(shared_memory->shmfifolist[curr_fifo],O_RDONLY); 
            char potid[100];
            i=0;            
            read(fifofd1,potid,sizeof(int));
            memcpy(&curr_potato,potid,sizeof(int));

            if(curr_potato == -1){
                write(1,"No left potatoes, exiting.\n",strlen("No left potatoes, exiting.\n"));
                exit(EXIT_SUCCESS);
            }
            //write(1,"HERE-READ2\n",strlen("HERE-READ2\n"));
            sem_wait(shm_semaphore);
            shared_memory->shmfifoused[curr_fifo]=2;
            sprintf(printformat,"pid=%d receiving potato number %d from %s\n",getpid(),curr_potato,shared_memory->shmfifolist[curr_fifo]);
            write(1,printformat,strlen(printformat));
            for(i=0;i<t;i++){
                if(shared_memory->potato_id[i]==curr_potato){
                    shared_memory->potato_cooldown[i]--;
                    break;
                }
            }
            
            if(shared_memory->potato_cooldown[i]<=0){
                sprintf(printformat,"pid=%d; potato number %d has cooled down.\n",getpid(),curr_potato); 
                write(1,printformat,strlen(printformat));
                shared_memory->shmfifoused[curr_fifo]=1;
                curr_potato=0;
                is_potato_left=0;
                for(i=0;i<t;i++){
                    if(shared_memory->potato_cooldown[i]>0){
                        is_potato_left=1;
                        break;                    
                    }
                }
            }
            
            sem_post(shm_semaphore);
            close(fifofd1);
            //write(1,"HERE-READ3\n",strlen("HERE-READ3\n"));            
        }

    }
    while(is_potato_left==1);
    
    write(1,"All potatoes are cold.\n",strlen("All potatoes are cold.\n"));
    int fifofd3;
    int all_done=-1;
    for(i=0;i<t;i++){
        if(i!=curr_fifo && shared_memory->shmfifoused[i]==1){
            fifofd3=open(shared_memory->shmfifolist[i],O_WRONLY);
            if(fifofd3<0){
                perror("Open Error");
                exit(EXIT_FAILURE);
            }
            write(fifofd3,&all_done,sizeof(all_done));
            close(fifofd3);
        }        
    }
    write(1,"Freeing all resources.\n",strlen("Freeing all resources.\n"));
    unlink(shared_memory->shmfifolist[curr_fifo]);

    munmap(shared_memory,sizeof(sharedm));
    close(shmfd1);
    shm_unlink(sharedmemory_name);

    sem_close(shm_semaphore);
    sem_unlink(semaphore_name);
    
    close(fd1);
    
    
    return 0;
}

void SIGINT_handler(int signa){
    write(1,"SIGINT catched.\n",strlen("SIGINT catched.\n"));
    exit(EXIT_FAILURE);
}