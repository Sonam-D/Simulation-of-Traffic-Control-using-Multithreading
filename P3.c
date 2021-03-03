#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
//double time;
double GetTime ()
{
struct timeval t;
int rc = gettimeofday (&t, NULL);
assert (rc == 0);
return (double) t.tv_sec + (double) t.tv_usec / 1e6;

} 
 
void Spin (int howlong)
{
double t = GetTime ();
while ((GetTime () - t) < (double) howlong)
    
;				// do nothing in loop
} 


unsigned int usecs;
int Car_Arrive_N_N= 0;//this means a car is already present in the signal North 0 - no car, 1 - car present
int Car_Arrive_S_S =0;//this means a car is already present in the signal south 0 - no car, 1 - car present
int Car_Arrive_E_E =0;//this means a car is already present in the signal east 0 - no car, 1 - car present
int Car_Arrive_W_W =0;//this means a car is already present in the signal west 0 - no car, 1 - car present
int counter = 0;
pthread_mutex_t linehold_mutex;// linehold mutex for car in North
pthread_mutex_t lineholdS_mutex;// linehold mutex for car in South
pthread_mutex_t lineholdE_mutex;// linehold mutex for car in East
pthread_mutex_t lineholdW_mutex;// linehold mutex for car in West

pthread_mutex_t N_mutex;//mutex lock for car going from North to north
pthread_mutex_t N_W_mutex; //mutex lock for car going from North to west
pthread_mutex_t S_mutex; //mutex lock for car going from South to south
pthread_mutex_t E_mutex; //mutex lock for car going from East to east
pthread_mutex_t W_mutex; //mutex lock for car going from West to west

pthread_cond_t linehold_cv;// condition variable to signal car is gone for North(original direction)
pthread_cond_t lineholdS_cv;// condition variable to signal car is gone for Nsouth(original direction)
pthread_cond_t lineholdE_cv;// condition variable to signal car is gone for East(original direction)
pthread_cond_t lineholdW_cv;// condition variable to signal car is gone for west(original direction)
pthread_cond_t N_W_cv;//CV to signal car(original dir- North, Target dir- West) has passed
pthread_cond_t N_N_cv ;//CV to signal car(original dir- North, Target dir- North) has passed
typedef struct _directions
{
  char dir_original;
  int car_id;
  char dir_target;
}directions;

int local_time=0,local_time1=0,local_time2=0,local_time3=0, local_time5=0;//variables to output time
int N_N,N_W,S_S,E_N,W_N;//if N_N= 1-> car crossing from North to north
int N_G=0,N_Y=0,N_R=0,S_G=0,S_R=0,S_Y=0;//Green ,red,Yelllow lights for Norts and South
int E_G=0,E_Y=0,E_R=0,W_G=0,W_Y=0,W_R=0; //Green ,red,Yelllow lights for East & West

void ArriveIntersection(void *arg);
void CrossIntersection(void *arg) ;
void ExitIntersection(void *arg) ;

void *traffic_signal() {

N_G=1;S_G=1;E_R=1;W_R=1;//initial values of traffic signal for 1st 18 sec
usleep(18000000);//delay of 18 sec
local_time2= local_time2+18000000;

N_G=0;S_G=0;//after 18sec , green lights are off and yellow is ON
N_Y=1;S_Y=1;
usleep(2000000);//delay
local_time2= local_time2+2000000;
//after another 2 sec, East and West become Green, and the corresponding threads are
//signalled
E_G=1;W_G=1;N_R=1;S_R=1;N_Y=0;S_Y=0;
if(E_G==1){
    pthread_mutex_lock(&lineholdE_mutex);
        E_G=1;E_R=0;
        pthread_cond_signal(&lineholdE_cv);
    pthread_mutex_unlock(&lineholdE_mutex);
}
}


void *car(void *arg) {
directions *m = (directions *) arg;

    ArriveIntersection(arg);
    CrossIntersection(arg);
    ExitIntersection(arg);
}

