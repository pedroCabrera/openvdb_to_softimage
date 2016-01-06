#include "Main.h"
#include "vdbHelpers.h"
#include <openvdb/openvdb.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <algorithm>

using openvdb::tools::PolygonPool;
using openvdb::tools::PolygonPoolList;

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_VDBGrid ,
	//ID_IN_VDBGridMask ,
	//ID_IN_VDBGridAdaptivity,
	ID_IN_NbParts,
	ID_IN_ActPartID,


	ID_IN_IsoValue ,
	ID_IN_Adaptivity,
	ID_IN_Partition ,
	ID_IN_InvertPoly ,

	ID_G_100 = 100,

	ID_OUT_PPos = 200,
	ID_OUT_Desc = 201,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};






CStatus VDB_GridToMesh_Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_GridToMesh", L"VDB Grid To Mesh");

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
   
   // stupid default arguments wont work have to add ULONG_MAX
   st = nodeDef.AddInputPort(ID_IN_VDBGrid, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"VDBGrid", L"VDBGrid",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   
   st = nodeDef.AddInputPort(ID_IN_IsoValue, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"IsoValue", L"IsoVal", 0.5); st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_Adaptivity, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Adaptivity", L"Adaptivity", 0.0); st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_InvertPoly, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"InvertPolygons", L"InvertPolygons", false); st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_NbParts, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"PartitionsCount", L"PartitionsNb", 1); st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_ActPartID, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"ActivePartitionID", L"ActivePartitionID", 0); st.AssertSucceeded();
  

   // Add output ports.
   st = nodeDef.AddOutputPort(ID_OUT_PPos, siICENodeDataVector3, siICENodeStructureArray, siICENodeContextSingleton,L"PointPosition", L"PointPosition");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_Desc, siICENodeDataLong, siICENodeStructureArray, siICENodeContextSingleton,L"PolygonalDescription", L"PolygonalDesc");  st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

   return CStatus::OK;
}



struct VDB_GridToMesh_cache_t : public VDB_ICENode_cacheBase_t
{

	clock_t m_inputTime;

	float m_iso;
	float m_adapt;
	bool m_invertPoly;
	int m_nbParts;
	int m_actPartID;

		std::vector <LONG> m_indices;
	std::vector <MATH::CVector3f> m_positions;

	inline bool IsDirty ( float iso, float adapt, bool invert ,clock_t & in_evalTime, int nbparts, int actpart )
	{
		bool isClean = ( m_inputTime==in_evalTime && m_iso==iso && m_adapt==adapt &&m_invertPoly==invert &&
			m_nbParts== nbparts &&  m_actPartID==actpart);

		if ( isClean == false )
		{	
			m_inputTime=in_evalTime ; 
			m_iso=iso ; 
			m_adapt=adapt;
			m_invertPoly = invert;
			m_nbParts= nbparts;
			m_actPartID=actpart;
		};

		return !isClean;
	};

	inline void Remesh  ( VDB_ICEFlowDataWrapper_t * p_inWrapper )
	{
		TIMER timer;

		// recalc and store
		// Setup level set mesher
		openvdb::tools::VolumeToMesh mesher(m_iso, Clamp ( 0.f, FLT_MAX, m_adapt ));
		mesher.partition ( m_nbParts, m_actPartID );

		// level set is only correct grid for mesh
		if (p_inWrapper->m_grid->getGridClass() != openvdb::GRID_LEVEL_SET) 		
			Application().LogMessage ( L"[VDB][MESHER]: Grid is not level set, possible incorrect result, use positive isoValue if so" );

		// check for float
		if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>() == false) 
		{
			Application().LogMessage(L"[VDB][MESHER]: Non-float grid on input, bypassed", siWarningMsg );
			m_indices.clear ();
			m_positions.clear ();
			return;
		}

		openvdb::FloatGrid::Ptr levelSetGrid;
		levelSetGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
		mesher(  *(levelSetGrid)  );

		//########################################################################
		// get positions
		const openvdb::tools::PointList& vdbPointsList = mesher.pointList();
		LLONG nbPoints = mesher.pointListSize();
		m_positions.resize(nbPoints);
		for (LLONG it=0; it<nbPoints; ++it)
			m_positions[it].Set ( vdbPointsList[it].x(), vdbPointsList[it].y(), vdbPointsList[it].z() );

		//########################################################################
		// get polygonal description
		LLONG nbPolyPools = mesher.polygonPoolListSize();
		const PolygonPoolList& poolsList = mesher.polygonPoolList();
		m_indices.clear ();
		for (LLONG poolIt=0; poolIt<nbPolyPools; ++poolIt)
		{
			const PolygonPool& currentPool = poolsList[poolIt];
			LLONG nbTriangles = currentPool.numTriangles ();
			LLONG nbQuads = currentPool.numQuads ();

			for ( LLONG triIt = 0; triIt < nbTriangles; ++triIt )
			{
                openvdb::Vec3I currTri = currentPool.triangle(triIt);
				m_indices.push_back ( currTri.x() );
				m_indices.push_back ( currTri.y() );
				m_indices.push_back ( currTri.z() );
				m_indices.push_back ( -2 );

			}

			for ( LLONG quadIt = 0; quadIt < nbQuads; ++quadIt )
			{
                openvdb::Vec4I currQuad = currentPool.quad(quadIt);
				m_indices.push_back ( currQuad.x() );
				m_indices.push_back ( currQuad.y() );
				m_indices.push_back ( currQuad.z() );
				m_indices.push_back ( currQuad.w() );
				m_indices.push_back ( -2 );

			}
		};

