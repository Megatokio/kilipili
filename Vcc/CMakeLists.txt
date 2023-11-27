
cmake_minimum_required(VERSION 3.13)


####################################
# Vcc/CMakeLists.txt 


add_library(kilipili_vcc STATIC 
	Vcc.cpp
	Vcc.h
	idf_id.h
	idf_ids.h
	tokenize.cpp
	Opcodes.h
	DReg.h
	Var.h
	VxRunner.cpp
	VxRunner.h
	Type.h
	Signature.h
	Symbol.h
	ObjCode.h
	Names.h	
	)

target_compile_definitions(kilipili_vcc PUBLIC 
	ON=1 OFF=0
	)

# add current dir to 'include search path':
target_include_directories(kilipili_vcc PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kilipili_vcc PUBLIC
	kilipili_utilities
	#kilipili_common
	#pico_stdlib
	)


	


 
