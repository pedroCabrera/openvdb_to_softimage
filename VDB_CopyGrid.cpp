#include "Main.h"
#include "vdbHelpers.h"


enum IDs
{
	ID_IN_VDBGrid ,


	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_CopyGrid_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_CopyGrid", L"VDB Deep Copy Grid");

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
	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_CopyGrid_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_CopyGrid_cache_t ( ) { };


	clock_t  m_inputEvalTime;


	bool inline IsDirty ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		bool isClean = ( m_inputEvalTime == (*in_grid).m_lastEvalTime  );

		if ( isClean == false )
		{		
			m_inputEvalTime = (*in_grid).m_lastEvalTime;
		};

		return !isClean;

	};

	inline void Copy ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		TIMER timer;

		try 
		{

			m_primaryGrid.m_grid = in_grid->m_grid->deepCopyGrid ();
			m_primaryGrid.m_grid->setGridClass (  openvdb::v2_1_0::GridClass::GRID_LEVEL_SET );

			m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
			m_primaryGrid.m_lastEvalTime = clock();
			Application().LogMessage(L"[VDB][COPYGRID]: Stamped at=" + CString ( m_primaryGrid.m_lastEvalTime));
			Application().LogMessage(L"[VDB][COPYGRID]: Done in=" + CString (timer.GetElapsedTime ()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][COPYGRID]: " + CString (e.what()), siErrorMsg);
		};
	};


};

SICALLBACK VDB_CopyGrid_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

				CDataArrayCustomType outData( in_ctxt );     

			// get cache object
			VDB_CopyGrid_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][COPYGRID]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_CopyGrid_cache_t*)(CValue::siPtrType)userData;
				p_cacheNodeObject->PackGrid ( outData );

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);

			// check for prim grid
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL || !p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][COPYGRID]: Empty grid on input" );
				return CStatus::OK;
			};
	

			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper )==false )
			{
				Application().LogMessage ( L"[VDB][COPYGRID]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->Copy ( p_inWrapper );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK VDB_CopyGrid_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_CopyGrid_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_CopyGrid_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][COPYGRID]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK VDB_CopyGrid_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][COPYGRID]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_CopyGrid_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_CopyGrid_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}