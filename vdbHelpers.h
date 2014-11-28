#pragma once
#include <openvdb/openvdb.h>




#include "MathUtils.h"
#include <iostream>

struct VDB_ICEFlowDataWrapper_t 
{
	openvdb::GridBase::Ptr  m_grid; 
	clock_t					m_lastEvalTime;
};



struct VDB_ICENode_cacheBase_t
{
	inline void PackGrid  ( CDataArrayCustomType & outData )
	{
	
		VDB_ICEFlowDataWrapper_t * p_outRef = &(m_primaryGrid);
		void * pOut = outData.Resize(0, sizeof(p_outRef));  
		memcpy ( pOut, &(p_outRef), sizeof(p_outRef));
		return;
	};

	inline VDB_ICEFlowDataWrapper_t * UnpackGrid ( CDataArrayCustomType & inData  )
	{
		// get input grid		
		ULONG inDataSize;
		void * p_in = NULL;
		inData.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

		// check for data
		if (inDataSize==0L || p_in==NULL) 
		return NULL;

		VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
		if (!p_inWrapper->m_grid )
			return NULL;

		if ( IsEqual ( p_inWrapper->m_grid ) )
		{
			Application().LogMessage ( L"[VDB]: Trying to get selfgrid on inport!" , siErrorMsg );
			return NULL;
		};

		return p_inWrapper;
	};

	static VDB_ICEFlowDataWrapper_t * UnpackGridStatic ( CDataArrayCustomType & inData  )
	{
		// get input grid		
		ULONG inDataSize;
		void * p_in = NULL;
		inData.GetData(0, (const CDataArrayCustomType::TData**)&p_in, inDataSize);

		// check for data
		if (inDataSize==0L || p_in==NULL) 
		return NULL;

		VDB_ICEFlowDataWrapper_t * p_inWrapper = *((VDB_ICEFlowDataWrapper_t**)p_in);
		if (!p_inWrapper->m_grid )
			return NULL;

		return p_inWrapper;
	};

	inline bool IsEqual ( openvdb::GridBase::Ptr & in_gridRefRef )
	{
		return m_primaryGrid.m_grid == in_gridRefRef;
	};

	inline void SetFromBaseGrid ( openvdb::GridBase::Ptr & in_gridRefRef )
	{
		m_primaryGrid.m_grid = in_gridRefRef;
		m_primaryGrid.m_lastEvalTime = clock ();
	};

	inline void ResetGridHolder ( )
	{
		m_primaryGrid.m_grid.reset ( );
	};

	inline clock_t GetLastEvalTime ( ) const
	{
		return m_primaryGrid.m_lastEvalTime;
	};

protected:
		VDB_ICEFlowDataWrapper_t m_primaryGrid;
};




inline bool IsPortArray ( ICENodeContext & in_ctxt, ULONG in_portID )
{
	// obtain inports structs
	XSI::siICENodeDataType _PORTTYPE;
	XSI::siICENodeStructureType _PORTSTRUCT;
	XSI::siICENodeContextType _PORTCONTEXT;

	in_ctxt.GetPortInfo( in_portID , _PORTTYPE, _PORTSTRUCT, _PORTCONTEXT );

	return  ( _PORTSTRUCT == XSI::siICENodeStructureArray ); 		
};

