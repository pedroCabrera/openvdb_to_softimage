#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_userdatablob.h>

#include <openvdb/tools/LevelSetRebuild.h>
enum IDs
{
	ID_IN_VDBGrid ,

	ID_IN_ExtWidth ,
	ID_IN_IntWidth, 
	ID_IN_WidthInVoxels, 
	ID_IN_IsoVal, 

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_RebuildLevelSet_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_RebuildLevelSet", L"VDB Rebuild LevelSet");

	CStatus st;
	st = nodeDef.PutColor(VDB_NODE_COLOR);
	st.AssertSucceeded();

	st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
	st.AssertSucceeded();

	// Add custom types definition
	st = nodeDef.DefineCustomType(VDB_DATA_NAME,VDB_DATA_NAME, VDB_DATA_DESC, VDB_STREAM_COLOR); 
	st.AssertSucceeded();

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);
	st.AssertSucceeded();

	// Add custom type names.
	CStringArray customTypes(1);
	customTypes[0] = VDB_DATA_NAME;

	st = nodeDef.AddInputPort(ID_IN_VDBGrid, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"In VDB Grid", L"inVDBGrid",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	
	 st = nodeDef.AddInputPort(ID_IN_ExtWidth, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"ExteriorWidth", L"ExteriorWidth",1.5f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
 st = nodeDef.AddInputPort(ID_IN_IntWidth, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"InteriorWidth", L"InteriorWidth",1.5f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_WidthInVoxels, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"Width In Voxels", L"WidthInVoxels",true,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
    st = nodeDef.AddInputPort(ID_IN_IsoVal, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"IsoValue", L"IsoValue",0.5f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_RebuildLevelSet_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_RebuildLevelSet_cache_t ( ) { };



	clock_t  m_inputEvalTime;
	float m_iso;
	float m_ext;
	float m_int;



	bool inline IsDirty ( VDB_ICEFlowDataWrapper_t * in_grid, float iso, float extr, float intr )
	{
		bool isClean = ( m_inputEvalTime == (*in_grid).m_lastEvalTime && m_iso == iso && m_ext == extr && m_int == intr  );

		if ( isClean == false )
		{	
		
			m_inputEvalTime = (*in_grid).m_lastEvalTime;
		
			m_iso = iso ;
			m_ext = extr ;
			m_int = intr;
		};

		return !isClean;

	};

	inline void Change ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		TIMER timer;
	

		openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid> ( in_grid->m_grid );

		

		try
		{
			m_primaryGrid.m_grid = openvdb::tools::levelSetRebuild( *grid, m_iso, m_ext, m_int);
			m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
			m_primaryGrid.m_lastEvalTime = clock();
			Application().LogMessage(L"[VDB][REBUILDLS]: Stamped at=" + CString ( m_primaryGrid.m_lastEvalTime));
			Application().LogMessage(L"[VDB][REBUILDLS]: Done in=" + CString (timer.GetElapsedTime ()));
		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][REBUILDLS]: " + CString ( e.what()), siErrorMsg);
		};

		
	};


};

SICALLBACK VDB_RebuildLevelSet_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

				CDataArrayCustomType outData( in_ctxt );   

			// get cache object
			VDB_RebuildLevelSet_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_RebuildLevelSet_cache_t*)(CValue::siPtrType)userData;

				p_cacheNodeObject->PackGrid ( outData );
		
						// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Empty levelset grid on input" );
				return CStatus::OK;
			};

			if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>()==false )
			{
				Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Invalid grid data type! Float only allowed!", siWarningMsg );
				return CStatus::OK;
			}
			
			CDataArrayFloat inExt ( in_ctxt, ID_IN_ExtWidth );
			CDataArrayFloat inInt ( in_ctxt, ID_IN_IntWidth );
			CDataArrayFloat inIso ( in_ctxt, ID_IN_IsoVal );
			CDataArrayBool inBSinVoxels ( in_ctxt, ID_IN_WidthInVoxels );
		              
		


			float vxsz = p_inWrapper->m_grid->transform().voxelSize().x() ;

			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper, inIso[0],
			inBSinVoxels[0]?vxsz*inExt[0]:inExt[0], 
				inBSinVoxels[0]?vxsz*inInt[0]:inInt[0] )==false )
			{
				Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->Change ( p_inWrapper );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK VDB_RebuildLevelSet_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_RebuildLevelSet_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_RebuildLevelSet_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK VDB_RebuildLevelSet_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_RebuildLevelSet_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_RebuildLevelSet_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}