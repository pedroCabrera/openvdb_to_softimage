#include <xsi_argument.h>
#include <xsi_utils.h>

#include "Main.h"
#include "vdbHelpers.h"


SICALLBACK VDBGetFileInfo_Init( CRef& in_ctxt )
{
        Context ctxt( in_ctxt );
        Command oCmd;
        oCmd = ctxt.GetSource();
        oCmd.PutDescription(L"Create an instance of VDBGetFileInfo command");
        ArgumentArray args = oCmd.GetArguments();
        oCmd.SetFlag(siNoLogging,true);


		args.Add(L"VDBFilePath", CString());

        // TODO: You may want to add some arguments to this command so that the operator
        // can be applied to objects without depending on their specific names.
        // Tip: the Collection ArgumentHandler is very useful
        return CStatus::OK;
}



SICALLBACK VDBGetFileInfo_Execute( CRef& in_ctxt )
{
        
   Context ctxt( in_ctxt );
   CValueArray args = ctxt.GetAttribute(L"Arguments");
   CValueArray cmdArgs;
   CValue returnVal;

   	// init openvdb stuff
	openvdb::initialize();

   // get string with path to vdb file	
   if ( args.GetCount ( ) != 1 )
   {
	   Application().LogMessage ( L"[VDB][GETFILEINFO]: Only path to the target vdb file as argument is allowed!" ,siWarningMsg);
	    return CStatus::OK;
   };


   CString vdbFilePath = CUtils:: ResolvePath ( args[0] );

   openvdb::GridPtrVecPtr grids;
    openvdb::io::File file(vdbFilePath.GetAsciiString());

   try
   {
      file.open();
      grids = file.getGrids();
      file.close();
   }
   catch (openvdb::Exception& e)
   {
      Application().LogMessage(L"[VDB][GETFILEINFO]: " + CString(e.what()) + L" : " + vdbFilePath, siWarningMsg);
	   return CStatus::OK;
   }

    Application().LogMessage(L"[VDB][GETFILEINFO]: Total nbGrids in file=" + CString (grids->size ()) );;
   LONG cnt = 0;
   for (openvdb::GridPtrVec::const_iterator it = grids->begin(); it != grids->end(); ++it, ++cnt)
   {
	   Application().LogMessage(L"[VDB][GETFILEINFO]: ############################################");
	   Application().LogMessage(L"[VDB][GETFILEINFO]: VDBFile=" + vdbFilePath);
	   Application().LogMessage(L"[VDB][GETFILEINFO]: Grid number in file=" + CString(cnt));


	   openvdb::GridBase::Ptr grid = *it;
	   if (!grid) 
		   continue;

	   // grid name and data type
	   Application().LogMessage(L"[VDB][GETFILEINFO]: Name=" + CString(grid->getName().c_str()));
	   Application().LogMessage(L"[VDB][GETFILEINFO]: DataType=" + CString(grid->valueType().c_str()));
	   Application().LogMessage(L"[VDB][GETFILEINFO]: NbActiveVoxels=" + CString(grid->activeVoxelCount()));
	   LLONG nbbytes = grid->memUsage();
	   Application().LogMessage(L"[VDB][GETFILEINFO]: NbAllocatedBytes=" + CString(nbbytes) + L"| in MB=" + CString(nbbytes/1024LL*1024LL) + L"| in GB=" + CString(nbbytes/1024LL*1024LL*1024LL));
	   

	 // grid class
	   Application().LogMessage(L"[VDB][GETFILEINFO]: GridClass=" + CString(   openvdb::GridBase::gridClassToString(grid->getGridClass()).c_str() ));


	   if (grid->isType<openvdb::BoolGrid>()) 
	   {
		   bool min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::BoolGrid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal=" + CString (min) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal=" + CString (max) );
	   }
	   else if (grid->isType<openvdb::FloatGrid>())		
	   {
		   		   float min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal=" + CString (min) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal=" + CString (max) );
	   }
	   else if (grid->isType<openvdb::DoubleGrid>())
	   {
		      double min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::DoubleGrid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage("[VDB][GETFILEINFO]: MinVal=" + CString (min) ); 
		    Application().LogMessage("[VDB][GETFILEINFO]: MaxVal=" + CString (max) );
	   }
	   else if (grid->isType<openvdb::Int32Grid>())
	   {
		     int min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::Int32Grid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal=" + CString (min) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal=" + CString (max) );
	   }
	   else if (grid->isType<openvdb::Int64Grid>())
	   {
		        LLONG min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::Int64Grid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal=" + CString (min) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal=" + CString (max) );
	   }
	   else if (grid->isType<openvdb::Vec3IGrid>())
	   {
		   openvdb::Vec3i min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::Vec3IGrid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal(length)=" + CString (min.length()) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal(length)=" + CString (max.length()) );
			  Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal(xyz)=" + CString (min.x()) + L" | " + CString (min.y()) + L" | " + CString (min.z()) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal(xyz)="+ CString (max.x()) + L" | " + CString (max.y()) + L" | " + CString (max.z()) ); 
	   }
	   else if (grid->isType<openvdb::Vec3SGrid>())
	   {
		      openvdb::Vec3s min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::Vec3SGrid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal(length)=" + CString (min.length()) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal(length)=" + CString (max.length()) );
			  Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal(xyz)=" + CString (min.x()) + L" | " + CString (min.y()) + L" | " + CString (min.z()) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal(xyz)="+ CString (max.x()) + L" | " + CString (max.y()) + L" | " + CString (max.z()) ); 
	   }
	   else if (grid->isType<openvdb::Vec3DGrid>())
	   {
		   openvdb::Vec3d min,max;
		   auto castedgrid = openvdb::gridPtrCast<openvdb::Vec3DGrid>(grid);
		   castedgrid->evalMinMax(min,max);
		   Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal(length)=" + CString (min.length()) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal(length)=" + CString (max.length()) );
			  Application().LogMessage(L"[VDB][GETFILEINFO]: MinVal(xyz)=" + CString (min.x()) + L" | " + CString (min.y()) + L" | " + CString (min.z()) ); 
		    Application().LogMessage(L"[VDB][GETFILEINFO]: MaxVal(xyz)="+ CString (max.x()) + L" | " + CString (max.y()) + L" | " + CString (max.z()) ); 
	   };
	   


	   // Application().LogMessage(gridName + "\t" + gridType + "\t" + dimensions + "\t" + voxelCount + "\t" + gridSizeInBytes);
	   Application().LogMessage(L"[VDB][GETFILEINFO]: ############################################");
   }

   return CStatus::OK;
}