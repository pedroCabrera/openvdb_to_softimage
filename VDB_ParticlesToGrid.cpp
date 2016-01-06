#include <openvdb/Grid.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/tools/ParticlesToLevelSet.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/util/Util.h>
  
#include "Main.h"
#include "vdbHelpers.h"
  
// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
      
    ID_IN_Spacing,
    ID_IN_GridName ,
  
	ID_IN_ConvertToFog, 


	ID_IN_Bandwidth,
	ID_IN_InVoxels,

	ID_IN_OutputMask,
	ID_IN_MaskWidth,

    ID_IN_SpherePos,
    ID_IN_SphereRad,
    ID_IN_SphereVel,

	ID_IN_AttrScalar,
	ID_IN_AttrVec3,
	ID_IN_AttrLong,

	ID_IN_RasterVel,
	ID_IN_VelSpacing,
  
    ID_G_100 = 100,
  
    ID_OUT_VDBGridPrimary = 200,
	 ID_OUT_VDBGridMask = 201,
  ID_OUT_VDBGridAttrScalar = 202,
   ID_OUT_VDBGridAttrVec3 = 203,
    ID_OUT_VDBGridAttrLong = 204,

    ID_TYPE_CNS = 400,
    ARR_CNS = 401,
    CTXT_CNS = 402,
  
    ID_UNDEF = ULONG_MAX
};
  
  
CStatus VDB_ParticlesToGrid_Register(PluginRegistrar& reg)
{
    ICENodeDef nodeDef;
    Factory factory = Application().GetFactory();
    nodeDef = factory.CreateICENodeDef(L"VDB_ParticlesToGrid", L"VDB Particles To Grid");
  
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

	st = nodeDef.AddInputPort(ID_IN_GridName, ID_G_100, siICENodeDataString,	siICENodeStructureSingle, siICENodeContextSingleton,L"Grid Name", L"gridName", L"");	st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Spacing, ID_G_100, siICENodeDataFloat,siICENodeStructureSingle, siICENodeContextSingleton,L"Voxel Size", L"voxelSize", CValue(0.1));st.AssertSucceeded();

	st = nodeDef.AddInputPort(ID_IN_Bandwidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"BandWidth", L"BandWidth", CValue(2.0));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_InVoxels, ID_G_100, siICENodeDataBool,	siICENodeStructureSingle, siICENodeContextSingleton,L"BandWidthInVoxels", L"BandWidthInVoxels", CValue(true));st.AssertSucceeded();

