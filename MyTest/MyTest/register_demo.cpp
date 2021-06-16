extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <iostream>
#include <string>

#include "demo_select.h"

struct _ScriptValue
{
public:
	_ScriptValue(int _id, const char* _name)
	{
		id = _id;
		name = _name;
	}
	int id;
	std::string name;
};
typedef struct _ScriptValue MyScriptValue;


static void stackDump(lua_State *L);
static void snapshot(lua_State* L, int appendIndex);
static void compare(lua_State* L, int beforeIndex, int afterIndex, int compareIndex);


#ifdef TEST_REGISTER
int main(int argc, char* const argv[])
#else 
int test(int argc, char* const argv[])
#endif
{
	//
	//--------------------------------------------------------------------------------------------------
	// 建立lua虚拟机 
	lua_State *L = luaL_newstate();

	luaL_openlibs(L); // 必须调用这个 否则lua层 string os等库就找不到了

	/* 
	luaL_openlibs 会调用base package等c函数,初始化lua环境和加载库
	static const luaL_Reg loadedlibs[] = {
		{ LUA_GNAME, luaopen_base },
		{ LUA_LOADLIBNAME, luaopen_package },
		{ LUA_COLIBNAME, luaopen_coroutine },
		{ LUA_TABLIBNAME, luaopen_table },
		{ LUA_IOLIBNAME, luaopen_io },
		{ LUA_OSLIBNAME, luaopen_os },
		{ LUA_STRLIBNAME, luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ LUA_UTF8LIBNAME, luaopen_utf8 },
		{ LUA_DBLIBNAME, luaopen_debug },
		{ NULL, NULL }
	};
	*/
	/* init lua library */
	//lua_pushcfunction(L, luaopen_base);
	//if (lua_pcall(L, 0, 0, 0) != 0)
	//{
	//	return 1;
	//}

	//lua_pushcfunction(L, luaopen_package);
	//if (lua_pcall(L, 0, 0, 0) != 0)
	//{
	//	return 2;
	//}



	//
	//--------------------------------------------------------------------------------------------------
	// 加载定义的lua文件
	int ret = luaL_loadfile(L, "DumpLua.lua");
	if (ret != LUA_OK) {
		// 返回结果和lua_load返回的结果相同
		// 0：没有错误
		// 1：有错误  
		// LUA_ERRFILE		如果打不开文件或者读取不了的时候有一个额外的错误码

		/*
		//  thread status
		#define LUA_OK		0
		#define LUA_YIELD	1
		#define LUA_ERRRUN	2
		#define LUA_ERRSYNTAX	3	预编译有语法错误
		#define LUA_ERRMEM	4		内存分配错误
		#define LUA_ERRERR	5

		#define  LUA_ERRFILE (LUA_ERRERR + 1)
		*/


		// C++调用lua 失败会把crash堆栈放到栈顶
		std::cout << "main.lua load fail, reason" << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		return 1;
	}
	else {

		// luaL_loadfile 会加载和编译Lua脚本，但不会运行, 栈顶是一个函数function
		// luaL_dofile	 不仅运行编译后的脚本，运行结束后还会把脚本pop出栈

		int type = lua_type(L, -1); // 看栈顶的数据类型
		const char* ctype = lua_typename(L, type);
		std::cout << "after luaL_loadfile stack top is " << ctype << " (" << type << ")" << std::endl; // 6
		stackDump(L);

		// nargs: 待调用函数的参数数量
		// nresults: 期望的结果个数
		// errfunc : 处理错误函数的索引
		// 返回之后 栈顶只有结果/错误 函数和参数已经弹出  
		if (lua_pcall(L, 0, 0, 0) != LUA_OK)
		{
			// 如果运行出错，lua_pcall会返回一个非零的结果, 
			// 如果指定了错误处理函数会先调用错误处理函数，然后再将错误信息入栈，
			// 在将返回结果和错误信息入栈之前会先将"函数和参数"从栈中移除 
			// 错误处理函数必须在被调用函数和其他参数之前入栈
			std::cout << "pcall error " << lua_tostring(L, -1) << std::endl;
			return 1;
		}
	}

	//
	//--------------------------------------------------------------------------------------------------
	// 插入元素之前先sanpshot
	snapshot(L, 1);

	//
	//--------------------------------------------------------------------------------------------------
	// 注册表 增加一个 key是lightuserdata(uuid) value是userdata  

	void* userdata = lua_newuserdata(L, sizeof(MyScriptValue));
	MyScriptValue* res = new(userdata)MyScriptValue{123, "hello world"};
	// -1 userdata 

	lua_pushlightuserdata(L, (void*)(res->id));
	// -1 lightuserdata id 
	// -2 userdata 

	lua_insert(L, -2);
	// -1 userdata           << value 
	// -2 lightuserdata id   << key 

	lua_rawset(L, LUA_REGISTRYINDEX); // 栈顶是table的value
	//  register[lightuserdata] =  userdata;


	//
	//--------------------------------------------------------------------------------------------------
	// 插入元素之后 snapshot注册表 
	snapshot(L, 2);

	compare(L, 1, 2, 3);

	// 搜索关键字 registry.[
	// userdata: 0000022C86A558A8	registry.[table:value]	1
	// userdata : 000000000000007B	registry.[table:key.userdata]	1

	//
	//--------------------------------------------------------------------------------------------------
	// 从注册表中删除刚刚的元素 
 
	void* uuid = (void*)res->id; // 假设uuid是从其他地方保存的, 用来索引userdata

	int top = lua_gettop(L);

	lua_pushlightuserdata(L, uuid);
	lua_rawget(L, LUA_REGISTRYINDEX);

	if (lua_isuserdata(L, -1))
	{
		// lua_pushnil(L);
		// lua_setmetatable(L, -2); // 设置userdata的metable 

		MyScriptValue* value = (MyScriptValue*)lua_touserdata(L, -1);
		std::cout << "get userdata from lua = " << value->name << std::endl;
		lua_pop(L, 1);

		lua_pushlightuserdata(L, uuid); // key 
		lua_pushnil(L);					// value  栈顶是value,所以最后才push
		
		stackDump(L);

		lua_rawset(L, LUA_REGISTRYINDEX);	
	}

	if (top == lua_gettop(L)) {
		std::cout << "stack is good" << std::endl;
	} else {
		std::cout << "stack is sick" << std::endl;
	}

	//
	//--------------------------------------------------------------------------------------------------
	// 删除元素之后 snapshot注册表 
	snapshot(L, 4);

	compare(L, 1, 4, 5); // 最开始和最后的snapshot对比

	return 0;
}


