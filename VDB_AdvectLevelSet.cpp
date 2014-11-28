#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_userdatablob.h>

#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/tree/ValueAccessor.h>
#include <openvdb/tools/LevelSetAdvect.h>
#include <openvdb/tools/GridOperators.h>

enum IDs
{
	ID_IN_VDBGrid ,
	ID_IN_VDBGridVel,

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

CStatus VDB_AdvectLevelSet_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_AdvectLevelSet", L"VDB Advect Level Set");

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

	st = nodeDef.AddInputPort(ID_IN_VDBGrid, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"In VDB LevelSet Grid", L"inVDBLSGrid",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_VDBGridVel, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"In VDB Velocity Grid", L"inVDBVELGrid",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();

	st = nodeDef.AddInputPort(ID_IN_UseMT, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"Multithreaded", L"Multithreaded",true,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();


	 st = nodeDef.AddInputPort(ID_IN_AdvTimeStep, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"AdvectionTimeStep", L"AdvectionTimeStep",0.04f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_AdvSpatialScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"AdvectionSpatialScheme", L"AdvectionSpatialScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_AdvTemporalScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"AdvectionTemporalScheme", L"AdvectionTemporalScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  
   st = nodeDef.AddInputPort(ID_IN_RenormSteps, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"RenormSteps", L"RenormSteps",3,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   st = nodeDef.AddInputPort(ID_IN_RenormSpatialScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"RenormSpatialScheme", L"RenormSpatialScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_RenormTemporalScheme, ID_G_100, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton, L"RenormTemporalScheme", L"RenormTemporalScheme",1,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
  
	/* 	ID_IN_AdvTimeStep,
	ID_IN_AdvSpatialScheme,
	ID_IN_AdvTemporalScheme,

	ID_IN_RenormSteps,
	ID_IN_RenormSpatialScheme,
	ID_IN_RenormTemporalScheme,*/

	
	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_AdvectLevelSet_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_AdvectLevelSet_cache_t ( ) { };


	clock_t  m_inputEvalTimeScalar;
	clock_t  m_inputEvalTimeVel;

	bool m_multithreaded;

	float m_advTime;
	LONG m_advSpat;
	LONG m_advTemp;
	LONG m_renSteps; 
	LONG m_renSpat;
	LONG m_renTemp;


	bool inline IsDirty ( VDB_ICEFlowDataWrapper_t * in_grid, VDB_ICEFlowDataWrapper_t * in_gridVel, 
		float advTime, LONG advSpat, LONG advTemp, LONG renSteps, LONG renSpat, LONG renTEmp, bool in_mt   )
	{
		bool isClean = ( m_inputEvalTimeScalar == (*in_grid).m_lastEvalTime && in_gridVel && m_inputEvalTimeVel == (*in_gridVel).m_lastEvalTime && 
			m_advTime == advTime &&	m_advSpat== advSpat&&m_advTemp==	 advTemp&&	m_renSteps== renSteps&&	m_renSpat== renSpat&&	m_renTemp== renTEmp&&
			m_multithreaded == in_mt);

		if ( isClean == false )
		{			
			m_inputEvalTimeScalar = (*in_grid).m_lastEvalTime;
			m_inputEvalTimeVel =  in_gridVel ? (*in_gridVel).m_lastEvalTime : 0;
		
		
		
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



	inline void AdvectLevelSet ( VDB_ICEFlowDataWrapper_t * in_grid, VDB_ICEFlowDataWrapper_t * in_gridVel )
	{

		TIMER timer;

		try {

		if ( !in_grid->m_grid || in_grid->m_grid->isType<openvdb::FloatGrid>() == false || in_grid->m_grid->getGridClass() != openvdb::GRID_LEVEL_SET )
		{	

			Application().LogMessage ( L"[VDB][ADVECTLS]: Input scalar grid is invalid or has non-supported type or type" );
			ResetGridHolder ( );
			return; 
		}



		// deep copy of input scalar grid
		openvdb::GridBase::Ptr tempBaseGrid	= in_grid->m_grid->deepCopyGrid ();
		openvdb::FloatGrid::Ptr scalarGrid  = openvdb::gridPtrCast<openvdb::FloatGrid> ( tempBaseGrid );

				openvdb::Vec3SGrid::Ptr velGrid ;


		// see if we need to generate vel field or get it from input
		if ( in_gridVel == NULL || !in_gridVel->m_grid || in_gridVel->m_grid->isType<openvdb::Vec3SGrid>() == false )
		{
			// generate velocity filed
			Application().LogMessage ( L"[VDB][ADVECTLS]: Input velocity grid is invalid or has non-supported type", siWarningMsg );
			m_primaryGrid .m_grid = scalarGrid;
			m_primaryGrid.m_lastEvalTime = in_grid->m_lastEvalTime;
			return;
		}
		else
	
			velGrid = openvdb::gridPtrCast<openvdb::Vec3SGrid> (  in_gridVel->m_grid );

		

		// advect
		typedef openvdb::tools::DiscreteField <openvdb::Vec3SGrid, openvdb::tools::BoxSampler> FieldT;
		FieldT field(*velGrid);
		 openvdb::tools::LevelSetAdvection<openvdb::FloatGrid, FieldT>  advection(*scalarGrid, field);

		 typedef openvdb::v2_1_0::math::TemporalIntegrationScheme  TIS_t ;
		 typedef openvdb::v2_1_0::math::BiasedGradientScheme  BGS_t ;

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
		 advection.setSpatialScheme(adv_bgs);
		 advection.setTemporalScheme(adv_tis);
		 advection.setTrackerSpatialScheme(renorm_bgs);
		 advection.setTrackerTemporalScheme(renorm_tis);
		 advection.setNormCount(m_renSteps);

		 if  ( m_multithreaded == false )
		 advection.setGrainSize ( 0 );

		 advection.advect(0, m_advTime);

		 m_primaryGrid.m_grid  = scalarGrid;
		 m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
		 m_primaryGrid.m_lastEvalTime = clock();	
		 Application().LogMessage(L"[VDB][ADVECTLS]: Stamped at=" + CString ( m_primaryGrid.m_lastEvalTime ) );
		 Application().LogMessage(L"[VDB][ADVECTLS]: Done in=" + CString (  timer.GetElapsedTime ( ) ));

		 }
		 catch ( openvdb::Exception & e )
		 {
			 Application().LogMessage(L"[VDB][ADVECTLS]: " + CString ( e.what() ), siErrorMsg );
		 };
	};

};

SICALLBACK VDB_AdvectLevelSet_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

			 CDataArrayCustomType outData( in_ctxt );     


			// get cache object
			VDB_AdvectLevelSet_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][ADVECTLS]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_AdvectLevelSet_cache_t*)(CValue::siPtrType)userData;
			p_cacheNodeObject->PackGrid ( outData );

			// get input grids
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			CDataArrayCustomType inVDBGridVelPort(in_ctxt, ID_IN_VDBGridVel);

			// check for prim ls
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][ADVECTLS]: Empty levelset grid on input" );
				return CStatus::OK;
			};


			// check for velgrid
			VDB_ICEFlowDataWrapper_t * p_inWrapperVel = p_cacheNodeObject->UnpackGrid ( inVDBGridVelPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][ADVECTLS]: Empty velocity grid on input" );
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
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper, p_inWrapperVel,			
			 inAdvTime[0], inAdvSpat[0], inAdvTemp[0], inRenormTime[0], inRenormSpat[0], inRenormTemp[0] , inUseMT[0] )		==false )
			{
					Application().LogMessage ( L"[VDB][ADVECTLS]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->AdvectLevelSet ( p_inWrapper, p_inWrapperVel  );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK VDB_AdvectLevelSet_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_AdvectLevelSet_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_AdvectLevelSet_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][ADVECTLS]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK VDB_AdvectLevelSet_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][ADVECTLS]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_AdvectLevelSet_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_AdvectLevelSet_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}