		// invert if enabled
		if ( m_invertPoly )
		{
			m_indices.pop_back (); // remove last -2
			std::reverse(m_indices.begin(), m_indices.end());
			m_indices.push_back ( -2 );

		};

        Application().LogMessage(L"[VDB][MESHER]: Stamped by=" + CString((LONG)m_inputTime) );
		Application().LogMessage(L"[VDB][MESHER]: Done in=" + CString(timer.GetElapsedTime()) );
	};

	inline void PackPositions ( CDataArray2DVector3f & outData )
	{
		if ( m_positions.size()==0 )
		{
			outData.Resize (0,0);
			return;
		};
		CDataArray2DVector3f::Accessor outAcc = outData.Resize ( 0,m_positions.size() );
		memcpy ( &(outAcc[0]), &(m_positions[0]), m_positions.size()*sizeof(MATH::CVector3f) );
	};

	inline void PackDescription ( CDataArray2DLong & outData )
	{
		if ( m_indices.size() ==0 )
			return ;
		CDataArray2DLong::Accessor outAcc = outData.Resize ( 0,m_indices.size() );
		memcpy ( &(outAcc[0]), &(m_indices[0]), m_indices.size()*sizeof(LONG) );
	};


};




SICALLBACK dlexport VDB_GridToMesh_Evaluate( ICENodeContext & in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_PPos:
		{
			CDataArray2DVector3f outData(in_ctxt);
		
					// get cache object
			VDB_GridToMesh_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][MESHER]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_GridToMesh_cache_t*)(CValue::siPtrType)userData;

			
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);

			// check for prim grid
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][MESHER]: Empty grid on input" );
				outData.Resize (0,0);
				return CStatus::OK;
			};

		

			// buff inputs
			CDataArrayFloat iso(in_ctxt, ID_IN_IsoValue);
			CDataArrayFloat adaptivity(in_ctxt, ID_IN_Adaptivity);
			CDataArrayBool invertPoly(in_ctxt, ID_IN_InvertPoly);
			CDataArrayLong inActPartID ( in_ctxt, ID_IN_ActPartID );
			CDataArrayLong inNbParts ( in_ctxt, ID_IN_NbParts );

			// check for dirty state
			if ( p_cacheNodeObject->IsDirty ( iso[0], adaptivity[0], invertPoly[0],  p_inWrapper->m_lastEvalTime, 
				Clamp(1L,LONG_MAX, inNbParts[0]), Clamp(0L,Clamp(0L,LONG_MAX, inNbParts[0])-1,inActPartID[0]) )==false )
			{
				Application().LogMessage ( L"[VDB][MESHER]: Data is not changed, used old positions" );		
				p_cacheNodeObject->PackPositions ( outData );
				return CStatus::OK;
			};

			p_cacheNodeObject->Remesh ( p_inWrapper );
			p_cacheNodeObject->PackPositions ( outData );
		
		}break;


	case ID_OUT_Desc:
		{
			CDataArray2DLong outData(in_ctxt);
					
	
		// get cache object
			VDB_GridToMesh_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][MESHER]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_GridToMesh_cache_t*)(CValue::siPtrType)userData;

			
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);

			// check for prim grid
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][MESHER]: Empty grid on input" );
				outData.Resize (0,0);
				return CStatus::OK;
			};

			// buff inputs
			CDataArrayFloat iso(in_ctxt, ID_IN_IsoValue);
			CDataArrayFloat adaptivity(in_ctxt, ID_IN_Adaptivity);
			CDataArrayBool invertPoly(in_ctxt, ID_IN_InvertPoly);
			CDataArrayLong inActPartID ( in_ctxt, ID_IN_ActPartID );
			CDataArrayLong inNbParts ( in_ctxt, ID_IN_NbParts );

			// check for dirty state
			if ( p_cacheNodeObject->IsDirty ( iso[0], adaptivity[0], invertPoly[0],  p_inWrapper->m_lastEvalTime, 
				Clamp(1L,LONG_MAX, inNbParts[0]), Clamp(0L,Clamp(0L,LONG_MAX, inNbParts[0])-1,inActPartID[0]) )==false )
			{
				Application().LogMessage ( L"[VDB][MESHER]: Data is not changed, used old indices" );
				p_cacheNodeObject->PackDescription ( outData );
				return CStatus::OK;
			};

			p_cacheNodeObject->Remesh ( p_inWrapper );
			p_cacheNodeObject->PackDescription ( outData );

		} break;
	default:
		break;

		return CStatus::OK;
	};

return CStatus::OK;
}




SICALLBACK dlexport VDB_GridToMesh_Init( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_GridToMesh_cache_t * p_vdbMeshObj;
   if (userData.IsEmpty())
   {
      p_vdbMeshObj = new VDB_GridToMesh_cache_t;
   }
   else
   {
	   Application().LogMessage ( L"[VDB][MESHER]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbMeshObj);
   return CStatus::OK;
}



SICALLBACK dlexport VDB_GridToMesh_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][MESHER]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_GridToMesh_cache_t * p_vdbMeshObj; 
   p_vdbMeshObj = (VDB_GridToMesh_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbMeshObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}


