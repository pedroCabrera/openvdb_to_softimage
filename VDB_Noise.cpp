#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_userdatablob.h>

#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tree/ValueAccessor.h>

#include "noises.h"

enum IDs
{
	ID_IN_VDBGrid ,

	ID_IN_Seed,
	ID_IN_Scale,
	ID_IN_Amount,
	ID_IN_Time,
	ID_IN_Lacunarity,
	ID_IN_Persistnce,
	ID_IN_NbOctaves,
	ID_IN_NSType,

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_Noise_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_Noise", L"VDB Noise");

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
	  st = nodeDef.AddInputPort(ID_IN_NSType ,ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"Noise Type", L"NoiseType",0,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	 st = nodeDef.AddInputPort(ID_IN_Seed, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"Seed", L"Seed",12345,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();

	st = nodeDef.AddInputPort(ID_IN_Scale, ID_G_100, siICENodeDataVector3, siICENodeStructureSingle, siICENodeContextSingleton, L"Scale", L"Scale",MATH::CVector3f(1,1,1),CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_Amount, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Amount", L"Amount",1.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_Time, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Time", L"Time",0.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_Lacunarity, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Lacunarity", L"Lacunarity",2.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_Persistnce, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Persistence", L"Persistence",0.8f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_NbOctaves, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"NbOctaves", L"NbOctaves",1.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_Noise_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_Noise_cache_t ( ) { };

	clock_t  m_inputEvalTime;

	LONG m_seed;
	MATH::CVector3f m_scale;
	float m_amount;
	float m_time;
	float m_lacun;
	float m_pers;
	int m_nbOcts;
	int m_gridDataType; // 0=float 1=vec3s

	int m_nsType;

	CSimplexNoise m_turbNoiser;
	CWorleyNoise m_cellNoiser;

	bool inline IsDirty ( VDB_ICEFlowDataWrapper_t * in_grid,  LONG seed,MATH::CVector3f scl, float am, float tm, float lac, float pers, int octs, int type, int nsType  )
	{
		bool isClean = ( m_inputEvalTime == (*in_grid).m_lastEvalTime && m_seed == seed && m_scale == scl &&
			m_amount == am && m_time == tm && m_lacun==lac && m_pers == pers && m_nbOcts == octs && m_gridDataType == type && m_nsType == nsType );

		if ( isClean == false )
		{			
			m_inputEvalTime = (*in_grid).m_lastEvalTime;
			m_seed = seed ; 
			m_scale = scl ;
			m_amount = am ;
			m_time = tm ;
			m_lacun=lac ; 
			m_pers = pers ;
			m_nbOcts = octs;
			m_gridDataType = type;
			m_nsType = nsType ;
		};

		return !isClean;

	};

	inline void ApplyNoise ( VDB_ICEFlowDataWrapper_t * in_grid )
	{

		TIMER timer;

		m_turbNoiser.Reseed ( m_seed );
		m_cellNoiser.Reseed ( m_seed );


		//	float noiseAttrs [3];
		//	noiseAttrs[0]=m_nbOcts; 
	 //noiseAttrs[1]=m_lacun; // frequency multiplier betewen octaves
	 //noiseAttrs[2]=m_pers; // should be 0.7-0.9 when multiple octaves used, accumulated influence of each next octave

	
	
		if ( in_grid->m_grid->isType<openvdb::FloatGrid>() )
		{
			openvdb::GridBase::Ptr baseTemp = in_grid->m_grid->deepCopyGrid ();
			openvdb::FloatGrid::Ptr grid =  openvdb::gridPtrCast<openvdb::FloatGrid>(baseTemp);

			if ( m_nsType )
				for ( openvdb::FloatGrid::ValueOnIter iter = grid->beginValueOn();iter; ++ iter  )
				{
                    openvdb::v2_3_0::math::Vec3d vdbVec3 = grid->indexToWorld ( iter.getCoord () );

					iter.setValue ( iter.getValue() + m_turbNoiser.GetSpectralSimplexNoise4D( m_time, 
						vdbVec3.x() * m_scale.GetX(), vdbVec3.y() * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers ) * m_amount );
				}
			else
				for ( openvdb::FloatGrid::ValueOnIter iter = grid->beginValueOn();iter; ++ iter  )
				{
                    openvdb::v2_3_0::math::Vec3d vdbVec3 = grid->indexToWorld ( iter.getCoord () );

					iter.setValue ( iter.getValue() + m_cellNoiser.GetSpectralWorleyNoise4D( m_time, 
						vdbVec3.x() * m_scale.GetX(), vdbVec3.y() * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers ) * m_amount );
				}

				m_primaryGrid .m_grid  = grid;
		}
		else if ( in_grid->m_grid->isType<openvdb::Vec3SGrid>() )
		{
			float dX_simplex = in_grid->m_grid->transform ().voxelSize().x () * 50.f;
			float dX_worley = in_grid->m_grid->transform ().voxelSize().x () / 2.f;
			openvdb::GridBase::Ptr baseTemp = in_grid->m_grid->deepCopyGrid ();
			openvdb::Vec3SGrid::Ptr grid =  openvdb::gridPtrCast<openvdb::Vec3SGrid>(baseTemp);
			if ( m_nsType )
				for ( openvdb::Vec3SGrid::ValueOnIter iter = grid->beginValueOn();iter; ++ iter  )
				{
                    openvdb::v2_3_0::math::Vec3s vdbVec3 = grid->indexToWorld ( iter.getCoord () );
                    openvdb::v2_3_0::math::Vec3s gradient;

					gradient.x () = 	m_turbNoiser.GetSpectralSimplexNoise4D ( m_time, 
						(vdbVec3.x()+dX_simplex) * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers )  - m_turbNoiser.GetSpectralSimplexNoise4D( m_time, 
						(vdbVec3.x()-dX_simplex) * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers );

					gradient.y () = 	m_turbNoiser.GetSpectralSimplexNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , (vdbVec3.y()+dX_simplex) * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers )  -m_turbNoiser.GetSpectralSimplexNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , (vdbVec3.y()-dX_simplex) * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers );

					gradient.z () = 	m_turbNoiser.GetSpectralSimplexNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), (vdbVec3.z()+dX_simplex) * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers )  - m_turbNoiser.GetSpectralSimplexNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), (vdbVec3.z()-dX_simplex) * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers );

