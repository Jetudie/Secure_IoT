#include <stdlib.h>
#include "lorenz.h"

#define master_x1m 0.1
#define master_x2m -0.1
#define master_x3m 0.4

#define slave_x1s 0.2
#define slave_x2s -0.5
#define slave_x3s 0.2
#define slave_alpha 0.5
#define slave_c 49

static int SyncLorenzMaster(Master* self)
{
    float x1, x2, x3;
	x1 = 0.990*self->x1m + 0.01*self->x2m;
	x2 = 0.028*self->x1m + 0.999*self->x2m - 0.001*self->x1m*self->x3m;
	x3 = 0.997*self->x3m + 0.001*self->x1m*self->x2m;

	self->x1m = x1;
	self->x2m = x2;
	self->x3m = x3;
    return 0;
}

static int SyncLorenzSlave(Slave* self, void* xm)
{
    float x1m = *((float*)xm);
    float x2m = *(((float*)xm)+1);
    float x3m = *(((float*)xm)+2);
    float x1, x2, x3;

	self->e1 = self->x1s - x1m;
	self->e2 = self->x2s - x2m;
	self->e3 = self->x3s - x3m;
    self->s=self->e2+self->c*self->e1;

	self->f = (0.028-0.01*self->c)*self->e1 + (0.01*self->c-0.001)*self->e2 + 0.001*x1m*x3m;
	self->u = -self->f - self->alpha*self->s;	
	x1 = 0.99*self->x1s + 0.01*self->x2s;
	x2 = 0.028*self->x1s + 0.999*self->x2s + self->u;
	x3 = 0.997*self->x3s + 0.001*self->x1s*x2m;

	self->x1s = x1;
	self->x2s = x2;
	self->x3s = x3;
    
    return 1;
}

int init_master(Master **self)
{
    if (NULL == (*self = malloc(sizeof(Master)))) return -1;
    (*self)->xm_init[0] = master_x1m;
    (*self)->xm_init[1] = master_x2m;
    (*self)->xm_init[2] = master_x3m;
    (*self)->x1m = (*self)->xm_init[0]; 
    (*self)->x2m = (*self)->xm_init[1]; 
    (*self)->x3m = (*self)->xm_init[2];
    (*self)->sync = SyncLorenzMaster;
    return 0;
}

int init_slave(Slave **self)
{
    if (NULL == (*self = malloc(sizeof(Slave)))) return -1;
    (*self)->x1s = slave_x1s; 
    (*self)->x2s = slave_x2s; 
    (*self)->x3s = slave_x3s;
    (*self)->e1 = 0; 
    (*self)->e2 = 0; 
    (*self)->e3 = 0;
    (*self)->f = 0; 
    (*self)->u = 0; 
    (*self)->s = 0; 
    (*self)->alpha = slave_alpha;
    (*self)->c = slave_c;
    (*self)->sync = SyncLorenzSlave;
    return 0;
}