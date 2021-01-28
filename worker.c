

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "worker.h"

#define DEBUG_WORKER 0
#define USE_COND 1

#if DEBUG_WORKER
extern int debug;
#ifdef dprintf
#undef dprintf
#endif
#define dprintf(format, args...) if (debug) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format,args...) /* no-op */
#endif

struct worker_info {
        int slot;
	volatile unsigned short flags;
        pthread_mutex_t lock;
	pthread_cond_t cond;
        pthread_t tid;
	worker_func_t func;
	void *arg;
};

#define FLAG_BUSY 0x01
#define FLAG_DONE 0x02
#define FLAG_KILL 0x04
#define ISBUSY(i) ((i->flags & FLAG_BUSY) != 0)
#define ISDONE(i) ((i->flags & FLAG_DONE) != 0)

static void *worker(void *);
void worker_cancel(worker_pool_t *pool);

static int create_worker(worker_info_t *wp, int num) {
	memset(wp,0,sizeof(*wp));
	wp->slot = num;
	pthread_mutex_init(&wp->lock, 0);
#if USE_COND
	pthread_cond_init(&wp->cond, 0);
#endif
	return pthread_create(&wp->tid, NULL, worker, wp);
}

worker_pool_t *worker_create_pool(int count) {
	worker_pool_t *pool;
	register int x;
//	pthread_attr_t attr;

	if (count < 1) return 0;

	pool = malloc(sizeof(*pool));
	if (!pool) {
		perror("malloc worker_pool");
		return 0;
	}
	memset(pool,0,sizeof(*pool));

	pool->workers = malloc(count * sizeof(struct worker_info));
	if (!pool->workers) {
		perror("malloc workers");
		free(pool);
		return 0;
	}

#if 0
	/* Create joinable attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif

	/* Init worker info */
	dprintf("creating %d workers...\n",count);
	for(x=0; x < count; x++) {
		if (create_worker(&pool->workers[x],x)) {
			free(pool->workers);
			free(pool);
			return 0;
		}
	}

	pool->count = count;
	return pool;
}

static int destroy_worker(worker_info_t *wp) {
	void *sptr;

	pthread_cancel(wp->tid);
	pthread_join(wp->tid,&sptr);
#if USE_COND
	pthread_cond_destroy(&wp->cond);
#endif
	pthread_mutex_destroy(&wp->lock);
	return 0;
}

int worker_destroy_pool(worker_pool_t *pool,int timeout) {
	register int x;

	if (!pool) return 0;

	/* Destroy workers */
	dprintf("destroying workers...\n");
	for(x=0; x < pool->count; x++) destroy_worker(&pool->workers[x]);

	/* Free memory */
	dprintf("freeing mem...\n");
	free(pool->workers);
	free(pool);

	dprintf("done!\n");
	return 0;
}

static void _doterm(int sig) {
	if (sig == SIGINT) pthread_exit(0);
}

/* The actual worker function */
static void *worker(void *arg) {
	struct worker_info *wp = arg;

	if (signal(SIGINT,_doterm) == SIG_ERR) {
		printf("error: worker[%d]: signal(SIGINT): %s\n", wp->slot, strerror(errno));
		pthread_exit(0);
	}

	while(1) {
		/* Lock this slot */
		pthread_mutex_lock(&wp->lock);

		/* Wait for busy to be set */
		dprintf("worker[%d]: flags: %x\n", wp->slot, wp->flags);
#if USE_COND
		while((wp->flags & FLAG_BUSY) == 0 && (wp->flags & FLAG_DONE) == 0) pthread_cond_wait(&wp->cond,&wp->lock);
		if (wp->flags & FLAG_DONE) {
			pthread_mutex_unlock(&wp->lock);
			break;
		}
#else
		if ((wp->flags & FLAG_BUSY) == 0)  {
			pthread_mutex_unlock(&wp->lock);
			/* Are we done? */
			if (wp->flags & FLAG_DONE) break;
			usleep(100);
			continue;
		}
#endif

		/* Unlock slot */
		pthread_mutex_unlock(&wp->lock);

		/* Exec the work item */
		dprintf("worker[%d]: running func: %p, arg: %p\n", wp->slot, wp->func, wp->arg);
		wp->func(wp->arg);

		/* Un-set busy flag */
		pthread_mutex_lock(&wp->lock);
		wp->flags &= ~FLAG_BUSY;
		pthread_mutex_unlock(&wp->lock);
	}
	dprintf("worker[%d]: exiting...\n", wp->slot);
	signal(SIGINT,SIG_DFL);
	pthread_exit(0);
//	return (void *)0;
}

