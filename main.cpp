

#include "Main.h"
#include "vdbHelpers.h"



XSI::CStatus VDB_GridToMesh_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_MeshToGrid_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_GetGridData_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_GetValueAtCoordinate_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_SetValueAtCoordinate_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_ParticlesToGrid_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_GridIO_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_Debug_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_FillBoxGrid_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_ChangeGridClass_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_Noise_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_AdvectLevelSet_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_MorphologicalFilter_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_RebuildLevelSet_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_ResampleGrid_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_Transform_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_Platonic_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_CreateGrid_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_CSG_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_CopyGrid_Register(XSI::PluginRegistrar& reg);
XSI::CStatus VDB_Morpher_Register(XSI::PluginRegistrar& reg);


SICALLBACK dlexport XSILoadPlugin( PluginRegistrar& in_reg )
{


	// init openvdb stuff
	openvdb::initialize();

	
	in_reg.PutAuthor(L"OlegBliznuk");
	in_reg.PutName(L"VDB_to_Softimage");
	in_reg.PutVersion(1,0);



	// register nodes
	VDB_GridToMesh_Register( in_reg );
	VDB_MeshToGrid_Register( in_reg );
	VDB_GetGridData_Register( in_reg );
	VDB_GetValueAtCoordinate_Register( in_reg );
	VDB_ParticlesToGrid_Register( in_reg );
	VDB_GridIO_Register( in_reg );
	VDB_FillBoxGrid_Register ( in_reg );
	//VDB_Debug_Register( in_reg );
	VDB_ChangeGridClass_Register( in_reg );
	VDB_Noise_Register ( in_reg );
	VDB_AdvectLevelSet_Register (  in_reg );
	VDB_MorphologicalFilter_Register(  in_reg );
	VDB_RebuildLevelSet_Register(in_reg);
	VDB_ResampleGrid_Register(in_reg);
	VDB_Transform_Register(in_reg);
	VDB_SetValueAtCoordinate_Register(in_reg);
	VDB_Platonic_Register( in_reg);
	VDB_CreateGrid_Register( in_reg);
	VDB_CSG_Register( in_reg);
	VDB_CopyGrid_Register( in_reg);
	VDB_Morpher_Register( in_reg);

	// register commands
	 in_reg.RegisterCommand(L"VDBGetFileInfo",L"VDBGetFileInfo");



	//RegistrationInsertionPoint - do not remove this line	

	return CStatus::OK;
}

SICALLBACK dlexport XSIUnloadPlugin( const PluginRegistrar& in_reg )
{


	CString strPluginName;
	strPluginName = in_reg.GetName();
	Application().LogMessage(strPluginName + L" has been unloaded.",siVerboseMsg);
	return CStatus::OK;
}
