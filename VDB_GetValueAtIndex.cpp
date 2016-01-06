


#include "Main.h"
#include "vdbHelpers.h"

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_VDBGrid = 0,
	ID_IN_X =1,
	ID_IN_Y =2,
	ID_IN_Z =3,

	ID_G_100 = 100,


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


CStatus VDB_GetValueAtIndex_Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_GetValueAtIndex", L"VDB Get Value At Index");

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
   st = nodeDef.AddInputPort(ID_IN_X, ID_G_100, siICENodeDataLong, siICENodeStructureAny, siICENodeContextSingleton, L"X", L"X",CValue(),CValue(),CValue(),ID_UNDEF,ID_STRUCT_CNS,ID_CTXT_CNS);  st.AssertSucceeded();
    st = nodeDef.AddInputPort(ID_IN_Y, ID_G_100, siICENodeDataLong, siICENodeStructureAny, siICENodeContextSingleton, L"Y", L"Y",CValue(),CValue(),CValue(),ID_UNDEF,ID_STRUCT_CNS,ID_CTXT_CNS);  st.AssertSucceeded();
 st = nodeDef.AddInputPort(ID_IN_Z, ID_G_100, siICENodeDataLong, siICENodeStructureAny, siICENodeContextSingleton, L"Z", L"Z",CValue(),CValue(),CValue(),ID_UNDEF,ID_STRUCT_CNS,ID_CTXT_CNS);  st.AssertSucceeded();

   // Add output ports.
 st = nodeDef.AddOutputPort(ID_OUT_Float, siICENodeDataFloat, siICENodeStructureAny, siICENodeContextSingleton,L"FloatData", L"FloatData", ID_UNDEF, ID_STRUCT_CNS,ID_CTXT_CNS);  st.AssertSucceeded();
  // st = nodeDef.AddOutputPort(ID_OUT_Vec3f, siICENodeDataVector3, siICENodeStructureArray, siICENodeContextSingleton,L"Vec3Data", L"Vec3Data");  st.AssertSucceeded();
  // st = nodeDef.AddOutputPort(ID_OUT_Long, siICENodeDataLong, siICENodeStructureArray, siICENodeContextSingleton,L"LongData", L"LongData");  st.AssertSucceeded();
   //st = nodeDef.AddOutputPort(ID_OUT_Bool, siICENodeDataBool, siICENodeStructureArray, siICENodeContextSingleton,L"BoolData", L"BoolData");  st.AssertSucceeded();
   //st = nodeDef.AddOutputPort(ID_OUT_String, siICENodeDataString, siICENodeStructureArray, siICENodeContextSingleton,L"StringData", L"StringData");  st.AssertSucceeded();


   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

   return CStatus::OK;
}







SICALLBACK VDB_GetValueAtIndex_Evaluate( ICENodeContext & in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_Float:
		{
			
			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][GETVALATPOS]: Empty levelset grid on input" );
				return CStatus::OK;
			};

		
			if (!p_inWrapper->m_grid)
			{
				Application().LogMessage ( L"[VDB][GETVALATINDEX]: Invalid grid object on input", siErrorMsg );
				return CStatus::OK;
			};
	
			if ( IsPortArray ( in_ctxt, ID_IN_X ) && IsPortArray ( in_ctxt, ID_IN_Y ) && IsPortArray ( in_ctxt, ID_IN_Z ))
			{
				CDataArray2DFloat outData ( in_ctxt);
				if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
				{
					CDataArray2DLong inX ( in_ctxt, ID_IN_X );
					CDataArray2DLong inY ( in_ctxt, ID_IN_Y );
					CDataArray2DLong inZ ( in_ctxt, ID_IN_Z );

					openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALATINDEX]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CIndexSet indexSet ( in_ctxt, ID_IN_X );

					// loop over all particles
					for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
					{
						CDataArray2DLong::Accessor inAccX = inX[it];
						CDataArray2DLong::Accessor inAccY = inY[it];
						CDataArray2DLong::Accessor inAccZ = inZ[it];

						LLONG nbIndices = inX.GetCount();
						if ( nbIndices != inAccY.GetCount() ||  nbIndices != inAccZ.GetCount() || nbIndices==0 )
							continue;


						CDataArray2DFloat::Accessor outAcc =outData.Resize ( it, nbIndices );

						// loop over all particle's indices
						for ( LLONG i=0;i<nbIndices; ++i )				
                            outAcc[i] = acc.getValue (openvdb::v2_3_0::math::Coord (inAccX[i],inAccY[i],inAccZ[i]));
					}


				}
				else if ( p_inWrapper->m_grid->isType<openvdb::DoubleGrid>() )
				{
					
					CDataArray2DLong inX ( in_ctxt, ID_IN_X );
					CDataArray2DLong inY ( in_ctxt, ID_IN_Y );
					CDataArray2DLong inZ ( in_ctxt, ID_IN_Z );

					openvdb::DoubleGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::DoubleGrid>(p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALATINDEX]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::DoubleGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CIndexSet indexSet ( in_ctxt, ID_IN_X );

					// loop over all particles
					for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
					{
						CDataArray2DLong::Accessor inAccX = inX[it];
						CDataArray2DLong::Accessor inAccY = inY[it];
						CDataArray2DLong::Accessor inAccZ = inZ[it];

						LLONG nbIndices = inX.GetCount();
						if ( nbIndices != inAccY.GetCount() ||  nbIndices != inAccZ.GetCount() || nbIndices==0 )
							continue;


						CDataArray2DFloat::Accessor outAcc =outData.Resize ( it, nbIndices );

						// loop over all particle's indices
						for ( LLONG i=0;i<nbIndices; ++i )				
                            outAcc[i] = acc.getValue (openvdb::v2_3_0::math::Coord (inAccX[i],inAccY[i],inAccZ[i]));
					}


				}
			}
			else if ( !IsPortArray ( in_ctxt, ID_IN_X ) && !IsPortArray ( in_ctxt, ID_IN_Y ) && !IsPortArray ( in_ctxt, ID_IN_Z ) )
			{
				CDataArrayFloat outData ( in_ctxt);
				if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
				{
					CDataArrayLong inX ( in_ctxt, ID_IN_X );
					CDataArrayLong inY ( in_ctxt, ID_IN_Y );
					CDataArrayLong inZ ( in_ctxt, ID_IN_Z );

					openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALATINDEX]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CIndexSet indexSet ( in_ctxt, ID_IN_X );

					// loop over all particles
					for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )							
                        outData[it] = acc.getValue (openvdb::v2_3_0::math::Coord (inX[it],inY[it],inZ[it]));

				}
				else if ( p_inWrapper->m_grid->isType<openvdb::DoubleGrid>() )
				{
					
					CDataArrayLong inX ( in_ctxt, ID_IN_X );
					CDataArrayLong inY ( in_ctxt, ID_IN_Y );
					CDataArrayLong inZ ( in_ctxt, ID_IN_Z );

					openvdb::DoubleGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::DoubleGrid>(p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALATINDEX]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::DoubleGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

					CIndexSet indexSet ( in_ctxt, ID_IN_X );

					// loop over all particles
					for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )							
                        outData[it] = acc.getValue (openvdb::math::Coord (inX[it],inY[it],inZ[it]));
					
				}
			};
		
			return CStatus::OK;
		} break;

	

	default:
		break;

		return CStatus::OK;
	};

return CStatus::OK;
};





