#include "Main.h"
#include "vdbHelpers.h"
#include <openvdb/tools/composite.h>
#include <openvdb/util/NullInterrupter.h>
#include <openvdb/tools/LevelSetRebuild.h>
#include <openvdb/tools/LevelSetMorph.h>
enum IDs
{
	ID_IN_VDBGridA ,
	ID_IN_VDBGridB,

	ID_IN_UseMT,

	ID_IN_AdvTimeStep,
	ID_IN_AdvSpatialScheme,
	ID_IN_AdvTemporalScheme,

	ID_IN_RenormSteps,
	ID_IN_RenormSpatialScheme,
	ID_IN_RenormTemporalScheme,

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_Morpher_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_Morpher", L"VDB Morpher");

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


	st = nodeDef.AddInputPort(ID_IN_UseMT, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"Multithreaded", L"Multithreaded",true,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();


	 st = nodeDef.AddInputPort(ID_IN_AdvTimeStep, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"BlendTime", L"BlendTime",0.04f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_AdvSpatialScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"AdvectionSpatialScheme", L"AdvectionSpatialScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_AdvTemporalScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"AdvectionTemporalScheme", L"AdvectionTemporalScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  
   st = nodeDef.AddInputPort(ID_IN_RenormSteps, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"RenormSteps", L"RenormSteps",3,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_RenormSpatialScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"RenormSpatialScheme", L"RenormSpatialScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_RenormTemporalScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"RenormTemporalScheme", L"RenormTemporalScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  
	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_Morpher_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_Morpher_cache_t ( ) { };


	clock_t  m_inputEvalTimeA;
	clock_t  m_inputEvalTimeB;

		bool m_multithreaded;

	float m_advTime;
	LONG m_advSpat;
	LONG m_advTemp;
	LONG m_renSteps; 
	LONG m_renSpat;
	LONG m_renTemp;


	bool inline IsDirty ( clock_t atime, clock_t btime, float advTime, LONG advSpat, LONG advTemp, LONG renSteps, LONG renSpat, LONG renTEmp, bool in_mt   )
	{
		bool isClean = ( m_inputEvalTimeA == atime && m_inputEvalTimeB == btime && 
			m_advTime == advTime &&	m_advSpat== advSpat&&m_advTemp==	 advTemp&&	m_renSteps== renSteps&&	m_renSpat== renSpat&&	m_renTemp== renTEmp&&
			m_multithreaded == in_mt);

		if ( isClean == false )
		{			
			m_inputEvalTimeA = atime ;
			m_inputEvalTimeB = btime ;

				m_advTime = advTime ;
			m_advSpat= advSpat;
			m_advTemp=	 advTemp;
			m_renSteps= renSteps;
			m_renSpat= renSpat;
			m_renTemp= renTEmp;

		
			m_multithreaded = in_mt;
		};

		return !isClean;

	};



	inline void Morph ( VDB_ICEFlowDataWrapper_t * in_gridA, VDB_ICEFlowDataWrapper_t * in_gridB )
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

				openvdb::tools::LevelSetMorphing <openvdb::FloatGrid> morpher (* floatCopyA.get(),* floatCopyB.get() );

			// set advction parameters
					 typedef openvdb::v2_3_0::math::TemporalIntegrationScheme  TIS_t ;
		 typedef openvdb::v2_3_0::math::BiasedGradientScheme  BGS_t ;

		 TIS_t adv_tis = TIS_t::TVD_RK1;
		 TIS_t renorm_tis = TIS_t::TVD_RK1;
		 BGS_t adv_bgs = BGS_t::FIRST_BIAS;
		 BGS_t renorm_bgs = BGS_t::FIRST_BIAS;


		 // spatial integration
		 if ( m_advSpat==0 )
			 adv_bgs = BGS_t::FIRST_BIAS;
		 if ( m_advSpat==1 )
			 adv_bgs = BGS_t::SECOND_BIAS;
		 if ( m_advSpat==2 )
			 adv_bgs = BGS_t::THIRD_BIAS;
		 if ( m_advSpat==3 )
			 adv_bgs = BGS_t::WENO5_BIAS;
		 if ( m_advSpat==4 )
			 adv_bgs = BGS_t::HJWENO5_BIAS;


		 if ( m_renSpat==0 )
			 renorm_bgs = BGS_t::FIRST_BIAS;
		 if ( m_renSpat==1 )
			 renorm_bgs = BGS_t::SECOND_BIAS;
		 if ( m_renSpat==2 )
			 renorm_bgs = BGS_t::THIRD_BIAS;
		 if ( m_renSpat==3 )
			 renorm_bgs = BGS_t::WENO5_BIAS;
		 if ( m_renSpat==4 )
			 renorm_bgs = BGS_t::HJWENO5_BIAS;

		 // temporal integration
		 if ( m_advTemp==0 )
			 adv_tis = TIS_t::TVD_RK1;
		 if ( m_advTemp==1 )
			 adv_tis = TIS_t::TVD_RK2;
		 if ( m_advTemp==2 )
			 adv_tis = TIS_t::TVD_RK3;

		 if ( m_renTemp==0 )
			 renorm_tis = TIS_t::TVD_RK1;
		 if ( m_renTemp==1 )
			 renorm_tis = TIS_t::TVD_RK2;
		 if ( m_renTemp==2 )
			 renorm_tis = TIS_t::TVD_RK3;


		 // setup advector
		 morpher.setSpatialScheme(adv_bgs);
		 morpher.setTemporalScheme(adv_tis);
		 morpher.setTrackerSpatialScheme(renorm_bgs);
		 morpher.setTrackerTemporalScheme(renorm_tis);
		 morpher.setNormCount(m_renSteps);

		 // blend
			morpher.advect ( 0.f, m_advTime );


			m_primaryGrid.m_grid =  floatCopyA ;

			m_primaryGrid.m_grid->setGridClass ( openvdb::GRID_LEVEL_SET );
			m_primaryGrid.m_grid->setName ( "morpherResult" );
			m_primaryGrid.m_lastEvalTime = clock();	
			Application().LogMessage(L"[VDB][Morpher]: Stamped at=" + CString ( m_primaryGrid.m_lastEvalTime ) );
			Application().LogMessage(L"[VDB][Morpher]: Done in=" + CString (  timer.GetElapsedTime ( ) ));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][Morpher]: " + CString ( e.what() ), siErrorMsg );
		};
	};

};