					gradient *= m_amount;
					iter.setValue (	iter.getValue () + gradient );
				}
			else
				for ( openvdb::Vec3SGrid::ValueOnIter iter = grid->beginValueOn();iter; ++ iter  )
				{
                    openvdb::v2_3_0::math::Vec3s vdbVec3 = grid->indexToWorld ( iter.getCoord () );
                    openvdb::v2_3_0::math::Vec3s gradient;

					gradient.x () = 	m_cellNoiser.GetSpectralWorleyNoise4D ( m_time, 
						(vdbVec3.x()+dX_worley) * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers )  - m_cellNoiser.GetSpectralWorleyNoise4D( m_time, 
						(vdbVec3.x()-dX_worley) * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers );

					gradient.y () = 	m_cellNoiser.GetSpectralWorleyNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , (vdbVec3.y()+dX_worley) * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers )  -m_cellNoiser.GetSpectralWorleyNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , (vdbVec3.y()-dX_worley) * m_scale.GetY(), vdbVec3.z() * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers );

					gradient.z () = 	m_cellNoiser.GetSpectralWorleyNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), (vdbVec3.z()+dX_worley) * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers )  - m_cellNoiser.GetSpectralWorleyNoise4D ( m_time, 
						vdbVec3.x() * m_scale.GetX()   , vdbVec3.y() * m_scale.GetY(), (vdbVec3.z()-dX_worley) * m_scale.GetZ(),
						m_nbOcts, m_lacun, m_pers );

					gradient *= m_amount;
					iter.setValue (	iter.getValue () + gradient );
				}
				m_primaryGrid.m_grid  = grid;
		}
		else
		{
			m_primaryGrid.m_grid = in_grid->m_grid->deepCopyGrid ();
			Application().LogMessage(L"[VDB][NOISE]: Input grid has non-float and non-vec3s data type!", siWarningMsg );
		};
			
	m_primaryGrid.m_lastEvalTime = clock();
		m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
		Application().LogMessage(L"[VDB][NOISE]: Stamped at=" + CString(m_primaryGrid.m_lastEvalTime));
		Application().LogMessage(L"[VDB][NOISE]: Done in=" + CString(timer.GetElapsedTime()));
	};


};

SICALLBACK VDB_Noise_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

				CDataArrayCustomType outData( in_ctxt );      

			// get cache object
			VDB_Noise_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][NOISE]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_Noise_cache_t*)(CValue::siPtrType)userData;
			p_cacheNodeObject->PackGrid ( outData );

						// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][NOISE]: Empty grid on input" );
				return CStatus::OK;
			};
			
		           
			

						int gridType = -1;
			if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
			
			gridType=0;
			else if  ( p_inWrapper->m_grid->isType<openvdb::Vec3SGrid>() )
				gridType = 1;

			CDataArrayLong inSeed ( in_ctxt, ID_IN_Seed );
			CDataArrayLong inOcts ( in_ctxt, ID_IN_NbOctaves );
			CDataArrayFloat inAmount ( in_ctxt,ID_IN_Amount );
			CDataArrayVector3f inScl ( in_ctxt, ID_IN_Scale );
			CDataArrayFloat inLac ( in_ctxt, ID_IN_Lacunarity );
			CDataArrayFloat inPers ( in_ctxt, ID_IN_Persistnce );
			CDataArrayFloat inTime ( in_ctxt, ID_IN_Time );
			CDataArrayLong inNsType ( in_ctxt, ID_IN_NSType );

			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper, inSeed[0], inScl[0], inAmount[0], 
				inTime[0], inLac[0], inPers[0], Clamp(0L,LONG_MAX, inOcts[0]), gridType, inNsType[0] )		==false )
			{
				Application().LogMessage(L"[VDB][NOISE]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->ApplyNoise ( p_inWrapper );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK VDB_Noise_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Noise_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_Noise_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][NOISE]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK VDB_Noise_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][NOISE]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_Noise_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_Noise_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
