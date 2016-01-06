#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_userdatablob.h>

#include <openvdb/tools/GridTransformer.h>
#include <openvdb/tools/LevelSetRebuild.h>
#include <openvdb/util/NullInterrupter.h>
enum IDs
{
	ID_IN_VDBGrid ,


	ID_IN_VoxelSize,
	ID_IN_PruneTol,
	ID_IN_ExteriorWidth,
	ID_IN_InteriorWidth,

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_ResampleGrid_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_ResampleGrid", L"VDB Resample Grid");

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

	st = nodeDef.AddInputPort(ID_IN_VoxelSize, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"VoxelSize", L"VoxelSize",0.5f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_PruneTol, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Prune Tolerance", L"PruneTol",0.f,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_ExteriorWidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"Exterior Width", L"extWidth", CValue(2.0));st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_InteriorWidth, ID_G_100, siICENodeDataFloat,	siICENodeStructureSingle, siICENodeContextSingleton,L"Interior Width", L"intWidth", CValue(2.0));st.AssertSucceeded();
	

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}




struct VDB_ResampleGrid_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_ResampleGrid_cache_t ( ) { };



	clock_t  m_inputEvalTime;
	float m_voxelsize;
	float m_pruneTol;
	float m_in;
	float m_ex;



	bool inline IsDirty ( clock_t  inevaltime, float vxsz, float prtol, float ex, float in)
	{
		bool isClean = ( m_inputEvalTime == inevaltime && m_voxelsize == vxsz && m_pruneTol == prtol && m_ex == ex && m_in == in  );

		if ( isClean == false )
		{			
			m_inputEvalTime = inevaltime;	
			m_voxelsize = vxsz ;
			m_pruneTol = prtol ;
			m_ex = ex ;
			m_in = in;
		};

		return !isClean;

	};

private:
	// ####################################################################
	// resample level set routine ( float and double grids )
	template<typename GridT>
	inline openvdb::GridBase::Ptr  _resmpLS ( const GridT & grid  )
	{
		openvdb::math::Transform::Ptr xform = openvdb::math::Transform::createLinearTransform(m_voxelsize);// grid.transform();
		openvdb::GridBase::Ptr outGrid;

		typedef typename GridT::ValueType ValueT;

		openvdb::util::NullInterrupter interrupter;

		outGrid = openvdb::tools::doLevelSetRebuild(grid,
			/*isovalue=*/openvdb::zeroVal<ValueT>(),
			/*exWidth=*/m_ex, /*inWidth=*/m_in, &(*xform),&interrupter);

		return outGrid;
	};

	// ####################################################################
	// resample regular grid 
	template<typename Sampler, typename GridT>
	inline void  _resmpFOG ( const GridT & inGrid, openvdb::GridBase::Ptr & outGridBase   )
	{
		openvdb::math::Transform::Ptr xform = openvdb::math::Transform::createLinearTransform(m_voxelsize);
		outGridBase->setTransform(xform);

typename		GridT::Ptr outGridCasted = openvdb::gridPtrCast<GridT>(outGridBase);
		openvdb::util::NullInterrupter interrupter;
		openvdb::tools::resampleToMatch<Sampler>(inGrid, *outGridCasted, interrupter);
	};
	// ####################################################################

public:
	inline void Resample ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		TIMER timer;

		if (in_grid->m_grid->isType<openvdb::StringGrid>())
		{
			Application().LogMessage(L"[VDB][RESAMPLE]: String dtat type is not allowed to resampling!", siErrorMsg );
			return;
		}

		openvdb::GridBase::Ptr outGrid ;



