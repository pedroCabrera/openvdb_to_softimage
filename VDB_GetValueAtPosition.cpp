
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/GridTransformer.h>

#include "Main.h"
#include "vdbHelpers.h"

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_VDBGrid ,
	ID_IN_Positions,
	ID_IN_Space ,

	ID_G_100 = 100,


	ID_OUT_Float = 250,
	ID_OUT_Vec3f  ,
	ID_OUT_Long  ,
	ID_OUT_Bool ,
	ID_OUT_String ,

	ID_OUT_SDFVector,
	ID_OUT_SDFDirectionNorm,


	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};


CStatus VDB_GetValueAtCoordinate_Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_GetValueAtCoordinate", L"VDB Get Value At Coordinate");

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
   st = nodeDef.AddInputPort(ID_IN_Positions, ID_G_100, siICENodeDataVector3, siICENodeStructureAny, siICENodeContextAny, L"Positions", L"Positions",CValue(),CValue(),CValue(),ID_UNDEF,ID_STRUCT_CNS,ID_CTXT_CNS);  st.AssertSucceeded();
     st = nodeDef.AddInputPort(ID_IN_Space, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"IndexSpace", L"IndexSpace",false,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();

   // Add output ports.
	 st = nodeDef.AddOutputPort(ID_OUT_Float, siICENodeDataFloat, siICENodeStructureAny, siICENodeContextAny, L"FloatData", L"FloatData", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();
	 st = nodeDef.AddOutputPort(ID_OUT_Vec3f, siICENodeDataVector3, siICENodeStructureAny, siICENodeContextAny, L"Vec3Data", L"Vec3Data", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();
	 st = nodeDef.AddOutputPort(ID_OUT_Long, siICENodeDataLong, siICENodeStructureAny, siICENodeContextAny, L"LongData", L"LongData", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();
	 st = nodeDef.AddOutputPort(ID_OUT_Bool, siICENodeDataBool, siICENodeStructureAny, siICENodeContextAny, L"BoolData", L"BoolData", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();
	 st = nodeDef.AddOutputPort(ID_OUT_String, siICENodeDataString, siICENodeStructureAny, siICENodeContextAny, L"StringData", L"StringData", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();

	 st = nodeDef.AddOutputPort(ID_OUT_SDFVector, siICENodeDataVector3, siICENodeStructureAny, siICENodeContextAny, L"SDFVector", L"SDFVector", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();
	 st = nodeDef.AddOutputPort(ID_OUT_SDFDirectionNorm, siICENodeDataVector3, siICENodeStructureAny, siICENodeContextAny, L"SDFDirectionNorm", L"SDFDirectionNorm", ID_UNDEF, ID_STRUCT_CNS, ID_CTXT_CNS);  st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

   return CStatus::OK;
}







SICALLBACK dlexport VDB_GetValueAtCoordinate_Evaluate( ICENodeContext & in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_Float:
		{
			

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			CDataArrayBool inUseIndexSpace (in_ctxt, ID_IN_Space);
			bool convertToIdxSpace = inUseIndexSpace[0];
			VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic ( inVDBGridPort );
			if (p_inWrapper==NULL ||! p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][GETVALAT]: Empty grid on input" );
				return CStatus::OK;
			};

			if ( IsPortArray ( in_ctxt, ID_IN_Positions ) )
			{
				CDataArray2DFloat outData ( in_ctxt);
				// ##################################################################################################
				// float
				if (  p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
				{
					openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

					CDataArray2DVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particles and directly visit voxels by index
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DFloat::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's indices
							for ( LLONG i=0;i<nbPositions; ++i )
							{
								actCoord.setX ( inAcc[i].GetX() );
								actCoord.setY ( inAcc[i].GetY() );
								actCoord.setZ ( inAcc[i].GetZ() );
								outAcc[i] = acc.getValue( actCoord );
							}
						};

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DFloat::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's positions
							for ( LLONG i=0;i<nbPositions; ++i )				
								outAcc[i] = interpolator.wsSample( openvdb::Vec3f ( inAcc[i].GetX(), inAcc[i].GetY(), inAcc[i].GetZ() ));
						}
					}
				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-float grid value on float outport!", siWarningMsg );		return CStatus::OK;	}

			}
			else
			{
				CDataArrayFloat outData ( in_ctxt);
				// non-array ctxt
				if (  p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
				{
					openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CDataArrayVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particle's indices
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							actCoord.setX ( inPos[it].GetX() );
							actCoord.setY ( inPos[it].GetY() );
							actCoord.setZ ( inPos[it].GetZ() );
							outData[it] = acc.getValue( actCoord );
						}

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
							outData[it] = interpolator.wsSample( openvdb::Vec3f ( inPos[it].GetX(), inPos[it].GetY(), inPos[it].GetZ() ));
					}

				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-float grid value on float outport!", siWarningMsg );		return CStatus::OK;	}
				
			}

			return CStatus::OK;
		} break;






// @@ Gradient with length=distance to surface
		case ID_OUT_SDFVector:
		{


							 // get input grid
							 CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
							 CDataArrayBool inUseIndexSpace(in_ctxt, ID_IN_Space);
							 bool convertToIdxSpace = inUseIndexSpace[0];
							 VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic(inVDBGridPort);
							 if (p_inWrapper == NULL || !p_inWrapper->m_grid)
							 {
								 Application().LogMessage(L"[VDB][GETVALAT]: Empty grid on input");
								 return CStatus::OK;
							 };

							 if (IsPortArray(in_ctxt, ID_IN_Positions))
							 {
								 CDataArray2DVector3f outData(in_ctxt);
								 // ##################################################################################################
								 // float
								 if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>())
								 {
									 openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
									 if (!castedGridRef)
									 {
										 Application().LogMessage(L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg);		return CStatus::OK;
									 }
									 openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

									 CDataArray2DVector3f inPos(in_ctxt, ID_IN_Positions);
									 CIndexSet indexSet(in_ctxt, ID_IN_Positions);

									 if (convertToIdxSpace)
									 {
										 // current coordinate
										 openvdb::Coord actCoord;

										 // loop over all particles and directly visit voxels by index
										 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
										 {
											 CDataArray2DVector3f::Accessor inAcc = inPos[it];
											 LLONG nbPositions = inAcc.GetCount();

											 if (nbPositions == 0)
												 continue;

											 CDataArray2DVector3f::Accessor outAcc = outData.Resize(it, nbPositions);

											 // loop over all particle's indices
											 for (LLONG i = 0; i < nbPositions; ++i)
											 {
												 actCoord.setX(inAcc[i].GetX());
												 actCoord.setY(inAcc[i].GetY());
												 actCoord.setZ(inAcc[i].GetZ());

												 const float minx = acc.getValue(actCoord - openvdb::Coord(1, 0, 0));
												 const float maxx = acc.getValue(actCoord + openvdb::Coord(1, 0, 0));

												 const float miny = acc.getValue(actCoord - openvdb::Coord(0, 1, 0));
												 const float maxy = acc.getValue(actCoord + openvdb::Coord(0, 1, 0));

												 const float minz = acc.getValue(actCoord - openvdb::Coord(0, 0, 1));
												 const float maxz = acc.getValue(actCoord + openvdb::Coord(0, 0, 1));

												 const float depth = acc.getValue(actCoord);

												 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
												 grad.normalize();
												 grad *= -depth;

												 outAcc[i].Set(grad.x(), grad.y(), grad.z());
											 }
										 };

									 }
									 else
									 {
										 // interpolator
										 openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

										 // loop over all particles and sample voxels by position
										 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
										 {
											 CDataArray2DVector3f::Accessor inAcc = inPos[it];
											 LLONG nbPositions = inAcc.GetCount();

											 if (nbPositions == 0)
												 continue;

											 CDataArray2DVector3f::Accessor outAcc = outData.Resize(it, nbPositions);
											 const float voxelSize = castedGridRef->voxelSize().x();

											 // loop over all particle's positions
											 for (LLONG i = 0; i<nbPositions; ++i)
											 {
												 openvdb::Vec3s centerPos = openvdb::Vec3f(inAcc[i].GetX(), inAcc[i].GetY(), inAcc[i].GetZ());

												 const float minx = interpolator.wsSample(centerPos - openvdb::Vec3s(voxelSize, 0, 0));
												 const float maxx = interpolator.wsSample(centerPos + openvdb::Vec3s(voxelSize, 0, 0));

												 const float miny = interpolator.wsSample(centerPos - openvdb::Vec3s(0, voxelSize, 0));
												 const float maxy = interpolator.wsSample(centerPos + openvdb::Vec3s(0, voxelSize, 0));

												 const float minz = interpolator.wsSample(centerPos - openvdb::Vec3s(0, 0, voxelSize));
												 const float maxz = interpolator.wsSample(centerPos + openvdb::Vec3s(0, 0, voxelSize));

												 const float depth = interpolator.wsSample(centerPos);

												 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
												 grad.normalize();
												 grad *= -depth;

												 outAcc[i].Set(grad.x(), grad.y(), grad.z());
											 }
										 }
									 }
								 }
								 else
								 {
									 Application().LogMessage(L"[VDB][GETVALAT]: Trying to get non-float grid SDF vector!", siWarningMsg);		return CStatus::OK;
								 }

							 }
							 else
							 {
								 CDataArrayVector3f outData(in_ctxt);
								 // non-array ctxt
								 if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>())
								 {
									 openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
									 if (!castedGridRef)
									 {
										 Application().LogMessage(L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg);		return CStatus::OK;
									 }
									 openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


									 CDataArrayVector3f inPos(in_ctxt, ID_IN_Positions);
									 CIndexSet indexSet(in_ctxt, ID_IN_Positions);

									 if (convertToIdxSpace)
									 {
										 // current coordinate
										 openvdb::Coord actCoord;

										 // loop over all particle's indices
										 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
										 {
											 
											 actCoord.setX(inPos[it].GetX());
											 actCoord.setY(inPos[it].GetY());
											 actCoord.setZ(inPos[it].GetZ());

											 const float minx = acc.getValue(actCoord - openvdb::Coord(1, 0, 0));
											 const float maxx = acc.getValue(actCoord + openvdb::Coord(1, 0, 0));

											 const float miny = acc.getValue(actCoord - openvdb::Coord(0, 1, 0));
											 const float maxy = acc.getValue(actCoord + openvdb::Coord(0, 1, 0));

											 const float minz = acc.getValue(actCoord - openvdb::Coord(0, 0, 1));
											 const float maxz = acc.getValue(actCoord + openvdb::Coord(0, 0, 1));

											 const float depth = acc.getValue(actCoord);

											 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
											 grad.normalize();
											 grad *= -depth;

											 outData[it].Set(grad.x(), grad.y(), grad.z());

										 }

									 }
									 else
									 {
										 // interpolator
										 openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());
										 const float voxelSize = castedGridRef->voxelSize().x();

										 // loop over all particles and sample voxels by position
										 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
										 {
											 
											 openvdb::Vec3s centerPos = openvdb::Vec3f(inPos[it].GetX(), inPos[it].GetY(), inPos[it].GetZ());

											 const float minx = interpolator.wsSample(centerPos - openvdb::Vec3s(voxelSize, 0, 0));
											 const float maxx = interpolator.wsSample(centerPos + openvdb::Vec3s(voxelSize, 0, 0));

											 const float miny = interpolator.wsSample(centerPos - openvdb::Vec3s(0, voxelSize, 0));
											 const float maxy = interpolator.wsSample(centerPos + openvdb::Vec3s(0, voxelSize, 0));

											 const float minz = interpolator.wsSample(centerPos - openvdb::Vec3s(0, 0, voxelSize));
											 const float maxz = interpolator.wsSample(centerPos + openvdb::Vec3s(0, 0, voxelSize));

											 const float depth = interpolator.wsSample(centerPos);

											 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
											 grad.normalize();
											 grad *= -depth;

											 outData[it].Set(grad.x(), grad.y(), grad.z());

										 }
									 }

								 }
								 else
								 {
									 Application().LogMessage(L"[VDB][GETVALAT]:  Trying to get non-float grid SDF vector!", siWarningMsg);		return CStatus::OK;
								 }

							 }

							 return CStatus::OK;
		} break;


			case ID_OUT_SDFDirectionNorm:
			{


									 // get input grid
									 CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
									 CDataArrayBool inUseIndexSpace(in_ctxt, ID_IN_Space);
									 bool convertToIdxSpace = inUseIndexSpace[0];
									 VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic(inVDBGridPort);
									 if (p_inWrapper == NULL || !p_inWrapper->m_grid)
									 {
										 Application().LogMessage(L"[VDB][GETVALAT]: Empty grid on input");
										 return CStatus::OK;
									 };

									 if (IsPortArray(in_ctxt, ID_IN_Positions))
									 {
										 CDataArray2DVector3f outData(in_ctxt);
										 // ##################################################################################################
										 // float
										 if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>())
										 {
											 openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
											 if (!castedGridRef)
											 {
												 Application().LogMessage(L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg);		return CStatus::OK;
											 }
											 openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

											 CDataArray2DVector3f inPos(in_ctxt, ID_IN_Positions);
											 CIndexSet indexSet(in_ctxt, ID_IN_Positions);

											 if (convertToIdxSpace)
											 {
												 // current coordinate
												 openvdb::Coord actCoord;

												 // loop over all particles and directly visit voxels by index
												 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
												 {
													 CDataArray2DVector3f::Accessor inAcc = inPos[it];
													 LLONG nbPositions = inAcc.GetCount();

													 if (nbPositions == 0)
														 continue;

													 CDataArray2DVector3f::Accessor outAcc = outData.Resize(it, nbPositions);

													 // loop over all particle's indices
													 for (LLONG i = 0; i < nbPositions; ++i)
													 {
														 actCoord.setX(inAcc[i].GetX());
														 actCoord.setY(inAcc[i].GetY());
														 actCoord.setZ(inAcc[i].GetZ());

														 const float minx = acc.getValue(actCoord - openvdb::Coord(1, 0, 0));
														 const float maxx = acc.getValue(actCoord + openvdb::Coord(1, 0, 0));

														 const float miny = acc.getValue(actCoord - openvdb::Coord(0, 1, 0));
														 const float maxy = acc.getValue(actCoord + openvdb::Coord(0, 1, 0));

														 const float minz = acc.getValue(actCoord - openvdb::Coord(0, 0, 1));
														 const float maxz = acc.getValue(actCoord + openvdb::Coord(0, 0, 1));

													

														 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
														 grad.normalize();
														 grad *= 1.f;

														 outAcc[i].Set(grad.x(), grad.y(), grad.z());
													 }
												 };

											 }
											 else
											 {
												 // interpolator
												 openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

												 // loop over all particles and sample voxels by position
												 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
												 {
													 CDataArray2DVector3f::Accessor inAcc = inPos[it];
													 LLONG nbPositions = inAcc.GetCount();

													 if (nbPositions == 0)
														 continue;

													 CDataArray2DVector3f::Accessor outAcc = outData.Resize(it, nbPositions);
													 const float voxelSize = castedGridRef->voxelSize().x();

													 // loop over all particle's positions
													 for (LLONG i = 0; i<nbPositions; ++i)
													 {
														 openvdb::Vec3s centerPos = openvdb::Vec3f(inAcc[i].GetX(), inAcc[i].GetY(), inAcc[i].GetZ());

														 const float minx = interpolator.wsSample(centerPos - openvdb::Vec3s(voxelSize, 0, 0));
														 const float maxx = interpolator.wsSample(centerPos + openvdb::Vec3s(voxelSize, 0, 0));

														 const float miny = interpolator.wsSample(centerPos - openvdb::Vec3s(0, voxelSize, 0));
														 const float maxy = interpolator.wsSample(centerPos + openvdb::Vec3s(0, voxelSize, 0));

														 const float minz = interpolator.wsSample(centerPos - openvdb::Vec3s(0, 0, voxelSize));
														 const float maxz = interpolator.wsSample(centerPos + openvdb::Vec3s(0, 0, voxelSize));

													
														 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
														 grad.normalize();
														 grad *= 1.f;

														 outAcc[i].Set(grad.x(), grad.y(), grad.z());
													 }
												 }
											 }
										 }
										 else
										 {
											 Application().LogMessage(L"[VDB][GETVALAT]: Trying to get non-float grid SDF direction!", siWarningMsg);		return CStatus::OK;
										 }

									 }
									 else
									 {
										 CDataArrayVector3f outData(in_ctxt);
										 // non-array ctxt
										 if (p_inWrapper->m_grid->isType<openvdb::FloatGrid>())
										 {
											 openvdb::FloatGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::FloatGrid>(p_inWrapper->m_grid);
											 if (!castedGridRef)
											 {
												 Application().LogMessage(L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg);		return CStatus::OK;
											 }
											 openvdb::FloatGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


											 CDataArrayVector3f inPos(in_ctxt, ID_IN_Positions);
											 CIndexSet indexSet(in_ctxt, ID_IN_Positions);

											 if (convertToIdxSpace)
											 {
												 // current coordinate
												 openvdb::Coord actCoord;

												 // loop over all particle's indices
												 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
												 {

													 actCoord.setX(inPos[it].GetX());
													 actCoord.setY(inPos[it].GetY());
													 actCoord.setZ(inPos[it].GetZ());

													 const float minx = acc.getValue(actCoord - openvdb::Coord(1, 0, 0));
													 const float maxx = acc.getValue(actCoord + openvdb::Coord(1, 0, 0));

													 const float miny = acc.getValue(actCoord - openvdb::Coord(0, 1, 0));
													 const float maxy = acc.getValue(actCoord + openvdb::Coord(0, 1, 0));

													 const float minz = acc.getValue(actCoord - openvdb::Coord(0, 0, 1));
													 const float maxz = acc.getValue(actCoord + openvdb::Coord(0, 0, 1));

													
													 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
													 grad.normalize();
													 grad *= 1.f;
													 outData[it].Set(grad.x(), grad.y(), grad.z());

												 }

											 }
											 else
											 {
												 // interpolator
												 openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());
												 const float voxelSize = castedGridRef->voxelSize().x();

												 // loop over all particles and sample voxels by position
												 for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
												 {

													 openvdb::Vec3s centerPos = openvdb::Vec3f(inPos[it].GetX(), inPos[it].GetY(), inPos[it].GetZ());

													 const float minx = interpolator.wsSample(centerPos - openvdb::Vec3s(voxelSize, 0, 0));
													 const float maxx = interpolator.wsSample(centerPos + openvdb::Vec3s(voxelSize, 0, 0));

													 const float miny = interpolator.wsSample(centerPos - openvdb::Vec3s(0, voxelSize, 0));
													 const float maxy = interpolator.wsSample(centerPos + openvdb::Vec3s(0, voxelSize, 0));

													 const float minz = interpolator.wsSample(centerPos - openvdb::Vec3s(0, 0, voxelSize));
													 const float maxz = interpolator.wsSample(centerPos + openvdb::Vec3s(0, 0, voxelSize));

													 openvdb::Vec3s grad(maxx - minx, maxy - miny, maxz - minz);
													 grad.normalize();
													 grad *= 1.f;

													 outData[it].Set(grad.x(), grad.y(), grad.z());

												 }
											 }

										 }
										 else
										 {
											 Application().LogMessage(L"[VDB][GETVALAT]:  Trying to get non-float grid SDF direction!", siWarningMsg);		return CStatus::OK;
										 }

									 }

									 return CStatus::OK;
			} break;


		









		case ID_OUT_Vec3f:
		{
			

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			CDataArrayBool inUseIndexSpace (in_ctxt, ID_IN_Space);
			bool convertToIdxSpace = inUseIndexSpace[0];
			VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic ( inVDBGridPort );
			if (p_inWrapper==NULL||! p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][GETVALAT]: Empty grid on input" );
				return CStatus::OK;
			};

			if ( IsPortArray ( in_ctxt, ID_IN_Positions ) )
			{
				CDataArray2DVector3f outData ( in_ctxt);
				// ##################################################################################################
				// float
				if (  p_inWrapper->m_grid->isType<openvdb::Vec3SGrid>() )
				{
					openvdb::Vec3SGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::Vec3SGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::Vec3SGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

					CDataArray2DVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particles and directly visit voxels by index
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DVector3f::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's indices
							for ( LLONG i=0;i<nbPositions; ++i )
							{
								actCoord.setX ( inAcc[i].GetX() );
								actCoord.setY ( inAcc[i].GetY() );
								actCoord.setZ ( inAcc[i].GetZ() );
								openvdb::Vec3s v =	acc.getValue( actCoord );
								outAcc[i] .Set ( v.x(), v.y(), v.z() );
							}
						};

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::Vec3STree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DVector3f::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's positions
							for ( LLONG i=0;i<nbPositions; ++i )	
							{
								 openvdb::Vec3s v =interpolator.wsSample( openvdb::Vec3f ( inAcc[i].GetX(), inAcc[i].GetY(), inAcc[i].GetZ() ));									
								outAcc[i] .Set ( v.x(), v.y(), v.z() );
							}
						}
					}
				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-vec3s grid value on vec3s outport!", siWarningMsg );		return CStatus::OK;	}

			}
			else
			{
				CDataArrayVector3f outData ( in_ctxt);
				// non-array ctxt
				if (  p_inWrapper->m_grid->isType<openvdb::Vec3SGrid>() )
				{
					openvdb::Vec3SGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::Vec3SGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::Vec3SGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CDataArrayVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particle's indices
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							actCoord.setX ( inPos[it].GetX() );
							actCoord.setY ( inPos[it].GetY() );
							actCoord.setZ ( inPos[it].GetZ() );
							  openvdb::Vec3s v = acc.getValue( actCoord );
							  outData[it] .Set (  v.x(), v.y(), v.z() );
						}

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::Vec3STree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							  openvdb::Vec3s v =  interpolator.wsSample( openvdb::Vec3f ( inPos[it].GetX(), inPos[it].GetY(), inPos[it].GetZ() ));
							outData[it].Set  (  v.x(), v.y(), v.z() );
						}
					}

				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-vec3s grid value on vec3s outport!", siWarningMsg );		return CStatus::OK;	}
				
			}

			return CStatus::OK;
		} break;
		case ID_OUT_Bool:
		{
			

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			CDataArrayBool inUseIndexSpace (in_ctxt, ID_IN_Space);
			bool convertToIdxSpace = inUseIndexSpace[0];
			VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic ( inVDBGridPort );
			if (p_inWrapper==NULL ||! p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][GETVALAT]: Empty grid on input" );
				return CStatus::OK;
			};

			if ( IsPortArray ( in_ctxt, ID_IN_Positions ) )
			{
				CDataArray2DBool outData ( in_ctxt);
				// ##################################################################################################
				// float
				if (  p_inWrapper->m_grid->isType<openvdb::BoolGrid>() )
				{
					openvdb::BoolGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::BoolGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::BoolGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

					CDataArray2DVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particles and directly visit voxels by index
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DBool::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's indices
							for ( LLONG i=0;i<nbPositions; ++i )
							{
								actCoord.setX ( inAcc[i].GetX() );
								actCoord.setY ( inAcc[i].GetY() );
								actCoord.setZ ( inAcc[i].GetZ() );
								outAcc.Set (i, acc.getValue( actCoord ));
							}
						};

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::BoolTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DBool::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's positions
							for ( LLONG i=0;i<nbPositions; ++i )				
								outAcc.Set (i, interpolator.wsSample( openvdb::Vec3f ( inAcc[i].GetX(), inAcc[i].GetY(), inAcc[i].GetZ() )));
						}
					}
				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-bool grid value on bool outport!", siWarningMsg );		return CStatus::OK;	}

			}
			else
			{
				CDataArrayBool outData ( in_ctxt);
				// non-array ctxt
				if (  p_inWrapper->m_grid->isType<openvdb::BoolGrid>() )
				{
					openvdb::BoolGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::BoolGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::BoolGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CDataArrayVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particle's indices
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							actCoord.setX ( inPos[it].GetX() );
							actCoord.setY ( inPos[it].GetY() );
							actCoord.setZ ( inPos[it].GetZ() );
							outData.Set (it, acc.getValue( actCoord ));
						}

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::BoolTree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
							outData.Set (it,interpolator.wsSample( openvdb::Vec3f ( inPos[it].GetX(), inPos[it].GetY(), inPos[it].GetZ() )));
					}

				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-bool grid value on bool outport!", siWarningMsg );		return CStatus::OK;	}
				
			}

			return CStatus::OK;
		} break;


		case ID_OUT_Long:
		{
			

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			CDataArrayBool inUseIndexSpace (in_ctxt, ID_IN_Space);
			bool convertToIdxSpace = inUseIndexSpace[0];
			VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic ( inVDBGridPort );
			if (p_inWrapper==NULL||! p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][GETVALAT]: Empty grid on input" );
				return CStatus::OK;
			};

			if ( IsPortArray ( in_ctxt, ID_IN_Positions ) )
			{
				CDataArray2DLong outData ( in_ctxt);
				// ##################################################################################################
				// float
				if (  p_inWrapper->m_grid->isType<openvdb::FloatGrid>() )
				{
					openvdb::Int32Grid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::Int32Grid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::Int32Grid::ConstAccessor acc = castedGridRef->getConstAccessor();

					CDataArray2DVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particles and directly visit voxels by index
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DLong::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's indices
							for ( LLONG i=0;i<nbPositions; ++i )
							{
								actCoord.setX ( inAcc[i].GetX() );
								actCoord.setY ( inAcc[i].GetY() );
								actCoord.setZ ( inAcc[i].GetZ() );
								outAcc[i] = acc.getValue( actCoord );
							}
						};

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::Int32Tree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DLong::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's positions
							for ( LLONG i=0;i<nbPositions; ++i )				
								outAcc[i] = interpolator.wsSample( openvdb::Vec3f ( inAcc[i].GetX(), inAcc[i].GetY(), inAcc[i].GetZ() ));
						}
					}
				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-int32 grid value on int32 outport!", siWarningMsg );		return CStatus::OK;	}

			}
			else
			{
				CDataArrayLong outData ( in_ctxt);
				// non-array ctxt
				if (  p_inWrapper->m_grid->isType<openvdb::Int32Grid>() )
				{
					openvdb::Int32Grid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::Int32Grid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::Int32Grid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CDataArrayVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particle's indices
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							actCoord.setX ( inPos[it].GetX() );
							actCoord.setY ( inPos[it].GetY() );
							actCoord.setZ ( inPos[it].GetZ() );
							outData[it] = acc.getValue( actCoord );
						}

					}
					else
					{
						// interpolator
						openvdb::tools::GridSampler<openvdb::Int32Tree, openvdb::tools::BoxSampler>  interpolator(castedGridRef->constTree(), castedGridRef->transform());

						// loop over all particles and sample voxels by position
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
							outData[it] = interpolator.wsSample( openvdb::Vec3f ( inPos[it].GetX(), inPos[it].GetY(), inPos[it].GetZ() ));
					}

				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-int32 grid value on int32 outport!", siWarningMsg );		return CStatus::OK;	}
				
			}

			return CStatus::OK;
		} break;



		case ID_OUT_String:
		{
			

			// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			CDataArrayBool inUseIndexSpace (in_ctxt, ID_IN_Space);
			bool convertToIdxSpace = inUseIndexSpace[0];
			VDB_ICEFlowDataWrapper_t * p_inWrapper = VDB_ICENode_cacheBase_t::UnpackGridStatic ( inVDBGridPort );
			if (p_inWrapper==NULL ||! p_inWrapper->m_grid )
			{
				Application().LogMessage ( L"[VDB][GETVALAT]: Empty grid on input" );
				return CStatus::OK;
			};

			if ( IsPortArray ( in_ctxt, ID_IN_Positions ) )
			{
				CDataArray2DString outData ( in_ctxt);
				// ##################################################################################################
				// float
				if (  p_inWrapper->m_grid->isType<openvdb::BoolGrid>() )
				{
					openvdb::StringGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::StringGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::StringGrid::ConstAccessor acc = castedGridRef->getConstAccessor();

					CDataArray2DVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particles and directly visit voxels by index
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DString::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's indices
							for ( LLONG i=0;i<nbPositions; ++i )
							{
								actCoord.setX ( inAcc[i].GetX() );
								actCoord.setY ( inAcc[i].GetY() );
								actCoord.setZ ( inAcc[i].GetZ() );
								outAcc.SetData (i, acc.getValue( actCoord ).c_str());
							}
						};

					}
					else // for string there is no interpolation
					{ 

						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particles and directly visit voxels by index
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor inAcc = inPos[it];
							LLONG nbPositions = inAcc.GetCount();

							if ( nbPositions == 0 )
								continue;

							CDataArray2DString::Accessor outAcc =outData.Resize ( it, nbPositions );

							// loop over all particle's indices
							for ( LLONG i=0;i<nbPositions; ++i )
							{
								actCoord.setX ( inAcc[i].GetX() );
								actCoord.setY ( inAcc[i].GetY() );
								actCoord.setZ ( inAcc[i].GetZ() );
								openvdb::Vec3d transformed =	castedGridRef->indexToWorld ( actCoord );
								actCoord.setX( transformed.x());
								actCoord.setY( transformed.y());
								actCoord.setZ( transformed.z());
								outAcc.SetData (i, acc.getValue( actCoord ).c_str());
							}
						};

					}
				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-string grid value on string outport!", siWarningMsg );		return CStatus::OK;	}

			}
			else
			{
				CDataArrayString outData ( in_ctxt);
				// non-array ctxt
				if (  p_inWrapper->m_grid->isType<openvdb::StringGrid>() )
				{
					openvdb::StringGrid::Ptr castedGridRef = openvdb::gridPtrCast<openvdb::StringGrid>( p_inWrapper->m_grid);
					if (!castedGridRef)
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Unable to get derived grid, bypassed", siErrorMsg );		return CStatus::OK;	}
					openvdb::StringGrid::ConstAccessor acc = castedGridRef->getConstAccessor();


					CDataArrayVector3f inPos ( in_ctxt, ID_IN_Positions );
					CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

					if ( convertToIdxSpace )
					{
						// current coordinate
						openvdb::Coord actCoord;

						// loop over all particle's indices
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							actCoord.setX ( inPos[it].GetX() );
							actCoord.setY ( inPos[it].GetY() );
							actCoord.setZ ( inPos[it].GetZ() );

							outData.SetData (it, acc.getValue( actCoord ).c_str());
						}

					}
					else // for string there is no interpolation
					{
							// current coordinate
						openvdb::Coord actCoord;

						// loop over all particle's indices
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )	
						{
							actCoord.setX ( inPos[it].GetX() );
							actCoord.setY ( inPos[it].GetY() );
							actCoord.setZ ( inPos[it].GetZ() );
							openvdb::Vec3d transformed =	castedGridRef->indexToWorld ( actCoord );
								actCoord.setX( transformed.x());
								actCoord.setY( transformed.y());
								actCoord.setZ( transformed.z());
							outData.SetData (it, acc.getValue( actCoord ).c_str());
						}

					}

				}
				else
					{ Application().LogMessage ( L"[VDB][GETVALAT]: Trying to get non-string grid value on string outport!", siWarningMsg );		return CStatus::OK;	}
				
			}

			return CStatus::OK;
		} break;


	default:
		break;

		return CStatus::OK;
	};

return CStatus::OK;
};





