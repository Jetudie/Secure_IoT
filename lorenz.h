#ifndef LORENZ_H_
#define LORENZ_H_

typedef struct master Master;
typedef struct slave Slave;

typedef int (*func_master)(Master *);
typedef int (*func_slave)(Slave *, void *);

int init_master(Master **self);
int init_slave(Slave **self);

static int SyncLorenzMaster(Master *);
static int SyncLorenzSlave(Slave *, void *);

struct master{
    int confirm;
	float xm_init[3];
    float x1m;
    float x2m;
    float x3m;
    func_master sync;
};

struct slave{
    float x1s;
    float x2s;
    float x3s;
    float e1;
    float e2;
    float e3;
	float f;
	float u;
    float s;
    float alpha;
    int c;
    func_slave sync;
};

#endif