void ArriveIntersection(void *arg) {
    
directions *m = (directions *) arg;//unpack - car ID, and directions
printf("Time %.2f:Car %d (->%c->%c ) arriving\n ",usecs/1000000.0,m->car_id,m->dir_original, m->dir_target);

if(m->dir_original == 'N'){
pthread_mutex_lock(&linehold_mutex);
while(Car_Arrive_N_N==1 || N_R ==1){//car ahead of you or red signal
            pthread_cond_wait(&linehold_cv, &linehold_mutex);}
pthread_mutex_unlock(&linehold_mutex);

}

if(m->dir_original == 'S' && m->dir_target== 'S'){
pthread_mutex_lock(&lineholdS_mutex);
while(Car_Arrive_S_S==1 || S_R ==1){//car ahead of you or red signal
        pthread_cond_wait(&lineholdS_cv, &lineholdS_mutex);}
pthread_mutex_unlock(&lineholdS_mutex);    
    
pthread_mutex_lock(&N_W_mutex);
while(N_W==1 || S_R ==1){//N_W = 1 means a car is crossing from North to West..so this one should //wait
            pthread_cond_wait(&N_W_cv, &N_W_mutex);}
pthread_mutex_unlock(&N_W_mutex);

}

if(m->dir_original == 'S' && m->dir_target== 'E'){

pthread_mutex_lock(&N_W_mutex);
pthread_mutex_lock(&N_mutex);
while(N_W==1 || S_R ==1 || N_N ==1){//conditions for wait - Car going in North to West direc; 
                                    //Car going from North to Norrth
            pthread_cond_wait(&N_W_cv, &N_W_mutex);
            pthread_cond_wait(&N_N_cv, &N_mutex);
}
pthread_mutex_unlock(&N_mutex);
pthread_mutex_unlock(&N_W_mutex);

}

if(m->dir_original == 'E' && m->dir_target== 'N'){
pthread_mutex_lock(&lineholdE_mutex);
while(Car_Arrive_E_E==1 || E_R ==1){//condition for wait - car ahead of you or red signal
    pthread_cond_wait(&lineholdE_cv, &lineholdE_mutex);}
   pthread_mutex_unlock(&lineholdE_mutex); 
      }

if(m->dir_original == 'W' && m->dir_target== 'N'){//line hold lock
    pthread_mutex_lock(&lineholdW_mutex);
    while(Car_Arrive_W_W==1){//car ahead of you or red signal
            pthread_cond_wait(&lineholdW_cv, &lineholdW_mutex);}
pthread_mutex_unlock(&lineholdW_mutex);  
  
pthread_mutex_lock(&W_mutex);
while(N_N==1){//N_N = 1 means a car is crossing from North to North..so this one should wait
            pthread_cond_wait(&N_N_cv, &W_mutex);}
pthread_mutex_unlock(&W_mutex);  
}

}

