
extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream>

#include "demo_select.h"

using namespace std;

int n = 0;

int func(lua_State *L)
{
	printf("func_top=%d top=%s\n", lua_gettop(L), lua_tostring(L, -1));

	if (!n)
	{
		++n;
		int result = lua_yield(L, 1); // nresult = 1 
		printf("continue running  %d\n", result);
		return 2;
	}
	else {
		return 1;
	}
}

#ifdef TEST_COROUTINE 
int main(int argc, char* const argv[])
#else 
int test(int argc, char* const argv[])
#endif 
{

	lua_State *L = luaL_newstate();

	/* init lua library */
	lua_pushcfunction(L, luaopen_base);
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		return 1;
	}


	lua_pushcfunction(L, luaopen_package);
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		return 2;
	}

	/* create the coroutine */
	lua_State *LL = lua_newthread(L);

	lua_pushcfunction(LL, func);
	lua_pushstring(LL, "hello world"); // ��ջ 

	const char * str = lua_tostring(LL, -1);
	std::cout << "ջ����һ��(��ջ)" << str << std::endl;    // ��ջ�ж�ȡ�ַ���
	std::cout << "ջ���ڶ����Ƿ�c���� " << (lua_iscfunction(LL, -2) ? "True" : "False") << std::endl;
	std::cout << "begin top " << lua_gettop(LL) << std::endl;

	/* first time resume */
	int res = 0;
	if (lua_resume(LL, L, 1, &res) == LUA_YIELD)
	{
		// int lua_gettop(lua_State* lua)	���ջ�Ĵ�С

		printf("first_top=%d top_string=%s\n", lua_gettop(LL), lua_tostring(LL, -1));


		/* twice resume */
		if (lua_resume(LL, L, 1, &res) == 0)
		{
			printf("second_top=%d top_string=%s\n", lua_gettop(LL), lua_tostring(LL, -1));
		}
	}

	lua_close(L);

	return 0;
}



void printLuaStack(lua_State* lua) {
	/*

	��ջ����ջ�����μ�С���ȿ�����������Ҳ�����Ǹ��� ��
	�������������ջ��Ԫ�ص������ȿ�����5��Ҳ���Է��������-1�����ߵȼ�

	========= content of stack from top to bottom: ===========
	5 [-1]	number: 	100.00
	4 [-2]	boolean: 	1
	3 [-3]	string: 	hello world
	2 [-4]	number: 	1.20
	1 [-5]	nil
	stackSize = 5

	*/


	std::cout << "========= content of stack from top to bottom: ===========" << std::endl;

	int stackSize = lua_gettop(lua);                    // ���ջ��Ԫ�ظ���
	for (int i = stackSize; i>0; --i) {
		//pv("%d [%d]\t", i, -1 - (stackSize - i));
		int t = lua_type(lua, i);                       // �жϵ�ǰԪ������
		switch (t) {
		case LUA_TNUMBER:
			//pv("%s: \t%.2f\n", lua_typename(lua, t), lua_tonumber(lua, i));     // ��ӡ�������ƺ�ֵ
			break;
		case LUA_TBOOLEAN:
			//pv("%s: \t%d\n", lua_typename(lua, t), lua_toboolean(lua, i));
			break;
		case LUA_TSTRING:
			//pv("%s: \t%s\n", lua_typename(lua, t), lua_tostring(lua, i));
			break;
		default:
			// LUA_TTABLE
			// LUA_TTHREAD
			// LUA_TFUNCTION
			// LUA_TLIGHTUSERDATA
			// LUA_TUSERDATA
			// LUA_TNIL
			//pv("%s\n", lua_typename(lua, t));                                   // ���ô�ӡ�����ͣ���ʱ����ӡ��������
			break;
		}
	}
	std::cout << stackSize << std::endl;
}
