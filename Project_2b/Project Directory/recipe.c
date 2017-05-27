#include "recipe.h"

/*
	Function in charge of filling the recipe of an Engine with a test recipe
*/
void Fill_Test_Recipe1(Engine* e) {
	int i = 0;

	e->recipe[i++] = MOV + 5;
	e->recipe[i++] = WAIT + 31;

	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 5;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 4;

	e->recipe[i++] = LOOP_START + 5;
	e->recipe[i++] = MOV + 1;
	e->recipe[i++] = MOV + 4;
	e->recipe[i++] = END_LOOP;

	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 2;
	e->recipe[i++] = WAIT + 0;
	e->recipe[i++] = MOV + 1;
	e->recipe[i++] = MOV + 3;

	e->recipe[i++] = WAIT + 31;
	e->recipe[i++] = WAIT + 31;
	e->recipe[i++] = WAIT + 31;

	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 1;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 2;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 3;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 4;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 5;

	e->recipe[i++] = RECIPE_END;
}


void Fill_Test_Recipe2(Engine* e) {
	int i = 0;

	e->recipe[i++] = MOV + 5;
	e->recipe[i++] = WAIT + 31;

	e->recipe[i++] = LOOP_START + 5;
	e->recipe[i++] = MOV + 1;
	e->recipe[i++] = MOV + 4;
	e->recipe[i++] = END_LOOP;

	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 1;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 2;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 3;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 4;
	e->recipe[i++] = MOV + 0;
	e->recipe[i++] = MOV + 5;

	e->recipe[i++] = RECIPE_END;
}