st = nodeDef.AddInputPort(ID_IN_ConvertToFog, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"ConvertToFogvolume", L"ConvertToFogvolume", CValue(true));st.AssertSucceeded();

	//#################3
	st = nodeDef.AddInputPort(ID_IN_SpherePos, ID_G_100, siICENodeDataVector3,  siICENodeStructureSingle, siICENodeContextAny,L"PointPosition", L"PointPosition", CValue(), ID_UNDEF, ARR_CNS, CTXT_CNS) ;st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_SphereRad, ID_G_100, siICENodeDataFloat,    siICENodeStructureSingle, siICENodeContextAny,L"PointRadius", L"PointRadius", CValue(), ID_UNDEF, ARR_CNS, CTXT_CNS) ;st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_SphereVel, ID_G_100, siICENodeDataVector3,  siICENodeStructureSingle, siICENodeContextAny,L"PointVelocity", L"PointVelocity", CValue(), ID_UNDEF, ARR_CNS, CTXT_CNS) ;st.AssertSucceeded();

	//st = nodeDef.AddInputPort(ID_IN_AttrScalar, ID_G_100, siICENodeDataFloat,    siICENodeStructureSingle, siICENodeContextAny,L"AttributeScalar", L"AttributeScalar", CValue(), ID_UNDEF, ARR_CNS, CTXT_CNS) ;st.AssertSucceeded();
	//st = nodeDef.AddInputPort(ID_IN_AttrVec3, ID_G_100, siICENodeDataVector3,  siICENodeStructureSingle, siICENodeContextAny,L"AttributeVec3", L"AttributeVec3", CValue(), ID_UNDEF, ARR_CNS, CTXT_CNS) ;st.AssertSucceeded();
	//st = nodeDef.AddInputPort(ID_IN_AttrLong, ID_G_100, siICENodeDataLong,  siICENodeStructureSingle, siICENodeContextAny,L"AttributeLong", L"AttributeLong", CValue(), ID_UNDEF, ARR_CNS, CTXT_CNS) ;st.AssertSucceeded();
	//#################3
	st = nodeDef.AddInputPort(ID_IN_RasterVel, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"RasterizeVelocity", L"RasterizeVelocity", CValue(false));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_VelSpacing, ID_G_100, siICENodeDataFloat,    siICENodeStructureSingle, siICENodeContextSingleton,L"VelocitySpacing", L"VelocitySpacing", CValue(2.f)) ;st.AssertSucceeded();
	


	//st = nodeDef.AddInputPort(ID_IN_OutputMask, ID_G_100, siICENodeDataBool,siICENodeStructureSingle, siICENodeContextSingleton,L"OutputMask", L"OutputMask", CValue(false));st.AssertSucceeded();
	//st = nodeDef.AddInputPort(ID_IN_MaskWidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"MaskWidth", L"MaskWidth", CValue(2.0));st.AssertSucceeded();

  
    // Add custom type names.
    CStringArray customTypes(1);
    customTypes[0] = VDB_DATA_NAME;
  
    st = nodeDef.AddOutputPort( ID_OUT_VDBGridPrimary, customTypes, siICENodeStructureSingle,  siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");st.AssertSucceeded();
 // st = nodeDef.AddOutputPort( ID_OUT_VDBGridMask, customTypes, siICENodeStructureSingle,  siICENodeContextSingleton, L"VDB Alpha Mask", L"outVDBMask");st.AssertSucceeded();
  // st = nodeDef.AddOutputPort( ID_OUT_VDBGridAttrScalar, customTypes, siICENodeStructureSingle,  siICENodeContextSingleton, L"VDB Scalar Attr", L"outVDBAttrScalar");st.AssertSucceeded();
   //st = nodeDef.AddOutputPort( ID_OUT_VDBGridAttrVec3, customTypes, siICENodeStructureSingle,  siICENodeContextSingleton, L"VDB Vec3 Attr", L"outVDBAttrVec3f");st.AssertSucceeded();
 // st = nodeDef.AddOutputPort( ID_OUT_VDBGridAttrLong, customTypes, siICENodeStructureSingle,  siICENodeContextSingleton, L"VDB Long Attr", L"outVDBAttrLong");st.AssertSucceeded();
  

    PluginItem nodeItem = reg.RegisterICENode(nodeDef);
    nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);
  
    return CStatus::OK;
}
  
typedef double Real;
typedef openvdb::math::Vec3<Real> Vec3R;

class ParticleList
{

public:
	// Return the total number of particles in list.
	// Always required!
	inline size_t size() const
	{   
		return m_ppos.size();   
	};

	// Get the world space position of n'th particle.
	// Required by ParticledToLevelSet::rasterizeSphere(*this,radius).
	inline void getPos(size_t n, Vec3R & xyz) const
	{
		xyz.x() = m_ppos[n].GetX();
		xyz.y() = m_ppos[n].GetY();
		xyz.z() = m_ppos[n].GetZ();
	};

	// Get the world space position and radius of n'th particle.
	// Required by ParticledToLevelSet::rasterizeSphere(*this).
	inline void getPosRad(size_t n, Vec3R& xyz, Real & rad) const
	{
		xyz.x() = m_ppos[n].GetX();
		xyz.y() = m_ppos[n].GetY();
		xyz.z() = m_ppos[n].GetZ();
		rad = m_radiuses[n];
	};

