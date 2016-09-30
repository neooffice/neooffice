/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *		 - GNU General Public License Version 2.1
 *
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

#ifndef _SV_SALGDI_H
#define _SV_SALGDI_H

#include <hash_map>

#include <vcl/sv.h>

#include <premac.h>
#include <ApplicationServices/ApplicationServices.h>
#include <postmac.h>
#undef check

#include "impfont.hxx"
#include "outfont.hxx"
#include "salgdi.hxx"
#include "sallayout.hxx"
#include "java/salbmp.h"
#include "java/salframe.h"
#include "java/salprn.h"
#include "java/salvd.h"

// Fix bug 3051 by setting the printer resolution to twips
#define MIN_PRINTER_RESOLUTION 1440

#define MIN_SCREEN_RESOLUTION 96

class ImplDevFontAttributes;
class ImplFontSelectData;
class SalATSLayout;

// --------------------
// - JavaImplFontData -
// --------------------

class JavaImplFontData : public ImplFontData
{
public:
	static ::std::map< sal_IntPtr, sal_IntPtr >	maBadNativeFontCheckedMap;
	static ::std::map< sal_IntPtr, sal_IntPtr >	maBadNativeFontIDMap;
	static ::std::map< OUString, OUString >	maBadNativeFontNameMap;
	::rtl::OUString			maFontName;
	mutable sal_IntPtr		mnNativeFontID;
	::std::list< JavaImplFontData* >	maChildren;
	::rtl::OUString			maFamilyName;

	static void				ClearNativeFonts();
	static void				HandleBadFont( const JavaImplFontData *pFontData );
	static bool				IsBadFont( const JavaImplFontData *pFontData, bool bHandleIfBadFont = true );
	DECL_STATIC_LINK( JavaImplFontData, RunNativeFontsTimer, void* );

							JavaImplFontData( const ImplDevFontAttributes& rAttibutes, const ::rtl::OUString& rFontName, sal_IntPtr nNativeFontID, const ::rtl::OUString& rFamilyName );
	virtual					~JavaImplFontData();

	virtual ImplFontEntry*	CreateFontInstance( ImplFontSelectData& rData ) const;
	virtual ImplFontData*	Clone() const;
	virtual sal_IntPtr		GetFontId() const;
};

// ----------------------
// - JavaSalGraphicsOp -
// ----------------------

class JavaSalGraphicsOp
{
protected:
	CGPathRef				maFrameClipPath;
	CGPathRef				maNativeClipPath;
	bool					mbInvert;
	bool					mbXOR;
	float					mfLineWidth;
	size_t					mnXORBitmapPadding;
	CGLayerRef				maXORLayer;
	CGContextRef			maSavedContext;
	size_t					mnBitmapCapacity;
	sal_uInt8*				mpDrawBits;
	CGContextRef			maDrawBitmapContext;
	sal_uInt8*				mpXORBits;
	CGContextRef			maXORBitmapContext;
	CGRect					maXORRect;
	
public:
							JavaSalGraphicsOp( const CGPathRef aFrameClip, const CGPathRef aNativeClipPath, bool bInvert = false, bool bXOR = false, float fLineWidth = 0 );
	virtual					~JavaSalGraphicsOp();

	virtual	void			drawOp( JavaSalGraphics *, CGContextRef, CGRect ) {}

protected:
	void					restoreClipXORGState();
	CGContextRef			saveClipXORGState( JavaSalGraphics *pGraphics, CGContextRef aContext, CGRect aDrawBounds = CGRectNull );
};

// ------------------------------
// - JavaSalGraphicsDrawImageOp -
// ------------------------------

class SAL_DLLPRIVATE JavaSalGraphicsDrawImageOp : public JavaSalGraphicsOp
{
	CGImageRef				maImage;
	CGRect					maRect;

public:
							JavaSalGraphicsDrawImageOp( const CGPathRef aFrameClip, const CGPathRef aNativeClipPath, bool bInvert, bool bXOR, CGDataProviderRef aProvider, int nDataBitCount, size_t nDataScanlineSize, size_t nDataWidth, size_t nDataHeight, const CGRect aSrcRect, const CGRect aRect );
	virtual					~JavaSalGraphicsDrawImageOp();

	virtual	void			drawOp( JavaSalGraphics *pGraphics, CGContextRef aContext, CGRect aBounds );
};

// -----------------------------
// - JavaSalGraphicsDrawPathOp -
// -----------------------------