SICALLBACK VDB_Morpher_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

			 CDataArrayCustomType outData( in_ctxt );     


			// get cache object
			VDB_Morpher_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][Morpher]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_Morpher_cache_t*)(CValue::siPtrType)userData;
			p_cacheNodeObject->PackGrid ( outData );

			// get input grids
			CDataArrayCustomType inVDBGridA(in_ctxt, ID_IN_VDBGridA);
			CDataArrayCustomType inVDBGridB(in_ctxt, ID_IN_VDBGridB);

			// check for a
			VDB_ICEFlowDataWrapper_t * p_inWrapperA = p_cacheNodeObject->UnpackGrid ( inVDBGridA );
			if (p_inWrapperA==NULL || !p_inWrapperA->m_grid )
			{
				Application().LogMessage ( L"[VDB][Morpher]: Empty grid on a-input" );
				return CStatus::OK;
			};

		
			// check for b
			VDB_ICEFlowDataWrapper_t * p_inWrapperB = p_cacheNodeObject->UnpackGrid ( inVDBGridB );
			if (p_inWrapperB==NULL || !p_inWrapperB->m_grid )
			{
				Application().LogMessage ( L"[VDB][Morpher]: Empty grid on b-input" );
				return CStatus::OK;
			};

			// only level set for now
			if (p_inWrapperA->m_grid->getGridClass() != openvdb::GRID_LEVEL_SET || p_inWrapperB->m_grid->getGridClass() != openvdb::GRID_LEVEL_SET ||
				p_inWrapperA->m_grid->isType<openvdb::FloatGrid>()==false ||p_inWrapperB->m_grid->isType<openvdb::FloatGrid>()==false )
			{
				Application().LogMessage ( L"[VDB][Morpher]: Only float level sets are allowed!", siWarningMsg );
				return CStatus::OK;
			};


			CDataArrayFloat inAdvTime ( in_ctxt, ID_IN_AdvTimeStep );
			CDataArrayLong inAdvSpat ( in_ctxt, ID_IN_AdvSpatialScheme );
			CDataArrayLong inAdvTemp ( in_ctxt, ID_IN_AdvTemporalScheme );

			CDataArrayLong inRenormTime ( in_ctxt, ID_IN_RenormSteps );
			CDataArrayLong inRenormSpat ( in_ctxt, ID_IN_RenormSpatialScheme );
			CDataArrayLong inRenormTemp ( in_ctxt, ID_IN_RenormTemporalScheme );
		
			CDataArrayBool inUseMT ( in_ctxt, ID_IN_UseMT );



			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapperA->m_lastEvalTime, p_inWrapperB->m_lastEvalTime, 
				 inAdvTime[0], inAdvSpat[0], inAdvTemp[0], inRenormTime[0], inRenormSpat[0], inRenormTemp[0] , inUseMT[0])		==false )
			{
					Application().LogMessage ( L"[VDB][Morpher]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->Morph ( p_inWrapperA, p_inWrapperB  );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK VDB_Morpher_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Morpher_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_Morpher_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][Morpher]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK VDB_Morpher_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][Morpher]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_Morpher_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_Morpher_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}