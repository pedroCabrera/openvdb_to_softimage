#include "Main.h"
#include "vdbHelpers.h"

#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/util/Util.h>

// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{

	ID_IN_VDBGrid ,
	ID_IN_Positions,
	ID_IN_Space ,
	ID_IN_Value,

	ID_G_100 = 100,

	ID_OUT_VDBGrid = 200,


	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};



CStatus VDB_SetValueAtCoordinate_Register(PluginRegistrar& reg)
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_SetValueAtCoordinate", L"VDB Set Value At Coordinate");

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


	int valType = siICENodeDataFloat | siICENodeDataBool | siICENodeDataLong | siICENodeDataVector3 | siICENodeDataString ;


	st = nodeDef.AddInputPort(ID_IN_VDBGrid, ID_G_100, customTypes, siICENodeStructureSingle, siICENodeContextSingleton, L"VDBGrid", L"VDBGrid",ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Positions, ID_G_100, siICENodeDataVector3, siICENodeStructureAny, siICENodeContextAny, L"Positions", L"Positions",CValue(),CValue(),CValue(),ID_UNDEF,ID_STRUCT_CNS,ID_CTXT_CNS);  st.AssertSucceeded();
	  st = nodeDef.AddInputPort(ID_IN_Space, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"IndexSpace", L"IndexSpace",false,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
st = nodeDef.AddInputPort(ID_IN_Value, ID_G_100, valType, siICENodeStructureAny, siICENodeContextAny, L"Value", L"Value",CValue(),CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);  st.AssertSucceeded();
	

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");st.AssertSucceeded();

	PluginItem nodeItem = reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}