class SAL_DLLPRIVATE JavaSalGraphicsDrawPathOp : public JavaSalGraphicsOp
{
	bool					mbAntialias;
	SalColor				mnFillColor;
	SalColor				mnLineColor;
	CGPathRef				maPath;
	bool					mbShiftLines;
	::basegfx::B2DLineJoin	meLineJoin;
	bool					mbLineDash;

public:
							JavaSalGraphicsDrawPathOp( const CGPathRef aFrameClip, const CGPathRef aNativeClipPath, bool bInvert, bool bXOR, bool bAntialias, SalColor nFillColor, SalColor nLineColor, const CGPathRef aPath, bool bShiftLines = true, float fLineWidth = 0, ::basegfx::B2DLineJoin eLineJoin = ::basegfx::B2DLINEJOIN_NONE, bool bLineDash = false );
	virtual					~JavaSalGraphicsDrawPathOp();

	virtual	void			drawOp( JavaSalGraphics *pGraphics, CGContextRef aContext, CGRect aBounds );
};

// ----------------
// - JavaImplFont -
// ----------------

class JavaImplFont
{
	::rtl::OUString			maPSName;
	sal_IntPtr				mnNativeFont;
	short					mnOrientation;
	double					mfScaleX;
	float					mfSize;
	sal_Bool				mbAntialiased;
	sal_Bool				mbVertical;
	sal_Bool				mbNativeFontOwner;

public:
	static void				clearNativeFonts();

							JavaImplFont( ::rtl::OUString aName, float fSize, short nOrientation, sal_Bool bAntialiased, sal_Bool bVertical, double fScaleX );
							JavaImplFont( JavaImplFont *pFont );
	virtual					~JavaImplFont();

	sal_IntPtr				getNativeFont();
	short					getOrientation();
	::rtl::OUString			getPSName();
	double					getScaleX();
	float					getSize();
	sal_Bool				isAntialiased();
	sal_Bool				isVertical();
};

// -------------------
// - JavaSalGraphics -
// -------------------

class JavaSalGraphics : public SalGraphics
{
private:
	::osl::Mutex			maUndrawnNativeOpsMutex;
	::std::list< JavaSalGraphicsOp* >	maUndrawnNativeOpsList;
	SalColor				mnBackgroundColor;
	CGLayerRef				maLayer;
	sal_uInt32				mnPixelContextData;
	CGContextRef			maPixelContext;
	CGRect					maNeedsDisplayRect;

public:
	SalColor				mnFillColor;
	SalColor				mnLineColor;
	SalColor				mnTextColor;
	SalColor				mnFillTransparency;
	SalColor				mnLineTransparency;
	JavaSalFrame*			mpFrame;
	JavaSalPrinter*			mpPrinter;
	JavaSalVirtualDevice*	mpVirDev;
	JavaImplFontData*		mpFontData;
	JavaImplFont*			mpFont;
	::std::hash_map< int, JavaImplFont* >	maFallbackFonts;
	::std::hash_map< int, Size >	maFallbackFontSizes;
	ImplLayoutRuns			maFallbackRuns;
	FontFamily				mnFontFamily;
	FontWeight				mnFontWeight;
	bool					mbFontItalic;
	FontPitch				mnFontPitch;
	FontWidth				mnFontWidthType;
	sal_Int32				mnDPIX;
	sal_Int32				mnDPIY;
	CGPathRef				maFrameClipPath;
	CGMutablePathRef		maNativeClipPath;
	bool					mbInvert;
	bool					mbXOR;
	Orientation				meOrientation;
	sal_Bool				mbPaperRotated;
	float					mfPageTranslateX;
	float					mfPageTranslateY;
	float					mfPageOutputWidth;
	float					mfPageOutputHeight;
	CGRect					maNativeBounds;

	static float			getContextBackingFactor( CGContextRef aContext );
	static void				setContextDefaultSettings( CGContextRef aContext, const CGPathRef aFrameClipPath, const CGPathRef aClipPath, float fLineWidth );

							JavaSalGraphics();
	virtual					~JavaSalGraphics();

