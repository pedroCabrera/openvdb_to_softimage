#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_userdatablob.h>

#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tree/ValueAccessor.h>

enum IDs
{
	ID_IN_VDBGrid ,

	ID_IN_Revert ,
	

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_ChangeGridClass_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_ChangeGridClass", L"VDB Change Grid Class");

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
	st = nodeDef.AddInputPort(ID_IN_Revert, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"SDF to FOG", L"SDFtoFOG",true,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_ChangeGridClass_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_ChangeGridClass_cache_t ( ) { };


	bool m_order;
	clock_t  m_inputEvalTime;



	bool inline IsDirty ( VDB_ICEFlowDataWrapper_t * in_grid, bool order )
	{
		bool isClean = ( m_inputEvalTime == (*in_grid).m_lastEvalTime && m_order == order  );

		if ( isClean == false )
		{	
		
			m_inputEvalTime = (*in_grid).m_lastEvalTime;
			m_order = order;
		};

		return !isClean;

	};

	inline void Change ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		TIMER timer;

		try {

		if ( m_order ) // sdf to fog
		{
            if ( in_grid->m_grid->getGridClass() == openvdb::v2_3_0::GRID_FOG_VOLUME )
			{
				m_primaryGrid.m_grid = in_grid->m_grid->deepCopyGrid ();
				Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: Input grid  is already FOG_VOLUME class", siWarningMsg );
				return ;
			}
			if ( in_grid->m_grid->isType<openvdb::FloatGrid>() )
			{
				openvdb::GridBase::Ptr tempBaseGrid  = in_grid->m_grid->deepCopyGrid ();
				openvdb::FloatGrid::Ptr grid =  openvdb::gridPtrCast<openvdb::FloatGrid>(tempBaseGrid);
				openvdb::tools::sdfToFogVolume ( *grid );
                grid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );
				m_primaryGrid.m_grid  = grid;
			};
			
		}
		else // fog to sdf
		{
            if ( in_grid->m_grid->getGridClass() == openvdb::v2_3_0::GRID_LEVEL_SET )
			{
				m_primaryGrid.m_grid = in_grid->m_grid->deepCopyGrid ();
				m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
				Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: Input grid  is already LEVEL_SET class", siWarningMsg );
				return ;
			}
			openvdb::FloatGrid & grid = *( openvdb::gridPtrCast<openvdb::FloatGrid>(in_grid->m_grid));
			openvdb::tools::VolumeToMesh mesher(0.5);
			mesher(grid);
			// Convert to SDF
			openvdb::math::Transform::Ptr transform = grid.transformPtr();
			std::vector<openvdb::math::Vec3s> points;
			points.reserve(mesher.pointListSize());
			for (size_t i = 0, n = mesher.pointListSize(); i < n; i++) {
				// The MeshToVolume conversion further down, requires the
				// points to be in grid index space.
				points.push_back(transform->worldToIndex(mesher.pointList()[i]));
			}

			openvdb::tools::PolygonPoolList& polygonPoolList = mesher.polygonPoolList();

            std::vector<openvdb::v2_3_0::Vec4I> primitives;
			size_t numPrimitives = 0;
			for (size_t n = 0, N = mesher.polygonPoolListSize(); n < N; ++n) {
				const openvdb::tools::PolygonPool& polygons = polygonPoolList[n];
				numPrimitives += polygons.numQuads();
				numPrimitives += polygons.numTriangles();
			}
			primitives.reserve(numPrimitives);

			for (size_t n = 0, N = mesher.polygonPoolListSize(); n < N; ++n) {

				const openvdb::tools::PolygonPool& polygons = polygonPoolList[n];

				// Copy quads
				for (size_t i = 0, I = polygons.numQuads(); i < I; ++i) {
					primitives.push_back(polygons.quad(i));
				}

				// Copy triangles (adaptive mesh)
				if (polygons.numTriangles() != 0) {
					openvdb::Vec4I quad;
					quad[3] = openvdb::util::INVALID_IDX;
					for (size_t i = 0, I = polygons.numTriangles(); i < I; ++i) {
						const openvdb::Vec3I& triangle = polygons.triangle(i);
						quad[0] = triangle[0];
						quad[1] = triangle[1];
						quad[2] = triangle[2];
						primitives.push_back(quad);
					}
				}
			}

			openvdb::tools::MeshToVolume<openvdb::FloatGrid> vol(transform);
			vol.convertToLevelSet(points, primitives, 1.0, 1.0);
			m_primaryGrid.m_grid =vol.distGridPtr ();
            m_primaryGrid.m_grid->setGridClass (  openvdb::v2_3_0::GRID_LEVEL_SET );
		};

		m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
			m_primaryGrid.m_lastEvalTime = clock();
            Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: Stamped at=" + CString ( (LONG)m_primaryGrid.m_lastEvalTime));
			Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: Done in=" + CString (timer.GetElapsedTime ()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: " + CString (e.what()), siErrorMsg);
		};
	};


};

SICALLBACK dlexport VDB_ChangeGridClass_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

				CDataArrayCustomType outData( in_ctxt );     

			// get cache object
			VDB_ChangeGridClass_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][CHANGEGRIDCLASS]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_ChangeGridClass_cache_t*)(CValue::siPtrType)userData;
				p_cacheNodeObject->PackGrid ( outData );

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);

			// check for prim grid
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL || !p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Empty grid on input" );
				return CStatus::OK;
			};

			// only float grids
			if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>()==false )
			{
				Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Invalid grid data type! Float only allowed!", siWarningMsg );
				return CStatus::OK;
			}
			
			CDataArrayBool inOrder ( in_ctxt, ID_IN_Revert );
		            
		

			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper, inOrder[0] )==false )
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

SICALLBACK dlexport VDB_ChangeGridClass_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_ChangeGridClass_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_ChangeGridClass_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK dlexport VDB_ChangeGridClass_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][CHANGEGRIDCLASS]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_ChangeGridClass_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_ChangeGridClass_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
