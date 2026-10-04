set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)

if (WP_BUILD_CPP)
	add_subdirectory(./Application)
	add_subdirectory(./Game)
	add_subdirectory(./HelloWorld)
	add_subdirectory(./Physics)
	add_subdirectory(./Vehicle)
	add_subdirectory(./VehicleAdvanced)
	
	set_target_properties (SampleApplication PROPERTIES FOLDER Samples/cpp)
	set_target_properties (SampleGame PROPERTIES FOLDER Samples/cpp)
	set_target_properties (SampleHelloWorld PROPERTIES FOLDER Samples/cpp)
	set_target_properties (SamplePhysics PROPERTIES FOLDER Samples/cpp)
	set_target_properties (SampleVehicle PROPERTIES FOLDER Samples/cpp)
	set_target_properties (SampleVehicleAdvanced PROPERTIES FOLDER Samples/cpp)
endif()



