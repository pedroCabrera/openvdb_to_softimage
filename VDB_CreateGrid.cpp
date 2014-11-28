#include "Main.h"
#include "vdbHelpers.h"

 #include <openvdb/util/Util.h>
 #include <openvdb/Types.h>
#include <openvdb/Grid.h>
 #include <openvdb/math/Math.h>
 #include <openvdb/math/Transform.h>
 #include <openvdb/util/NullInterrupter.h>

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_TopologyRefGrid,
	ID_IN_Spacing ,
	ID_IN_BackgroundValue,	
	ID_IN_GridName,


	ID_G_100 = 100,

	ID_OUT_VDBGrid = 200,


	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};



CStatus VDB_CreateGrid_Register(PluginRegistrar& reg)
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_CreateGrid", L"VDB Create Grid");

	CStatus st;
	st = nodeDef.PutColor(VDB_NODE_COLOR);
	st.AssertSucceeded();

	st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
	st.AssertSucceeded();

	// Add custom types definition
	st = nodeDef.DefineCustomType(VDB_DATA_NAME,VDB_DATA_NAME, VDB_DATA_DESC, VDB_STREAM_COLOR); 
	st.AssertSucceeded();

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);	st.AssertSucceeded();

		// Add custom type names.
	CStringArray customTypes(1);
	customTypes[0] = VDB_DATA_NAME;

	int valType = siICENodeDataFloat | siICENodeDataBool | siICENodeDataLong | siICENodeDataString | siICENodeDataVector3;

	st = nodeDef.AddInputPort(ID_IN_TopologyRefGrid, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"Topo Reference Grid", L"TopoRefGrid",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();	

	st = nodeDef.AddInputPort(ID_IN_BackgroundValue, ID_G_100, valType,siICENodeStructureSingle, siICENodeContextSingleton,L"Background Value", L"BackgroundValue", CValue(),CValue(),CValue(),ID_TYPE_CNS,ID_UNDEF,ID_UNDEF );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Spacing, ID_G_100, siICENodeDataFloat,siICENodeStructureSingle, siICENodeContextSingleton,L"Voxel Size", L"voxelSize", CValue(0.1f) );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_GridName, ID_G_100, siICENodeDataString,	siICENodeStructureSingle, siICENodeContextSingleton,L"Grid Name", L"gridName", CValue());	st.AssertSucceeded();



	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");st.AssertSucceeded();

	PluginItem nodeItem = reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}

using namespace openvdb;

template < typename treein, typename gridin, typename Tin >
gridin * CopyTopo ( Tin & grid  )
{	
	treein::Ptr attTree(new treein ( grid->tree(), gridin::ValueType(), openvdb::TopologyCopy()));
	return new  gridin(attTree);
};