	virtual void			drawPixel( long nX, long nY );
	virtual void			drawPixel( long nX, long nY, SalColor nSalColor );
	virtual void			drawLine( long nX1, long nY1, long nX2, long nY2 );
	virtual void			drawRect( long nX, long nY, long nWidth, long nHeight );
	virtual void			drawPolyLine( sal_uInt32 nPoints, const SalPoint* pPtAry );
	virtual void			drawPolygon( sal_uInt32 nPoints, const SalPoint* pPtAry );
	virtual void			drawPolyPolygon( sal_uInt32 nPoly, const sal_uInt32* pPoints, PCONSTSALPOINT* pPtAry );
	virtual bool			drawPolyPolygon( const ::basegfx::B2DPolyPolygon& rPolyPoly, double fTransparency );
	virtual bool			drawPolyLine( const ::basegfx::B2DPolygon& rPoly, double fTransparency, const ::basegfx::B2DVector& rLineWidths, basegfx::B2DLineJoin eLineJoin, com::sun::star::drawing::LineCap nLineCap );
	virtual sal_Bool		drawPolyLineBezier( sal_uInt32 nPoints, const SalPoint* pPtAry, const sal_uInt8* pFlgAry );
	virtual sal_Bool		drawPolygonBezier( sal_uInt32 nPoints, const SalPoint* pPtAry, const sal_uInt8* pFlgAry );
	virtual sal_Bool		drawPolyPolygonBezier( sal_uInt32 nPoly, const sal_uInt32* pPoints, const SalPoint* const* pPtAry, const sal_uInt8* const* pFlgAry );
	virtual void			copyArea( long nDestX, long nDestY, long nSrcX, long nSrcY, long nSrcWidth, long nSrcHeight, sal_uInt16 nFlags );
	virtual void			copyBits( const SalTwoRect& rPosAry, SalGraphics* pSrcGraphics );
	virtual void			drawBitmap( const SalTwoRect& rPosAry, const SalBitmap& rSalBitmap );
	virtual void			drawBitmap( const SalTwoRect& rPosAry, const SalBitmap& rSalBitmap, SalColor nTransparentColor );
	virtual void			drawBitmap( const SalTwoRect& rPosAry, const SalBitmap& rSalBitmap, const SalBitmap& rTransparentBitmap );
	virtual void			drawMask( const SalTwoRect& rPosAry, const SalBitmap& rSalBitmap, SalColor nMaskColor );
	virtual SalBitmap*		getBitmap( long nX, long nY, long nWidth, long nHeight );
	virtual SalColor		getPixel( long nX, long nY );
	virtual void			invert( long nX, long nY, long nWidth, long nHeight, SalInvert nFlags);
	virtual void			invert( sal_uInt32 nPoints, const SalPoint* pPtAry, SalInvert nFlags );
	virtual sal_Bool		drawEPS( long nX, long nY, long nWidth, long nHeight, void* pPtr, sal_uLong nSize );
	virtual void			GetResolution( sal_Int32& rDPIX, sal_Int32& rDPIY );
	virtual sal_uInt16		GetBitCount();
	virtual long			GetGraphicsWidth() const;
	virtual void			ResetClipRegion();
	virtual bool			setClipRegion( const Region& rRegion );
	virtual void			SetLineColor();
	virtual void			SetLineColor( SalColor nSalColor );
	virtual void			SetFillColor();
	virtual void			SetFillColor( SalColor nSalColor );
	virtual void			SetXORMode( bool bSet, bool bInvertOnly );
	virtual void			SetROPLineColor( SalROPColor nROPColor );
	virtual void			SetROPFillColor( SalROPColor nROPColor );
	virtual void			SetTextColor( SalColor nSalColor );
	virtual sal_uInt16		SetFont( ImplFontSelectData* pFont, int nFallbackLevel );
	virtual void			GetFontMetric( ImplFontMetricData* pMetric, int nFallbackLevel = 0 );
	virtual sal_uLong		GetKernPairs( sal_uLong nPairs, ImplKernPairData* pKernPairs );
	virtual ImplFontCharMap*	GetImplFontCharMap() const;
	virtual void			GetDevFontList( ImplDevFontList* );
	virtual void			GetDevFontSubstList( OutputDevice* );
	virtual bool			AddTempDevFont( ImplDevFontList*, const String& rFileURL, const String& rFontName );
	virtual sal_Bool		CreateFontSubset( const rtl::OUString& rToFile, const ImplFontData* pFont, sal_GlyphId* pGlyphIDs, sal_uInt8* pEncoding, sal_Int32* pWidths, int nGlyphs, FontSubsetInfo& rInfo );
	virtual const Ucs2SIntMap*	GetFontEncodingVector( const ImplFontData*, const Ucs2OStrMap** ppNonEncoded );
	virtual const void*		GetEmbedFontData( const ImplFontData* pFont, const sal_Ucs* pUnicodes, sal_Int32* pWidths, FontSubsetInfo& rInfo, long* pDataLen );
	virtual void			FreeEmbedFontData( const void* pData, long nDataLen );
	virtual void			GetGlyphWidths( const ImplFontData* pFont, bool bVertical, Int32Vector& rWidths, Ucs2UIntMap& rUnicodeEnc );
	virtual bool			GetGlyphBoundRect( sal_GlyphId nIndex, Rectangle& );
	virtual bool			GetGlyphOutline( sal_GlyphId nIndex, basegfx::B2DPolyPolygon& rPolyPoly );
	virtual SalLayout*		GetTextLayout( ImplLayoutArgs&, int nFallbackLevel );
	virtual void			DrawServerFontLayout( const ServerFontLayout& );
	virtual sal_Bool		IsNativeControlSupported( ControlType nType, ControlPart nPart );
	virtual sal_Bool		hitTestNativeControl( ControlType nType, ControlPart nPart, const Rectangle& rControlRegion, const Point& aPos, sal_Bool& rIsInside );
	virtual sal_Bool		drawNativeControl( ControlType nType, ControlPart nPart, const Rectangle& rControlRegion, ControlState nState, const ImplControlValue& aValue, const rtl::OUString& rCaption );
	virtual sal_Bool		drawNativeControlText( ControlType nType, ControlPart nPart, const Rectangle& rControlRegion, ControlState nState, const ImplControlValue& aValue, const rtl::OUString& rCaption );
	virtual sal_Bool		getNativeControlRegion( ControlType nType, ControlPart nPart, const Rectangle& rControlRegion, ControlState nState, const ImplControlValue& aValue, const rtl::OUString& rCaption, Rectangle& rNativeBoundingRegion, Rectangle& rNativeContentRegion );
	virtual bool			drawAlphaBitmap( const SalTwoRect& rPosAry, const SalBitmap& rSourceBitmap, const SalBitmap& rAlphaBitmap );
	virtual bool			drawTransformedBitmap( const basegfx::B2DPoint& rNull, const basegfx::B2DPoint& rX, const basegfx::B2DPoint& rY, const SalBitmap& rSourceBitmap, const SalBitmap* pAlphaBitmap );
	virtual bool			drawAlphaRect( long nX, long nY, long nWidth, long nHeight, sal_uInt8 nTransparency );
	virtual bool			supportsOperation( OutDevSupportType ) const;
	virtual SystemGraphicsData	GetGraphicsData() const;
	virtual SystemFontData	GetSysFontData( int nFallbacklevel ) const;

