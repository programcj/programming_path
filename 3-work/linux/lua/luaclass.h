/*
 * luaclass.h
 *
 *  Created on: 2017年12月26日
 *      Author: cj
 */

#ifndef LUACLASS_H_
#define LUACLASS_H_

#include "lua5.1.5/lua.h"
#include "lua5.1.5/lualib.h"
#include "lua5.1.5/lauxlib.h"

int luaopen_luaclass(lua_State *L) ;
void luaL_openluaclass(lua_State *L);

#endif /* LUACLASS_H_ */
