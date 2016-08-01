/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef INCLUDED_PAGEBORDERSHANDLER_HXX
#define INCLUDED_PAGEBORDERSHANDLER_HXX

#include "BorderHandler.hxx"
#include "PropertyMap.hxx"

#include <WriterFilterDllApi.hxx>
#include <resourcemodel/WW8ResourceModel.hxx>
#include <boost/shared_ptr.hpp>

#if SUPD == 310
#include <com/sun/star/table/BorderLine.hpp>
#define BorderLine2 BorderLine
#else	 // SUPD == 310
#include <com/sun/star/table/BorderLine2.hpp>
#endif	// 

#include <vector>


namespace writerfilter {
namespace dmapper {

class _PgBorder
{
public:
    com::sun::star::table::BorderLine2 m_rLine;
    sal_Int32   m_nDistance;
    BorderPosition m_ePos;

    _PgBorder( );
    ~_PgBorder( );
};

class WRITERFILTER_DLLPRIVATE PageBordersHandler : public Properties
{
private:

    // See implementation of SectionPropertyMap::ApplyBorderToPageStyles
    sal_Int32 m_nDisplay;
    sal_Int32 m_nOffset;
    vector<_PgBorder> m_aBorders;

public:
    PageBordersHandler( );
    ~PageBordersHandler( );

    // Properties
    virtual void attribute( Id eName, Value& rVal );
    virtual void sprm( Sprm& rSprm );

    inline sal_Int32 GetDisplayOffset( ) 
    { 
        return ( m_nOffset << 5 ) + m_nDisplay;
    };
    void SetBorders( SectionPropertyMap* pSectContext );
};
typedef boost::shared_ptr< PageBordersHandler > PageBordersHandlerPtr;

} }

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */