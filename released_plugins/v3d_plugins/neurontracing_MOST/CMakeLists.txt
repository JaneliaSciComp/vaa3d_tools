include_directories( "${V3DMAINDIR}/v3d" )
include_directories( "${V3DMAINDIR}/neuron_editing" )

include_directories( "${V3D_BUILD_DIR}/v3d_main/v3d" ) # for the ui_*.h file 


set(PLUGIN_DIRECTORY_NAME mostVesselTracer/ )
set(PLUGIN_SOURCES mostVesselTracer.cpp mostimage.cpp voxelcluster.cpp srb.c tools.cpp arthurstyle.cpp arthurwidgets.cpp hoverpoints.cpp
	"${V3DMAINDIR}/neuron_editing/v_neuronswc.cpp"   "${V3DMAINDIR}/neuron_editing/neuron_format_converter.cpp")
set(PLUGIN_HEADERS mostVesselTracer.h mostimage.h voxelcluster.h srb.h tools.h arthurwidgets.h arthurstyle.h hoverpoints.h)

configure_v3d_plugin(mostVesselTracer)
