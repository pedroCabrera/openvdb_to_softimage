#include "Main.h"
#include "vdbHelpers.h"

#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/util/Util.h>

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_Geometry ,
	ID_IN_Spacing ,
	ID_IN_InVoxels,
	ID_IN_DFType ,

	//ID_IN_FillInterior = 4,
	ID_IN_ExteriorWidth ,
	ID_IN_InteriorWidth ,
	//ID_IN_BGValue = 8,

	ID_IN_GridName ,
	ID_IN_ConvertToFog,

	ID_G_100 = 100,

	ID_OUT_VDBGrid = 200,


	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};


CStatus VDB_MeshToGrid_Register(PluginRegistrar& reg)
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_MeshToGrid", L"VDB Mesh To Grid");

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

	st = nodeDef.AddInputPort(ID_IN_Geometry, ID_G_100, siICENodeDataGeometry,	siICENodeStructureSingle, siICENodeContextSingleton,L"Geometry", L"geometry");st.AssertSucceeded();
	//st = nodeDef.AddInputPort(ID_IN_FillInterior, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"Fill interior", L"fillInt", CValue(false));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Spacing, ID_G_100, siICENodeDataFloat,siICENodeStructureSingle, siICENodeContextSingleton,L"Voxel Size", L"voxelSize", CValue(0.1));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_ExteriorWidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"Exterior Width", L"extWidth", CValue(2.0));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_InteriorWidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"Interior Width", L"intWidth", CValue(2.0));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_InVoxels, ID_G_100, siICENodeDataBool,	siICENodeStructureSingle, siICENodeContextSingleton,L"BandWidthInVoxels", L"BandWidthInVoxels", CValue(true));st.AssertSucceeded();

	st = nodeDef.AddInputPort(ID_IN_GridName, ID_G_100, siICENodeDataString,	siICENodeStructureSingle, siICENodeContextSingleton,L"Grid Name", L"gridName", L"");	st.AssertSucceeded();
	//st = nodeDef.AddInputPort(ID_IN_BGValue, ID_G_100, siICENodeDataFloat,siICENodeStructureSingle, siICENodeContextSingleton,L"Background Value", L"BGValue", CValue(0.0));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_DFType, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"UnsignedDistanceField", L"UnsignedDistanceField", CValue(false));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_ConvertToFog, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"ConvertToFogvolume", L"ConvertToFogvolume", CValue(true));st.AssertSucceeded();
	
	// Add custom type names.
	CStringArray customTypes(1);
	customTypes[0] = VDB_DATA_NAME;

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");st.AssertSucceeded();

	PluginItem nodeItem = reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}