struct VDB_CreateGrid_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_CreateGrid_cache_t ( ) :  m_voxelSize (-1.f) { };


	CValue m_BGValue;

	float m_voxelSize;
	clock_t m_inputEvalTime; // -1 if no grid ref
	CString m_gridName;
	short m_dataType;

	inline bool IsDirty ( CValue bgval, CString name, VDB_ICEFlowDataWrapper_t * inrefgrid, float vxsz, short dataType )
	{
			clock_t tm = (inrefgrid && inrefgrid->m_grid )? inrefgrid->m_lastEvalTime : -1;
		bool isClean = (m_BGValue == bgval &&  m_voxelSize==vxsz &&
			m_gridName == name && m_dataType == dataType && m_inputEvalTime==tm );

	


		if ( isClean == false )
		{
			m_BGValue = bgval ; 
		
			m_voxelSize=vxsz ;
			m_inputEvalTime=tm;
			m_gridName = name;
			m_dataType = dataType;

		};

		return !isClean;

	};

	inline void Create ( VDB_ICEFlowDataWrapper_t * inrefgrid ) 
	{
		TIMER timer;
		openvdb::math::Transform::Ptr transf = openvdb::math::Transform::createLinearTransform(m_voxelSize);

		try {

		switch (m_dataType)
		{
		case 0:
			{
				

				if ( inrefgrid && inrefgrid->m_grid )
				{
					if (inrefgrid->m_grid->isType<openvdb::BoolGrid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<BoolGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::FloatGrid>()) m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<FloatGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::DoubleGrid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<DoubleGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int32Grid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<Int32Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int64Grid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<Int64Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3IGrid>()) m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<Vec3IGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3SGrid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<Vec3SGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3DGrid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<Vec3DGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::StringGrid>())m_primaryGrid.m_grid =  FloatGrid::Ptr(CopyTopo <FloatTree, FloatGrid > (gridPtrCast<StringGrid>  (inrefgrid->m_grid)));

			gridPtrCast<FloatGrid> (m_primaryGrid.m_grid)->setBackground ( float(m_BGValue) );
			m_primaryGrid.m_grid->setTransform ( inrefgrid->m_grid->transformPtr	());
				}
				else
				{
					openvdb::FloatGrid::Ptr cleanGrid = openvdb::FloatGrid::create ( float(m_BGValue) );
						cleanGrid->setTransform ( transf );
					m_primaryGrid.m_grid = cleanGrid;
				}

				

			}break;
		case 1:
			{
				
			
			
	if ( inrefgrid && inrefgrid->m_grid )
				{
					if (inrefgrid->m_grid->isType<openvdb::BoolGrid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<BoolGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::FloatGrid>()) m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<FloatGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::DoubleGrid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<DoubleGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int32Grid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<Int32Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int64Grid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<Int64Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3IGrid>()) m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<Vec3IGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3SGrid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<Vec3SGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3DGrid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<Vec3DGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::StringGrid>())m_primaryGrid.m_grid =  BoolGrid::Ptr(CopyTopo <BoolTree, BoolGrid > (gridPtrCast<StringGrid>  (inrefgrid->m_grid)));
		
					gridPtrCast<BoolGrid> (m_primaryGrid.m_grid)->setBackground ( bool(m_BGValue) );
					m_primaryGrid.m_grid->setTransform ( inrefgrid->m_grid->transformPtr	());
				}
				else
				{
				openvdb::BoolGrid::Ptr cleanGrid = openvdb::BoolGrid::create ( bool(m_BGValue) );
					cleanGrid->setTransform ( transf );
					m_primaryGrid.m_grid = cleanGrid;
				}
			
			}break;

		case 2:
			{
			
			
			

	if ( inrefgrid && inrefgrid->m_grid )
				{
					if (inrefgrid->m_grid->isType<openvdb::BoolGrid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<BoolGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::FloatGrid>()) m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<FloatGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::DoubleGrid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<DoubleGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int32Grid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<Int32Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int64Grid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<Int64Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3IGrid>()) m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<Vec3IGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3SGrid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<Vec3SGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3DGrid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<Vec3DGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::StringGrid>())m_primaryGrid.m_grid =  Vec3SGrid::Ptr(CopyTopo <Vec3STree, Vec3SGrid > (gridPtrCast<StringGrid>  (inrefgrid->m_grid)));
		MATH::CVector3f vec3xsi = m_BGValue;
					gridPtrCast<Vec3SGrid> (m_primaryGrid.m_grid)->setBackground (  openvdb::Vec3s(vec3xsi[0], vec3xsi[1], vec3xsi[2]) );
					m_primaryGrid.m_grid->setTransform ( inrefgrid->m_grid->transformPtr	());
				}
				else
				{
					MATH::CVector3f vec3xsi = m_BGValue;
				openvdb::Vec3SGrid::Ptr cleanGrid = openvdb::Vec3SGrid::create ( openvdb::Vec3s(vec3xsi[0], vec3xsi[1], vec3xsi[2]) );
						cleanGrid->setTransform ( transf );
					m_primaryGrid.m_grid = cleanGrid;
				}
			

			}break;

		case 3:
			{
				
				

					if ( inrefgrid && inrefgrid->m_grid )
				{
					if (inrefgrid->m_grid->isType<openvdb::BoolGrid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<BoolGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::FloatGrid>()) m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<FloatGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::DoubleGrid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<DoubleGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int32Grid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<Int32Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int64Grid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<Int64Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3IGrid>()) m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<Vec3IGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3SGrid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<Vec3SGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3DGrid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<Vec3DGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::StringGrid>())m_primaryGrid.m_grid =  StringGrid::Ptr(CopyTopo <StringTree, StringGrid > (gridPtrCast<StringGrid>  (inrefgrid->m_grid)));
		
					gridPtrCast<StringGrid> (m_primaryGrid.m_grid)->setBackground ( CString(m_BGValue).GetAsciiString() );
					m_primaryGrid.m_grid->setTransform ( inrefgrid->m_grid->transformPtr	());
				}
				else
				{
					openvdb::StringGrid::Ptr cleanGrid = openvdb::StringGrid::create ( CString(m_BGValue).GetAsciiString() );
							cleanGrid->setTransform ( transf );
					m_primaryGrid.m_grid = cleanGrid;
				}
			
			}break;

		case 4:
			{
		
				

				if ( inrefgrid && inrefgrid->m_grid )
				{
					if (inrefgrid->m_grid->isType<openvdb::BoolGrid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<BoolGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::FloatGrid>()) m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<FloatGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::DoubleGrid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<DoubleGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int32Grid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<Int32Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Int64Grid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<Int64Grid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3IGrid>()) m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<Vec3IGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3SGrid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<Vec3SGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::Vec3DGrid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<Vec3DGrid>  (inrefgrid->m_grid)));
			else if (inrefgrid->m_grid->isType<openvdb::StringGrid>())m_primaryGrid.m_grid =  Int32Grid::Ptr(CopyTopo <Int32Tree, Int32Grid > (gridPtrCast<StringGrid>  (inrefgrid->m_grid)));
		
					gridPtrCast<Int32Grid> (m_primaryGrid.m_grid)->setBackground ( LONG(m_BGValue) );
					m_primaryGrid.m_grid->setTransform ( inrefgrid->m_grid->transformPtr	());
				}
				else
				{
						openvdb::Int32Grid::Ptr cleanGrid = openvdb::Int32Grid::create ( LONG(m_BGValue) );
								cleanGrid->setTransform ( transf );
					m_primaryGrid.m_grid = cleanGrid;
				}
			
			} break;


		default:
			return;
			break;
		}

		m_primaryGrid.m_grid->setGridClass ( openvdb::v2_1_0::GridClass::GRID_FOG_VOLUME );
		m_primaryGrid.m_grid->setName ( m_gridName.GetAsciiString() );
		m_primaryGrid.m_lastEvalTime = clock ();
		Application().LogMessage(L"[VDB][CREATEGRID]: Stamped at=" + CString(m_primaryGrid.m_lastEvalTime));
		Application().LogMessage(L"[VDB][CREATEGRID]: Done in=" + CString(timer.GetElapsedTime()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][CREATEGRID]: " + CString(e.what()),siErrorMsg);
		};
	};


};