struct VDB_SetValueAtCoordinate_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_SetValueAtCoordinate_cache_t ( ) { };

	clock_t m_inputEvalTime;
	short m_dataType;
	std::vector<MATH::CVector3f> m_pos;
	std::vector<CValue> m_vals;
	bool m_indexSpace;

	inline bool IsDirty (  short dataType, clock_t time, std::vector<MATH::CVector3f> & pos,  std::vector<CValue> & vals, bool idxsp )
	{
		bool isClean = ( m_dataType== dataType  && m_inputEvalTime==time
			&&  pos.size()==m_pos.size() && memcmp (&(m_pos[0]),&(pos[0]),sizeof(MATH::CVector3f)*m_pos.size() )==0
			&& m_vals .size()== vals.size() && m_indexSpace == idxsp );

		// additional check for values
	if (  isClean==true )
		for ( LONG it=0; it<vals.size(); ++it )
			if (m_vals[it]==vals[it])
			{
				isClean=false; 
				break;
			}


		if ( isClean == false )
		{
			m_indexSpace = idxsp;
		m_dataType= dataType ; 
		m_inputEvalTime=time;

		m_pos.swap ( pos );
		m_vals.swap ( vals );
		};

		return !isClean;

	};

	inline void Reassign (  VDB_ICEFlowDataWrapper_t * p_inWrapper  )
	{
		TIMER timer;

		openvdb::GridBase::Ptr tempBaseGrid = p_inWrapper->m_grid->deepCopyGrid ();
		openvdb::math::Coord actCoord;

		switch (m_dataType)
		{
		case 0: // float
			{
				openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid> ( tempBaseGrid );
				LONG nbVals = m_vals.size ();
				auto gridAcc = 	grid->getAccessor ( );
				if ( m_indexSpace )				
					for ( LONG it=0; it<nbVals; ++ it )
					{
						actCoord.setX ( m_pos[it].GetX() );
						actCoord.setY ( m_pos[it].GetY() );
						actCoord.setZ ( m_pos[it].GetZ() );
						gridAcc.setValue (  actCoord, float(m_vals[it]));
					}		
				else
					for ( LONG it=0; it<nbVals; ++ it )
					{
						openvdb::Vec3d vec3d ( m_pos[it].GetX(),m_pos[it].GetY(),m_pos[it].GetZ() );
						vec3d =	grid->worldToIndex (vec3d );
						actCoord.setX ( vec3d.x() );
						actCoord.setY ( vec3d.y() );
						actCoord.setZ ( vec3d.z() );
						gridAcc.setValue (  actCoord, float(m_vals[it]));
					};


				m_primaryGrid.m_grid = grid;
			}break;
				case 1: // bool 
			{
					openvdb::BoolGrid::Ptr grid = openvdb::gridPtrCast<openvdb::BoolGrid> ( tempBaseGrid );
					LONG nbVals = m_vals.size ();
						auto gridAcc = 	grid->getAccessor ( );
				if ( m_indexSpace )				
					for ( LONG it=0; it<nbVals; ++ it )
					{
						actCoord.setX ( m_pos[it].GetX() );
						actCoord.setY ( m_pos[it].GetY() );
						actCoord.setZ ( m_pos[it].GetZ() );
						gridAcc.setValue (  actCoord, bool(m_vals[it]));
					}		
				else
					for ( LONG it=0; it<nbVals; ++ it )
					{
						openvdb::Vec3d vec3d ( m_pos[it].GetX(),m_pos[it].GetY(),m_pos[it].GetZ() );
						vec3d =	grid->worldToIndex (vec3d );
						actCoord.setX ( vec3d.x() );
						actCoord.setY ( vec3d.y() );
						actCoord.setZ ( vec3d.z() );
						gridAcc.setValue (  actCoord, bool(m_vals[it]));
					};
				m_primaryGrid.m_grid = grid;
			}break;
	case 2: // vec3
			{
					openvdb::Vec3SGrid::Ptr grid = openvdb::gridPtrCast<openvdb::Vec3SGrid> ( tempBaseGrid );
					LONG nbVals = m_vals.size ();
						auto gridAcc = 	grid->getAccessor ( );
				if ( m_indexSpace )				
					for ( LONG it=0; it<nbVals; ++ it )
					{
						actCoord.setX ( m_pos[it].GetX() );
						actCoord.setY ( m_pos[it].GetY() );
						actCoord.setZ ( m_pos[it].GetZ() );
						MATH::CVector3f v3tmp(m_vals[it]);
						gridAcc.setValue (  actCoord, openvdb::Vec3s (v3tmp.GetX(),v3tmp.GetY(),v3tmp.GetZ()) );
					}		
				else
					for ( LONG it=0; it<nbVals; ++ it )
					{
						openvdb::Vec3d vec3d ( m_pos[it].GetX(),m_pos[it].GetY(),m_pos[it].GetZ() );
						vec3d =	grid->worldToIndex (vec3d );
						actCoord.setX ( vec3d.x() );
						actCoord.setY ( vec3d.y() );
						actCoord.setZ ( vec3d.z() );
						MATH::CVector3f v3tmp(m_vals[it]);
						gridAcc.setValue (  actCoord, openvdb::Vec3s (v3tmp.GetX(),v3tmp.GetY(),v3tmp.GetZ()) );
					};
				m_primaryGrid.m_grid = grid;
			}break;
	case 3: // str
			{	
				openvdb::StringGrid::Ptr grid = openvdb::gridPtrCast<openvdb::StringGrid> ( tempBaseGrid );
				LONG nbVals = m_vals.size ();
					auto gridAcc = 	grid->getAccessor ( );
				if ( m_indexSpace )				
					for ( LONG it=0; it<nbVals; ++ it )
					{
						actCoord.setX ( m_pos[it].GetX() );
						actCoord.setY ( m_pos[it].GetY() );
						actCoord.setZ ( m_pos[it].GetZ() );
						gridAcc.setValue (  actCoord, CString(m_vals[it]).GetAsciiString());
					}		
				else
					for ( LONG it=0; it<nbVals; ++ it )
					{
						openvdb::Vec3d vec3d ( m_pos[it].GetX(),m_pos[it].GetY(),m_pos[it].GetZ() );
						vec3d =	grid->worldToIndex (vec3d );
						actCoord.setX ( vec3d.x() );
						actCoord.setY ( vec3d.y() );
						actCoord.setZ ( vec3d.z() );
						gridAcc.setValue (  actCoord, CString(m_vals[it]).GetAsciiString());
					};
				m_primaryGrid.m_grid = grid;

			}break;
				case 4: // long
			{
					openvdb::Int32Grid::Ptr grid = openvdb::gridPtrCast<openvdb::Int32Grid> ( tempBaseGrid );
					LONG nbVals = m_vals.size ();
						auto gridAcc = 	grid->getAccessor ( );
				if ( m_indexSpace )				
					for ( LONG it=0; it<nbVals; ++ it )
					{
						actCoord.setX ( m_pos[it].GetX() );
						actCoord.setY ( m_pos[it].GetY() );
						actCoord.setZ ( m_pos[it].GetZ() );
						gridAcc.setValue (  actCoord, LONG(m_vals[it]));
					}		
				else
					for ( LONG it=0; it<nbVals; ++ it )
					{
						openvdb::Vec3d vec3d ( m_pos[it].GetX(),m_pos[it].GetY(),m_pos[it].GetZ() );
						vec3d =	grid->worldToIndex (vec3d );
						actCoord.setX ( vec3d.x() );
						actCoord.setY ( vec3d.y() );
						actCoord.setZ ( vec3d.z() );
						gridAcc.setValue (  actCoord, LONG(m_vals[it]));
					};
					m_primaryGrid.m_grid = grid;
			}break;

		default:
			break;
		}



				m_primaryGrid.m_lastEvalTime = clock();
		Application().LogMessage(L"[VDB][SETVALAT]: Stamped at=" + CString(m_primaryGrid.m_lastEvalTime));
		Application().LogMessage(L"[VDB][SETVALAT]: Done in=" + CString(timer.GetElapsedTime()));
	};


};


