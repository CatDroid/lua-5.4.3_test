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
	// ����lua����� 
	lua_State *L = luaL_newstate();

	luaL_openlibs(L); // ���������� ����lua�� string os�ȿ���Ҳ�����

	/* 
	luaL_openlibs �����base package��c����,��ʼ��lua�����ͼ��ؿ�
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
	// ���ض����lua�ļ�
	int ret = luaL_loadfile(L, "DumpLua.lua");
	if (ret != LUA_OK) {
		// ���ؽ����lua_load���صĽ����ͬ
		// 0��û�д���
		// 1���д���  
		// LUA_ERRFILE		����򲻿��ļ����߶�ȡ���˵�ʱ����һ������Ĵ�����

		/*
		//  thread status
		#define LUA_OK		0
		#define LUA_YIELD	1
		#define LUA_ERRRUN	2
		#define LUA_ERRSYNTAX	3	Ԥ�������﷨����
		#define LUA_ERRMEM	4		�ڴ�������
		#define LUA_ERRERR	5

		#define  LUA_ERRFILE (LUA_ERRERR + 1)
		*/


		// C++����lua ʧ�ܻ��crash��ջ�ŵ�ջ��
		std::cout << "main.lua load fail, reason" << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
		return 1;
	}
	else {

		// luaL_loadfile ����غͱ���Lua�ű�������������, ջ����һ������function
		// luaL_dofile	 �������б����Ľű������н����󻹻�ѽű�pop��ջ

		int type = lua_type(L, -1); // ��ջ������������
		const char* ctype = lua_typename(L, type);
		std::cout << "after luaL_loadfile stack top is " << ctype << " (" << type << ")" << std::endl; // 6
		stackDump(L);

		// nargs: �����ú����Ĳ�������
		// nresults: �����Ľ������
		// errfunc : ���������������
		// ����֮�� ջ��ֻ�н��/���� �����Ͳ����Ѿ�����  
		if (lua_pcall(L, 0, 0, 0) != LUA_OK)
		{
			// ������г���lua_pcall�᷵��һ������Ľ��, 
			// ���ָ���˴����������ȵ��ô���������Ȼ���ٽ�������Ϣ��ջ��
			// �ڽ����ؽ���ʹ�����Ϣ��ջ֮ǰ���Ƚ�"�����Ͳ���"��ջ���Ƴ� 
			// �������������ڱ����ú�������������֮ǰ��ջ
			std::cout << "pcall error " << lua_tostring(L, -1) << std::endl;
			return 1;
		}
	}

	//
	//--------------------------------------------------------------------------------------------------
	// ����Ԫ��֮ǰ��sanpshot
	snapshot(L, 1);

	//
	//--------------------------------------------------------------------------------------------------
	// ע��� ����һ�� key��lightuserdata(uuid) value��userdata  

	void* userdata = lua_newuserdata(L, sizeof(MyScriptValue));
	MyScriptValue* res = new(userdata)MyScriptValue{123, "hello world"};
	// -1 userdata 

	lua_pushlightuserdata(L, (void*)(res->id));
	// -1 lightuserdata id 
	// -2 userdata 

	lua_insert(L, -2);
	// -1 userdata           << value 
	// -2 lightuserdata id   << key 

	lua_rawset(L, LUA_REGISTRYINDEX); // ջ����table��value
	//  register[lightuserdata] =  userdata;


	//
	//--------------------------------------------------------------------------------------------------
	// ����Ԫ��֮�� snapshotע��� 
	snapshot(L, 2);

	compare(L, 1, 2, 3);

	// �����ؼ��� registry.[
	// userdata: 0000022C86A558A8	registry.[table:value]	1
	// userdata : 000000000000007B	registry.[table:key.userdata]	1

	//
	//--------------------------------------------------------------------------------------------------
	// ��ע�����ɾ���ոյ�Ԫ�� 
 
	void* uuid = (void*)res->id; // ����uuid�Ǵ������ط������, ��������userdata

	int top = lua_gettop(L);

	lua_pushlightuserdata(L, uuid);
	lua_rawget(L, LUA_REGISTRYINDEX);

	if (lua_isuserdata(L, -1))
	{
		// lua_pushnil(L);
		// lua_setmetatable(L, -2); // ����userdata��metable 

		MyScriptValue* value = (MyScriptValue*)lua_touserdata(L, -1);
		std::cout << "get userdata from lua = " << value->name << std::endl;
		lua_pop(L, 1);

		lua_pushlightuserdata(L, uuid); // key 
		lua_pushnil(L);					// value  ջ����value,��������push
		
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
	// ɾ��Ԫ��֮�� snapshotע��� 
	snapshot(L, 4);

	compare(L, 1, 4, 5); // �ʼ������snapshot�Ա�

	return 0;
}


static void snapshot(lua_State* L, int appendIndex)
{
	stackDump(L); // stack len:0

				   // ѹ�뺯���Ͳ���
	int luaType = lua_getglobal(L, "Snapshot");	// �����õĺ���  ���û�еĻ������ص���0?  0�����ȡ������nil(ջ��) 
												    
	lua_pushstring(L, "profile");				// ѹ���һ������
	lua_pushinteger(L, appendIndex);			// ѹ��ڶ�������

	std::cout << "lua_getglobal(Snapshot) type is " << lua_typename(L, luaType) << " (" << luaType << ")" << std::endl; // funcOnStackIdx = 6
	
	stackDump(L);

	// ��ɵ��� (2������  1�����) ִ��֮��ѹ��ı����ᱻ����
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
			lua_pop(L,num)������ջ����ʼ�Ƴ�
				��num>0ʱ��ջ���Ƴ�ָ������ 
				��num=0ʱջ����Ӱ��
				��num=-1ʱջ��Ԫ��ȫ���Ƴ�
			*/
			std::cout << "function \"Snapshot\" run result = " << result << std::endl;
		}
		lua_pop(L, 1); // �ѽ������
		stackDump(L);
	}
}

static void compare(lua_State* L, int beforeIndex, int afterIndex, int compareIndex)
{
	// ��ȫ�ֱ��� name ���ֵѹջ�����ظ�ֵ�����͡�

	int valueType = lua_getglobal(L, "Compare");
	if (valueType != LUA_TFUNCTION) {
		lua_pop(L, 1); // ��ջ����nil ���� 
		std::cout << "lua do NOT have \"Compare\" function" << std::endl;
		return; 
	}

	lua_pushstring(L, "profile");				// ѹ���һ������ lua������һ����������ջ
	lua_pushstring(L, "compare");				// ѹ��ڶ�������
	lua_pushinteger(L, beforeIndex);			
	lua_pushinteger(L, afterIndex);
	lua_pushinteger(L, compareIndex);
	if (lua_pcall(L, 5, 1, 0) != 0)  // ����ջ��4����Ϊ���� ��������һ��
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
		lua_pop(L, 1); // �ѽ������
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

	// ����ջ��Ԫ�ص����� ��Ϊ�����Ǵ�1 ��ʼ��ŵ� �������������ڶ�ջ�ϵ�Ԫ�ظ���
	// ���� 0 ��ʾ��ջΪ��
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