SICALLBACK VDB_CreateGrid_Evaluate(ICENodeContext& in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_VDBGrid:
		{
			CDataArrayCustomType outData(in_ctxt);

			// get cache object
			VDB_CreateGrid_cache_t * p_gridOwner = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][CREATEGRID]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};



			// get cache
			p_gridOwner = (VDB_CreateGrid_cache_t*)(CValue::siPtrType)userData;

			// send grid
			p_gridOwner->PackGrid ( outData );



						
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_TopologyRefGrid);

			// check for prim grid
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_gridOwner->UnpackGrid ( inVDBGridPort );


			// buff inputs
			CDataArrayFloat inVoxelSize( in_ctxt, ID_IN_Spacing);
			CDataArrayString gridName( in_ctxt, ID_IN_GridName);

			float voxelSizeSafe = Clamp(0.0001f,FLT_MAX,inVoxelSize[0]);


			// obtain inports structs
			XSI::siICENodeDataType portDataType;
			XSI::siICENodeStructureType param1;
			XSI::siICENodeContextType param2;
			in_ctxt.GetPortInfo( ID_IN_BackgroundValue , portDataType, param1, param2 );


			CValue BGval ;
			CValue actVal;
			short datatype=-1;

			switch (portDataType)
			{
			case siICENodeDataType::siICENodeDataFloat :
				{
					CDataArrayFloat bgval( in_ctxt, ID_IN_BackgroundValue );
				
					BGval = bgval[0];
				
					datatype = 0;
				}break;
			case siICENodeDataType::siICENodeDataBool :
				{
					CDataArrayBool bgval( in_ctxt, ID_IN_BackgroundValue );
				
					BGval = bgval[0];
				
					datatype = 1;
				}break;
			case siICENodeDataType::siICENodeDataVector3 :
				{
					CDataArrayVector3f bgval( in_ctxt, ID_IN_BackgroundValue );
				
					BGval = bgval[0];
				
					datatype = 2;
				}break;
			case siICENodeDataType::siICENodeDataString :
				{
					CDataArrayString bgval( in_ctxt, ID_IN_BackgroundValue );
			
					BGval = bgval[0];
				
					datatype = 3;
				}break;
			case siICENodeDataType::siICENodeDataLong :
				{
					CDataArrayLong bgval( in_ctxt, ID_IN_BackgroundValue );
				
					BGval = bgval[0];
				
					datatype = 4;
				}break;
			}



			// compare with prev res
			if  (	p_gridOwner->IsDirty ( BGval, gridName[0], p_inWrapper, voxelSizeSafe, datatype ) == false )
			{
				Application().LogMessage( L"[VDB][CREATEGRID]: Data is not changed, used prev result" );
				return CStatus::OK;
			};


			// refill
			p_gridOwner->Create( p_inWrapper );



		}break;
	default:
		break;
	};

	return CStatus::OK;
};

// lets cache this
SICALLBACK VDB_CreateGrid_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();
	VDB_CreateGrid_cache_t * p_gridOwner;
	if (userData.IsEmpty())
	{
		p_gridOwner = new VDB_CreateGrid_cache_t;
	}
	else
	{
		Application().LogMessage ( L"[VDB][CREATEGRID]: Fatal, unknow data is present on initialization", siFatalMsg );
		return CStatus::Fail;
	}

	ctxt.PutUserData((CValue::siPtrType)p_gridOwner);
	return CStatus::OK;
}



SICALLBACK VDB_CreateGrid_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();

	if ( userData.IsEmpty () )
	{
		Application().LogMessage ( L"[VDB][CREATEGRID]: Fatal, no data on termination", siFatalMsg );
		return CStatus::Fail;
	}

	VDB_CreateGrid_cache_t * p_gridOwner;
	p_gridOwner = (VDB_CreateGrid_cache_t*)(CValue::siPtrType)userData;


	delete p_gridOwner;
	ctxt.PutUserData(CValue());

	return CStatus::OK;
}