struct VDB_Grid2Mesh_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_Grid2Mesh_cache_t ( ) : m_extW(0) , m_intW (0), m_voxelSize (-1.f)/*, m_BGVal (0.f)*/ { };

	CDoubleArray m_vertices;
	CLongArray	 m_indices;
	MATH::CMatrix4f m_srt;

	float m_extW;
	float m_intW;
	//bool  m_fillInt;
	float m_voxelSize;
	LONG  m_gridType;
	CString m_gridName;
	//float m_BGVal;
	bool m_DFType; // 0=SDF  1=UDF
	bool m_toFog;

	inline bool IsDirty ( CDoubleArray& v, CLongArray& i, MATH::CMatrix4f& srt, 
		float extr, float intr,/* bool fill,*/ float vxsz, LONG gType, CString& name,/*float  in_BGVal,*/ bool in_DFType, bool in_toFog)
	{
		bool isClean = ( v==m_vertices && i==m_indices && srt==m_srt 
			&& extr==m_extW && intr==m_intW /*&& fill==m_fillInt*/ && vxsz==m_voxelSize && gType==m_gridType &&
			name== m_gridName /*&& m_BGVal == in_BGVal*/ && m_DFType == in_DFType && m_toFog == in_toFog);

		if ( isClean == false )
		{
			m_vertices = v;
			m_indices = i;
			 m_srt = srt;

			m_extW = extr;
			 m_intW = intr;
			// m_fillInt = fill;
			 m_voxelSize = vxsz;
			 m_gridType = gType;
			 m_gridName = name;

			 //m_BGVal = in_BGVal;
			 m_DFType = in_DFType;
			 m_toFog = in_toFog;
		};

		return !isClean;
		
	};

	inline void Convert ( )
	{
		TIMER timer;
		ULONG nbPoints = m_vertices.GetCount()/3;
		ULONG nbTriangles = m_indices.GetCount()/3;
		ULONG nbIndices = m_indices.GetCount();

		std::vector<openvdb::Vec3s> pointList;
		pointList.reserve(nbPoints);

		std::vector<openvdb::Vec4I> polygonList;
		polygonList.reserve(nbTriangles);

		openvdb::math::Transform::Ptr transf = openvdb::math::Transform::createLinearTransform(m_voxelSize);
		for (LONG i=0; i<m_vertices.GetCount(); i+=3)
		{

			MATH::CVector3f tmp (m_vertices[i], m_vertices[i+1], m_vertices[i+2]);
			tmp.MulByMatrix4InPlace ( m_srt );

			openvdb::Vec3s pnt(tmp.GetX(),tmp.GetY(),tmp.GetZ());
			pointList.push_back(transf->worldToIndex(pnt));
		}

		for (LONG i=0; i<nbIndices; i+=3)
		{
			// look into PolygonPool for meshes with quads and triangles
			openvdb::Vec4I tri(m_indices[i], m_indices[i+1], m_indices[i+2], openvdb::util::INVALID_IDX);
			polygonList.push_back(tri);
		}


		openvdb::tools::MeshToVolume<openvdb::FloatGrid> converter(transf);

		openvdb::FloatGrid::Ptr tempPtr;

		try {

		// check distance field type
		if ( m_DFType==false ) //  signed level set ( SDF with given ext\int width )
		{
			converter.convertToLevelSet(pointList, polygonList, m_extW, m_intW);
			tempPtr = converter.distGridPtr();  
		}
		else  // unsignedlevel set ( unsigned distance to mesh )
		{
			converter.convertToUnsignedDistanceField(pointList, polygonList,m_extW);
			tempPtr = converter.distGridPtr(); 				
		};

		// check if we nedd to convert it to fogvolume
		if ( m_toFog )
		{
			openvdb::tools::sdfToFogVolume(*tempPtr);
			tempPtr->setGridClass ( openvdb::v2_1_0::GridClass::GRID_FOG_VOLUME );
		}
		else
		{
			tempPtr->setGridClass ( openvdb::v2_1_0::GridClass::GRID_LEVEL_SET );
		};


		// attach new grid
		m_primaryGrid.m_grid = tempPtr;

		// set name
		if (m_gridName.IsEmpty()==false) 
			m_primaryGrid.m_grid->setName(m_gridName.GetAsciiString());

		// mark time of creation for dirty-stating
		m_primaryGrid.m_lastEvalTime = clock();
		Application().LogMessage(L"[VDB][MESHTOGRID]: Stamped at=" + CString(m_primaryGrid.m_lastEvalTime));
		Application().LogMessage(L"[VDB][MESHTOGRID]: Done in=" + CString(timer.GetElapsedTime()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][MESHTOGRID]:" + CString(e.what()), siErrorMsg);
		};
	};
};


