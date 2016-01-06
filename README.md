Basic OpenVDB connector for Autodesk Softimage

In order to build you have to install Visual studio (2010-2015) on Windows or GCC if you are on linux. 
Next, install qtcreator IDE (make sure it is compatible with your vs/gcc version).
Define XSISDK_ROOT system variable, so it will point to the actual softimage sdk folder, for example: 
/usr/Softimage/Softimage_2013_SP1/XSISDK
or
C:\Program Files\Autodesk\Softimage 2013 SP1\XSISDK

Clone the repo and open openvdb_to_softimage.pro in qtcreator, configure your toolchain and build.
To connect the plugin in softimage, open the plug-in manager and connect "repofolder/build/XSI_OPENVDB_Workgroup" as workgroup.
Restart Softimage.