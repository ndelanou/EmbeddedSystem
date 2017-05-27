#include "recipe.h"

/*
	Function in charge of filling the recipe of an Engine with a test recipe
*/
void Fill_Test_Recipe1(Engine* e) {
	e->recipe[0] = MOV + 0;
	e->recipe[1] = MOV + 5;
	e->recipe[2] = MOV + 0;
	e->recipe[3] = MOV + 4;
	e->recipe[4] = LOOP_START + 5;
	e->recipe[5] = MOV + 1;
	e->recipe[6] = MOV + 4;
	e->recipe[7] = END_LOOP;
	e->recipe[8] = MOV + 0;
	e->recipe[9] = MOV + 2;
	e->recipe[10] = WAIT + 0;
	e->recipe[11] = SYNC;
	e->recipe[12] = WAIT + 0;
	e->recipe[13] = MOV + 2;
	e->recipe[14] = MOV + 3;
	e->recipe[15] = WAIT + 31;
	e->recipe[16] = WAIT + 31;
	e->recipe[17] = WAIT + 31;
	e->recipe[18] = MOV + 4;
	e->recipe[19] = MOV + 5;
	e->recipe[20] = MOV + 0;
	e->recipe[21] = MOV + 1;
	e->recipe[22] = MOV + 0;
	e->recipe[23] = MOV + 2;
	e->recipe[24] = MOV + 0;
	e->recipe[25] = MOV + 3;
	e->recipe[26] = MOV + 0;
	e->recipe[27] = MOV + 4;
	e->recipe[28] = MOV + 0;
	e->recipe[29] = MOV + 5;
	e->recipe[30] = RECIPE_END;
}


void Fill_Test_Recipe2(Engine* e) {
	e->recipe[0] = MOV + 0;
	e->recipe[1] = MOV + 5;
	e->recipe[2] = MOV + 0;
	e->recipe[3] = MOV + 4;
	e->recipe[4] = LOOP_START + 5;
	e->recipe[5] = MOV + 1;
	e->recipe[6] = MOV + 5;
	e->recipe[7] = END_LOOP;
	e->recipe[8] = MOV + 0;
	e->recipe[9] = MOV + 2;
	e->recipe[10] = WAIT + 0;
	e->recipe[11] = SYNC;
	e->recipe[12] = WAIT + 0;
	e->recipe[13] = MOV + 2;
	e->recipe[14] = MOV + 3;
	e->recipe[15] = WAIT + 31;
	e->recipe[16] = WAIT + 31;
	e->recipe[17] = WAIT + 31;
	e->recipe[18] = MOV + 4;
	e->recipe[19] = MOV + 5;
	e->recipe[20] = MOV + 0;
	e->recipe[21] = MOV + 1;
	e->recipe[22] = MOV + 0;
	e->recipe[23] = MOV + 2;
	e->recipe[24] = MOV + 0;
	e->recipe[25] = MOV + 3;
	e->recipe[26] = MOV + 0;
	e->recipe[27] = MOV + 4;
	e->recipe[28] = MOV + 0;
	e->recipe[29] = MOV + 5;
	e->recipe[30] = RECIPE_END;
}