	void					setLineTransparency( sal_uInt16 nTransparency );
	void					setFillTransparency( sal_uInt16 nTransparency );
	void					setFrameClipPath( CGPathRef aFrameClipPath );
	void					addNeedsDisplayRect( const CGRect aRect, float fLineWidth );
	void					addUndrawnNativeOp( JavaSalGraphicsOp *pOp );
	void					copyFromGraphics( JavaSalGraphics *pSrcGraphics, CGRect aSrcRect, CGRect aDestRect, bool bAllowXOR );
	void					copyToContext( const CGPathRef aFrameClipPath, const CGPathRef aNativeClipPath, bool bInvert, bool bXOR, CGContextRef aDestContext, CGRect aDestBounds, CGRect aSrcRect, CGRect aDestRect, bool bDestIsWindow = false, bool bDestIsUnflipped = false );
	void					drawUndrawnNativeOps( CGContextRef aContext, CGRect aRect );
	sal_uLong				getBitmapDirectionFormat();
	CGLayerRef				getLayer() { return maLayer; }
	sal_Bool				getNativeControlTextColor( ControlType nType, ControlPart nPart, ControlState nState, const ImplControlValue& aValue, SalColor& textColor );
	float					getNativeLineWidth();
	::osl::Mutex&			getUndrawnNativeOpsMutex() { return maUndrawnNativeOpsMutex; }
	void					setBackgroundColor( SalColor nBackgroundColor );
	void					setLayer( CGLayerRef aLayer );
	void					setNeedsDisplay( NSView *pView );
};

SAL_DLLPRIVATE bool AddPolygonToPaths( CGMutablePathRef aCGPath, const ::basegfx::B2DPolygon& rPolygon, bool bClosePath, CGRect aUnflippedBounds );
SAL_DLLPRIVATE bool AddPolyPolygonToPaths( CGMutablePathRef aCGPath, const ::basegfx::B2DPolyPolygon& rPolyPoly, CGRect aUnflippedBounds );
SAL_DLLPRIVATE CGColorRef CreateCGColorFromSalColor( SalColor nColor );
SAL_DLLPRIVATE CGPoint UnflipFlippedPoint( CGPoint aFlippedPoint, CGRect aUnflippedBounds );
SAL_DLLPRIVATE CGRect UnflipFlippedRect( CGRect aFlippedRect, CGRect aUnflippedBounds );

#endif // _SV_SALGDI_H