/* Put a worker to work */
int worker_exec(worker_pool_t *pool, worker_func_t func, void *arg) {
	worker_info_t *wp;
	int x,found;

	if (!pool) return 0;
	if (!func) return 1;

	dprintf("slots: %d, func: %p, arg: %p\n", pool->count, func, arg);

	found = 0;
	do {
		dprintf("pool->count: %d\n", pool->count);
		for(x=0; x < pool->count; x++) {
			wp = &pool->workers[x];

			/* Lock this slot */
			dprintf("worker[%d]: locking slot...\n",wp->slot);
			pthread_mutex_lock(&wp->lock);

			/* If busy, unlock and continue on */
			dprintf("worker[%d]: busy: %d\n", wp->slot, ISBUSY(wp));
			if (ISBUSY(wp)) {
				dprintf("worker[%d]: unlocking slot...\n",wp->slot);
				pthread_mutex_unlock(&wp->lock);
				continue;
			}

			/* Set work item */
			wp->func = func;
			wp->arg = arg;

			/* Set busy flag */
			wp->flags |= FLAG_BUSY;

#if USE_COND
			/* Set condition */
			dprintf("worker[%d]: signaling...\n",wp->slot);
			pthread_cond_signal(&wp->cond);
#endif

			/* Unlock */
			dprintf("worker[%d]: unlocking...\n",wp->slot);
			pthread_mutex_unlock(&wp->lock);

			/* Done */
			return 0;
		} 
//		dprintf("sleeping...\n");
		usleep(100);
	} while(!found);

	/* Should never return here */
	return -1;
}

void worker_wait(struct worker_pool *pool, int timeout) {
	register int x;
	struct worker_info *wp;
	int busy;
	time_t start,t;

	if (!pool) return;

	/* Wait for all jobs to finish */
	dprintf("*** WAITING ON FINISH ****\n");
	time(&start);
	while(1) {
		busy = 0;
		for(x=0; x < pool->count; x++) {
			wp = &pool->workers[x];
			pthread_mutex_lock(&wp->lock);
//			dprintf("worker[%d]: busy: %d\n", wp->slot, ISBUSY(wp));
			if (wp->flags & FLAG_BUSY) busy++;
			pthread_mutex_unlock(&wp->lock);
		}
//		dprintf("busy: %d\n", busy);
		if (!busy) break;
		time(&t);
//		dprintf("timeout: %d, t: %d, start: %d\n", timeout,(int) t, (int)start);
		if (timeout > 0 && (t - start) > timeout) break;
		usleep(100);
		continue;
	}
}

void worker_killbusy(worker_pool_t *pool) {
	register int x;
	struct worker_info *wp;

	if (!pool) return;

	/* Send signal to workers */
	for(x=0; x < pool->count; x++) {
		wp = &pool->workers[x];

		/* Lock this slot */
		dprintf("worker[%d]: locking slot...\n",wp->slot);
		pthread_mutex_lock(&wp->lock);

		/* If busy, unlock and send kill signal */
		dprintf("worker[%d]: busy: %d\n", wp->slot, ISBUSY(wp));
		if (ISBUSY(wp)) {
			dprintf("worker[%d]: unlocking slot...\n",wp->slot);
			pthread_mutex_unlock(&wp->lock);
			printf(">>> worker[%d]: killing slot! <<<\n",wp->slot);
			dprintf("worker[%d]: killing slot...\n",wp->slot);
			pthread_kill(wp->tid, SIGINT);
			wp->flags |= FLAG_KILL;
			continue;
		}
		/* Unlock and go to next */
		dprintf("worker[%d]: unlocking...\n",wp->slot);
		pthread_mutex_unlock(&wp->lock);
	}
	/* Re-create any slot that was killed */
	for(x=0; x < pool->count; x++) {
		wp = &pool->workers[x];
		if (wp->flags & FLAG_KILL) {
			dprintf("worker[%d]: locking slot...\n",wp->slot);
			pthread_mutex_lock(&wp->lock);

			dprintf("worker[%d]: re-creating thread...\n",wp->slot);
			pthread_create(&wp->tid, NULL, worker, wp);

			dprintf("worker[%d]: clearing flags...\n",wp->slot);
			wp->flags = 0;

			dprintf("worker[%d]: unlocking...\n",wp->slot);
			pthread_mutex_unlock(&wp->lock);
		}
	}
}

void worker_kill(worker_pool_t *pool) {
	register int x;

	if (!pool) return;

	/* Send signal to workers */
	dprintf("killing workers!\n");
	for(x=0; x < pool->count; x++) pthread_kill(pool->workers[x].tid, SIGTERM);
}

void worker_cancel(worker_pool_t *pool) {
	register int x;

	if (!pool) return;

	/* Kill the workers - like all good pharoahs */
	dprintf("killing workers...\n");
	for(x=0; x < pool->count; x++) pthread_cancel(pool->workers[x].tid);
}

void worker_finish(worker_pool_t *pool, int timeout) {
	register int x;
	worker_info_t *wp;

	if (!pool) return;

	/* Mark done */
	dprintf("Waiting for finish...\n");
	for(x=0; x < pool->count; x++) {
		wp = &pool->workers[x];
		pthread_mutex_lock(&wp->lock);
		wp->flags |= FLAG_DONE;
#if USE_COND
		/* Signal worker */
		dprintf("worker[%d]: signaling...\n",wp->slot);
		pthread_cond_signal(&wp->cond);
#endif
		pthread_mutex_unlock(&wp->lock);
	}

	worker_wait(pool,timeout);
	worker_cancel(pool);
}