SICALLBACK VDB_SetValueAtCoordinate_Evaluate(ICENodeContext& in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{
	case ID_OUT_VDBGrid:
		{
			CDataArrayCustomType outData(in_ctxt);

			// get cache object
			VDB_SetValueAtCoordinate_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][SETVALAT]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_SetValueAtCoordinate_cache_t*)(CValue::siPtrType)userData;


		
						// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][SETVALAT]: Empty grid on input" );
				return CStatus::OK;
			};

			p_cacheNodeObject->PackGrid ( outData );

			// obtain inports structs
			XSI::siICENodeDataType param1;
			XSI::siICENodeDataType valPortDataType;
			XSI::siICENodeStructureType valPortDataStruct;
			XSI::siICENodeStructureType posPortDataStruct;
			XSI::siICENodeContextType param3;
			in_ctxt.GetPortInfo( ID_IN_Value ,    valPortDataType, valPortDataStruct, param3 );
			in_ctxt.GetPortInfo( ID_IN_Positions ,         param1, posPortDataStruct, param3 ); // positions has higher priority since it can be different and values can be singleton

			if (  posPortDataStruct != valPortDataStruct )
			{
				p_cacheNodeObject->ResetGridHolder ( );
				Application().LogMessage  ( L"[VDB][SETVALAT]: Position and value ports structure mismatch!" ,siWarningMsg);
				return CStatus::OK;
			};

			CDataArrayBool inUseIndexSpace ( in_ctxt, ID_IN_Space );

			std::vector<CValue>  vals;
			std::vector<MATH::CVector3f> pos;
			short icedatatype=-1;

			if ( posPortDataStruct == XSI::siICENodeStructureType::siICENodeStructureArray )
			{
				CIndexSet indexSet ( in_ctxt, ID_IN_Positions );
				CDataArray2DVector3f inPos (in_ctxt, ID_IN_Positions );
				switch (valPortDataType)
				{

				case siICENodeDataType::siICENodeDataFloat :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArray2DFloat inVal( in_ctxt, ID_IN_Value );
						pos.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						vals.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor posAcc = inPos[it];
							CDataArray2DFloat::Accessor inAcc = inVal[it];

							if ( posAcc.GetCount () != inAcc.GetCount () )
								continue; // do not log, just skip

							for ( LONG j=0;j<posAcc.GetCount (); ++j )
							{
								pos.push_back ( posAcc[j] );
								vals.push_back ( inAcc[j] );
							}
						}

						icedatatype = 0;
					}break;
				case siICENodeDataType::siICENodeDataBool :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::BoolGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArray2DBool  inVal( in_ctxt, ID_IN_Value );
									pos.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						vals.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor posAcc = inPos[it];
							CDataArray2DBool::Accessor inAcc = inVal[it];

							if ( posAcc.GetCount () != inAcc.GetCount () )
								continue; // do not log, just skip

							for ( LONG j=0;j<posAcc.GetCount (); ++j )
							{
								pos.push_back ( posAcc[j] );
								vals.push_back ( inAcc[j] );
							}
						}
						icedatatype =1;
					}break;
				case siICENodeDataType::siICENodeDataVector3 :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::Vec3SGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArray2DVector3f  inVal( in_ctxt, ID_IN_Value );
									pos.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						vals.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor posAcc = inPos[it];
							CDataArray2DVector3f::Accessor inAcc = inVal[it];

							if ( posAcc.GetCount () != inAcc.GetCount () )
								continue; // do not log, just skip

							for ( LONG j=0;j<posAcc.GetCount (); ++j )
							{
								pos.push_back ( posAcc[j] );
								vals.push_back ( inAcc[j] );
							}
						}
						icedatatype =2;
					}break;
				case siICENodeDataType::siICENodeDataString :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::StringGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArray2DString  inVal( in_ctxt, ID_IN_Value );
										pos.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						vals.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor posAcc = inPos[it];
							CDataArray2DString::Accessor inAcc = inVal[it];

							if ( posAcc.GetCount () != inAcc.GetCount () )
								continue; // do not log, just skip

							for ( LONG j=0;j<posAcc.GetCount (); ++j )
							{
								pos.push_back ( posAcc[j] );
								vals.push_back ( inAcc[j] );
							}
						}
						icedatatype =3;
					}break;
				case siICENodeDataType::siICENodeDataLong :
					{
						if ( p_inWrapper->m_grid->isType<openvdb::Int32Grid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}


						CDataArray2DLong  inVal( in_ctxt, ID_IN_Value );
										pos.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						vals.reserve ( inPos.GetCount()*std::max(1UL,inPos[0].GetCount()) );
						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							CDataArray2DVector3f::Accessor posAcc = inPos[it];
							CDataArray2DLong::Accessor inAcc = inVal[it];

							if ( posAcc.GetCount () != inAcc.GetCount () )
								continue; // do not log, just skip

							for ( LONG j=0;j<posAcc.GetCount (); ++j )
							{
								pos.push_back ( posAcc[j] );
								vals.push_back ( inAcc[j] );
							}
						}
						icedatatype =4;
					}break;
				}
			}
			else
			{
				CIndexSet indexSet ( in_ctxt, ID_IN_Positions );

				CDataArrayVector3f inPos (in_ctxt, ID_IN_Positions );

				pos.reserve ( inPos.GetCount() );
				vals.reserve ( inPos.GetCount() );

				switch (valPortDataType)
				{
				case siICENodeDataType::siICENodeDataFloat :
					{
						if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArrayFloat inVal( in_ctxt, ID_IN_Value );

						for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							pos.push_back ( inPos[it] );
							vals.push_back ( inVal[it] );
						}

						icedatatype = 0;
					}break;
				case siICENodeDataType::siICENodeDataBool :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::BoolGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArrayBool  inVal( in_ctxt, ID_IN_Value );
												for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							pos.push_back ( inPos[it] );
							vals.push_back ( inVal[it] );
						}
						icedatatype =1;
					}break;
				case siICENodeDataType::siICENodeDataVector3 :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::Vec3SGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArrayVector3f  inVal( in_ctxt, ID_IN_Value );
												for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							pos.push_back ( inPos[it] );
							vals.push_back ( inVal[it] );
						}
						icedatatype =2;
					}break;
				case siICENodeDataType::siICENodeDataString :
					{

						if ( p_inWrapper->m_grid->isType<openvdb::StringGrid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArrayString  inVal( in_ctxt, ID_IN_Value );
												for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							pos.push_back ( inPos[it] );
							vals.push_back ( inVal[it] );
						}
						icedatatype =3;
					}break;
				case siICENodeDataType::siICENodeDataLong :
					{
						if ( p_inWrapper->m_grid->isType<openvdb::Int32Grid> () == false )
						{
							p_cacheNodeObject->ResetGridHolder ( );
							Application().LogMessage  ( L"[VDB][SETVALAT]: Grid type and Value type mismatch!" ,siWarningMsg);
							return CStatus::OK;
						}

						CDataArrayLong  inVal( in_ctxt, ID_IN_Value );
												for ( CIndexSet::Iterator it=indexSet.Begin(); it.HasNext(); it.Next() )
						{
							pos.push_back ( inPos[it] );
							vals.push_back ( inVal[it] );
						}
						icedatatype =4;
					}break;
				}
			}


			// compare with prev res
				if  (	p_cacheNodeObject->IsDirty ( icedatatype, p_inWrapper->m_lastEvalTime, pos, vals, inUseIndexSpace[0]) == false )
			{
				Application().LogMessage( L"[VDB][CREATEGRID]: Data is not changed, used prev result" );
				return CStatus::OK;
			};


			// refill
				p_cacheNodeObject->Reassign(p_inWrapper  );



		}break;
	default:
		break;
	};

	return CStatus::OK;
};

// lets cache this
SICALLBACK VDB_SetValueAtCoordinate_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();
	VDB_SetValueAtCoordinate_cache_t * p_gridOwner;
	if (userData.IsEmpty())
	{
		p_gridOwner = new VDB_SetValueAtCoordinate_cache_t;
	}
	else
	{
		Application().LogMessage ( L"[VDB][SETVALAT]: Fatal, unknow data is present on initialization", siFatalMsg );
		return CStatus::Fail;
	}

	ctxt.PutUserData((CValue::siPtrType)p_gridOwner);
	return CStatus::OK;
}



SICALLBACK VDB_SetValueAtCoordinate_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	CValue userData = ctxt.GetUserData();

	if ( userData.IsEmpty () )
	{
		Application().LogMessage ( L"[VDB][SETVALAT]: Fatal, no data on termination", siFatalMsg );
		return CStatus::Fail;
	}

	VDB_SetValueAtCoordinate_cache_t * p_gridOwner;
	p_gridOwner = (VDB_SetValueAtCoordinate_cache_t*)(CValue::siPtrType)userData;


	delete p_gridOwner;
	ctxt.PutUserData(CValue());

	return CStatus::OK;
}