	// Get the world space position, radius and velocity of n'th particle.
	// Required by ParticledToLevelSet::rasterizeSphere(*this,radius).
	inline void getPosRadVel(size_t n, Vec3R& xyz, Real& rad, Vec3R& vel) const
	{
		xyz.x() = m_ppos[n].GetX();
		xyz.y() = m_ppos[n].GetY();
		xyz.z() = m_ppos[n].GetZ();
		rad = m_radiuses[n];
		vel.x() = m_vels[n].GetX();
		vel.y() = m_vels[n].GetY();
		vel.z() = m_vels[n].GetZ();
	};

	inline bool IsEqual ( std::vector <MATH::CVector3f>&pos, std::vector <MATH::CVector3f>&vels, std::vector <float>&rad )
	{
		bool retVal = false;
		if (  m_ppos.size () == pos.size () && m_vels.size() == vels.size() && m_radiuses.size () == rad.size () &&
			memcmp ( &(m_ppos[0]),&(pos[0]), sizeof(MATH::CVector3f)*m_ppos.size () ) == 0 &&
			memcmp ( &(m_vels[0]),&(vels[0]), sizeof(MATH::CVector3f)*m_vels.size () ) == 0 &&
			memcmp ( &(m_radiuses[0]),&(rad[0]), sizeof(float)*m_radiuses.size () ) == 0 )
			retVal = true;

		return retVal;
	};

	inline void Swap (  std::vector <MATH::CVector3f>&pos, std::vector <MATH::CVector3f>&vels, std::vector <float>&rad ) 
	{
		m_ppos.swap ( pos );
		m_vels.swap ( vels );
		m_radiuses.swap ( rad );
	};

	// Get the attribute of the n'th particle. AttributeType is user-defined!
	// Only required is attribute transfer is enabled in ParticledToLevelSet.
	//void getAtt(AttributeType& att) const;

	// cached data
	std::vector <MATH::CVector3f> m_ppos;
	std::vector <MATH::CVector3f> m_vels;
	std::vector <float>               m_radiuses;

};

  
struct VDB_Particles2Mesh_cache_t
{
    VDB_Particles2Mesh_cache_t ( ) :  m_voxelSize (-1.f) { };
	VDB_ICEFlowDataWrapper_t m_primaryGrid;
	//VDB_ICEFlowDataWrapper_t m_mask;
	//VDB_ICEFlowDataWrapper_t m_attrScalar;
	//VDB_ICEFlowDataWrapper_t m_attrVec3;
	//VDB_ICEFlowDataWrapper_t m_attrLong;

	// particles data
    ParticleList m_particlesList;
  
  
	// controls
    float m_voxelSize;
    CString m_gridName;

	bool m_useFog;
	float m_bandwidth;
//	float m_maskWidth;
//	bool m_useMask;
	bool m_rasterVel;
	float m_velSpacing;




	std::vector<float> m_scalarAttr;
	std::vector<MATH::CVector3f> m_vec3Attr;
	std::vector<LONG> m_longAttr;


  
    inline bool IsDirty (std::vector <MATH::CVector3f>&pos, std::vector <MATH::CVector3f>&vels, std::vector <float>&rad,
         float vxsz, const CString& name,  bool fog, float bw,
	//	 float mw, bool useMask,
		 bool rastvel, float velsp )
		// CDataArrayFloat  & ascalar, CDataArrayVector3f & avec3, CDataArrayLong & along)
    {
        bool isClean = ( m_particlesList.IsEqual (pos,vels,rad  )
			&& vxsz==m_voxelSize && name== m_gridName  && m_useFog==fog && m_bandwidth==bw 
			//&& m_maskWidth==mw			&& m_useMask==useMask 
			&& m_rasterVel==rastvel && m_velSpacing==velsp );
		
  
        if ( isClean == false )
        {
            m_particlesList.Swap ( pos, vels, rad );

			m_voxelSize = vxsz;
			m_gridName = name;  

			m_useFog=fog ;
			m_bandwidth=bw ;
		//	m_maskWidth=mw;
		//	m_useMask=useMask ;
			m_rasterVel=rastvel ;
			m_velSpacing=velsp ;



        };
  
        return !isClean;    
    };

