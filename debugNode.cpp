#include "Main.h"
#include "noises.h"



inline bool IsPortArray ( ICENodeContext & in_ctxt, ULONG in_portID )
{
	// obtain inports structs
	XSI::siICENodeDataType _PORTTYPE;
	XSI::siICENodeStructureType _PORTSTRUCT;
	XSI::siICENodeContextType _PORTCONTEXT;

	in_ctxt.GetPortInfo( in_portID , _PORTTYPE, _PORTSTRUCT, _PORTCONTEXT );

	return  ( _PORTSTRUCT == XSI::siICENodeStructureArray ); 		
};








// Defines port, group and map identifiers used for registering the ICENode
enum IDs
{
	ID_IN_Float1 ,
	ID_IN_Float2 ,
	ID_IN_Float3 ,
	
	ID_IN_Color1 ,
	ID_IN_Color2 ,
	ID_IN_Color3,

	ID_IN_Vec1,
	ID_IN_Vec2,

	ID_G_100 ,


	ID_OUT_Float1,


	ID_TYPE_CNS,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};


CStatus VDB_Debug_Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_Debug", L"VDB_Debug");

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
   st = nodeDef.AddInputPort(ID_IN_Vec1, ID_G_100, siICENodeDataVector3, siICENodeStructureAny, siICENodeContextSingleton, L"Pos", L"Pos",1.f,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
    st = nodeDef.AddInputPort(ID_IN_Float1, ID_G_100, siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton, L"Time", L"Vec3",1.f,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	st = nodeDef.AddInputPort(ID_IN_Vec2, ID_G_100, siICENodeDataVector4, siICENodeStructureSingle, siICENodeContextSingleton, L"params", L"params",MATH::CVector4f(1,0.8,0.8,1),CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();

   // Add output ports.
   st = nodeDef.AddOutputPort(ID_OUT_Float1, siICENodeDataFloat, siICENodeStructureArray, siICENodeContextSingleton,L"noise", L"noise");  st.AssertSucceeded();


   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

   return CStatus::OK;
}


SICALLBACK VDB_Debug_Evaluate( ICENodeContext & in_ctxt)
{


	// The current output port being evaluated...
	ULONG evaluatedPort = in_ctxt.GetEvaluatedOutputPortID();

	switch (evaluatedPort)
	{

		case ID_OUT_Float1:
			{
				CDataArray2DFloat outData ( in_ctxt );
			
				if ( IsPortArray ( in_ctxt, ID_IN_Vec1 ) == false )
					return CStatus::OK;


				CDataArray2DVector3f inPos ( in_ctxt,ID_IN_Vec1 );
				CDataArrayFloat inTime ( in_ctxt,ID_IN_Float1 );
				CDataArrayVector4f inParams ( in_ctxt,ID_IN_Vec2 );

				CDataArray2DVector3f::Accessor inAcc = inPos[0];
				CDataArray2DFloat::Accessor outAcc = outData.Resize ( 0, inAcc.GetCount ( ) );



				float attrs[3];
				attrs[0]= inParams[0].GetX ();
				attrs[1]= inParams[0].GetY();
				attrs[2]= inParams[0].GetZ ();
				float scale = inParams[0].GetW ();


				CWorleyNoise ns ( 1 );


				for ( LLONG it=0; it<inAcc.GetCount ( ); ++it )
				{
	
				//	outAcc [ it]  = ns.GetSpectralWorleyNoise4D ( inTime[0], inAcc[it].GetX() * scale, inAcc[it].GetY() * scale, inAcc[it].GetZ() * scale, attrs  );//Worley ( (float*)(&(inAcc[it])));; // (color_border * (1.0f - gradc)) + (color_fill * gradc); 
				};
				//ns.GetSpectralSimplexNoise4D ( inTime[0], inAcc[it].GetX() * scale, inAcc[it].GetY() * scale, inAcc[it].GetZ() * scale, attrs );


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




