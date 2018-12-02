//
// Created by steve on 7/9/18.
//
#include <Catch.hpp>
#include <sys/types.h>
#include <sys/mman.h>
#include "headers/Detour/x64Detour.hpp"
#include "headers/CapstoneDisassembler.hpp"

#include "headers/Tests/TestEffectTracker.hpp"

EffectTracker effects;

/**These tests can spontaneously fail if the compiler desides to optimize away
the handler or inline the function. NOINLINE attempts to fix the latter, the former
is out of our control but typically returning volatile things, volatile locals, and a
printf inside the body can mitigate this significantly. Do serious checking in debug
or releasewithdebinfo mode (relwithdebinfo optimizes sliiiightly less)**/

NOINLINE void hookMe1() {
	volatile int var = 1;
	volatile int var2 = 0;
	var2 += 3;
	var2 = var + var2;
	var2 *= 30 / 3;
	var = 2;
	printf("%d %d\n", var, var2); // 2, 40
	REQUIRE(var == 2);
	REQUIRE(var2 == 40);
}
uint64_t hookMe1Tramp = NULL;

NOINLINE void h_hookMe1() {
	std::cout << "Hook 1 Called!" << std::endl;
	effects.PeakEffect().trigger();
	return PLH::FnCast(hookMe1Tramp, &hookMe1)();
}

NOINLINE void hookMe2() {
	for (int i = 0; i < 10; i++) {
		printf("%d\n", i);
	}
}
uint64_t hookMe2Tramp = NULL;

NOINLINE void h_hookMe2() {
	std::cout << "Hook 2 Called!" << std::endl;
	effects.PeakEffect().trigger();
	return PLH::FnCast(hookMe2Tramp, &hookMe2)();
}

unsigned char hookMe3[] = {
0x57, // push rdi 
0x74,0xf9,
0x74, 0xf0,//je 0x0
0x90, 0x90, 0x90, 0x90,
0x90, 0x90, 0x90, 0x90,
0x90, 0x90, 0x90, 0x90,
0xc3
};

unsigned char hookMe4[] = {
	0x57, // push rdi
	0x48, 0x83, 0xec, 0x30, //sub rsp, 0x30
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	0x74,0xf2, //je 0x0
	0xc3
};

uint64_t nullTramp = NULL;
NOINLINE void h_nullstub() {
	volatile int i = 0;
}

#include <stdlib.h>
uint64_t hookMallocTramp = NULL;
NOINLINE void* h_hookMalloc(size_t size, size_t size2) {
	volatile int i = 0;
    volatile int var2 = 0;
    volatile int var3 = 0;
    i = var2 + var3;
	//effects.PeakEffect().trigger();
    if (hookMallocTramp)
        return PLH::FnCast(hookMallocTramp, &calloc)(size, size2);
    else
        return 0;
}

TEST_CASE("Testing 64 detours", "[x64Detour],[ADetour]") {
	PLH::CapstoneDisassembler dis(PLH::Mode::x64);
//
//	SECTION("Normal function") {
//		hookMe1();
//		PLH::x64Detour detour((char*)&hookMe1, (char*)&h_hookMe1, &hookMe1Tramp, dis);
//		hookMe1();
//		REQUIRE(detour.hook() == true);
//
//		effects.PushEffect();
//		hookMe1();
//		REQUIRE(effects.PopEffect().didExecute());
//		detour.unHook();
//	}
//	
//	SECTION("Normal function2") {
//		PLH::x64Detour detour((char*)&hookMe1, (char*)&h_hookMe1, &hookMe1Tramp, dis);
//		hookMe1();
//		REQUIRE(detour.hook() == true);
//		
//		effects.PushEffect();
//		hookMe1();
//		REQUIRE(effects.PopEffect().didExecute());
//		detour.unHook();
//	}
//
//	SECTION("Loop function") {
//		PLH::x64Detour detour((char*)&hookMe2, (char*)&h_hookMe2, &hookMe2Tramp, dis);
//		REQUIRE(detour.hook() == true);
//
//		effects.PushEffect();
//		hookMe2();
//		REQUIRE(effects.PopEffect().didExecute());
//		detour.unHook();
//	}
//
//	SECTION("Jmp into prol w/src in range") {
//		PLH::x64Detour detour((char*)&hookMe3, (char*)&h_nullstub, &nullTramp, dis);
//		REQUIRE(detour.hook() == true);
//		detour.unHook();
//	}
//
//	SECTION("Jmp into prol w/src out of range") {
//		PLH::x64Detour detour((char*)&hookMe4, (char*)&h_nullstub, &nullTramp, dis);
//		REQUIRE(detour.hook() == true);
//		detour.unHook();
//	}

	SECTION("hook malloc") {
		
		effects.PushEffect();
		PLH::x64Detour detour((char*)&calloc, (char*)&h_hookMalloc, &hookMallocTramp, dis);
		//effects.PushEffect(); // catch does some allocations, push effect first so peak works
		bool result = detour.hook();
		
		REQUIRE(result == true);

		void* pMem = calloc(16, 16);
		free(pMem);
		detour.unHook(); // unhook so we can popeffect safely w/o catch allocation happening again
		REQUIRE(effects.PopEffect().didExecute());
	}
}