	// build grids
	inline void Rebuild ( )
	{
		TIMER timer;

		openvdb::math::Transform::Ptr transf = openvdb::math::Transform::createLinearTransform(m_voxelSize);
		const float backGroundVal = m_bandwidth;


		openvdb::FloatGrid::Ptr rasterizedGrid =openvdb::FloatGrid::create(backGroundVal);
		rasterizedGrid->setGridClass(openvdb::GRID_LEVEL_SET);
		rasterizedGrid->setTransform(transf);

		try {

		openvdb::tools::ParticlesToLevelSet<openvdb::FloatGrid>  raster(*rasterizedGrid);


		raster.setRmin ( 0.f );
		raster.setRmax ( 1e15f );



		// rasterize
		if ( m_rasterVel )				
			raster.rasterizeTrails (  m_particlesList, m_velSpacing );				
		else				
			raster.rasterizeSpheres ( m_particlesList );

		raster.finalize();




			// check if we nedd to convert it to fogvolume
		if ( m_useFog )
			{
			 openvdb::tools::sdfToFogVolume(*rasterizedGrid);
                rasterizedGrid->setGridClass ( openvdb::v2_3_0::GRID_FOG_VOLUME );
			};

		


		m_primaryGrid.m_grid = rasterizedGrid;
		m_primaryGrid.m_grid->setName(m_gridName.GetAsciiString());
		m_primaryGrid.m_lastEvalTime = clock ();
        Application().LogMessage(L"[VDB][PARTICLESTOGRID]: Stamped at=" + CString((LONG)m_primaryGrid.m_lastEvalTime ) );
		Application().LogMessage(L"[VDB][PARTICLESTOGRID]: Done in=" + CString(timer.GetElapsedTime()) );

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][PARTICLESTOGRID]: " + CString(e.what()), siErrorMsg );
		};
	};



	// prim grid
	inline void PackPrimaryGrid ( CDataArrayCustomType & outData )
	{
		VDB_ICEFlowDataWrapper_t * p_outRef = &(m_primaryGrid);
		void * pOut = outData.Resize(0, sizeof(p_outRef));  
		memcpy ( pOut, &(p_outRef), sizeof(p_outRef));
		return;
	};
};
  
  
SICALLBACK dlexport VDB_ParticlesToGrid_Evaluate(ICENodeContext& in_ctxt)
{
      
  
    // The current output port being evaluated...
    ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();
  
    switch (evaluatedPort)
    {
    case ID_OUT_VDBGridPrimary:
        {
			CDataArrayCustomType outData(in_ctxt);

			// get cache object
			VDB_Particles2Mesh_cache_t * p_gridOwner = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][PARTICLESTOGRID]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};

			// get cache
			p_gridOwner = (VDB_Particles2Mesh_cache_t*)(CValue::siPtrType)userData;

			// pack out primary grid
			p_gridOwner->PackPrimaryGrid ( outData );

			// ###########################################################
			// buff partcl data
			CDataArrayVector3f inPos  ( in_ctxt, ID_IN_SpherePos );
			CDataArrayFloat inRad  ( in_ctxt, ID_IN_SphereRad );
			CDataArrayVector3f inVel  ( in_ctxt, ID_IN_SphereVel );
			
			// containers, will be swapped so no extra memory needed
			std::vector <MATH::CVector3f> ppos;
			std::vector <MATH::CVector3f> vels;
			std::vector <float>           radiuses;
			

			// fill out particle primary data
			LLONG nbParticles = inPos.GetCount ();
			ppos.resize ( nbParticles );
			vels.resize ( nbParticles );
			radiuses.resize ( nbParticles );

			CIndexSet indexSet ( in_ctxt, ID_IN_SpherePos );
			for ( CIndexSet::Iterator it=indexSet.Begin (); it.HasNext(); it.Next ( ) )
			{					
				ppos[it] = inPos[it];
				vels[it] = inVel[it];
				radiuses[it] = Clamp( 0.0001f, 1e15f, inRad[it] );
			};

			// see if we need to hook attributes
			//	CDataArrayFloat inScalarAttr ( in_ctxt, ID_IN_AttrScalar );
			//if ( inScalarAttr.GetCount() > 1 )



			// ###########################################################
			// buff control inputs
			CDataArrayFloat inVoxelSize( in_ctxt, ID_IN_Spacing); 
			CDataArrayString gridName( in_ctxt, ID_IN_GridName);
	
			CDataArrayBool inToFog ( in_ctxt, ID_IN_ConvertToFog );
			CDataArrayFloat inBandwidth( in_ctxt, ID_IN_Bandwidth); 
			CDataArrayBool inBSInVoxels( in_ctxt, ID_IN_InVoxels); 
		//	CDataArrayFloat inMaskWidth ( in_ctxt, ID_IN_MaskWidth );
		//	CDataArrayBool inOutputMaskGrid ( in_ctxt, ID_IN_OutputMask );
			CDataArrayBool inRasterVel ( in_ctxt, ID_IN_RasterVel );
			CDataArrayFloat inVelSpacing ( in_ctxt, ID_IN_VelSpacing );

			// ###########################################################
			// buff attrs	
			//CDataArrayFloat scalarAttrList( in_ctxt, ID_IN_AttrScalar );
			//CDataArrayVector3f vec3AttrList( in_ctxt, ID_IN_AttrScalar );
			//CDataArrayLong longAttrList( in_ctxt, ID_IN_AttrScalar );
					
			float voxelSizeSafe = Clamp(0.0001f,FLT_MAX, inVoxelSize[0]);
  
            // compare it with our cache data
            // and recal if that is dirty
			if  ( p_gridOwner->IsDirty (ppos, vels,radiuses,  voxelSizeSafe, gridName[0],  inToFog[0],
				inBSInVoxels[0]?inBandwidth[0]*voxelSizeSafe:inBandwidth[0],
				inRasterVel[0], Clamp(0.001f,FLT_MAX,inVelSpacing[0]))== false )
            {
                Application().LogMessage( L"[VDB][PARTICLESTOGRID]: Data is not changed, used prev result" );
                return CStatus::OK;
            }
  
  
			// rebuild as we are dirty
			p_gridOwner->Rebuild ( );
  
  
        }break;
    default:
        break;
    };
  
    return CStatus::OK;
};
  
// lets cache this
SICALLBACK dlexport VDB_ParticlesToGrid_Init( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Particles2Mesh_cache_t * p_gridOwner;
   if (userData.IsEmpty())
   {
      p_gridOwner = new VDB_Particles2Mesh_cache_t;
   }
   else
   {
       Application().LogMessage ( L"[VDB][PARTICLESTOMESH]: Fatal, unknow data is present on initialization", siFatalMsg );
        return CStatus::Fail;
   }
  
   ctxt.PutUserData((CValue::siPtrType)p_gridOwner);
   return CStatus::OK;
}
  
  
  
SICALLBACK dlexport VDB_ParticlesToGrid_Term( CRef& in_ctxt )
{
    Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
  
   if ( userData.IsEmpty () )
   {
       Application().LogMessage ( L"[VDB][PARTICLESTOMESH]: Fatal, no data on termination", siFatalMsg );
       return CStatus::Fail;
   }
  
   VDB_Particles2Mesh_cache_t * p_gridOwner;
   p_gridOwner = (VDB_Particles2Mesh_cache_t*)(CValue::siPtrType)userData;
          
  
     delete p_gridOwner;
    ctxt.PutUserData(CValue());
  
        return CStatus::OK;
} 
