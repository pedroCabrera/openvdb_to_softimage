#include "Main.h"
#include "vdbHelpers.h"

#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/util/Util.h>

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{

	ID_IN_Spacing ,
	ID_IN_BackgroundValue,

	ID_IN_Fillbox,
	ID_IN_AABB,
	ID_IN_Fillboxvalue,
	ID_IN_Dense,
	ID_IN_GridName,


	ID_G_100 = 100,

	ID_OUT_VDBGrid = 200,


	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};



CStatus VDB_FillBoxGrid_Register(PluginRegistrar& reg)
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_FillBoxGrid", L"VDB Fill Box Grid");

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


	int valType = siICENodeDataFloat | siICENodeDataBool | siICENodeDataLong | siICENodeDataString | siICENodeDataVector3;


	st = nodeDef.AddInputPort(ID_IN_BackgroundValue, ID_G_100, valType,siICENodeStructureSingle, siICENodeContextSingleton,L"Background Value", L"BackgroundValue", CValue(),CValue(),CValue(),ID_TYPE_CNS,ID_UNDEF,ID_UNDEF );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Spacing, ID_G_100, siICENodeDataFloat,siICENodeStructureSingle, siICENodeContextSingleton,L"Voxel Size", L"voxelSize", CValue(0.1f) );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Fillbox, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"Fill Box", L"Fillbox", CValue(false) );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Dense, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"Densify Box", L"Dense", CValue(false) );st.AssertSucceeded();	
	st = nodeDef.AddInputPort(ID_IN_AABB, ID_G_100, siICENodeDataMatrix33,siICENodeStructureSingle, siICENodeContextSingleton,L"Box Corners", L"BoxCorners", CValue() );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Fillboxvalue, ID_G_100, valType,siICENodeStructureSingle, siICENodeContextSingleton,L"Box Value", L"FillBoxValue", CValue(),CValue(),CValue(),ID_TYPE_CNS,ID_UNDEF,ID_UNDEF );st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_GridName, ID_G_100, siICENodeDataString,	siICENodeStructureSingle, siICENodeContextSingleton,L"Grid Name", L"gridName", CValue());	st.AssertSucceeded();

	// Add custom type names.
	CStringArray customTypes(1);
	customTypes[0] = VDB_DATA_NAME;

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");st.AssertSucceeded();

	PluginItem nodeItem = reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}



