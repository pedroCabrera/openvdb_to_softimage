#include "Main.h"
#include "vdbHelpers.h"

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_VDBGrid = 0,

	
	ID_G_100 = 100,

	ID_OUT_Type= 200,	
	ID_OUT_Name = 201,
	ID_OUT_BBox =202,
	ID_OUT_VoxelSize = 203,
	ID_OUT_DataType = 204,

	ID_OUT_ActiveVoxelPos = 214,

	ID_OUT_Float = 250,
	ID_OUT_Vec3f = 251,
	ID_OUT_Long = 252,
	ID_OUT_Bool = 253,
	ID_OUT_String = 254,



	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};


CStatus VDB_GetGridData_Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_GetGridData", L"VDB Get Grid Data");

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


   // Add output ports.
   st = nodeDef.AddOutputPort(ID_OUT_Name, siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton,L"GridName", L"GridName");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_Type, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton,L"GridType", L"GridType");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_DataType, siICENodeDataLong, siICENodeStructureSingle, siICENodeContextSingleton,L"GridDataType", L"GridDataType");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_VoxelSize, siICENodeDataVector3, siICENodeStructureSingle, siICENodeContextSingleton,L"VoxelSize", L"VoxelSize");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_BBox, siICENodeDataMatrix33, siICENodeStructureSingle, siICENodeContextSingleton,L"BoundingBox", L"BoundingBox");  st.AssertSucceeded();

   st = nodeDef.AddOutputPort(ID_OUT_ActiveVoxelPos, siICENodeDataVector3, siICENodeStructureArray, siICENodeContextSingleton,L"ActiveVoxelsPosition", L"ActiveVoxelsPosition");  st.AssertSucceeded();
   

   st = nodeDef.AddOutputPort(ID_OUT_Float, siICENodeDataFloat, siICENodeStructureArray, siICENodeContextSingleton,L"FloatData", L"FloatData");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_Vec3f, siICENodeDataVector3, siICENodeStructureArray, siICENodeContextSingleton,L"Vec3Data", L"Vec3Data");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_Long, siICENodeDataLong, siICENodeStructureArray, siICENodeContextSingleton,L"LongData", L"LongData");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_Bool, siICENodeDataBool, siICENodeStructureArray, siICENodeContextSingleton,L"BoolData", L"BoolData");  st.AssertSucceeded();
   st = nodeDef.AddOutputPort(ID_OUT_String, siICENodeDataString, siICENodeStructureArray, siICENodeContextSingleton,L"StringData", L"StringData");  st.AssertSucceeded();


   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

   return CStatus::OK;
}






template < typename TreeType >
void FilloutActiveVoxelPositions (  openvdb::GridBase::Ptr & in_baseRef, CDataArray2DVector3f & outData )
{
	LONG nbActVoxels = in_baseRef->activeVoxelCount ();
	LONG xsiSafeDataSizeInBytes =  1073741824UL;
	if ( nbActVoxels * sizeof(MATH::CVector3f) > xsiSafeDataSizeInBytes )	
		Application().LogMessage ( L"[VDB][GETGRIDDATA]: Too many voxels to display (>1GB), will clamping voxelcount to avoid XSI crash", siWarningMsg );
	
	nbActVoxels = Clamp ( 0ULL, xsiSafeDataSizeInBytes/sizeof(MATH::CVector3f), nbActVoxels );
	CDataArray2DVector3f::Accessor outAcc = outData.Resize ( 0, nbActVoxels );

	openvdb::Grid<TreeType>::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::Grid<TreeType>> (in_baseRef);

	LLONG cnt = 0;
	for ( openvdb::Grid<TreeType>::ValueOnCIter iter = castedGridRef->cbeginValueOn(); cnt < nbActVoxels; ++ cnt  )
	{
		openvdb::v2_1_0::math::Vec3d vdbVec3 = castedGridRef->indexToWorld ( iter.getCoord () );
		outAcc[cnt].Set ( vdbVec3.x (),vdbVec3.y (),vdbVec3.z () );
		++iter;
	};

	return;
};