		try 
		{
			// LS and FOG are processed separately
			const bool isLevelSet = ((in_grid->m_grid->getGridClass() == openvdb::GRID_LEVEL_SET)
				&& (in_grid->m_grid->isType<openvdb::FloatGrid>()  || in_grid->m_grid->isType<openvdb::DoubleGrid>()));

			// create output grid
			outGrid = openvdb::GridBase::createGrid (  in_grid->m_grid->type () );		 

			if ( isLevelSet )
			{
				if ( in_grid->m_grid->isType<openvdb::FloatGrid>() )
				{
					openvdb::FloatGrid::Ptr flGrid = openvdb::gridPtrCast<openvdb::FloatGrid> ( in_grid->m_grid );
					outGrid =	 _resmpLS( *flGrid );
				}
				else 
				{
					openvdb::DoubleGrid::Ptr flGrid = openvdb::gridPtrCast<openvdb::DoubleGrid> ( in_grid->m_grid );
					outGrid =	 _resmpLS( *flGrid );
				}		

				outGrid->setGridClass ( openvdb::GRID_LEVEL_SET );
			}
			else
			{
				
				if (in_grid->m_grid->isType<openvdb::BoolGrid>())  _resmpFOG <openvdb::tools::PointSampler> (* openvdb::gridPtrCast<openvdb::BoolGrid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::FloatGrid>()) _resmpFOG <openvdb::tools::BoxSampler> ( *openvdb::gridPtrCast<openvdb::FloatGrid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::DoubleGrid>())_resmpFOG <openvdb::tools::BoxSampler> ( *openvdb::gridPtrCast<openvdb::DoubleGrid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::Int32Grid>())_resmpFOG <openvdb::tools::BoxSampler> ( *openvdb::gridPtrCast<openvdb::Int32Grid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::Int64Grid>())_resmpFOG <openvdb::tools::BoxSampler> (* openvdb::gridPtrCast<openvdb::Int64Grid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::Vec3IGrid>())_resmpFOG <openvdb::tools::BoxSampler> ( *openvdb::gridPtrCast<openvdb::Vec3IGrid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::Vec3SGrid>())_resmpFOG <openvdb::tools::BoxSampler> ( *openvdb::gridPtrCast<openvdb::Vec3SGrid>(in_grid->m_grid), outGrid );
				else if (in_grid->m_grid->isType<openvdb::Vec3DGrid>())_resmpFOG <openvdb::tools::BoxSampler> ( *openvdb::gridPtrCast<openvdb::Vec3DGrid>(in_grid->m_grid), outGrid );
				else 
				{
					Application().LogMessage(L"[VDB][RESAMPLE]: Unsupported grid data type!", siErrorMsg );
					return;
				}

				outGrid->setGridClass ( openvdb::GRID_FOG_VOLUME );


			}



			if (m_pruneTol) 
				outGrid->pruneGrid(m_pruneTol);

			m_primaryGrid.m_grid = outGrid;
			m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
			m_primaryGrid.m_lastEvalTime = clock();
            Application().LogMessage(L"[VDB][RESAMPLE]: Stamped at=" + CString ((LONG) m_primaryGrid.m_lastEvalTime));
			Application().LogMessage(L"[VDB][RESAMPLE]: Done in=" + CString (timer.GetElapsedTime ()));

		}
		catch ( openvdb::Exception & e )
		{
			Application().LogMessage(L"[VDB][RESAMPLE]:" + CString ( e.what() ), siErrorMsg );
		}








	};


};

SICALLBACK dlexport VDB_ResampleGrid_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

			CDataArrayCustomType outData( in_ctxt );    

			// get cache object
			VDB_ResampleGrid_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][RESAMPLE]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_ResampleGrid_cache_t*)(CValue::siPtrType)userData;
			p_cacheNodeObject->PackGrid ( outData );

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][RESAMPLE]: Empty grid on input" );
				return CStatus::OK;
			};


			CDataArrayFloat inPruneTol ( in_ctxt, ID_IN_PruneTol );
			CDataArrayFloat inVoxelSize ( in_ctxt, ID_IN_VoxelSize );
			CDataArrayFloat inExt ( in_ctxt, ID_IN_ExteriorWidth );
			CDataArrayFloat inInt ( in_ctxt, ID_IN_InteriorWidth );

			float voxelSafeSize = Clamp ( 0.00001f, FLT_MAX, inVoxelSize[0] );


			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper->m_lastEvalTime, voxelSafeSize,inPruneTol[0], inExt[0],inInt[0])==false )
			{
				Application().LogMessage ( L"[VDB][RESAMPLE]: No changes on input, used prev result" );
				return CStatus::OK;
			}

			// change if dirty
			p_cacheNodeObject->Resample ( p_inWrapper );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK dlexport VDB_ResampleGrid_Init( CRef& in_ctxt )
{

	// init openvdb stuff
	openvdb::initialize();


	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();
	VDB_ResampleGrid_cache_t * p_vdbObj;

	if (userData.IsEmpty()) 	   
		p_vdbObj = new VDB_ResampleGrid_cache_t ( );
	else
	{
		Application().LogMessage ( L"[VDB][RESAMPLE]: Fatal, unknow data is present on initialization", siFatalMsg );
		return CStatus::Fail;
	}

	ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
	return CStatus::OK;
}



SICALLBACK dlexport VDB_ResampleGrid_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();

	if ( userData.IsEmpty () )
	{
		Application().LogMessage ( L"[VDB][RESAMPLE]: Fatal, no data on termination", siFatalMsg );
		return CStatus::Fail;
	}

	VDB_ResampleGrid_cache_t * p_vdbObj; 
	p_vdbObj = (VDB_ResampleGrid_cache_t*)(CValue::siPtrType)userData;


	delete p_vdbObj;
	ctxt.PutUserData(CValue());

	return CStatus::OK;
}
