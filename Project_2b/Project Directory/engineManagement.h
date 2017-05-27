#include <time.h>
#include <stdint.h>       	/* for uintptr_t */

#define MOV (0x20)
#define WAIT (0x40)
#define LOOP_START (0x80)
#define END_LOOP (0xA0)
#define RECIPE_END (0x00)
#define JUMP (0xE0)
#define SYNC (0xC0)

#define AUTORESET_20MS_80MHZ (1600000)
#define POSITION_0 (600000)
#define POSITION_1 (900000)
#define POSITION_2 (1150000)
#define POSITION_3 (1500000)
#define POSITION_4 (1800000)
#define POSITION_5 (2450000)

//typedef enum Channel Channel
enum Channel {
	CHANNEL_1_id, CHANNEL_2_id
};

typedef struct Engine Engine;
struct Engine
{
	unsigned char recipe[100];
	int wait_cpt;
	int index_recipe;
	int index_loop;
	int start_loop_index;
	int current_postion;
	int recipe_ended;
	enum Channel chan;
	int in_Pause;
	int in_Sync;
	struct timespec upTime;
	uintptr_t data_handler;
};

void Engine_Management(Engine* e);
void Engine_Init(Engine* e, enum Channel channel_id);
void Mov(Engine* e, unsigned char val);
void Wait(Engine* e, unsigned char val);
void Loop_Start(Engine* e, unsigned char val);
void End_Loop(Engine* e);
void Recipe_End(Engine* e);
void Jump(Engine* e, unsigned char val);
void Sync(Engine* e);
void SetPosition(Engine* e, int posotion);
int goLeft(Engine* e);
int goRight(Engine* e);
void Restart_Recipe(Engine* e);