SICALLBACK VDB_GetGridData_Evaluate( ICENodeContext & in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_Type:
		{
			CDataArrayLong outData ( in_ctxt);
			outData[0]=0;
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			ULONG inDataSize;
			void * p_in = NULL;
			inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

			// check for data
			if (inDataSize==0L || p_in==NULL) 
				return CStatus::OK;

			VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
			if (p_inWrapper==NULL || !p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input");
				return CStatus::OK;
			};

	
			outData[0] =p_inWrapper->m_grid->getGridClass();
			/*
			GRID_UNKNOWN = 0,
			GRID_LEVEL_SET,
			GRID_FOG_VOLUME,
			GRID_STAGGERED
			*/
			return CStatus::OK;
		} break;

		case ID_OUT_DataType:
		{
			CDataArrayLong outData ( in_ctxt );
			outData[0]=0;
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			ULONG inDataSize;
			void * p_in = NULL;
			inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

			// check for data
			if (inDataSize==0L || p_in==NULL) 
				return CStatus::OK;

			VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
			if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
			{
				Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
				return CStatus::OK;
			};

			outData[0]=0;
			if (p_inWrapper->m_grid->isType<openvdb::BoolGrid>()) outData[0]=1;
			else if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>()) outData[0]=2; 
			else if (p_inWrapper->m_grid->isType<openvdb::DoubleGrid>())outData[0]=3;
			else if (p_inWrapper->m_grid->isType<openvdb::Int32Grid>()) outData[0]=4;
			else if (p_inWrapper->m_grid->isType<openvdb::Int64Grid>()) outData[0]=5;
			else if (p_inWrapper->m_grid->isType<openvdb::Vec3IGrid>()) outData[0]=6; 
			else if (p_inWrapper->m_grid->isType<openvdb::Vec3SGrid>()) outData[0]=7;
			else if (p_inWrapper->m_grid->isType<openvdb::Vec3DGrid>()) outData[0]=8;
			else if (p_inWrapper->m_grid->isType<openvdb::StringGrid>())outData[0]=9; 

			return CStatus::OK;
		} break;

		case ID_OUT_Name:
		{
			CDataArrayString outData ( in_ctxt );
			outData.SetData ( 0, L"" );
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			ULONG inDataSize;
			void * p_in = NULL;
			inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

			// check for data
			if (inDataSize==0L || p_in==NULL) 
				return CStatus::OK;

			VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
			if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
			{
				Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
				return CStatus::OK;
			};

			CString gridname(  p_inWrapper->m_grid->getName ().c_str() );
			//Application().LogMessage ( L"[VDB][GETGRIDDATA]: Grid name=" + gridname, siErrorMsg );
			outData.SetData ( 0,gridname) ;

			return CStatus::OK;
		} break;

		case ID_OUT_VoxelSize:
		{
			CDataArrayVector3f outData ( in_ctxt );
			outData[0].SetNull();
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			ULONG inDataSize;
			void * p_in = NULL;
			inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

			// check for data
			if (inDataSize==0L || p_in==NULL) 
				return CStatus::OK;

			VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
			if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
			{
				Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
				return CStatus::OK;
			};


			outData[0].Set
			(	p_inWrapper->m_grid->transform ().voxelSize().x (),
				p_inWrapper->m_grid->transform ().voxelSize().y (),
				p_inWrapper->m_grid->transform ().voxelSize().z ());

			return CStatus::OK;
		} break;

		case ID_OUT_BBox:
				{
			CDataArrayMatrix3f outData ( in_ctxt );
			outData[0].SetIdentity();
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			ULONG inDataSize;
			void * p_in = NULL;
			inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

			// check for data
			if (inDataSize==0L || p_in==NULL) 
				return CStatus::OK;

			VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
			if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
			{
				Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
				return CStatus::OK;
			};

			openvdb::v2_1_0::CoordBBox bbox;		
			//grid->baseTreePtr()->getIndexRange ( bbox );
		
		bbox =p_inWrapper->m_grid->evalActiveVoxelBoundingBox ();
			auto minp = p_inWrapper->m_grid->indexToWorld ( bbox.min() );
			auto maxp = p_inWrapper->m_grid->indexToWorld ( bbox.max() );
			auto midp =(minp+maxp)*0.5;

			outData[0].Set ( minp.x(),minp.y(),minp.z(),
				midp.x(),midp.y(),midp.z(),
				maxp.x(),maxp.y(),maxp.z());


			return CStatus::OK;
		} break;


				// active voxels
		case ID_OUT_ActiveVoxelPos:
			{
				CDataArray2DVector3f outData ( in_ctxt );

				CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
				ULONG inDataSize;
				void * p_in = NULL;
				inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

				// check for data
				if (inDataSize==0L || p_in==NULL) 
				{
									outData.Resize (0,0);
					return CStatus::OK;
				}

				VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
				if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
				{
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
					return CStatus::OK;
				};

				// eval type of grid and get all active voxels and their positions


				if (p_inWrapper->m_grid->isType<openvdb::BoolGrid>()) FilloutActiveVoxelPositions <openvdb::BoolTree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>()) FilloutActiveVoxelPositions <openvdb::FloatTree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::DoubleGrid>())FilloutActiveVoxelPositions <openvdb::DoubleTree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::Int32Grid>()) FilloutActiveVoxelPositions <openvdb::Int32Tree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::Int64Grid>()) FilloutActiveVoxelPositions <openvdb::Int64Tree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::Vec3IGrid>())FilloutActiveVoxelPositions <openvdb::Vec3ITree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::Vec3SGrid>())FilloutActiveVoxelPositions <openvdb::Vec3STree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::Vec3DGrid>()) FilloutActiveVoxelPositions <openvdb::Vec3DTree>  ( p_inWrapper->m_grid, outData ) ;
			else if (p_inWrapper->m_grid->isType<openvdb::StringGrid>())FilloutActiveVoxelPositions <openvdb::StringTree>  ( p_inWrapper->m_grid, outData ) ;



				return CStatus::OK;
			}
			break;


		case ID_OUT_Float:
			{
				CDataArray2DFloat outData ( in_ctxt );
				CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
				ULONG inDataSize;
				void * p_in = NULL;
				inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

								// check for data
				if (inDataSize==0L || p_in==NULL) 
				{
									outData.Resize (0,0);
					return CStatus::OK;
				}

				VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
				if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
				{
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
					return CStatus::OK;
				};

			
				// get all active values
				LONG nbActVoxels = p_inWrapper->m_grid->activeVoxelCount ();
				LONG xsiSafeDataSizeInBytes =  1073741824UL;
				if ( nbActVoxels*sizeof(float) > xsiSafeDataSizeInBytes )	
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Too many voxels to display (>1GB), will clamping voxelcount to avoid XSI crash", siWarningMsg );

				nbActVoxels = Clamp ( 0ULL, xsiSafeDataSizeInBytes/sizeof(float), nbActVoxels );
		
			//	if (gridbaseRef.isType<openvdb::BoolGrid>()) FilloutActiveVoxelPositions <openvdb::BoolTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::FloatGrid>()) FilloutActiveVoxelPositions <openvdb::FloatTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::DoubleGrid>())FilloutActiveVoxelPositions <openvdb::DoubleTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int32Grid>()) FilloutActiveVoxelPositions <openvdb::Int32Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int64Grid>()) FilloutActiveVoxelPositions <openvdb::Int64Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3IGrid>())FilloutActiveVoxelPositions <openvdb::Vec3ITree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3SGrid>())FilloutActiveVoxelPositions <openvdb::Vec3STree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3DGrid>()) FilloutActiveVoxelPositions <openvdb::Vec3DTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::StringGrid>())FilloutActiveVoxelPositions <openvdb::StringTree>  ( gridbaseRefRef, outData ) ;

				if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
				{
									CDataArray2DFloat::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::FloatGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::FloatGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::FloatGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++] = iter.getValue ();
					
				}
				else if ( p_inWrapper->m_grid->isType<openvdb::DoubleGrid>() )
				{
									CDataArray2DFloat::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::DoubleGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::DoubleGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::DoubleGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++] = iter.getValue ();
				}
				else
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Trying to get non-float or non-double value on float outport", siWarningMsg );
			
				return CStatus::OK;
				// end of outport eval
			}
			break;
		case ID_OUT_Vec3f:
			{
				CDataArray2DVector3f outData ( in_ctxt );
				CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
				ULONG inDataSize;
				void * p_in = NULL;
				inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

								// check for data
				if (inDataSize==0L || p_in==NULL) 
				{
									outData.Resize (0,0);
					return CStatus::OK;
				}

				VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
				if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
				{
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
					return CStatus::OK;
				};

			
				// get all active values
				LONG nbActVoxels = p_inWrapper->m_grid->activeVoxelCount ();
				LONG xsiSafeDataSizeInBytes =  1073741824UL;
				if ( nbActVoxels*sizeof(MATH::CVector3f) > xsiSafeDataSizeInBytes )	
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Too many voxels to display (>1GB), will clamping voxelcount to avoid XSI crash", siWarningMsg );

				nbActVoxels = Clamp ( 0ULL, xsiSafeDataSizeInBytes/sizeof(MATH::CVector3f), nbActVoxels );
		
			//	if (gridbaseRef.isType<openvdb::BoolGrid>()) FilloutActiveVoxelPositions <openvdb::BoolTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::FloatGrid>()) FilloutActiveVoxelPositions <openvdb::FloatTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::DoubleGrid>())FilloutActiveVoxelPositions <openvdb::DoubleTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int32Grid>()) FilloutActiveVoxelPositions <openvdb::Int32Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int64Grid>()) FilloutActiveVoxelPositions <openvdb::Int64Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3IGrid>())FilloutActiveVoxelPositions <openvdb::Vec3ITree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3SGrid>())FilloutActiveVoxelPositions <openvdb::Vec3STree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3DGrid>()) FilloutActiveVoxelPositions <openvdb::Vec3DTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::StringGrid>())FilloutActiveVoxelPositions <openvdb::StringTree>  ( gridbaseRefRef, outData ) ;

				if ( p_inWrapper->m_grid->isType<openvdb::Vec3DGrid>() )
				{
									CDataArray2DVector3f::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::Vec3DGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::Vec3DGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::Vec3DGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++].Set ( iter.getValue ().x(),iter.getValue ().y(),iter.getValue ().z() );
					
				}
				else if ( p_inWrapper->m_grid->isType<openvdb::Vec3SGrid>() )
				{
								CDataArray2DVector3f::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::Vec3SGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::Vec3SGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::Vec3SGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++].Set ( iter.getValue ().x(),iter.getValue ().y(),iter.getValue ().z() );
				}
				else if 
					( p_inWrapper->m_grid->isType<openvdb::Vec3IGrid>() )
				{
								CDataArray2DVector3f::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::Vec3IGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::Vec3IGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::Vec3IGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++].Set ( iter.getValue ().x(),iter.getValue ().y(),iter.getValue ().z() );
				}
				else
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Trying to get non-vector3(3single,3double,3integer) value on vector3 outport", siWarningMsg );
			
				return CStatus::OK;
				// end of outport eval
			}
			break;
		case ID_OUT_Long:
			{
				CDataArray2DLong outData ( in_ctxt );
				CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
				ULONG inDataSize;
				void * p_in = NULL;
				inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

								// check for data
				if (inDataSize==0L || p_in==NULL) 
				{
									outData.Resize (0,0);
					return CStatus::OK;
				}

				VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
				if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
				{
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
					return CStatus::OK;
				};

				
				// get all active values
				LONG nbActVoxels = p_inWrapper->m_grid->activeVoxelCount ();
				LONG xsiSafeDataSizeInBytes =  1073741824UL;
				if ( nbActVoxels*sizeof(LONG) > xsiSafeDataSizeInBytes )	
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Too many voxels to display (>1GB), will clamping voxelcount to avoid XSI crash", siWarningMsg );

				nbActVoxels = Clamp ( 0ULL, xsiSafeDataSizeInBytes/sizeof(LONG), nbActVoxels );
		
			//	if (gridbaseRef.isType<openvdb::BoolGrid>()) FilloutActiveVoxelPositions <openvdb::BoolTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::FloatGrid>()) FilloutActiveVoxelPositions <openvdb::FloatTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::DoubleGrid>())FilloutActiveVoxelPositions <openvdb::DoubleTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int32Grid>()) FilloutActiveVoxelPositions <openvdb::Int32Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int64Grid>()) FilloutActiveVoxelPositions <openvdb::Int64Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3IGrid>())FilloutActiveVoxelPositions <openvdb::Vec3ITree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3SGrid>())FilloutActiveVoxelPositions <openvdb::Vec3STree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3DGrid>()) FilloutActiveVoxelPositions <openvdb::Vec3DTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::StringGrid>())FilloutActiveVoxelPositions <openvdb::StringTree>  ( gridbaseRefRef, outData ) ;

				if ( p_inWrapper->m_grid->isType<openvdb::Int32Grid>() )
				{
					CDataArray2DLong::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::Int32Grid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::Int32Grid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::Int32Grid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++]= iter.getValue ();
					
				}
				else if ( p_inWrapper->m_grid->isType<openvdb::Int64Grid>() )
				{
								CDataArray2DLong::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::Int64Grid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::Int64Grid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::Int64Grid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					
						outAcc[cnt++]=iter.getValue ();
				}			
				else
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Trying to get non-integer value on integer outport", siWarningMsg );
			
				return CStatus::OK;
				// end of outport eval
			}
			break;
		case ID_OUT_Bool:
			{
				CDataArray2DBool outData ( in_ctxt );
				CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
				ULONG inDataSize;
				void * p_in = NULL;
				inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

							// check for data
				if (inDataSize==0L || p_in==NULL) 
				{
									outData.Resize (0,0);
					return CStatus::OK;
				}

				VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
				if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
				{
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
					return CStatus::OK;
				};

			
				// get all active values
				LONG nbActVoxels = p_inWrapper->m_grid->activeVoxelCount ();
				LONG xsiSafeDataSizeInBytes =  1073741824UL;
				if ( nbActVoxels > xsiSafeDataSizeInBytes )	
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Too many voxels to display (>1GB), will clamping voxelcount to avoid XSI crash", siWarningMsg );

				nbActVoxels = Clamp ( 0L, xsiSafeDataSizeInBytes, nbActVoxels );
		
			//	if (gridbaseRef.isType<openvdb::BoolGrid>()) FilloutActiveVoxelPositions <openvdb::BoolTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::FloatGrid>()) FilloutActiveVoxelPositions <openvdb::FloatTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::DoubleGrid>())FilloutActiveVoxelPositions <openvdb::DoubleTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int32Grid>()) FilloutActiveVoxelPositions <openvdb::Int32Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int64Grid>()) FilloutActiveVoxelPositions <openvdb::Int64Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3IGrid>())FilloutActiveVoxelPositions <openvdb::Vec3ITree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3SGrid>())FilloutActiveVoxelPositions <openvdb::Vec3STree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3DGrid>()) FilloutActiveVoxelPositions <openvdb::Vec3DTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::StringGrid>())FilloutActiveVoxelPositions <openvdb::StringTree>  ( gridbaseRefRef, outData ) ;

				if ( p_inWrapper->m_grid->isType<openvdb::BoolGrid>() )
				{
					CDataArray2DBool::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::BoolGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::BoolGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::BoolGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					 
						outAcc.Set ( cnt++, iter.getValue () );

				}			
				else
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Trying to get non-bool value on bool outport", siWarningMsg );

				return CStatus::OK;
				// end of outport eval
			}
			break;
		case ID_OUT_String:
			{
					CDataArray2DString outData ( in_ctxt );
				CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
				ULONG inDataSize;
				void * p_in = NULL;
				inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

								// check for data
				if (inDataSize==0L || p_in==NULL) 
				{
									outData.Resize (0,0);
					return CStatus::OK;
				}

				VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
				if (p_inWrapper==NULL ||!p_inWrapper->m_grid)
				{
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Invalid grid object on input" );
					return CStatus::OK;
				};

				
				// get all active values
				LONG nbActVoxels = p_inWrapper->m_grid->activeVoxelCount ();
				LONG xsiSafeDataSizeInBytes =  1073741824UL;
				if ( nbActVoxels*sizeof(CString) > xsiSafeDataSizeInBytes )	
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Too many voxels to display (>1GB), will clamping voxelcount to avoid XSI crash", siWarningMsg );

				nbActVoxels = Clamp ( 0ULL, xsiSafeDataSizeInBytes/sizeof(CString), nbActVoxels );
		
			//	if (gridbaseRef.isType<openvdb::BoolGrid>()) FilloutActiveVoxelPositions <openvdb::BoolTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::FloatGrid>()) FilloutActiveVoxelPositions <openvdb::FloatTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::DoubleGrid>())FilloutActiveVoxelPositions <openvdb::DoubleTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int32Grid>()) FilloutActiveVoxelPositions <openvdb::Int32Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Int64Grid>()) FilloutActiveVoxelPositions <openvdb::Int64Tree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3IGrid>())FilloutActiveVoxelPositions <openvdb::Vec3ITree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3SGrid>())FilloutActiveVoxelPositions <openvdb::Vec3STree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::Vec3DGrid>()) FilloutActiveVoxelPositions <openvdb::Vec3DTree>  ( gridbaseRefRef, outData ) ;
			//else if (gridbaseRef.isType<openvdb::StringGrid>())FilloutActiveVoxelPositions <openvdb::StringTree>  ( gridbaseRefRef, outData ) ;

				if ( p_inWrapper->m_grid->isType<openvdb::StringGrid>() )
				{
					CDataArray2DString::Accessor outAcc = outData.Resize ( 0, nbActVoxels );
					openvdb::StringGrid::Ptr castedGridRef= openvdb::gridPtrCast<openvdb::StringGrid> (p_inWrapper->m_grid);
					LLONG cnt = 0;
					for ( openvdb::StringGrid::ValueOnCIter iter = castedGridRef->cbeginValueOn(); iter; ++iter )					 
						outAcc.SetData ( cnt++, iter.getValue ().c_str() );

				}			
				else
					Application().LogMessage ( L"[VDB][GETGRIDDATA]: Trying to get non-string value on string outport", siWarningMsg );

				return CStatus::OK;
				// end of outport eval
			}
			break;

	default:
		break;

		return CStatus::OK;
	};

return CStatus::OK;
};

// lets cache this
SICALLBACK VDB_GetGridData_Init( CRef& in_ctxt )
{

  Application().LogMessage ( L"[VDB][GETGRIDDATA]: Grid types: 0=UNKNOWN | 1=LEVEL_SET | 2=FOG_VOLUME | 3= STAGGERED" );
  Application().LogMessage ( L"[VDB][GETGRIDDATA]: Data types: 0=bool | 1=float | 2=double | 3=int32 | 4=int64 | 5=vec3int | 6=vec3float | 7=vec3double | 8=string" );
   return CStatus::OK;
}



