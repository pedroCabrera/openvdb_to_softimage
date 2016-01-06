#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_userdatablob.h>

#include <openvdb/tools/LevelSetRebuild.h>
enum IDs
{
	ID_IN_VDBGrid ,

	ID_IN_MOV,
	ID_IN_SCL,
	ID_IN_PIVOT,

	ID_G_100 = 100,

	ID_OUT_VDBGrid ,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus VDB_Transform_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_Transform", L"VDB Transform");

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
	
	 st = nodeDef.AddInputPort(ID_IN_MOV, ID_G_100, siICENodeDataVector3, siICENodeStructureSingle, siICENodeContextSingleton, L"Translation", L"Translation",CValue(),CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	 st = nodeDef.AddInputPort(ID_IN_SCL, ID_G_100, siICENodeDataVector3, siICENodeStructureSingle, siICENodeContextSingleton, L"Scale", L"Scale",MATH::CVector3f(1,1,1),CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
	  st = nodeDef.AddInputPort(ID_IN_PIVOT, ID_G_100, siICENodeDataVector3, siICENodeStructureSingle, siICENodeContextSingleton, L"Pivot", L"Pivot",MATH::CVector3f(0,0,0),CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_Transform_cache_t : public VDB_ICENode_cacheBase_t
{
	VDB_Transform_cache_t ( ) { };



	clock_t  m_inputEvalTime;
	//MATH::CMatrix4f m_srt;
	MATH::CVector3f m_mov;
	MATH::CVector3f m_scl;
	MATH::CVector3f m_piv;

	bool inline IsDirty ( clock_t lastEvalTime, MATH::CVector3f mov, MATH::CVector3f scl, MATH::CVector3f piv )
	{
		bool isClean = ( m_inputEvalTime == lastEvalTime && m_mov == mov && m_scl == scl && m_piv == piv);

		if ( isClean == false )
		{	
		
			m_inputEvalTime = lastEvalTime;		
			m_mov = mov ;
			m_scl = scl;
			m_piv = piv;
		};

		return !isClean;

	};

	inline void Transform ( VDB_ICEFlowDataWrapper_t * in_grid )
	{
		TIMER timer;

		m_primaryGrid.m_grid = in_grid->m_grid->deepCopyGrid ();

		// NOTE: vdb rotation and transformations at whole is something extremely ridiculus, use only scale and translate now


		/*	float x = -atan(m_srt.GetValue(2,1)/m_srt.GetValue(2,2));
		float z = asin(m_srt.GetValue(2,0));
		float y = -atan(m_srt.GetValue(1,0)/m_srt.GetValue(0,0));
		*/
		//	\theta_x = arctan(r_{3,2}/r_{3,3})
		//\theta_y = -arcsin(r_{3,1})
		//\theta_z = arctan(r_{2,1}/r_{1,1})

		openvdb::math::Vec3d scl  ( m_scl[0], m_scl[1], m_scl[2] );
		openvdb::math::Vec3d rot  ( 0, 0, 0 );//( x, y, z );
		openvdb::math::Vec3d mov  ( m_mov[0], m_mov[1], m_mov[2] );
		openvdb::math::Vec3d piv  ( m_piv[0], m_piv[1], m_piv[2] );
		//m_srt.SetValue ( 3,0,0.f ), m_srt.SetValue ( 3,1,0.f ), m_srt.SetValue ( 3,2,0.f );
		//MATH::CVector3f sclv3 ( 1.f, 1.f, 1.f );
		//sclv3.MulByMatrix4InPlace ( m_srt );




	openvdb::Mat4R mat(openvdb::Mat4R::identity());

		try
		{

			mat.preTranslate(piv);
			//mat.preRotate(openvdb::math::X_AXIS, rot[0]);
			//mat.preRotate(openvdb::math::Y_AXIS, rot[1]);
			//mat.preRotate(openvdb::math::Z_AXIS, rot[2]);
			mat.preScale(scl);
			mat.preTranslate(-piv);
			mat.preTranslate(mov);

			openvdb::math::AffineMap map(mat);
			openvdb::math:: AffineMap::Ptr compound(  new openvdb::math::AffineMap(*(m_primaryGrid.m_grid->transform().baseMap()->getAffineMap()), map));
			m_primaryGrid.m_grid->setTransform(	openvdb::math::Transform::Ptr(new 	openvdb::math::Transform(openvdb::math::simplify(compound))));

		}
		catch ( openvdb::Exception & e)
		{
			Application().LogMessage(L"[VDB][TRANSFORM]: " + CString(e.what()), siWarningMsg );
		}

		

	
			m_primaryGrid.m_grid->setName ( in_grid->m_grid->getName ( ) );
			m_primaryGrid.m_lastEvalTime = clock();
            Application().LogMessage(L"[VDB][TRANSFORM]: Stamped at=" + CString ( (LONG)m_primaryGrid.m_lastEvalTime));
			Application().LogMessage(L"[VDB][TRANSFORM]: Done in=" + CString (timer.GetElapsedTime ()));
		
		
	};


};

SICALLBACK dlexport VDB_Transform_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{

				CDataArrayCustomType outData( in_ctxt );     

			// get cache object
			VDB_Transform_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][TRANSFORM]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_Transform_cache_t*)(CValue::siPtrType)userData;

			p_cacheNodeObject->PackGrid ( outData );
		
						// get input grid
			CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][TRANSFORM]: Empty levelset grid on input" );
				return CStatus::OK;
			};

			if ( p_inWrapper->m_grid->isType<openvdb::FloatGrid>()==false )
			{
				Application().LogMessage ( L"[VDB][TRANSFORM]: Invalid grid data type! Float only allowed!", siWarningMsg );
				return CStatus::OK;
			}
	
			CDataArrayVector3f inMOV( in_ctxt, ID_IN_MOV );
			CDataArrayVector3f inSCL( in_ctxt, ID_IN_SCL );
			CDataArrayVector3f inPIV( in_ctxt, ID_IN_PIVOT );
		            
			

			// check dirty state
			if ( p_cacheNodeObject->IsDirty ( p_inWrapper->m_lastEvalTime, inMOV[0],inSCL[0],inPIV[0] )==false )
			{
				Application().LogMessage ( L"[VDB][TRANSFORM]: No changes on input, used prev result" );
				return CStatus::OK;
			}
			
			// change if dirty
			p_cacheNodeObject->Transform ( p_inWrapper );

		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK dlexport VDB_Transform_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();


   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_Transform_cache_t * p_vdbObj;

   if (userData.IsEmpty()) 	   
      p_vdbObj = new VDB_Transform_cache_t ( );
   else
   {
	   Application().LogMessage ( L"[VDB][TRANSFORM]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK dlexport VDB_Transform_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][TRANSFORM]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

  VDB_Transform_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_Transform_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