struct VDB_FillBoxGrid_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_FillBoxGrid_cache_t ( ) :  m_voxelSize (-1.f) { };


	CValue m_BGValue;
	CValue m_boxValue;

	float m_voxelSize;
	bool m_fillBox;
	bool m_dense;

	MATH::CMatrix3f m_bbox;
	CString m_gridName;

	short m_dataType;

	inline bool IsDirty ( CValue bgval, CValue boxval, MATH::CMatrix3f bbox, CString name, bool fill, float vxsz, short dataType, bool dense )
	{
		bool isClean = (m_BGValue == bgval && m_boxValue== boxval && m_voxelSize==vxsz && m_fillBox==fill && m_bbox==bbox &&
			m_gridName == name && m_dataType == dataType && m_dense == dense);

		if ( isClean == false )
		{
			m_BGValue = bgval ; 
			m_boxValue= boxval ;
			m_voxelSize=vxsz ;
			m_fillBox=fill ;
			m_bbox=bbox ;
			m_gridName = name;
			m_dataType = dataType;
			m_dense = dense;
		};

		return !isClean;

	};

	inline void Rebuild ( ) 
	{
		TIMER timer;
		openvdb::math::Transform::Ptr transf = openvdb::math::Transform::createLinearTransform(m_voxelSize);

		switch (m_dataType)
		{
		case 0:
			{
				openvdb::FloatGrid::Ptr ref = openvdb::FloatGrid::create ( float(m_BGValue) );
				ref->setTransform ( transf );
				if ( m_gridName.IsEmpty() == false )
					ref->setName ( m_gridName.GetAsciiString () );

				if ( m_fillBox )
				{
					openvdb::CoordBBox bbox;
					bbox.min ().setX ( m_bbox.GetValue (0,0) );
					bbox.min ().setY ( m_bbox.GetValue (0,1) );
					bbox.min ().setZ ( m_bbox.GetValue (0,2) );
					bbox.max ().setX ( m_bbox.GetValue (2,0) );
					bbox.max ().setY ( m_bbox.GetValue (2,1) );
					bbox.max ().setZ ( m_bbox.GetValue (2,2) );



					if ( m_dense )
					{
                        float val = float(m_boxValue);
                        openvdb::FloatGrid::Accessor acc = ref->getAccessor();
						LONG nbZ = bbox.max().z ();
						LONG nbY = bbox.max().y ();
						LONG nbX = bbox.max().x ();
						for ( LONG z=bbox.min().z ();z<=nbZ; ++ z )
							for ( LONG y=bbox.min().y ();y<=nbY; ++ y )
								for ( LONG x=bbox.min().x ();x<=nbX; ++ x )
									acc.setValue (  openvdb::Coord (x,y,z), val );
					}
					else
					{
						ref->fill  (bbox, float(m_boxValue), true ) ;
						ref->pruneGrid ( 0.f );
					}
				};

				m_primaryGrid.m_grid = ref;
                m_primaryGrid.m_grid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );

			}break;
		case 1:
			{
				openvdb::BoolGrid::Ptr ref = openvdb::BoolGrid::create ( bool(m_BGValue) );
				ref->setTransform ( transf );
				if ( m_gridName.IsEmpty() == false )
					ref->setName ( m_gridName.GetAsciiString () );

				if ( m_fillBox )
				{
					openvdb::CoordBBox bbox;
					bbox.min ().setX ( m_bbox.GetValue (0,0) );
					bbox.min ().setY ( m_bbox.GetValue (0,1) );
					bbox.min ().setZ ( m_bbox.GetValue (0,2) );
					bbox.max ().setX ( m_bbox.GetValue (2,0) );
					bbox.max ().setY ( m_bbox.GetValue (2,1) );
					bbox.max ().setZ ( m_bbox.GetValue (2,2) );			

					if ( m_dense )
					{
                        bool val = bool(m_boxValue);
                        openvdb::BoolGrid::Accessor acc = ref->getAccessor();
						LONG nbZ = bbox.max().z ();
						LONG nbY = bbox.max().y ();
						LONG nbX = bbox.max().x ();
						for ( LONG z=bbox.min().z ();z<=nbZ; ++ z )
							for ( LONG y=bbox.min().y ();y<=nbY; ++ y )
								for ( LONG x=bbox.min().x ();x<=nbX; ++ x )
									acc.setValue (  openvdb::Coord (x,y,z), val );
					}
					else
					{
						ref->fill  (bbox, bool(m_boxValue), true ) ;
						ref->pruneGrid ( 0.f );
					}

				};

				m_primaryGrid.m_grid = ref;
                m_primaryGrid.m_grid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );

			}break;

		case 2:
			{
				MATH::CVector3f vec3xsi = m_BGValue;
				openvdb::Vec3SGrid::Ptr ref = openvdb::Vec3SGrid::create ( openvdb::Vec3s(vec3xsi[0], vec3xsi[1], vec3xsi[2]) );
				ref->setTransform ( transf );
				if ( m_gridName.IsEmpty() == false )
					ref->setName ( m_gridName.GetAsciiString () );

				if ( m_fillBox )
				{
					openvdb::CoordBBox bbox;
					bbox.min ().setX ( m_bbox.GetValue (0,0) );
					bbox.min ().setY ( m_bbox.GetValue (0,1) );
					bbox.min ().setZ ( m_bbox.GetValue (0,2) );
					bbox.max ().setX ( m_bbox.GetValue (2,0) );
					bbox.max ().setY ( m_bbox.GetValue (2,1) );
					bbox.max ().setZ ( m_bbox.GetValue (2,2) );
					vec3xsi = m_boxValue;


					if ( m_dense )
					{
                        openvdb::Vec3s val = openvdb::Vec3s(vec3xsi[0], vec3xsi[1], vec3xsi[2]);
                        openvdb::Vec3SGrid::Accessor acc = ref->getAccessor();
						LONG nbZ = bbox.max().z ();
						LONG nbY = bbox.max().y ();
						LONG nbX = bbox.max().x ();
						for ( LONG z=bbox.min().z ();z<=nbZ; ++ z )
							for ( LONG y=bbox.min().y ();y<=nbY; ++ y )
								for ( LONG x=bbox.min().x ();x<=nbX; ++ x )
									acc.setValue (  openvdb::Coord (x,y,z), val );
					}
					else
					{
						ref->fill  (bbox, openvdb::Vec3s(vec3xsi[0], vec3xsi[1], vec3xsi[2]), true ) ;
						ref->pruneGrid ( 0.f );
					}
				};


				m_primaryGrid.m_grid = ref;
                m_primaryGrid.m_grid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );

			}break;

		case 3:
			{
				openvdb::StringGrid::Ptr ref = openvdb::StringGrid::create ( CString(m_BGValue).GetAsciiString() );
				ref->setTransform ( transf );
				if ( m_gridName.IsEmpty() == false )
					ref->setName ( m_gridName.GetAsciiString () );

				if ( m_fillBox )
				{
					openvdb::CoordBBox bbox;
					bbox.min ().setX ( m_bbox.GetValue (0,0) );
					bbox.min ().setY ( m_bbox.GetValue (0,1) );
					bbox.min ().setZ ( m_bbox.GetValue (0,2) );
					bbox.max ().setX ( m_bbox.GetValue (2,0) );
					bbox.max ().setY ( m_bbox.GetValue (2,1) );
					bbox.max ().setZ ( m_bbox.GetValue (2,2) );


					if ( m_dense )
					{
                        const char* val = CString(m_boxValue).GetAsciiString();
                        openvdb::StringGrid::Accessor acc = ref->getAccessor();
						LONG nbZ = bbox.max().z ();
						LONG nbY = bbox.max().y ();
						LONG nbX = bbox.max().x ();
						for ( LONG z=bbox.min().z ();z<=nbZ; ++ z )
							for ( LONG y=bbox.min().y ();y<=nbY; ++ y )
								for ( LONG x=bbox.min().x ();x<=nbX; ++ x )
									acc.setValue (  openvdb::Coord (x,y,z), val );
					}
					else
					{
						ref->fill  (bbox, CString(m_boxValue).GetAsciiString(), true ) ;
						ref->pruneGrid ( 0.f );
					}


				};

				m_primaryGrid.m_grid = ref;
                m_primaryGrid.m_grid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );

			}break;

		case 4:
			{
				openvdb::Int32Grid::Ptr ref = openvdb::Int32Grid::create ( LONG(m_BGValue) );
				ref->setTransform ( transf );
				if ( m_gridName.IsEmpty() == false )
					ref->setName ( m_gridName.GetAsciiString () );

				if ( m_fillBox )
				{
					openvdb::CoordBBox bbox;
					bbox.min ().setX ( m_bbox.GetValue (0,0) );
					bbox.min ().setY ( m_bbox.GetValue (0,1) );
					bbox.min ().setZ ( m_bbox.GetValue (0,2) );
					bbox.max ().setX ( m_bbox.GetValue (2,0) );
					bbox.max ().setY ( m_bbox.GetValue (2,1) );
					bbox.max ().setZ ( m_bbox.GetValue (2,2) );


					if ( m_dense )
					{
                        LONG val = LONG(m_boxValue);
                        openvdb::Int32Grid::Accessor acc = ref->getAccessor();
						LONG nbZ = bbox.max().z ();
						LONG nbY = bbox.max().y ();
						LONG nbX = bbox.max().x ();
						for ( LONG z=bbox.min().z ();z<=nbZ; ++ z )
							for ( LONG y=bbox.min().y ();y<=nbY; ++ y )
								for ( LONG x=bbox.min().x ();x<=nbX; ++ x )
									acc.setValue (  openvdb::Coord (x,y,z), val );
					}
					else
					{
						ref->fill  (bbox, LONG(m_boxValue), true ) ;
						ref->pruneGrid ( 0.f );
					}

				};

				m_primaryGrid.m_grid = ref;
                m_primaryGrid.m_grid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );
			} break;


		default:
			return;
			break;
		}

		m_primaryGrid.m_lastEvalTime = clock ();
        Application().LogMessage(L"[VDB][FILLBOX]: Stamped at=" + CString((LONG)m_primaryGrid.m_lastEvalTime));
		Application().LogMessage(L"[VDB][FILLBOX]: Done in=" + CString(timer.GetElapsedTime()));
	};


};


