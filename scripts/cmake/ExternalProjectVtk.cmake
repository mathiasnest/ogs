INCLUDE(ThirdPartyLibVersions)
INCLUDE(ExternalProject)

SET(OGS_VTK_VERSION 6.1.0)
#SET(OGS_VTK_REQUIRED_LIBS
#	vtkIOXML
#)
IF(OGS_BUILD_VIS)
	SET(OGS_VTK_REQUIRED_LIBS vtkRenderingCore)
ENDIF()
IF(OGS_BUILD_GUI)
	SET(OGS_VTK_REQUIRED_LIBS ${OGS_VTK_REQUIRED_LIBS}
		vtkRenderingCore
		vtkGUISupportQt
		vtkInteractionWidgets
		vtkFiltersTexture
		vtkIONetCDF
		vtkIOLegacy
		vtkIOExport
		vtkIOXML
	)
ENDIF()

IF(NOT DEFINED OGS_VTK_REQUIRED_LIBS)
	RETURN()
ENDIF()

FIND_PACKAGE(VTK ${OGS_VTK_VERSION} COMPONENTS ${OGS_VTK_REQUIRED_LIBS}
	PATH_SUFFIXES lib/cmake/vtk-6.1 )

IF(VTK_FOUND)
	INCLUDE( ${VTK_USE_FILE} )
	IF(VTK_DIR MATCHES "${CMAKE_BINARY_DIR}/External/vtk/src/VTK-build")
		MESSAGE(STATUS "Using local vtk.")
		INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_BINARY_DIR}/External/vtk/src/VTK/ThirdParty/netcdf/vtknetcdf/cxx)
	ELSE()
		MESSAGE(STATUS "Using system vtk.")
		FOREACH(DIR ${VTK_INCLUDE_DIRS})
			INCLUDE_DIRECTORIES(SYSTEM ${DIR}/vtknetcdf/include)
		ENDFOREACH()
		INCLUDE_DIRECTORIES(SYSTEM ${VTK_DIR}/../ThirdParty/netcdf/vtknetcdf/cxx)
		RETURN()
	ENDIF()
ENDIF()

IF(NOT VTK_DIR)
	SET(VTK_DIR ${CMAKE_BINARY_DIR}/External/vtk/src/VTK-build CACHE PATH "" FORCE)
	FIND_PACKAGE(VTK ${OGS_VTK_VERSION} COMPONENTS ${OGS_VTK_REQUIRED_LIBS} QUIET )
	IF(VTK_FOUND)
		INCLUDE( ${VTK_USE_FILE} )
		INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_BINARY_DIR}/External/vtk/src/VTK/ThirdParty/netcdf/vtknetcdf/cxx)
	ELSE()
		MESSAGE(STATUS "VTK will be build locally.")
	ENDIF()
ENDIF()

IF(OGS_SYSTEM_VTK_ONLY AND NOT VTK_FOUND AND NOT ParaView_FOUND)
	MESSAGE(FATAL_ERROR "Vtk / ParaView was not found but is required! Try to set VTK_DIR or ParaView_DIR to its install or build directory.")
ENDIF()

IF(VTK_FOUND)
	INCLUDE( ${VTK_USE_FILE} )
ENDIF()


IF(OGS_BUILD_GUI)
	SET(OGS_VTK_CMAKE_ARGS "-DModule_vtkGUISupportQt:BOOL=ON")
ENDIF()

IF(WIN32)
	FOREACH(VTK_LIB ${OGS_VTK_REQUIRED_LIBS})
		IF(NOT DEFINED VTK_MAKE_COMMAND)
			SET(VTK_MAKE_COMMAND
				cmake --build . --config Release --target ${VTK_LIB} &&
				cmake --build . --config Debug --target ${VTK_LIB})
		ELSE()
			SET(VTK_MAKE_COMMAND
				${VTK_MAKE_COMMAND} &&
				cmake --build . --config Release --target ${VTK_LIB} &&
				cmake --build . --config Debug --target ${VTK_LIB})
		ENDIF()
	ENDFOREACH()
	# MESSAGE(STATUS ${VTK_MAKE_COMMAND})
ELSE()
	IF($ENV{CI})
		SET(VTK_MAKE_COMMAND make ${OGS_VTK_REQUIRED_LIBS})
	ELSE()
		SET(VTK_MAKE_COMMAND make -j ${NUM_PROCESSORS} ${OGS_VTK_REQUIRED_LIBS})
	ENDIF()
ENDIF()

ExternalProject_Add(VTK
	PREFIX ${CMAKE_BINARY_DIR}/External/vtk
	GIT_REPOSITORY ${VTK_GIT_URL}
	URL ${OGS_VTK_URL}
	URL_MD5 ${OGS_VTK_MD5}
	CMAKE_ARGS
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DBUILD_TESTING:BOOL=OFF
		-DCMAKE_BUILD_TYPE:STRING=Release
		${OGS_VTK_CMAKE_ARGS}
	BUILD_COMMAND ${VTK_MAKE_COMMAND}
	INSTALL_COMMAND ""
)


IF(NOT ${VTK_FOUND})
	# Rerun cmake in initial build
	ADD_CUSTOM_TARGET(VtkRescan ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} DEPENDS VTK)
ELSE()
	ADD_CUSTOM_TARGET(VtkRescan) # dummy target for caching
ENDIF()
