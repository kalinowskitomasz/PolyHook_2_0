cmake_minimum_required(VERSION 3.12.3)
project(PolyHook_2)
set(CMAKE_CXX_STANDARD 17)

# Disable all capstone options except x86
set(CAPSTONE_ARM_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_ARM_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_ARM64_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_M680X_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_M68K_SUPPORT  0 CACHE INTERNAL "")
set(CAPSTONE_MIPS_SUPPORT  0 CACHE INTERNAL "")
set(CAPSTONE_PPC_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_SPARC_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_SYSZ_SUPPORT  0 CACHE INTERNAL "")
set(CAPSTONE_XCORE_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_TMS320C64X_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_M680X_SUPPORT 0 CACHE INTERNAL "")
set(CAPSTONE_EVM_SUPPORT 0 CACHE INTERNAL "")

add_subdirectory(capstone)

option(FEATURE_DETOURS "Implement detour functionality" ON)
option(FEATURE_EXCEPTION "Implement all exception hooking functionality" OFF)
option(FEATURE_VIRTUALS "Implement all virtual table hooking functionality" OFF)
option(FEATURE_PE "Implement all win pe hooking functionality" OFF)

# Detect architecture, it determines if x86 or x64 hooks will be build 
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message("Detected 64 bit")
	set(ARCHITECTURE x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message("Detected 32 bit")
	set(ARCHITECTURE x86)
endif()

#IDE's like it when header file are included as source files
set(HEADER_FILES ${PROJECT_SOURCE_DIR}/headers/ADisassembler.hpp
		${PROJECT_SOURCE_DIR}/headers/CapstoneDisassembler.hpp
		${PROJECT_SOURCE_DIR}/headers/Enums.hpp
		${PROJECT_SOURCE_DIR}/headers/IHook.hpp
		${PROJECT_SOURCE_DIR}/headers/Instruction.hpp
		${PROJECT_SOURCE_DIR}/headers/Misc.hpp
		${PROJECT_SOURCE_DIR}/headers/UID.hpp
		${PROJECT_SOURCE_DIR}/headers/ErrorLog.hpp
		${PROJECT_SOURCE_DIR}/headers/MemProtector.hpp)

set(HEADER_IMP_SOURCES 
		${PROJECT_SOURCE_DIR}/sources/CapstoneDisassembler.cpp
		${PROJECT_SOURCE_DIR}/sources/MemProtector.cpp
		${PROJECT_SOURCE_DIR}/sources/TestEffectTracker.cpp)

set(UNIT_TEST_SOURCES 
		${PROJECT_SOURCE_DIR}/MainTests.cpp
		${PROJECT_SOURCE_DIR}/UnitTests/TestDisassembler.cpp
		${PROJECT_SOURCE_DIR}/UnitTests/TestMemProtector.cpp)

# Headers, Sources, and Test for detours
if(FEATURE_DETOURS MATCHES ON) 
	set(DETOUR_HEADER_FILES ${PROJECT_SOURCE_DIR}/headers/Detour/ADetour.hpp
			${PROJECT_SOURCE_DIR}/headers/Detour/${ARCHITECTURE}Detour.hpp)

	set(DETOUR_IMP_SOURCES 
			${PROJECT_SOURCE_DIR}/sources/ADetour.cpp
			${PROJECT_SOURCE_DIR}/sources/${ARCHITECTURE}Detour.cpp)

	set(HEADER_FILES ${HEADER_FILES} ${DETOUR_HEADER_FILES})
	set(HEADER_IMP_SOURCES ${HEADER_IMP_SOURCES} ${DETOUR_IMP_SOURCES})

	set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} 
		${PROJECT_SOURCE_DIR}/UnitTests/TestDetour${ARCHITECTURE}.cpp)
endif()

