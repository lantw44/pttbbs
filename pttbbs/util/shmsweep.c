#include "bbs.h"

int main() {
#if 0
    int i, shm, counter;
    struct utmpfile_t *utmpshm;

    
    shm = shmget(UTMPSHM_KEY, USHM_SIZE, SHM_R | SHM_W);
    if(shm == -1) {
	perror("shmget");
	exit(0);
    }
    
    utmpshm = shmat(shm, NULL, 0);
    if(utmpshm == (struct utmpfile_t *)-1) {
	perror("shmat");
	exit(0);
    }
    
    for(i = counter = 0; i < USHM_SIZE; i++)
	if(SHM->uinfo[i].pid) {
	    char buf[256];
	    userinfo_t *f;
	    struct stat sb;
		
	    f = &utmpshm->uinfo[i];
	    sprintf(buf, "/proc/%d", f->pid);
	    if(stat(buf, &sb)) {
		f->pid = 0;
		utmpshm->number--;
		counter++;
	    }
	}
    printf("clear %d slots\n", counter);
#endif
    return 0;
}
