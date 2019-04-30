/*
 * luaclass.c
 *
 *  Created on: 2017年12月26日
 *      Author: cj
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "luaclass.h"

#define classname "luaclass"

struct luaclassObject {
	int id;
	char name[10];
	void *context;
};

static int luaclass_new(lua_State * L) {
	struct luaclassObject *object = lua_newuserdata(L, sizeof(struct luaclassObject));
	memset(object, 0, sizeof(struct luaclassObject));
	luaL_getmetatable(L, classname);
	lua_setmetatable(L, -2);
	object->context = malloc(100);
	memset(object->context, 'Q', 99);
	return 1;
}

static int luaclass_gc(lua_State * L) {
	struct luaclassObject *object = luaL_checkudata(L, 1, classname);
	if (object->context) {
		free(object->context);
		printf("free->luaclass\n");
	}
	return 1;
}

//
static int luaclass_test(lua_State * L) {
	struct luaclassObject *object = luaL_checkudata(L, 1, classname);
	printf("%s\n", (char*)object->context);
	return 1;
}

static const luaL_Reg luaclass[] = {
		{ "new", luaclass_new },
		{ "__gc", luaclass_gc },
		{ "test",	luaclass_test },
		{ NULL, NULL }
};

int luaopen_luaclass(lua_State *L) {
	luaL_newmetatable(L, classname); /* create metatable */
	lua_pushvalue(L, -1);  /* metatable.__index = metatable */
	lua_setfield(L, -2, "__index");
	luaL_register(L, NULL, luaclass); /* fill metatable */
	lua_pop(L, 1);
	luaL_register(L, classname, luaclass); /* create module */
	return 1;
}

void luaL_openluaclass(lua_State *L) {
	lua_pushcfunction(L, luaopen_luaclass);
	lua_pushstring(L, classname);
	lua_call(L, 1, 0);
}