SICALLBACK VDB_MeshToGrid_Evaluate(ICENodeContext& in_ctxt)
{
	

	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_VDBGrid:
		{
			CDataArrayCustomType outData(in_ctxt);

			// get cache object
			VDB_Grid2Mesh_cache_t * p_gridOwner = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][MESHTOGRID]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};

			// get cache
			p_gridOwner = (VDB_Grid2Mesh_cache_t*)(CValue::siPtrType)userData;

			p_gridOwner->PackGrid ( outData );

			CICEGeometry inGeo(in_ctxt, ID_IN_Geometry);

			 // check for poly on input
			bool isGroup = false;
			if ( inGeo.GetGeometryType() != CICEGeometry::siMeshSurfaceType )
			{
				if ( inGeo.GetGeometryType() == CICEGeometry::siGroupType )
				{
					for ( ULONG i = 0; i< inGeo.GetSubGeometryCount(); i++ )
					{
						if ( inGeo.GetSubGeometry(i).GetGeometryType() != CICEGeometry::siMeshSurfaceType ) // perform out if encountered at least one non-poly type
						{
							//Application().LogMessage ( L"[VDB][MESHTOGRID]: Non-poly type encountered, bypassed", siWarningMsg );
							return CStatus::OK;
						}
					}
					isGroup = true;
				}
				else
				{
					//Application().LogMessage ( L"[VDB][MESHTOGRID]: Non-poly type encountered, bypassed", siWarningMsg );
					return CStatus::OK;
				};
			};



			// check if we really need to recalc or can jus use prev result
			// ###########################################################
			// buff inputs
			CDataArrayFloat inVoxelSize( in_ctxt, ID_IN_Spacing);
			CDataArrayFloat extWidth( in_ctxt, ID_IN_ExteriorWidth);
			CDataArrayFloat intWidth( in_ctxt, ID_IN_InteriorWidth);
			CDataArrayString gridName( in_ctxt, ID_IN_GridName);
			//CDataArrayBool fillInt ( in_ctxt, ID_IN_FillInterior );
			//CDataArrayFloat inBGVal( in_ctxt, ID_IN_BGValue);
			CDataArrayBool inDFType ( in_ctxt, ID_IN_DFType );
			CDataArrayBool inToFog ( in_ctxt, ID_IN_ConvertToFog );
			CDataArrayBool inBSInVoxels( in_ctxt, ID_IN_InVoxels); 

			// get triangulated mesh
	
			CDoubleArray points;
			inGeo.GetPointPositions(points);

			CLongArray triangles;
			inGeo.GetTrianglePointIndices(triangles);

			MATH::CMatrix4f srt;
			inGeo.GetTransformation (srt);

			float voxelSizeSafe = Clamp(0.0001f,FLT_MAX,inVoxelSize[0]);

			// compare with prev res
			if  (	p_gridOwner->IsDirty (points, triangles, srt,
				inBSInVoxels[0]?extWidth[0]*voxelSizeSafe:extWidth[0],
				inBSInVoxels[0]?intWidth[0]*voxelSizeSafe:intWidth[0],
				/*fillInt[0],*/ voxelSizeSafe, 0, gridName[0], /*inBGVal[0], */inDFType[0], inToFog[0] ) == false )
			{
				Application().LogMessage( L"[VDB][MESHTOGRID]: Data is not changed, used prev result" );
				return CStatus::OK;
			}

			// convert
			p_gridOwner->Convert ();



		}break;
	default:
		break;
	};

	return CStatus::OK;
};

// lets cache this
SICALLBACK VDB_MeshToGrid_Init( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Grid2Mesh_cache_t * p_gridOwner;
   if (userData.IsEmpty())
   {
      p_gridOwner = new VDB_Grid2Mesh_cache_t;
   }
   else
   {
	   Application().LogMessage ( L"[VDB][GRIDTOMESH]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }



    Application().LogMessage ( L"[VDB][GRIDTOMESH]: UnsigendDistanceField = when enabled, produces densification of mesh" ); 
	 Application().LogMessage ( L"where each voxel in band with width specified by the exteriorBandWidth param contain distance to closes point on mesh. " );
	  Application().LogMessage ( L"When false, produces signed distance with exterion and interior widthes defined by inputs respectivelty. Requires closed mesh for correct result. " );
	    Application().LogMessage ( L" ConvertToFogvolume = when enabled, converts DistanceField (only correct for the Signed) to filled volume which interior is 1 and exterior has a falloff with width of exterior width." );
   ctxt.PutUserData((CValue::siPtrType)p_gridOwner);
   return CStatus::OK;
}



SICALLBACK VDB_MeshToGrid_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][GRIDTOMESH]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_Grid2Mesh_cache_t * p_gridOwner;
   p_gridOwner = (VDB_Grid2Mesh_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_gridOwner;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