if(FEATURE_EXCEPTION MATCHES ON)
	set(EXCEPTION_HEADER_FILES 
		${PROJECT_SOURCE_DIR}/headers/Exceptions/AVehHook.hpp
		${PROJECT_SOURCE_DIR}/headers/Exceptions/BreakPointHook.hpp
		${PROJECT_SOURCE_DIR}/headers/Exceptions/HWBreakPointHook.hpp)

	set(EXCEPTION_IMP_SOURCES 
		${PROJECT_SOURCE_DIR}/sources/AVehHook.cpp
		${PROJECT_SOURCE_DIR}/sources/BreakPointHook.cpp
		${PROJECT_SOURCE_DIR}/sources/HWBreakPointHook.cpp)

	set(HEADER_FILES ${HEADER_FILES} ${EXCEPTION_HEADER_FILES})
	set(HEADER_IMP_SOURCES ${HEADER_IMP_SOURCES} ${EXCEPTION_IMP_SOURCES})

	set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} 
		${PROJECT_SOURCE_DIR}/UnitTests/TestBreakpointHook.cpp
		${PROJECT_SOURCE_DIR}/UnitTests/TestHWBreakpointHook.cpp)
endif()

if(FEATURE_VIRTUALS MATCHES ON)
	set(VIRTUAL_HEADER_FILES 
		${PROJECT_SOURCE_DIR}/headers/Virtuals/VTableSwapHook.hpp
		${PROJECT_SOURCE_DIR}/headers/Virtuals/VFuncSwapHook.hpp)

	set(VIRTUAL_IMP_SOURCES
		${PROJECT_SOURCE_DIR}/sources/VTableSwapHook.cpp
		${PROJECT_SOURCE_DIR}/sources/VFuncSwapHook.cpp)

	set(HEADER_FILES ${HEADER_FILES} ${VIRTUAL_HEADER_FILES})
	set(HEADER_IMP_SOURCES ${HEADER_IMP_SOURCES} ${VIRTUAL_IMP_SOURCES})

	set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} 
		${PROJECT_SOURCE_DIR}/UnitTests/TestVTableSwapHook.cpp
		${PROJECT_SOURCE_DIR}/UnitTests/TestVFuncSwapHook.cpp)
endif()

if(FEATURE_PE MATCHES ON)
	set(PE_HEADER_FILES
		${PROJECT_SOURCE_DIR}/headers/PE/EatHook.hpp
		${PROJECT_SOURCE_DIR}/headers/PE/IatHook.hpp
		${PROJECT_SOURCE_DIR}/headers/PE/PEB.hpp)

	set(PE_IMP_SOURCES 
		${PROJECT_SOURCE_DIR}/sources/EatHook.cpp
		${PROJECT_SOURCE_DIR}/sources/IatHook.cpp)

	set(HEADER_FILES ${HEADER_FILES} ${PE_HEADER_FILES})
	set(HEADER_IMP_SOURCES ${HEADER_IMP_SOURCES} ${PE_IMP_SOURCES})

	set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES}
			${PROJECT_SOURCE_DIR}/UnitTests/TestEatHook.cpp
			${PROJECT_SOURCE_DIR}/UnitTests/TestIatHook.cpp)
endif()

include_directories(${PROJECT_SOURCE_DIR})

set(SOURCE_FILES_PLH ${HEADER_IMP_SOURCES} ${HEADER_FILES})

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(PolyHook_2 ${SOURCE_FILES_PLH})
add_executable(TestRunner ${SOURCE_FILES_PLH} ${UNIT_TEST_SOURCES})

# require c++ 17 (for std::optional) and add WALL + PDB flags
if(MSVC)
	set(MY_DEBUG_OPTIONS /W4 /WX /Z7)
else()
	set(MY_DEBUG_OPTIONS -std=c++17)
endif()

target_compile_options(PolyHook_2 PUBLIC "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")
target_compile_options(PolyHook_2 PUBLIC "$<$<CONFIG:RELEASE>:${MY_DEBUG_OPTIONS}>")

target_compile_options(TestRunner PUBLIC "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")
target_compile_options(TestRunner PUBLIC "$<$<CONFIG:RELEASE>:${MY_DEBUG_OPTIONS}>")

target_link_libraries(PolyHook_2 PUBLIC capstone-static)

target_link_libraries(TestRunner PUBLIC capstone-static)