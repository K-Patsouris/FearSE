set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
	"${SOURCE_DIR}/Forms/FearForms.cpp"
	"${SOURCE_DIR}/Forms/FearForms.h"
	"${SOURCE_DIR}/Forms/HurdlesForms.cpp"
	"${SOURCE_DIR}/Forms/HurdlesForms.h"
	"${SOURCE_DIR}/Forms/RulesForms.cpp"
	"${SOURCE_DIR}/Forms/RulesForms.h"
	"${SOURCE_DIR}/Forms/VanillaForms.cpp"
	"${SOURCE_DIR}/Forms/VanillaForms.h"
	
	"${SOURCE_DIR}/MiscFuncs/GenericFunctions.cpp"
	"${SOURCE_DIR}/MiscFuncs/GenericFunctions.h"
	"${SOURCE_DIR}/MiscFuncs/TestFunctions.cpp"
	"${SOURCE_DIR}/MiscFuncs/TestFunctions.h"
	
	"${SOURCE_DIR}/Types/FlagTypes.cpp"
	"${SOURCE_DIR}/Types/FlagTypes.h"
	"${SOURCE_DIR}/Types/LazyVector.cpp"
	"${SOURCE_DIR}/Types/LazyVector.h"
	"${SOURCE_DIR}/Types/NumericTypes.cpp"
	"${SOURCE_DIR}/Types/NumericTypes.h"
	"${SOURCE_DIR}/Types/SLHelpers.cpp"
	"${SOURCE_DIR}/Types/SLHelpers.h"
	"${SOURCE_DIR}/Types/StrongTypes.cpp"
	"${SOURCE_DIR}/Types/StrongTypes.h"
	"${SOURCE_DIR}/Types/SyncTypes.cpp"
	"${SOURCE_DIR}/Types/SyncTypes.h"
	"${SOURCE_DIR}/Types/TrivialHandle.cpp"
	"${SOURCE_DIR}/Types/TrivialHandle.h"
	
	"${SOURCE_DIR}/Utils/GameDataUtils.cpp"
	"${SOURCE_DIR}/Utils/GameDataUtils.h"
	"${SOURCE_DIR}/Utils/MenuUtils.cpp"
	"${SOURCE_DIR}/Utils/MenuUtils.h"
	"${SOURCE_DIR}/Utils/OutUtils.cpp"
	"${SOURCE_DIR}/Utils/OutUtils.h"
	"${SOURCE_DIR}/Utils/PrimitiveUtils.cpp"
	"${SOURCE_DIR}/Utils/PrimitiveUtils.h"
	"${SOURCE_DIR}/Utils/RNG.cpp"
	"${SOURCE_DIR}/Utils/RNG.h"
	"${SOURCE_DIR}/Utils/ScriptUtils.cpp"
	"${SOURCE_DIR}/Utils/ScriptUtils.h"
	"${SOURCE_DIR}/Utils/StringUtils.cpp"
	"${SOURCE_DIR}/Utils/StringUtils.h"
	"${SOURCE_DIR}/Utils/VectorUtils.cpp"
	"${SOURCE_DIR}/Utils/VectorUtils.h"
	
	
	"${SOURCE_DIR}/main.cpp"
	"${SOURCE_DIR}/PCH.h"
	
	"${SOURCE_DIR}/DataDefs/BaseTypes.h"
	"${SOURCE_DIR}/DataDefs/EquipState.h"
	"${SOURCE_DIR}/DataDefs/FearInfo.h"
	"${SOURCE_DIR}/DataDefs/FearOps.h"
	"${SOURCE_DIR}/DataDefs/PlayerRules.h"
	"${SOURCE_DIR}/DataDefs/RulesOps.h"
	"${SOURCE_DIR}/DataDefs/Multivector.h"
	
	"${SOURCE_DIR}/Common.cpp"
	"${SOURCE_DIR}/Common.h"
	"${SOURCE_DIR}/CustomMenu.cpp"
	"${SOURCE_DIR}/CustomMenu.h"
	"${SOURCE_DIR}/Data.cpp"
	"${SOURCE_DIR}/Data.h"
	"${SOURCE_DIR}/Events.cpp"
	"${SOURCE_DIR}/Events.h"
	"${SOURCE_DIR}/ExtraKeywords.cpp"
	"${SOURCE_DIR}/ExtraKeywords.h"
	"${SOURCE_DIR}/Logger.cpp"
	"${SOURCE_DIR}/Logger.h"
	"${SOURCE_DIR}/MCM.cpp"
	"${SOURCE_DIR}/MCM.h"
	"${SOURCE_DIR}/Periodic.cpp"
	"${SOURCE_DIR}/Periodic.h"
	"${SOURCE_DIR}/RulesMenu.cpp"
	"${SOURCE_DIR}/RulesMenu.h"
	"${SOURCE_DIR}/Scaleform.cpp"
	"${SOURCE_DIR}/Scaleform.h"
	"${SOURCE_DIR}/Serialization.cpp"
	"${SOURCE_DIR}/Serialization.h"
	
)

source_group(TREE "${ROOT_DIR}" FILES ${SOURCE_FILES})

set(VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/Version.h")
configure_file(
	"${ROOT_DIR}/cmake/Version.h.in"
	"${VERSION_HEADER}"
	@ONLY
)

source_group("src" FILES "${VERSION_HEADER}")

configure_file(
	"${ROOT_DIR}/cmake/version.rc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	@ONLY
)

add_library(
	"${PROJECT_NAME}"
	SHARED
	${SOURCE_FILES}
	"${VERSION_HEADER}"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	"${ROOT_DIR}/.clang-format"
	"${ROOT_DIR}/.editorconfig"
)

target_compile_features(
	"${PROJECT_NAME}"
	PRIVATE
		cxx_std_23
		#c_std_17
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	target_compile_options(
		"${PROJECT_NAME}"
		PRIVATE
			"/std:c17"	# Force some relevant C for potential .c files
			
			"/sdl"	# Enable Additional Security Checks
			"/utf-8"	# Set Source and Executable character sets to UTF-8
			"/Zi"	# Debug Information Format

			"/permissive-"	# Standards conformance
			"/Zc:preprocessor"	# Enable preprocessor conformance mode

			"/wd4200" # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/O2;/Ob2>;/Oi;/Ot;/GL"
	)

	target_link_options(
		"${PROJECT_NAME}"
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>;/LTCG:incremental"
	)
endif()

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}/src"
		"${SOURCE_DIR}"
)

add_subdirectory("${ROOT_DIR}/extern/CommonLibSSE" CommonLibSSE EXCLUDE_FROM_ALL)

target_link_libraries(
	"${PROJECT_NAME}"
	PRIVATE
		CommonLibSSE::CommonLibSSE
)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		"${SOURCE_DIR}/PCH.h"
)

if("${COPY_OUTPUT}")
	if("${FEARSE_OUT_DIR}" STREQUAL "")
		message("Warning: Output copying to destination requested but FEARSE_OUT_DIR is empty. Specify an output path through -DFEARSE_OUT_DIR=\"...\"")
	else()
		message("FearSE.dll and FearSE.pdb will be copied to \"${FEARSE_OUT_DIR}\" after compilation.")
		add_custom_command(
			TARGET "${PROJECT_NAME}"
			POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${FEARSE_OUT_DIR}"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${FEARSE_OUT_DIR}"
			VERBATIM
		)
	endif()
endif()
