//========================================================================
// class SvxColorWindow_Impl --------------------------------------------------
//========================================================================
#ifndef __SVX_COLORWINDOW_HXX_
#define __SVX_COLORWINDOW_HXX_

#include <sfx2/tbxctrl.hxx>
#include <svtools/lstner.hxx>
#include <rtl/ustring.hxx>
#include <com/sun/star/frame/XFrame.hpp>

#ifdef USE_JAVA
#include <map>
#endif	// USE_JAVA

//========================================================================
// class SvxColorWindow_Impl --------------------------------------------------
//========================================================================

class SvxColorWindow_Impl : public SfxPopupWindow
{
	using FloatingWindow::StateChanged;

private:
	const USHORT	                                                    theSlotId;
	ValueSet		                                                    aColorSet;
    rtl::OUString                                                       maCommand;
#ifdef USE_JAVA
    std::map< ColorData, USHORT >                                       maColorSetMap;
    Link                                                                maColorChangedHdl;
#endif	// USE_JAVA

#if _SOLAR__PRIVATE
	DECL_LINK( SelectHdl, void * );
#endif

protected:
	virtual void    Resize();
	virtual BOOL	Close();

public:
    SvxColorWindow_Impl( const rtl::OUString& rCommand, 
                         USHORT nSlotId,
                         const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
					     const String& rWndTitle,
                         Window* pParentWindow );
    ~SvxColorWindow_Impl();
	void                StartSelection();

    virtual void        KeyInput( const KeyEvent& rKEvt );
    virtual void        StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );

	virtual SfxPopupWindow* Clone() const;

#ifdef USE_JAVA
    void                SetColorChangedHdl( const Link& rLink ) { maColorChangedHdl = rLink; }
#endif	// USE_JAVA
};

#endif