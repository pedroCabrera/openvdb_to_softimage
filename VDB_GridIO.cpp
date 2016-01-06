#include "Main.h"
#include "vdbHelpers.h"
#include <xsi_utils.h>
#include <xsi_icenode.h>
#include <xsi_userdatablob.h>
#include <xsi_x3dobject.h>
enum IDs
{
	ID_IN_VDBGrid = 0,

	ID_IN_Mode,
	ID_IN_FilePath ,
	ID_IN_PathIsRelative,
	ID_IN_FileName ,
	ID_IN_Frame ,
	ID_IN_AppendFrame ,
	ID_IN_ForceEval ,
	ID_IN_GridName ,
	ID_IN_OverrideNameOnWriting,
		ID_IN_ExportThisToRender ,

	ID_G_100 = 100,

	ID_OUT_VDBGrid = 200,

	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

using namespace XSI;


// $F = states that we have to consider frame number appending
// $FN where N is [0...9] and defines digits width, e.g. if frame=1337 and N=6 then res=001337
inline std::string ParseFileName ( const std::string & in_filename, int frame )
{
	std::string retVal;
    std::string filename = in_filename ;

	size_t resPos = filename.find ( "$F" );
	if ( resPos == std::string::npos  )
	{		
		retVal.append ( filename );
	
	}
	else
	{
		std::ostringstream oss;

		if ( filename.size ()-2 > resPos && filename[resPos+2]>=48 && filename[resPos+2]<=57 ) // check for digits 0...9
		{
			int numWidth = filename[resPos+2] - 48;	
			oss << std::setfill('0') << std::setw(numWidth) << frame;
			filename.erase ( resPos, 3 );
		}
		else
		{
			filename.erase ( resPos, 2 );
			oss << frame;
		}
		
		retVal.append ( filename );
		retVal.append ( oss.str() );
	
	}

	return retVal;
};

//  path formatting
inline std::string  BuildFilePath ( std::string & filepath, std::string & filename, int frame )
{
#ifdef _WIN32 
	for ( int i=0;i<filepath.size();++i)
		filepath[i] = filepath[i]=='/' ? '\\' : filepath[i]; 
#endif 

	if ( filepath[filepath.size()-1] != '\\' )
		filepath.append ( "\\" );


	std::string retVal;
	retVal.append ( filepath );

	retVal.append ( ParseFileName ( filename, frame  ) );

	retVal.append ( ".vdb" );
	return retVal;

};



CStatus VDB_GridIO_Register( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_GridIO", L"VDB Grid IO");

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
	/*st = nodeDef.AddInputPort(ID_IN_ExportThisToRender, ID_G_100, siICENodeDataBool, siICENodeStructureSingle, siICENodeContextSingleton, L"ExportThisToRender", L"ExportThisToRender",false,CValue(),CValue(), ID_UNDEF,ID_UNDEF,ID_UNDEF);  st.AssertSucceeded();
   */

	st = nodeDef.AddInputPort(ID_IN_FilePath,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"FilePath",L"FilePath",CString(L"[Project Path]/VDB_Grids/[Scene]"),CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded( ) ;

	st = nodeDef.AddInputPort(ID_IN_FileName,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"FileName",L"FileName",CString(L"VDBGrids$F4"),CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded( ) ;
		st = nodeDef.AddInputPort(ID_IN_GridName,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"ReadingGridName",L"GridName",CString(L"density"),CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_OverrideNameOnWriting,ID_G_100,siICENodeDataBool,siICENodeStructureSingle,siICENodeContextSingleton,L"OverrideNameOnWriting",L"OverrideNameOnWriting",true,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_Frame,ID_G_100,siICENodeDataLong,siICENodeStructureSingle,siICENodeContextSingleton,L"Frame",L"Frame",CValue(),CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded( ) ;

	st = nodeDef.AddInputPort(ID_IN_Mode,ID_G_100,siICENodeDataLong,siICENodeStructureSingle,siICENodeContextSingleton,L"Mode",L"Mode",0,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded( ) ;// 0=bypass | 1=read | 2=write 

	//st = nodeDef.AddInputPort(ID_IN_ForceEval,ID_G_100,siICENodeDataBool,siICENodeStructureSingle,siICENodeContextSingleton,L"Enforce Eval Each Frame",L"ForceEval",false,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	//st.AssertSucceeded( ) ;

	st = nodeDef.AddOutputPort( ID_OUT_VDBGrid, customTypes, siICENodeStructureSingle,	siICENodeContextSingleton, L"Out VDB Grid", L"outVDBGrid"); st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(VDB_PUT_CATEGORIES_NAME);

	return CStatus::OK;
}


struct VDB_GridIO_cache_t : public VDB_ICENode_cacheBase_t
{
    VDB_GridIO_cache_t ( const X3DObject & in_self )
	{

		/*CRef objRef; 
		if ( objRef.Set ( in_self.GetName() + L".VDB_UDB" ) == CStatus::OK )
		{	
			m_udb = objRef ;		 
		}
		else
			Application ().LogMessage ( L"[VDB][GRIDIO]: Unable to find UDB!" ); 	*/

	}


	int m_srtApplicationMode;
	CString m_filePath;
	CString m_gridName;
	int m_lastMode;
	clock_t m_lastInputTime;
	 bool m_nameoverride;
	//UserDataBlob m_udb;


	bool inline IsDirty ( CString path, CString gridname, int lastMode, clock_t inTime, bool nameoverride )
	{
		bool isClean = ( m_filePath == path && m_gridName == gridname && m_lastMode== lastMode	&& m_lastInputTime==inTime && m_nameoverride == nameoverride );

		

		if ( isClean == false )
		{	
			m_filePath=path;
			m_gridName=gridname;
			m_lastMode = lastMode;
			m_lastInputTime = inTime;
			m_nameoverride = nameoverride;
		};

		return !isClean;

	};

	
};

SICALLBACK dlexport VDB_GridIO_Evaluate( ICENodeContext& in_ctxt )
{

	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	switch( out_portID )

	{                
	case ID_OUT_VDBGrid :
		{


				// get cache object
			VDB_GridIO_cache_t * p_cacheNodeObject = NULL;
			CValue userData = in_ctxt.GetUserData();
			if ( userData.IsEmpty() )
			{
				Application().LogMessage(L"[VDB][GRIDIO]: Fatal, unable to find cache data on eval", siFatalMsg );
				return CStatus::Fail;
			};			
			p_cacheNodeObject = (VDB_GridIO_cache_t*)(CValue::siPtrType)userData;



			// init outdata buffer
			CDataArrayCustomType outData( in_ctxt );                 
			CDataArrayLong inMode ( in_ctxt, ID_IN_Mode );

		int ioMode = Clamp ( 0, 2, inMode [0] );


			// 0=bypass | 1=read | 2=write
			switch ( ioMode )
			{
			case 0:
				{
					outData.CopyFrom ( ID_IN_VDBGrid );
				}
				break;
			case 1:
				{
					TIMER timer;

					// reading
					CDataArrayLong inFrame ( in_ctxt, ID_IN_Frame );
					CDataArrayString inPath ( in_ctxt, ID_IN_FilePath );
					CDataArrayString inFileName ( in_ctxt, ID_IN_FileName );
					CDataArrayString inGridName ( in_ctxt, ID_IN_GridName );
					CDataArrayBool inOverrideName ( in_ctxt, ID_IN_OverrideNameOnWriting );

					CString resultFilePath =  CUtils::ResolveTokenString (inPath[0], CTime(), false) ;			
					resultFilePath += CUtils::Slash();  

					std::string flName = ParseFileName( std::string(inFileName [0].GetAsciiString()), inFrame[0]);
					resultFilePath += flName.c_str();
					resultFilePath += L".vdb";

					std::string grName = ParseFileName (std::string(inGridName [0].GetAsciiString()), inFrame[0]);

					// #####
					// buggy right now and should not be used 
					// if we need to export the cache path to render
					//CDataArrayBool inDoExport (in_ctxt, ID_IN_ExportThisToRender );
					//if ( inDoExport[0] )
					//{	
					//	if ( p_cacheNodeObject->m_udb.IsValid ( ) )
					//	{
					//		p_cacheNodeObject->m_udb.PutValue ((unsigned char*)(resultFilePath.GetAsciiString()),resultFilePath.Length()+1 );
					//	}
					//	else
					//		Application ().LogMessage ( L"[VDB][IO]: Non-valid UDB!" ,siWarningMsg ); 		
					//						
					//};


					// just save all parameters
					p_cacheNodeObject->IsDirty ( resultFilePath, grName.c_str(), ioMode, -1, inOverrideName[0] );

					// read grid and set to out                                  
					openvdb::io::File file(resultFilePath.GetAsciiString());
					try
					{
						file.open();
                      openvdb::GridBase::Ptr gptr =   file.readGrid(std::string (  inGridName[0].GetAsciiString()) );
                        p_cacheNodeObject->SetFromBaseGrid(
                                   gptr
                                    );
						file.close();

					
                        Application().LogMessage("[VDB][GRIDIO]: Readed at=" + CString((LONG)p_cacheNodeObject->GetLastEvalTime()));
						Application().LogMessage("[VDB][GRIDIO]: Done in=" + CString(timer.GetElapsedTime()));
					}
					catch (openvdb::Exception& e)
					{
						Application().LogMessage("[VDB][GRIDIO]: " + CString(e.what()) , siWarningMsg);
						p_cacheNodeObject->m_filePath.Clear ();
						p_cacheNodeObject->m_gridName.Clear ();
						p_cacheNodeObject->ResetGridHolder ();

					}

					p_cacheNodeObject->PackGrid ( outData );

				}
				break;
			case 2:
				{
					TIMER timer;

								// writing
					CDataArrayLong inFrame ( in_ctxt, ID_IN_Frame );
					CDataArrayString inPath ( in_ctxt, ID_IN_FilePath );
					CDataArrayString inFileName ( in_ctxt, ID_IN_FileName );
					CDataArrayString inGridName ( in_ctxt, ID_IN_GridName );
					CDataArrayBool inOverrideName ( in_ctxt, ID_IN_OverrideNameOnWriting );

					CString resultFilePath =  CUtils::ResolveTokenString (inPath[0], CTime(), false) ;			
					resultFilePath += CUtils::Slash();  

					std::string flName = ParseFileName( std::string(inFileName [0].GetAsciiString()), inFrame[0]);
					resultFilePath += flName.c_str();
					resultFilePath += L".vdb";

					// buggy right now and should not be used 
					// if we need to export the cache path to render
					/*CDataArrayBool inDoExport (in_ctxt, ID_IN_ExportThisToRender );
					if ( inDoExport[0] )
					{	
						if ( p_cacheNodeObject->m_udb.IsValid ( ) )
						{
							p_cacheNodeObject->m_udb.PutValue ((unsigned char*)(resultFilePath.GetAsciiString()),resultFilePath.Length()+1 );
						}
						else
							Application ().LogMessage ( L"[VDB][IO]: Non-valid UDB!" ,siWarningMsg ); 
							
					};*/

					// get input data
					CDataArrayCustomType inVDBGridPort(in_ctxt, ID_IN_VDBGrid);
			VDB_ICEFlowDataWrapper_t * p_inWrapper = p_cacheNodeObject->UnpackGrid ( inVDBGridPort );
			if (p_inWrapper==NULL )
			{
				Application().LogMessage ( L"[VDB][GRIDIO]: Empty grid on input" );
				return CStatus::OK;
			};

					if ( p_cacheNodeObject->IsDirty ( resultFilePath, L"", ioMode, p_inWrapper->m_lastEvalTime, inOverrideName[0] )==true )
					{
						//write and pass in ptr to out
						openvdb::io::File file(resultFilePath.GetAsciiString());

						std::string origName = p_inWrapper->m_grid->getName ( );

						if ( inOverrideName[0] )
							 p_inWrapper->m_grid->setName ( std::string(inGridName [0].GetAsciiString()) );

						try
						{
							// Add the grid pointer to a container.
							openvdb::GridPtrVec gridsArray;

							
							gridsArray.push_back(p_inWrapper->m_grid);
							file.write  (gridsArray);
							file.close();

						
                            Application().LogMessage("[VDB][GRIDIO]: Writed at=" + CString((LONG)p_inWrapper->m_lastEvalTime) );
							Application().LogMessage("[VDB][GRIDIO]: Done in=" + CString(timer.GetElapsedTime()) );


						}
						catch (openvdb::Exception& e)
						{
						
							Application().LogMessage("[VDB][GRIDIO]: " + CString(e.what()) , siWarningMsg);
						}

						 p_inWrapper->m_grid->setName ( origName );
						
					}


					outData.CopyFrom ( ID_IN_VDBGrid );
			

					return CStatus::OK;
				}
				break;
			};


		}
		break;
	};

	return CStatus::OK;
};

SICALLBACK dlexport VDB_GridIO_Init( CRef& in_ctxt )
{

		// init openvdb stuff
	openvdb::initialize();

   Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();
   VDB_GridIO_cache_t * p_vdbObj;

   if (userData.IsEmpty())
   {
	   	ICENode actNode = ctxt.GetSource();
      p_vdbObj = new VDB_GridIO_cache_t ( actNode.GetParent3DObject() );

   }
   else
   {
	   Application().LogMessage ( L"[VDB][GRIDIO]: Fatal, unknow data is present on initialization", siFatalMsg );
	    return CStatus::Fail;
   }

   ctxt.PutUserData((CValue::siPtrType)p_vdbObj);
   return CStatus::OK;
}



SICALLBACK dlexport VDB_GridIO_Term( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
   CValue userData = ctxt.GetUserData();

   if ( userData.IsEmpty () )
   {
	   Application().LogMessage ( L"[VDB][GRIDIO]: Fatal, no data on termination", siFatalMsg );
	   return CStatus::Fail;
   }

   VDB_GridIO_cache_t * p_vdbObj; 
   p_vdbObj = (VDB_GridIO_cache_t*)(CValue::siPtrType)userData;
        

	 delete p_vdbObj;
    ctxt.PutUserData(CValue());

        return CStatus::OK;
}