void CrossIntersection(void *arg) {
 //   double time = counter/10.0;
directions *m = (directions *) arg;//unpack

if(m->dir_original=='N'){
pthread_mutex_lock(&linehold_mutex);
        Car_Arrive_N_N=0;//release the linehold lock
         pthread_cond_signal(&linehold_cv);
        pthread_mutex_unlock(&linehold_mutex);
printf("Time %.2f:Car %d (->%c -> %c ) crossing\n ",usecs/1000000.0,m->car_id,m->dir_original, m->dir_target);   
}

if(m->dir_target=='N' && m->dir_original=='N'){
pthread_mutex_lock(&N_mutex);
N_N=1;
local_time= usecs+2000000;//time to simulate crossing
Spin(1);//delay
}

if(m->dir_target=='W' && m->dir_original=='N'){
pthread_mutex_lock(&N_W_mutex);//N_W
N_W=1;
int local_time = usecs +3000000;
Spin(1);
Spin(1);
Spin(1);
}

if(m->dir_original=='S'){
pthread_mutex_lock(&lineholdS_mutex);
        Car_Arrive_S_S=0;
         pthread_cond_signal(&lineholdS_cv);
        pthread_mutex_unlock(&lineholdS_mutex);

printf("Time %.2f:Car %d (->%c -> %c ) crossing\n ",local_time/1000000.0,m->car_id,m->dir_original, m->dir_target);

//after above conditions are fulfilled start crossing
pthread_mutex_lock(&S_mutex);
S_S=1;//started crossing
local_time5= local_time;
local_time= local_time+2000000;
Spin(1);
}

if(m->dir_target=='E' && m->dir_original=='S'){
pthread_mutex_lock(&N_W_mutex);//N_W
local_time5 = local_time5 +3000000;
Spin(1);//chech spin 2
Spin(1);
}

if(m->dir_original=='E'){
pthread_mutex_lock(&lineholdE_mutex);
        Car_Arrive_E_E=0;
         pthread_cond_signal(&lineholdE_cv);
        pthread_mutex_unlock(&lineholdE_mutex);
 
printf("Time %.2f:Car %d (->%c -> %c ) crossing\n ",local_time2/1000000.0,m->car_id,m->dir_original, m->dir_target);   //modify this tomo
pthread_mutex_lock(&E_mutex);//N_W
E_N=1;
 local_time2 = local_time2 +3000000;
Spin(1);//chech spin 2
Spin(1);
Spin(1);
    
}
if(m->dir_original == 'W' && m->dir_target== 'N'){
printf("Time %.2f:Car %d (%c - %c ) crossing3\n ",local_time/1000000.0,m->car_id,m->dir_original, m->dir_target);
pthread_mutex_lock(&W_mutex);//N_W
W_N=1;
 local_time3 = local_time +1000000;//right turn - 1sec
Spin(1);

}
}
void ExitIntersection(void *arg) {
directions *m = (directions *) arg;
if(m->dir_target=='N' && m->dir_original=='N'){

printf("Time %.2f:Car %d (%c - %c ) exit\n",local_time/1000000.0,m->car_id,m->dir_original, m->dir_target);
  N_N=0;
  Spin(1);
  pthread_cond_signal(&N_N_cv);
  pthread_mutex_unlock(&N_mutex);

}

if(m->dir_target=='S' && m->dir_original=='S'){

// local_time = usecs +2000000;
printf("Time %.2f:Car %d (->%c -> %c ) exit\n",(local_time)/1000000.0,m->car_id,m->dir_original, m->dir_target);
  S_S=0;
  Spin(1);
 pthread_mutex_unlock(&S_mutex);

}
if(m->dir_target=='W' && m->dir_original=='N'){
//int local_time = usecs +3000000;
printf("Time %.2f:Car %d (->%c -> %c ) exit\n",(local_time)/1000000.0,m->car_id,m->dir_original, m->dir_target);
  N_W=0;
 Spin(1);  
  pthread_cond_signal(&N_W_cv);
  pthread_mutex_unlock(&N_W_mutex);

}
    
if(m->dir_target=='E' && m->dir_original=='S'){
    
printf("Time %.2f:Car %d (->%c -> %c ) exit\n",(local_time5)/1000000.0,m->car_id,m->dir_original, m->dir_target);
 Spin(1);  
//  pthread_cond_signal(&N_W_cv);
  pthread_mutex_unlock(&N_W_mutex);
}

if(m->dir_original=='E'){
    
printf("Time %.2f:Car %d (->%c -> %c ) exit\n",local_time2/1000000.0,m->car_id,m->dir_original, m->dir_target);
 E_N=0;
 Spin(1);
 pthread_mutex_unlock(&E_mutex);

}

if(m->dir_original == 'W' && m->dir_target== 'N'){
    printf("Time %.2f:Car %d (->%c -> %c ) exit\n",local_time3/1000000.0,m->car_id,m->dir_original, m->dir_target);
W_N=0;
Spin(1);
pthread_mutex_unlock(&W_mutex);
}

}
int main () 
{
pthread_t a,t0,t1,t2,t3,t4,t5,t6,t7;
directions dir1,dir2,dir3,dir4,dir5,dir6,dir7,dir8;
usecs =0;//time variable
pthread_create(&a, NULL, traffic_signal, NULL);//create thread for traffic signal at time0
usleep(100000);
usecs =500000;
usleep (usecs);

usecs = usecs+600000;
usleep(usecs);

dir1.dir_original = 'N';
dir1.dir_target = 'N';
dir1.car_id = 0;

dir2.dir_original = 'N';
dir2.dir_target = 'N';
dir2.car_id = 1;

dir3.dir_original = 'N';
dir3.dir_target = 'W';
dir3.car_id = 2;

dir4.dir_original = 'S';
dir4.dir_target = 'S';
dir4.car_id = 3;

dir5.dir_original = 'S';
dir5.dir_target = 'E';
dir5.car_id = 4;

dir6.dir_original = 'N';
dir6.dir_target = 'N';
dir6.car_id = 5;

dir7.dir_original = 'E';
dir7.dir_target = 'N';
dir7.car_id = 6;

dir8.dir_original = 'W';
dir8.dir_target = 'N';
dir8.car_id = 7;
pthread_create(&t0, NULL, car, &dir1);//thread for Car0 at time 1.1
usleep (900000);
usecs= usecs + 900000; 

pthread_create(&t1, NULL, car, &dir2);//thread for Car1 at time 2
usleep (1300000);
usecs= usecs + 1300000; 

pthread_create(&t2, NULL, car, &dir3); //thread for Car2 at time 3.3
usleep(200000);
usecs= usecs +200000;

pthread_create(&t3, NULL, car, &dir4);//thread for Car3 at time 3.5
usleep(700000);
usecs= usecs + 700000;

pthread_create(&t4, NULL, car, &dir5);//thread for Car4 at time 4.2
usleep(200000);
usecs = usecs + 200000;

pthread_create(&t5, NULL, car, &dir6);//thread for Car5 at time 4.4
usleep(1300000);
usecs = usecs + 1300000;

pthread_create(&t6, NULL, car, &dir7);//thread for Car6 at time 5.7
usleep(200000);
usecs = usecs+200000;

pthread_create(&t7, NULL, car, &dir8);//thread for Car7 at time 5.9
usleep(27000000);//wait 
printf ("End of prog");
  
return 0;

}

