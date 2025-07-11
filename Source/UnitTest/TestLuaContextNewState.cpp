#include "../LuaCpp.hpp"
#include "gtest/gtest.h"

namespace LuaCpp {

    // Global hook count
    int hook_count = 0;

    class TestLuaContextNewState : public ::testing::Test {
      protected:
        virtual void SetUp() {}
    };

    TEST_F(TestLuaContextNewState, TestNewState) {
        /**
         * Test the newState() method of LuaContext
         */
        LuaContext ctx;

        // Test newState() without parameters
        std::unique_ptr<Engine::LuaState> L = ctx.newState();
        EXPECT_NE((Engine::LuaState *) NULL, L.get());

        // Test newState() with environment
        LuaEnvironment env;
        std::shared_ptr<Engine::LuaTString> str = std::make_shared<Engine::LuaTString>("test_value");
        env["test_str"] = str;

        std::unique_ptr<Engine::LuaState> L2 = ctx.newState(env);
        EXPECT_NE((Engine::LuaState *) NULL, L2.get());

        // Verify that the global variable is set in the new state
        lua_getglobal(*L2, "test_str");
        EXPECT_TRUE(lua_isstring(*L2, -1));
        EXPECT_STREQ("test_value", lua_tostring(*L2, -1));
        lua_pop(*L2, 1);
    }

    int SumFunction(lua_State* L) {
        int sum = 0;
        int n = lua_gettop(L); // Number of arguments
        for (int i = 1; i <= n; i++) {
            sum += lua_tonumber(L, i);
        }
        lua_pushnumber(L, sum);
        return 1;
    }

    TEST_F(TestLuaContextNewState, TestNewStateWithBuiltInFunctions) {
        /**
         * Test the newState() method with built-in functions
         */
        LuaContext ctx;

        // Add the built-in function to the context
        ctx.setBuiltInFnc("sum", SumFunction);

        // Create a new state
        std::unique_ptr<Engine::LuaState> L = ctx.newState();

        // Verify that the built-in function is available in the new state
        lua_getglobal(*L, "sum");
        EXPECT_TRUE(lua_isfunction(*L, -1));
        lua_pop(*L, 1);

        // Test the function with actual arguments
        lua_getglobal(*L, "sum");
        lua_pushnumber(*L, 1);
        lua_pushnumber(*L, 2);
        lua_pushnumber(*L, 3);
        lua_call(*L, 3, 1);
        EXPECT_DOUBLE_EQ(6.0, lua_tonumber(*L, -1));
        lua_pop(*L, 1);
    }

    int AddFunction(lua_State* L) {
        int a = lua_tonumber(L, 1);
        int b = lua_tonumber(L, 2);
        lua_pushnumber(L, a + b);
        return 1;
    }

    TEST_F(TestLuaContextNewState, TestSetBuiltInFnc) {
        /**
         * Test the setBuiltInFnc method with and without the replace flag
         */
        LuaContext ctx;

        // Add a built-in function without replace flag
        ctx.setBuiltInFnc("add", AddFunction);

        // Verify that the function is added
        std::unique_ptr<Engine::LuaState> L = ctx.newState();
        lua_getglobal(*L, "add");
        EXPECT_TRUE(lua_isfunction(*L, -1));
        lua_pop(*L, 1);

        // Add the same function with replace=true
        ctx.setBuiltInFnc("add", AddFunction, true);

        // Verify that the function is still there after replacement
        std::unique_ptr<Engine::LuaState> L2 = ctx.newState();
        lua_getglobal(*L2, "add");
        EXPECT_TRUE(lua_isfunction(*L2, -1));
        lua_pop(*L2, 1);

        // Test the function with actual arguments
        lua_getglobal(*L2, "add");
        lua_pushnumber(*L2, 1);
        lua_pushnumber(*L2, 2);
        lua_call(*L2, 2, 1);
        EXPECT_DOUBLE_EQ(3.0, lua_tonumber(*L2, -1));
        lua_pop(*L2, 1);
    }

    void CountHook(lua_State* L, lua_Debug* ar) {
        // This is a simple hook that just counts calls
        hook_count++;
    }

    TEST_F(TestLuaContextNewState, TestRegisterHooks) {
        /**
         * Test the registerHooks method with different hook types
         */
        LuaContext ctx;

        // Reset the hook count
        hook_count = 0;

        // Add hooks of different types
        ctx.addHook(lua_Hook(CountHook), "call", 0);
        ctx.addHook(lua_Hook(CountHook), "return", 0);
        ctx.addHook(lua_Hook(CountHook), "line", 1);
        ctx.addHook(lua_Hook(CountHook), "count", 10000);

        // Create a new state
        std::unique_ptr<Engine::LuaState> L = ctx.newState();

        // Call registerHooks to register all hooks
        ctx.registerHooks(*L);

        // Execute some Lua code to trigger the hooks
        luaL_dostring(*L, "for i = 1, 10000 do end"); // This will trigger the count hook

        // Verify that hooks were triggered
        EXPECT_GT(hook_count, 0);
    }

    int ExceptionFunction(lua_State* L) {
        throw std::runtime_error("Exception in Lua function");
        return 0; // Unreachable
    }

    TEST_F(TestLuaContextNewState, TestNewRuntimeException) {
        /**
         * Test that newState() properly handles exceptions
         */
        LuaContext ctx;

        // Add a built-in function that throws an exception
        ctx.setBuiltInFnc("exception", ExceptionFunction);


        // Create a new state
        std::unique_ptr<Engine::LuaState> L = ctx.newState();

        // Create a lambda to catch the exception
        auto testException = [&]() {
            lua_getglobal(*L, "exception");
            lua_call(*L, 0, 0);
        };

        // Verify that an exception is thrown
        EXPECT_THROW(testException(), std::runtime_error);
    }

    TEST_F(TestLuaContextNewState, TestSetBuiltInFncNullCFunction) {
         /**
          * Test that setBuiltInFnc throws an exception when a null C function is provided
          * and then newState is called.
          */
         LuaContext ctx;
 
         // Set a built-in function with a null C function
         ctx.setBuiltInFnc("null_func", nullptr);
 
         // Expect an exception when newState is called, as it tries to register the null function
         EXPECT_THROW(ctx.newState(), std::runtime_error);
     }
        
}