static void snapshot(lua_State* L, int appendIndex)
{
	stackDump(L); // stack len:0

				   // 压入函数和参数
	int luaType = lua_getglobal(L, "Snapshot");	// 待调用的函数  如果没有的话，返回的是0?  0代表获取到的是nil(栈顶) 
												    
	lua_pushstring(L, "profile");				// 压入第一个参数
	lua_pushinteger(L, appendIndex);			// 压入第二个参数

	std::cout << "lua_getglobal(Snapshot) type is " << lua_typename(L, luaType) << " (" << luaType << ")" << std::endl; // funcOnStackIdx = 6
	
	stackDump(L);

	// 完成调用 (2个参数  1个结果) 执行之后，压入的变量会被弹出
	if (lua_pcall(L, 2, 1, 0) != 0) 
	{
		const char* reason = lua_tostring(L, -1);
		std::cout << "error running function " << reason << std::endl;
	}
	else 
	{
		stackDump(L);
		if (!lua_isboolean(L, -1)) 
		{
			std::cout 
				<< "function \"Snapshot\" experted return boolean, but got " 
				<< lua_typename(L, lua_type(L, -1)) 
				<< std::endl;
		}
		else 
		{
			bool result = lua_toboolean(L, -1);
			/*
			lua_pop(L,num)函数从栈顶开始移除
				当num>0时从栈顶移除指定个数 
				当num=0时栈不受影响
				当num=-1时栈中元素全部移除
			*/
			std::cout << "function \"Snapshot\" run result = " << result << std::endl;
		}
		lua_pop(L, 1); // 把结果弹出
		stackDump(L);
	}
}

static void compare(lua_State* L, int beforeIndex, int afterIndex, int compareIndex)
{
	// 把全局变量 name 里的值压栈，返回该值的类型。

	int valueType = lua_getglobal(L, "Compare");
	if (valueType != LUA_TFUNCTION) {
		lua_pop(L, 1); // 把栈顶的nil 弹出 
		std::cout << "lua do NOT have \"Compare\" function" << std::endl;
		return; 
	}

	lua_pushstring(L, "profile");				// 压入第一个参数 lua函数第一个参数先入栈
	lua_pushstring(L, "compare");				// 压入第二个参数
	lua_pushinteger(L, beforeIndex);			
	lua_pushinteger(L, afterIndex);
	lua_pushinteger(L, compareIndex);
	if (lua_pcall(L, 5, 1, 0) != 0)  // 弹出栈上4个作为参数 期望返回一个
	{
		const char* reason = lua_tostring(L, -1);
		std::cout << "error running function " << reason << std::endl;
	}
	else
	{
		stackDump(L);
		if (!lua_isboolean(L, -1))
		{
			std::cout
				<< "function \"Compare\" experted return boolean, but got "
				<< lua_typename(L, lua_type(L, -1))
				<< std::endl;
		}
		else
		{
			bool result = lua_toboolean(L, -1);
			std::cout << "function \"Compare\" run result = " << result << std::endl;
		}
		lua_pop(L, 1); // 把结果弹出
	}

}

/*
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8
*/
static void stackDump(lua_State *L) {
	int i;

	// 返回栈顶元素的索引 因为索引是从1 开始编号的 所以这个结果等于堆栈上的元素个数
	// 返回 0 表示堆栈为空
	int top = lua_gettop(L);  

	printf("\n\n----------------------------------\n");
	printf("stack len:%d; %s", top, top == 0 ? "": "from botton to top is: ");
	for (i = 1; i <= top; i++) {
		int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING: {
			printf("%d:%s(string)", i, lua_tostring(L, i));
			break;
		}
		case LUA_TBOOLEAN: {
			printf("%d:%s(bool)", i, lua_toboolean(L, i) ? "true" : "false");
			break;
		}
		case LUA_TNUMBER: {
			printf("%d:%g(number)", i, lua_tonumber(L, i));
			break;
		}
		default:
			printf("%d:%s", i, lua_typename(L, t));
			break;
		}
		printf("\t");
	}
	printf("\n---------------------------------\n\n");
}