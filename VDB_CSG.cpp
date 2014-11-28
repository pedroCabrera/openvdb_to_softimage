#include "Main.h"
#include "vdbHelpers.h"
#include <openvdb/tools/composite.h>
#include <openvdb/util/NullInterrupter.h>
#include <openvdb/tools/LevelSetRebuild.h>

enum IDs
{
	ID_IN_VDBGridA ,
	ID_IN_VDBGridB,

	ID_IN_Op,
	ID_IN_MulA,
	ID_IN_MulB,
	ID_IN_PruneTol,

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_CSG_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_CSG", L"VDB CSG");

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

	st = nodeDef.AddInputPort(ID_IN_VDBGridA, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"VDBGrid A", L"inVDBA",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_VDBGridB, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"VDBGrid B", L"inVDBB",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();



	//st = nodeDef.AddInputPort(ID_IN_MulA, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"MultiplierA", L"MultiplierA",1.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	//  st = nodeDef.AddInputPort(ID_IN_MulB, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"MultiplierB", L"MultiplierB",1.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();

	  st = nodeDef.AddInputPort(ID_IN_Op, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"Function", L"Function",0,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	
	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_CSG_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_CSG_cache_t ( ) { };


	clock_t  m_inputEvalTimeA;
	clock_t  m_inputEvalTimeB;
	float m_amult;
	float m_bmult;
	int m_op;


	bool inline IsDirty ( clock_t atime, clock_t btime, float amult, float bmult, int op  )
	{
		bool isClean = ( m_inputEvalTimeA == atime && m_inputEvalTimeB == btime && m_amult == amult && m_bmult==bmult && m_op==op );

		if ( isClean == false )
		{			
			m_inputEvalTimeA = atime ;
			m_inputEvalTimeB = btime ;
			m_amult = amult ;
			m_bmult=bmult ;
			m_op=op;
		};

		return !isClean;

	};

	enum
	{
		UNKNOWN = -1,
		UNION ,
		DIFFERENCE_A_B,
		DIFFERENCE_B_A,
		INTERSECTION,

		SUM,
		MIN,
		MAX,
		MULT
	};

	inline void Combine ( VDB_ICEFlowDataWrapper_t * in_gridA, VDB_ICEFlowDataWrapper_t * in_gridB )
	{

		TIMER timer;

		try 
		{
			
			openvdb::GridBase::Ptr baseCopyA = in_gridA->m_grid->deepCopyGrid();			
			openvdb::FloatGrid::Ptr floatCopyA = openvdb::gridPtrCast<openvdb::FloatGrid> ( baseCopyA );

			openvdb::FloatGrid::Ptr floatCopyB;
	
			// resample if need
			if ( in_gridA->m_grid->constTransform () != in_gridB->m_grid->constTransform () )
			{
		
				auto tempPtr = openvdb::gridPtrCast<openvdb::FloatGrid> ( in_gridB->m_grid );

				float iso = 0.f;
				float bw = openvdb::LEVEL_SET_HALF_WIDTH;
				openvdb::math::Transform::Ptr xform = openvdb::math::Transform::createLinearTransform(floatCopyA->voxelSize().x());
				floatCopyB = openvdb::tools::levelSetRebuild(*tempPtr, 0.f, bw, bw, &(*xform) );
			}
			else
			{
				openvdb::GridBase::Ptr baseCopyB = in_gridB->m_grid->deepCopyGrid();
				floatCopyB = openvdb::gridPtrCast<openvdb::FloatGrid> ( baseCopyB );
			};

			switch (m_op)
			{
			case UNION: openvdb::tools::csgUnion ( *floatCopyA, *floatCopyB );  break;
			case DIFFERENCE_A_B: openvdb::tools::csgDifference ( *floatCopyA, *floatCopyB );  break;
			case DIFFERENCE_B_A: openvdb::tools::csgDifference ( *floatCopyB, *floatCopyA );  break;
			case INTERSECTION: openvdb::tools::csgIntersection ( *floatCopyA, *floatCopyB );  break;
			default:
				break;
			}


			m_primaryGrid.m_grid = m_op!=2 ? floatCopyA : floatCopyB;

			m_primaryGrid.m_grid->setGridClass ( openvdb::GRID_LEVEL_SET );
			m_primaryGrid.m_grid->setName ( "csgResultGrid" );
			m_primaryGrid.m_lastEvalTime = clock();	
			Application().LogMessage(L"[VDB][CSG]: Stamped at=" + CString ( m_primaryGrid.m_lastEvalTime ) );
			Application().LogMessage(L"[VDB][CSG]: Done in=" + CString (  timer.GetElapsedTime ( ) ));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][CSG]: " + CString ( e.what() ), siErrorMsg );
		};
	};

};

SICALLBACK VDB_CSG_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

			 CDataArrayCustomType outData( in_ctxt );     


			// get cache object
			VDB_CSG_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][CSG]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_CSG_cache_t*)(CValue::siPtrType)userData;
			p_cacheNodeObject->PackGrid ( outData );

			// get input grids
			CDataArrayCustomType inVDBGridA(in_ctxt, ID_IN_VDBGridA);
			CDataArrayCustomType inVDBGridB(in_ctxt, ID_IN_VDBGridB);

			// check for a
			VDB_ICEFlowDataWrapper_t * p_inWrapperA = p_cacheNodeObject->UnpackGrid ( inVDBGridA );
			if (p_inWrapperA==NULL || !p_inWrapperA->m_grid )
			{
				Application().LogMessage ( L"[VDB][CSG]: Empty grid on a-input" );
				return CStatus::OK;
			};

		
			// check for b
			VDB_ICEFlowDataWrapper_t * p_inWrapperB = p_cacheNodeObject->UnpackGrid ( inVDBGridB );
			if (p_inWrapperB==NULL || !p_inWrapperB->m_grid )
			{
				Application().LogMessage ( L"[VDB][CSG]: Empty grid on b-input" );
				return CStatus::OK;
			};

			// only level set for now
			if (p_inWrapperA->m_grid->getGridClass() != openvdb::GRID_LEVEL_SET || p_inWrapperB->m_grid->getGridClass() != openvdb::GRID_LEVEL_SET ||
				p_inWrapperA->m_grid->isType<openvdb::FloatGrid>()==false ||p_inWrapperB->m_grid->isType<openvdb::FloatGrid>()==false )
			{
				Application().LogMessage ( L"[VDB][CSG]: Only float level sets are supported for now!", siWarningMsg );
				return CStatus::OK;
			};


		//	CDataArrayFloat inAmult ( in_ctxt, ID_IN_MulA );
	//CDataArrayFloat inBmult ( in_ctxt, ID_IN_MulB );
			float ma = 1.f;//inAmult[0];
			float mb = 1.f;//inBmult[0];

	CDataArrayLong inOp ( in_ctxt, ID_IN_Op );

			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapperA->m_lastEvalTime, p_inWrapperB->m_lastEvalTime, ma, mb, Clamp(0L,3L,inOp[0]) )		==false )
			{
					Application().LogMessage ( L"[VDB][CSG]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->Combine ( p_inWrapperA, p_inWrapperB  );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK VDB_CSG_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_CSG_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_CSG_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][CSG]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK VDB_CSG_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][CSG]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_CSG_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_CSG_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}