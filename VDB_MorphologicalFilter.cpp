#include "Main.h"
#include "vdbHelpers.h"

#include <openvdb/tools/Filter.h>
#include <openvdb/tools/LevelSetFilter.h>

enum IDs
{
	ID_IN_VDBGrid ,

	ID_IN_Op ,
	ID_IN_NbIters,
	ID_IN_Radius,
	ID_IN_RadiusInVoxels,


	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_MorphologicalFilter_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_MorphologicalFilter", L"VDB Morphological Filter");

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
	st = nodeDef.AddInputPort(ID_IN_Op, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"Function", L"Op",0,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_Radius, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Radius", L"Radius",1.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_RadiusInVoxels, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"RadiusInVoxels", L"RadiusInVoxels",false,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_NbIters, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"NbInterations", L"NbInterations",3,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_Filter_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_Filter_cache_t ( ) { };

	clock_t  m_inputEvalTime;
	int m_gridType;
	int m_nbIters;
	int m_filterType;
	float m_radius;


	enum FilterOp
	{
		UNDEF = -1,
		OFFSET = 0,
		MEAN,
		GAUSS,
		MEDIAN,

		RENORM,
		MEAN_CURVATURE,
		LAPLASIAN_FLOW,
		DILATE,
		ERODE,
		OPEN,
		CLOSE,
		TRACK

	};


	bool inline IsDirty ( VDB_ICEFlowDataWrapper_t * in_grid, int gridtype, int nbiters, int filtertype, float rad )
	{
		bool isClean = ( m_inputEvalTime == (*in_grid).m_lastEvalTime && m_gridType == gridtype 
			&& m_nbIters == nbiters && m_filterType == filtertype && m_radius == rad);

		if ( isClean == false )
		{	

			m_inputEvalTime = (*in_grid).m_lastEvalTime;
			m_gridType = gridtype ; 
	
			m_nbIters = nbiters ; 
			m_filterType = filtertype;
			m_radius = rad;

		};

		return !isClean;

	};

	inline void Filter ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		TIMER timer;
	
		openvdb::GridBase::Ptr tempBaseGrid = in_grid->m_grid->deepCopyGrid();
		openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid> ( tempBaseGrid );
	
		try {

		// 0=fogvolume 1=levelset
		if ( m_gridType == 0 ) // fog volume is limited 
		{

			openvdb::tools::Filter<openvdb::FloatGrid> filter(*grid);

			switch (m_filterType)
			{
			case OFFSET:
				filter.offset ( m_radius );
				break;
			case MEAN:
				filter.mean( m_radius, m_nbIters );
				break;
			case GAUSS:
				filter.gaussian( m_radius, m_nbIters );
				break;
			case MEDIAN:
				filter.median( m_radius, m_nbIters );
				break;
			default:
				Application().LogMessage(L"[VDB][MORPHOLOGY]: This filter type is not allowed on FOG_VOLUME grids!", siWarningMsg);
				break;
			}
		}
		else
		{

			openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*grid);

			switch (m_filterType)
			{

			case MEAN:
				filter.mean ( m_radius );
				break;
			case DILATE:
				filter.offset ( -m_radius );
				break;
			case ERODE:
				filter.mean( m_radius );
				break;
			case GAUSS:
				filter.gaussian( m_radius );
				break;
			case MEDIAN:
				filter.median( m_radius );
				break;
			case MEAN_CURVATURE:
				filter.meanCurvature(  );
				break;
			case LAPLASIAN_FLOW:
				filter.laplacian(  );
				break;
			case RENORM:
				{
					filter.setSpatialScheme(openvdb::math::FIRST_BIAS);
					filter.normalize();
				}
				break;
			case TRACK:				
				filter.track ();					
				break;
			default:
				Application().LogMessage(L"[VDB][MORPHOLOGY]: This filter type is not allowed on LEVEL_SET grids!", siWarningMsg);
				break;
			}
		};

		
		m_primaryGrid.m_grid = grid;
		m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
		m_primaryGrid.m_lastEvalTime = clock();
        Application().LogMessage(L"[VDB][MORPHOLOGY]: Stamped at=" + CString ( (LONG)m_primaryGrid.m_lastEvalTime));
		Application().LogMessage(L"[VDB][MORPHOLOGY]: Done in=" + CString (timer.GetElapsedTime ()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][MORPHOLOGY]: " + CString ( e.what()), siErrorMsg);
		}
	};


};

SICALLBACK dlexport VDB_MorphologicalFilter_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

				CDataArrayCustomType outData( in_ctxt );    

			// get cache object
			VDB_Filter_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][MORPHOLOGY]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_Filter_cache_t*)(CValue::siPtrType)userData;
				p_cacheNodeObject->PackGrid ( outData );

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][MORPHOLOGY]: Empty grid on input" );
				return CStatus::OK;
			};

			if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>()==false )
			{
				Application().LogMessage ( L"[VDB][MORPHOLOGY]: Invalid grid data type! Float only allowed!", siWarningMsg );
				return CStatus::OK;
			}

		             
		
			
	
			CDataArrayLong inOp ( in_ctxt, ID_IN_Op );
			CDataArrayLong inIters ( in_ctxt, ID_IN_NbIters );
			CDataArrayFloat inRadius ( in_ctxt, ID_IN_Radius );
			CDataArrayBool inRadInVoxels ( in_ctxt, ID_IN_RadiusInVoxels );
	

			int gridType=0; // 0=fogvolume 1=levelset
			if ( p_inWrapper->m_grid->getGridClass () == openvdb::GRID_LEVEL_SET )
				gridType = 1;

			float safeRadius =  inRadius[0] ; //Clamp ( 0.f, FLT_MAX,  inRadInVoxels[0] ? inRadius[0]*p_inWrapper->m_grid->voxelSize().x() : inRadius[0]  );



			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper, gridType, Clamp(1L,LONG_MAX,inIters[0]), inOp[0], safeRadius )==false )
			{
				Application().LogMessage ( L"[VDB][MORPHOLOGY]: No changes on input, used prev result" );
				return CStatus::OK;
			}

			// change if dirty
			p_cacheNodeObject->Filter ( p_inWrapper );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK dlexport VDB_MorphologicalFilter_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Filter_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_Filter_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][MORPHOLOGY]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }


   Application().LogMessage ( L"[VDB][MORPHOLOGY]: FOG_VOLUME is only allowed to: Offset | MeanValue | Gaussian | MedianValue " );


   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK dlexport VDB_MorphologicalFilter_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][MORPHOLOGY]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_Filter_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_Filter_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