SICALLBACK dlexport VDB_FillBoxGrid_Evaluate(ICENodeContext& in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_VDBGrid:
		{
			CDataArrayCustomType outData(in_ctxt);

			// get cache object
			VDB_FillBoxGrid_cache_t * p_gridOwner = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][FILLBOX]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};



			// get cache
			p_gridOwner = (VDB_FillBoxGrid_cache_t*)(CValue::siPtrType)userData;

			// send grid
			p_gridOwner->PackGrid ( outData );

			// check if we really need to recalc or can jus use prev result
			// ###########################################################
			// buff inputs
			CDataArrayFloat inVoxelSize( in_ctxt, ID_IN_Spacing);
			CDataArrayString gridName( in_ctxt, ID_IN_GridName);
			CDataArrayBool fillBox ( in_ctxt, ID_IN_Fillbox );
			CDataArrayBool denseBox ( in_ctxt, ID_IN_Dense );
			CDataArrayMatrix3f bboxCor ( in_ctxt, ID_IN_AABB );

			float voxelSizeSafe = Clamp(0.0001f,FLT_MAX,inVoxelSize[0]);

			MATH::CMatrix3f corners = bboxCor[0];
			float * p_mat = (float*)(&corners);
			float &minx = p_mat[0];
			float &miny = p_mat[1];
			float &minz = p_mat[2];
			float &maxx = p_mat[6];
			float &maxy = p_mat[7];
			float &maxz = p_mat[8];

			minx = Clamp ( -FLT_MAX, maxx-0.0001f , minx )/voxelSizeSafe;
			miny = Clamp ( -FLT_MAX, maxy-0.0001f , miny )/voxelSizeSafe;
			minz = Clamp ( -FLT_MAX, maxz-0.0001f , minz )/voxelSizeSafe;
			maxx = Clamp ( minx+0.0001f , FLT_MAX, maxx )/voxelSizeSafe;
			maxy = Clamp ( miny+0.0001f , FLT_MAX, maxy )/voxelSizeSafe;
			maxz = Clamp ( minz+0.0001f , FLT_MAX, maxz )/voxelSizeSafe;

			// obtain inports structs
			XSI::siICENodeDataType portDataType;
			XSI::siICENodeStructureType param1;
			XSI::siICENodeContextType param2;
			in_ctxt.GetPortInfo( ID_IN_BackgroundValue , portDataType, param1, param2 );


			CValue BGval ;
			CValue BoxVal;
			short datatype=-1;

			switch (portDataType)
			{
            case siICENodeDataFloat :
				{
					CDataArrayFloat bgval( in_ctxt, ID_IN_BackgroundValue );
					CDataArrayFloat boxval( in_ctxt, ID_IN_Fillboxvalue );
					BGval = bgval[0];
					BoxVal = boxval[0];
					datatype = 0;
				}break;
            case siICENodeDataBool :
				{
					CDataArrayBool bgval( in_ctxt, ID_IN_BackgroundValue );
					CDataArrayBool boxval( in_ctxt, ID_IN_Fillboxvalue );
					BGval = bgval[0];
					BoxVal = boxval[0];
					datatype = 1;
				}break;
            case siICENodeDataVector3 :
				{
                    CDataArrayVector3f bgval( in_ctxt, ID_IN_BackgroundValue);
					CDataArrayVector3f boxval( in_ctxt, ID_IN_Fillboxvalue );
					BGval = bgval[0];
					BoxVal = boxval[0];
					datatype = 2;
				}break;
            case siICENodeDataString :
				{
					CDataArrayString bgval( in_ctxt, ID_IN_BackgroundValue );
					CDataArrayString boxval( in_ctxt, ID_IN_Fillboxvalue );
					BGval = bgval[0];
					BoxVal = boxval[0];
					datatype = 3;
				}break;
            case siICENodeDataLong :
				{
					CDataArrayLong bgval( in_ctxt, ID_IN_BackgroundValue );
					CDataArrayLong boxval( in_ctxt, ID_IN_Fillboxvalue );
					BGval = bgval[0];
					BoxVal = boxval[0];
					datatype = 4;
				}break;
			}



			// compare with prev res
			if  (	p_gridOwner->IsDirty ( BGval, BoxVal, corners, gridName[0], fillBox[0], voxelSizeSafe, datatype, denseBox[0] ) == false )
			{
				Application().LogMessage( L"[VDB][FILLBOX]: Data is not changed, used prev result" );
				return CStatus::OK;
			};


			// refill
			p_gridOwner->Rebuild ( );



		}break;
	default:
		break;
	};

	return CStatus::OK;
};

// lets cache this
SICALLBACK dlexport VDB_FillBoxGrid_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();
	VDB_FillBoxGrid_cache_t * p_gridOwner;
	if (userData.IsEmpty())
	{
		p_gridOwner = new VDB_FillBoxGrid_cache_t;
	}
	else
	{
		Application().LogMessage ( L"[VDB][FILLBOX]: Fatal, unknow data is present on initialization", siFatalMsg );
		return CStatus::Fail;
	}

	ctxt.PutUserData((CValue::siPtrType)p_gridOwner);
	return CStatus::OK;
}



SICALLBACK dlexport VDB_FillBoxGrid_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();

	if ( userData.IsEmpty () )
	{
		Application().LogMessage ( L"[VDB][FILLBOX]: Fatal, no data on termination", siFatalMsg );
		return CStatus::Fail;
	}

	VDB_FillBoxGrid_cache_t * p_gridOwner;
	p_gridOwner = (VDB_FillBoxGrid_cache_t*)(CValue::siPtrType)userData;


	delete p_gridOwner;
	ctxt.PutUserData(CValue());

	return CStatus::OK;
}
