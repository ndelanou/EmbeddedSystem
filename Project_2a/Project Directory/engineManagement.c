#include "engineManagement.h"

/*
	Initialisation of an Engine struct
*/
void Engine_Init(Engine* e, enum Channel channel_id) {
	e->index_loop = 0;
	e->index_recipe = 0;
	e->start_loop_index = 0;
	e->wait_cpt = 10;
	e->current_postion = 0;
	e->recipe_ended = 0;
	e->chan = channel_id;
	e->in_Pause = 0 ;
	e->in_Sync = 0;
}

/*
	Function called in the interrupt handler and in charge of following the recipe
	Calls the right function with the right parameters from a recipe instruction
*/
void Engine_Management(Engine* e) {
	
	if(!e->recipe_ended && !e->in_Pause && !e->in_Sync) {
		if(e->wait_cpt != 0) 
			e->wait_cpt--;
		else {
			switch(e->recipe[e->index_recipe] & 0xE0) {
			 case MOV  :
					Mov(e, e->recipe[e->index_recipe] & 0x1F);
					break;
			
			 case WAIT  :
					Wait(e, e->recipe[e->index_recipe] & 0x1F);
					break;
			 
			 case LOOP_START  :
					Loop_Start(e, e->recipe[e->index_recipe] & 0x1F);
					break;
			 
			 case END_LOOP  :
					End_Loop(e);
					break;
			 
			 case RECIPE_END  :
					Recipe_End(e);
					break;
			 
			 case JUMP :
					Jump(e, e->recipe[e->index_recipe] & 0x1F);
					break;
			 
			 case SYNC :
					Sync(e);
					break;
			}
			e->index_recipe++;
		}
	}
}

/*
	Recipe function which move the engine to a given position
*/
void Mov(Engine* e, uint8_t val) {
	SetPosition(e, val);
	
	if(val > e->current_postion) {
		e->wait_cpt = 2 * (val - e->current_postion);
	}
	else {
		e->wait_cpt = -2 * (val - e->current_postion);
	}
	
	e->current_postion = val;
}

/*
	Recipe function making an engine wait for a given amount of time
*/
void Wait(Engine* e, uint8_t val) {
	e->wait_cpt = val;
}

/*
	Recipe function managing the loop
*/
void Loop_Start(Engine* e, uint8_t val) {
	e->start_loop_index = e->index_recipe;
	e->index_loop = val;
}

/*
	Recipe function going back on the start of the loop if the loop_index isn't equal to 0
*/
void End_Loop(Engine* e) {
	if(e->index_loop != 0) {
		e->index_recipe = e->start_loop_index;
		e->index_loop--;
	}
}

/*
	End of the Recipe
*/
void Recipe_End(Engine* e) {
	e->recipe_ended = 1;
}

/*
	ADDITIONAL FUNCTION
	Recipe function jumping on the given instruction of the recipe
*/
void Jump(Engine* e, uint8_t val) {
	e->index_recipe = val-1;
}

/*
	ADDITIONAL FUNCTION
	Recipe function waiting for both engines on synchronisation to start again
	Could be use as a synchronize function
*/
void Sync(Engine* e) {
	e->in_Sync = 1;
}

/*
	Function called in the Mov(...) function and generating the right PWM for the given position
*/
void SetPosition(Engine* e, int position) {
	if(e->chan==CHANNEL_1_id) {
		switch (position) {
			case 0 : TIM2->CCR1 = POSITION_0;
			break;
			case 1 : TIM2->CCR1 = POSITION_1;
			break;
			case 2 : TIM2->CCR1 = POSITION_2;
			break;
			case 3 : TIM2->CCR1 = POSITION_3;
			break;
			case 4 : TIM2->CCR1 = POSITION_4;
			break;
			case 5 : TIM2->CCR1 = POSITION_5;
			break;
			default: break;
		}
	}
	else if(e->chan==CHANNEL_2_id) {
		switch (position) {
			case 0 : TIM2->CCR2 = POSITION_0;
			break;
			case 1 : TIM2->CCR2 = POSITION_1;
			break;
			case 2 : TIM2->CCR2 = POSITION_2;
			break;
			case 3 : TIM2->CCR2 = POSITION_3;
			break;
			case 4 : TIM2->CCR2 = POSITION_4;
			break;
			case 5 : TIM2->CCR2 = POSITION_5;
			break;
			default: break;
		}
	}
}

/*
	Make the enginne go left if possible
*/
int goLeft(Engine* e) {
	if(e->current_postion != 5 && (e->in_Pause || e->recipe_ended)) {
		SetPosition(e, ++e->current_postion);
		return 0;
	}
	else return -1;
}

/*
	Make the enginne go right if possible
*/
int goRight(Engine* e) {
	if(e->current_postion != 0 && (e->in_Pause || e->recipe_ended)) {
		SetPosition(e, --e->current_postion);
		return 0;
	}
	else return -1;
}

/*
	Restart the recipe of an engine
*/
void Restart_Recipe(Engine* e) {
	SetPosition(e, 0);
	e->current_postion = 0;
	e->index_loop = 0;
	e->index_recipe = 0;
	e->recipe_ended = 0;
	e->start_loop_index = 0;
	e->wait_cpt = 10;
}
