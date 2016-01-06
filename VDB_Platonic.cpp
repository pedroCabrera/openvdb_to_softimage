#include "Main.h"
#include "vdbHelpers.h"

#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/LevelSetUtil.h>

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_Pos ,
	ID_IN_Rad ,
	ID_IN_Spacing ,

	ID_IN_BandWidth ,
	ID_IN_ConvertToFog,

	ID_G_100 = 100,

	ID_OUT_VDBGrid = 200,


	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};


CStatus VDB_Platonic_Register(PluginRegistrar& reg)
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_Platonic", L"VDB Platonic");

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

	st = nodeDef.AddInputPort(ID_IN_Pos, ID_G_100, siICENodeDataVector3,	siICENodeStructureSingle, siICENodeContextSingleton,L"Center", L"Center");st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Rad, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"Radius", L"Radius");st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Spacing, ID_G_100, siICENodeDataFloat,siICENodeStructureSingle, siICENodeContextSingleton,L"Voxel Size", L"voxelSize", CValue(0.1));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_BandWidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"BandWidth", L"BandWidth", CValue(1.5f));st.AssertSucceeded();
st = nodeDef.AddInputPort(ID_IN_ConvertToFog, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"ConvertToFogvolume", L"ConvertToFogvolume", CValue(true));st.AssertSucceeded();
	
	// Add custom type names.
	CStringArray customTypes(1);
	customTypes[0] = VDB_DATA_NAME;

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");st.AssertSucceeded();

	PluginItem nodeItem = reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}



struct VDB_Platonic_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_Platonic_cache_t ( ) : m_voxelSize (-1.f) { };

	MATH::CVector3f m_pos;
	float m_rad;
	float m_bs;
	float m_voxelSize;
	bool m_toFog;

	inline bool IsDirty ( MATH::CVector3f pos,	float rad,	float bs, float voxelSize,	bool toFog )
	{
		bool isClean = ( m_pos==pos && m_rad==rad && m_bs==bs && m_voxelSize==voxelSize && m_toFog==toFog );

		if ( isClean == false )
		{
			m_pos=pos;
			m_rad=rad ;
			m_bs=bs ;
			m_voxelSize=voxelSize;
			m_toFog=toFog;
		};

		return !isClean;
		
	};

	inline void Create ( )
	{
		TIMER timer;
		

		openvdb::FloatGrid::Ptr tempPtr;
		try
		{
			tempPtr = openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>  (m_rad, openvdb::Vec3d(m_pos[0],m_pos[1],m_pos[2]), m_voxelSize, m_bs);


			// check if we nedd to convert it to fogvolume
			if ( m_toFog )
			{
				openvdb::tools::sdfToFogVolume(*tempPtr);
                tempPtr->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );
			}
			else
			{
                tempPtr->setGridClass ( openvdb::v2_3_0::GRID_LEVEL_SET );
			};


			// attach new grid
			m_primaryGrid.m_grid = tempPtr;

			// set name
			m_primaryGrid.m_grid->setName("PlatonicSphere");

			// mark time of creation for dirty-stating
			m_primaryGrid.m_lastEvalTime = clock();
            Application().LogMessage(L"[VDB][PLATONIC]: Stamped at=" + CString((LONG)m_primaryGrid.m_lastEvalTime));
			Application().LogMessage(L"[VDB][PLATONIC]: Done in=" + CString(timer.GetElapsedTime()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][PLATONIC]: " + CString(e.what()), siErrorMsg );
			return;
		}



	};
};


SICALLBACK dlexport VDB_Platonic_Evaluate(ICENodeContext& in_ctxt)
{
	

	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_VDBGrid:
		{
			CDataArrayCustomType outData(in_ctxt);

			// get cache object
			VDB_Platonic_cache_t * p_gridOwner = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][PLATONIC]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};

			// get cache
			p_gridOwner = (VDB_Platonic_cache_t*)(CValue::siPtrType)userData;
			p_gridOwner->PackGrid ( outData );

			
		


			// check if we really need to recalc or can jus use prev result
			// ###########################################################
			// buff inputs
			CDataArrayVector3f inPos ( in_ctxt, ID_IN_Pos );
			CDataArrayFloat inRad ( in_ctxt, ID_IN_Rad );
			CDataArrayFloat inVoxelSize( in_ctxt, ID_IN_Spacing);
			CDataArrayFloat inBS( in_ctxt, ID_IN_BandWidth);
			CDataArrayBool inToFog ( in_ctxt, ID_IN_ConvertToFog );


			

			float voxelSizeSafe = Clamp(0.0001f,FLT_MAX,inVoxelSize[0]);

			// compare with prev res
			if  (	p_gridOwner->IsDirty ( inPos[0], Clamp(0.000001f, FLT_MAX, inRad[0] ), 
			Clamp(1.f,FLT_MAX,	inBS[0]),  voxelSizeSafe,  inToFog[0] ) == false )
			{
				Application().LogMessage( L"[VDB][PLATONIC]: Data is not changed, used prev result" );
				return CStatus::OK;
			}

			// convert
			p_gridOwner->Create ();



		}break;
	default:
		break;
	};

	return CStatus::OK;
};

// lets cache this
SICALLBACK dlexport VDB_Platonic_Init( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Platonic_cache_t * p_gridOwner;
   if (userData.IsEmpty())
   {
      p_gridOwner = new VDB_Platonic_cache_t;
   }
   else
   {
	   Application().LogMessage ( L"[VDB][PLATONIC]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }



    Application().LogMessage ( L"[VDB][PLATONIC]: This node simply creates a signle sphere volume eqither sdf of gof for demonstration purposes. Note, bandwith is in voxels on this example." ); 
   ctxt.PutUserData((CValue::siPtrType)p_gridOwner);
   return CStatus::OK;
}



SICALLBACK dlexport VDB_Platonic_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][PLATONIC]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_Platonic_cache_t * p_gridOwner;
   p_gridOwner = (VDB_Platonic_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_gridOwner;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
