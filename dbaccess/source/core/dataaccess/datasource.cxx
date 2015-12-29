/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified March 2013 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_dbaccess.hxx"

#include "datasource.hxx"
#include "module_dba.hxx"
#include "userinformation.hxx"
#include "commandcontainer.hxx"
#include "dbastrings.hrc"
#include "core_resource.hxx"
#include "core_resource.hrc"
#include "connection.hxx"
#include "SharedConnection.hxx"
#include "databasedocument.hxx"

/** === begin UNO includes === **/
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyState.hpp>
#include <com/sun/star/beans/XPropertyContainer.hpp>
#include <com/sun/star/document/XDocumentSubStorageSupplier.hpp>
#include <com/sun/star/document/XEventBroadcaster.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/reflection/XProxyFactory.hpp>
#include <com/sun/star/sdbc/XDriverAccess.hpp>
#include <com/sun/star/sdbc/XDriverManager.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/ucb/AuthenticationRequest.hpp>
#include <com/sun/star/ucb/XInteractionSupplyAuthentication.hpp>
#include <com/sun/star/ui/XUIConfigurationManagerSupplier.hpp>
#include <com/sun/star/view/XPrintable.hpp>
/** === end UNO includes === **/

#include <comphelper/extract.hxx>
#include <comphelper/guarding.hxx>
#include <comphelper/interaction.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/property.hxx>
#include <comphelper/seqstream.hxx>
#include <comphelper/sequence.hxx>
#include <connectivity/dbexception.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <rtl/digest.h>
#include <tools/debug.hxx>
#include <tools/diagnose_ex.h>
#include <tools/urlobj.hxx>
#include <typelib/typedescription.hxx>
#include <unotools/confignode.hxx>
#include <unotools/sharedunocomponent.hxx>

#include <algorithm>

#if defined USE_JAVA && defined MACOSX

#include <dlfcn.h>

typedef sal_Bool Application_canUseJava_Type();

static Application_canUseJava_Type *pApplication_canUseJava = NULL;

#endif	// USE_JAVA && MACOSX

using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::embed;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::reflection;
using namespace ::cppu;
using namespace ::osl;
using namespace ::vos;
using namespace ::dbtools;
using namespace ::comphelper;
namespace css = ::com::sun::star;

//........................................................................
namespace dbaccess
{
//........................................................................

    //============================================================
    //= FlushNotificationAdapter
    //============================================================
    typedef ::cppu::WeakImplHelper1< XFlushListener > FlushNotificationAdapter_Base;
    /** helper class which implements a XFlushListener, and forwards all
        notification events to another XFlushListener

        The speciality is that the foreign XFlushListener instance, to which
        the notifications are forwarded, is held weak.

        Thus, the class can be used with XFlushable instance which hold
        their listeners with a hard reference, if you simply do not *want*
        to be held hard-ref-wise.
    */
    class FlushNotificationAdapter : public FlushNotificationAdapter_Base
    {
    private:
        WeakReference< XFlushable >     m_aBroadcaster;
        WeakReference< XFlushListener > m_aListener;

    public:
        static void installAdapter( const Reference< XFlushable >& _rxBroadcaster, const Reference< XFlushListener >& _rxListener )
        {
            Reference< XFlushListener > xAdapter( new FlushNotificationAdapter( _rxBroadcaster, _rxListener ) );
        }

    protected:
        FlushNotificationAdapter( const Reference< XFlushable >& _rxBroadcaster, const Reference< XFlushListener >& _rxListener );
        ~FlushNotificationAdapter();

        void SAL_CALL impl_dispose( bool _bRevokeListener );

    protected:
        // XFlushListener
        virtual void SAL_CALL flushed( const ::com::sun::star::lang::EventObject& rEvent ) throw (::com::sun::star::uno::RuntimeException);
        // XEventListener
        virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);
    };

    //------------------------------------------------------------
    DBG_NAME( FlushNotificationAdapter )
    //------------------------------------------------------------
    FlushNotificationAdapter::FlushNotificationAdapter( const Reference< XFlushable >& _rxBroadcaster, const Reference< XFlushListener >& _rxListener )
        :m_aBroadcaster( _rxBroadcaster )
        ,m_aListener( _rxListener )
    {
        DBG_CTOR( FlushNotificationAdapter, NULL );
        DBG_ASSERT( _rxBroadcaster.is(), "FlushNotificationAdapter::FlushNotificationAdapter: invalid flushable!" );

        osl_incrementInterlockedCount( &m_refCount );
        {
            if ( _rxBroadcaster.is() )
                _rxBroadcaster->addFlushListener( this );
        }
        osl_decrementInterlockedCount( &m_refCount );
        DBG_ASSERT( m_refCount == 1, "FlushNotificationAdapter::FlushNotificationAdapter: broadcaster isn't holding by hard ref!?" );
    }

    //------------------------------------------------------------
    FlushNotificationAdapter::~FlushNotificationAdapter()
    {
        DBG_DTOR( FlushNotificationAdapter, NULL );
    }

    //--------------------------------------------------------------------
    void SAL_CALL FlushNotificationAdapter::impl_dispose( bool _bRevokeListener )
    {
        Reference< XFlushListener > xKeepAlive( this );

        if ( _bRevokeListener )
        {
            Reference< XFlushable > xFlushable( m_aBroadcaster );
            if ( xFlushable.is() )
                xFlushable->removeFlushListener( this );
        }

        m_aListener = Reference< XFlushListener >();
        m_aBroadcaster = Reference< XFlushable >();
    }

    //--------------------------------------------------------------------
    void SAL_CALL FlushNotificationAdapter::flushed( const EventObject& rEvent ) throw (RuntimeException)
    {
        Reference< XFlushListener > xListener( m_aListener );
        if ( xListener.is() )
            xListener->flushed( rEvent );
        else
            impl_dispose( true );
    }

    //--------------------------------------------------------------------
    void SAL_CALL FlushNotificationAdapter::disposing( const EventObject& Source ) throw (RuntimeException)
    {
        DBG_ASSERT( Source.Source == m_aBroadcaster.get(), "FlushNotificationAdapter::disposing: where did this come from?" );

        Reference< XFlushListener > xListener( m_aListener );
        if ( xListener.is() )
            xListener->disposing( Source );

        impl_dispose( false );
    }

    //============================================================
    //= OAuthenticationContinuation
    //============================================================
	class OAuthenticationContinuation : public OInteraction< XInteractionSupplyAuthentication >
	{
		sal_Bool	m_bRemberPassword : 1;		// remember the password for this session ?

		::rtl::OUString		m_sUser;			// the user
		::rtl::OUString		m_sPassword;		// the user's password

	public:
		OAuthenticationContinuation();

		sal_Bool SAL_CALL canSetRealm(  ) throw(RuntimeException);
		void SAL_CALL setRealm( const ::rtl::OUString& Realm ) throw(RuntimeException);
		sal_Bool SAL_CALL canSetUserName(  ) throw(RuntimeException);
		void SAL_CALL setUserName( const ::rtl::OUString& UserName ) throw(RuntimeException);
		sal_Bool SAL_CALL canSetPassword(  ) throw(RuntimeException);
		void SAL_CALL setPassword( const ::rtl::OUString& Password ) throw(RuntimeException);
		Sequence< RememberAuthentication > SAL_CALL getRememberPasswordModes( RememberAuthentication& Default ) throw(RuntimeException);
		void SAL_CALL setRememberPassword( RememberAuthentication Remember ) throw(RuntimeException);
		sal_Bool SAL_CALL canSetAccount(  ) throw(RuntimeException);
		void SAL_CALL setAccount( const ::rtl::OUString& Account ) throw(RuntimeException);
		Sequence< RememberAuthentication > SAL_CALL getRememberAccountModes( RememberAuthentication& Default ) throw(RuntimeException);
		void SAL_CALL setRememberAccount( RememberAuthentication Remember ) throw(RuntimeException);

		::rtl::OUString	getUser() const				{ return m_sUser; }
		::rtl::OUString	getPassword() const			{ return m_sPassword; }
		sal_Bool		getRememberPassword() const	{ return m_bRemberPassword; }
	};

	//--------------------------------------------------------------------------
	OAuthenticationContinuation::OAuthenticationContinuation()
        :m_bRemberPassword(sal_True)	// TODO: a meaningfull default
	{
	}

	//--------------------------------------------------------------------------
	sal_Bool SAL_CALL OAuthenticationContinuation::canSetRealm(  ) throw(RuntimeException)
	{
		return sal_False;
	}

	//--------------------------------------------------------------------------
	void SAL_CALL OAuthenticationContinuation::setRealm( const ::rtl::OUString& /*Realm*/ ) throw(RuntimeException)
	{
		DBG_ERROR("OAuthenticationContinuation::setRealm: not supported!");
	}

	//--------------------------------------------------------------------------
	sal_Bool SAL_CALL OAuthenticationContinuation::canSetUserName(  ) throw(RuntimeException)
	{
        // we alwas allow this, even if the database document is read-only. In this case,
        // it's simply that the user cannot store the new user name.
		return sal_True;
	}

	//--------------------------------------------------------------------------
	void SAL_CALL OAuthenticationContinuation::setUserName( const ::rtl::OUString& _rUser ) throw(RuntimeException)
	{
		m_sUser = _rUser;
	}

	//--------------------------------------------------------------------------
	sal_Bool SAL_CALL OAuthenticationContinuation::canSetPassword(  ) throw(RuntimeException)
	{
		return sal_True;
	}

	//--------------------------------------------------------------------------
	void SAL_CALL OAuthenticationContinuation::setPassword( const ::rtl::OUString& _rPassword ) throw(RuntimeException)
	{
		m_sPassword = _rPassword;
	}

	//--------------------------------------------------------------------------
	Sequence< RememberAuthentication > SAL_CALL OAuthenticationContinuation::getRememberPasswordModes( RememberAuthentication& _reDefault ) throw(RuntimeException)
	{
		Sequence< RememberAuthentication > aReturn(1);
		_reDefault = aReturn[0] = RememberAuthentication_SESSION;
		return aReturn;
	}

	//--------------------------------------------------------------------------
	void SAL_CALL OAuthenticationContinuation::setRememberPassword( RememberAuthentication _eRemember ) throw(RuntimeException)
	{
		m_bRemberPassword = (RememberAuthentication_NO != _eRemember);
	}

	//--------------------------------------------------------------------------
	sal_Bool SAL_CALL OAuthenticationContinuation::canSetAccount(  ) throw(RuntimeException)
	{
		return sal_False;
	}

	//--------------------------------------------------------------------------
	void SAL_CALL OAuthenticationContinuation::setAccount( const ::rtl::OUString& ) throw(RuntimeException)
	{
		DBG_ERROR("OAuthenticationContinuation::setAccount: not supported!");
	}

	//--------------------------------------------------------------------------
	Sequence< RememberAuthentication > SAL_CALL OAuthenticationContinuation::getRememberAccountModes( RememberAuthentication& _reDefault ) throw(RuntimeException)
	{
		Sequence < RememberAuthentication > aReturn(1);
		aReturn[0] = RememberAuthentication_NO;
		_reDefault = RememberAuthentication_NO;
		return aReturn;
	}

	//--------------------------------------------------------------------------
	void SAL_CALL OAuthenticationContinuation::setRememberAccount( RememberAuthentication /*Remember*/ ) throw(RuntimeException)
	{
		DBG_ERROR("OAuthenticationContinuation::setRememberAccount: not supported!");
	}

	/** The class OSharedConnectionManager implements a structure to share connections.
		It owns the master connections which will be disposed when the last connection proxy is gone.
	*/
	typedef ::cppu::WeakImplHelper1< XEventListener > OConnectionHelper_BASE;
	// need to hold the digest
	struct TDigestHolder
	{
		sal_uInt8 m_pBuffer[RTL_DIGEST_LENGTH_SHA1];
		TDigestHolder()
		{
			m_pBuffer[0] = 0;
		}

	};

	class OSharedConnectionManager : public OConnectionHelper_BASE
	{

		 // contains the currently used master connections
		typedef struct
		{
			Reference< XConnection >	xMasterConnection;
			oslInterlockedCount			nALiveCount;
		} TConnectionHolder;

		// the less-compare functor, used for the stl::map
		struct TDigestLess : public ::std::binary_function< TDigestHolder, TDigestHolder, bool>
		{
			bool operator() (const TDigestHolder& x, const TDigestHolder& y) const
			{
				sal_uInt32 i;
				for(i=0;i < RTL_DIGEST_LENGTH_SHA1 && (x.m_pBuffer[i] >= y.m_pBuffer[i]); ++i)
					;
				return i < RTL_DIGEST_LENGTH_SHA1;
			}
		};

		typedef ::std::map< TDigestHolder,TConnectionHolder,TDigestLess>		TConnectionMap;		 // holds the master connections
		typedef ::std::map< Reference< XConnection >,TConnectionMap::iterator>	TSharedConnectionMap;// holds the shared connections

		::osl::Mutex				m_aMutex;
		TConnectionMap				m_aConnections;			// remeber the master connection in conjunction with the digest
		TSharedConnectionMap		m_aSharedConnection;	// the shared connections with conjunction with an iterator into the connections map
		Reference< XProxyFactory >	m_xProxyFactory;

	protected:
		~OSharedConnectionManager();

	public:
		OSharedConnectionManager(const Reference< XMultiServiceFactory >& _rxServiceFactory);

		void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw(RuntimeException);
		Reference<XConnection> getConnection(	const rtl::OUString& url,
												const rtl::OUString& user,
												const rtl::OUString& password,
												const Sequence< PropertyValue >& _aInfo,
												ODatabaseSource* _pDataSource);
		void addEventListener(const Reference<XConnection>& _rxConnection,TConnectionMap::iterator& _rIter);
	};

	DBG_NAME(OSharedConnectionManager)
	OSharedConnectionManager::OSharedConnectionManager(const Reference< XMultiServiceFactory >& _rxServiceFactory)
	{
		DBG_CTOR(OSharedConnectionManager,NULL);
		m_xProxyFactory.set(_rxServiceFactory->createInstance(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.reflection.ProxyFactory"))),UNO_QUERY);
	}

	OSharedConnectionManager::~OSharedConnectionManager()
	{
		DBG_DTOR(OSharedConnectionManager,NULL);
	}

	void SAL_CALL OSharedConnectionManager::disposing( const ::com::sun::star::lang::EventObject& Source ) throw(RuntimeException)
	{
		MutexGuard aGuard(m_aMutex);
		Reference<XConnection> xConnection(Source.Source,UNO_QUERY);
		TSharedConnectionMap::iterator aFind = m_aSharedConnection.find(xConnection);
		if ( m_aSharedConnection.end() != aFind )
		{
			osl_decrementInterlockedCount(&aFind->second->second.nALiveCount);
			if ( !aFind->second->second.nALiveCount )
			{
				::comphelper::disposeComponent(aFind->second->second.xMasterConnection);
				m_aConnections.erase(aFind->second);
			}
			m_aSharedConnection.erase(aFind);
		}
	}

	Reference<XConnection> OSharedConnectionManager::getConnection(	const rtl::OUString& url,
											const rtl::OUString& user,
											const rtl::OUString& password,
											const Sequence< PropertyValue >& _aInfo,
											ODatabaseSource* _pDataSource)
	{
		MutexGuard aGuard(m_aMutex);
		TConnectionMap::key_type nId;
		Sequence< PropertyValue > aInfoCopy(_aInfo);
		sal_Int32 nPos = aInfoCopy.getLength();
		aInfoCopy.realloc( nPos + 2 );
		aInfoCopy[nPos].Name	  = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("TableFilter"));
		aInfoCopy[nPos++].Value <<= _pDataSource->m_pImpl->m_aTableFilter;
		aInfoCopy[nPos].Name	  = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("TableTypeFilter"));
		aInfoCopy[nPos++].Value <<= _pDataSource->m_pImpl->m_aTableTypeFilter; // #22377# OJ

		::rtl::OUString sUser = user;
		::rtl::OUString sPassword = password;
		if ((0 == sUser.getLength()) && (0 == sPassword.getLength()) && (0 != _pDataSource->m_pImpl->m_sUser.getLength()))
		{	// ease the usage of this method. data source which are intended to have a user automatically
			// fill in the user/password combination if the caller of this method does not specify otherwise
			// 86951 - 05/08/2001 - frank.schoenheit@germany.sun.com
			sUser = _pDataSource->m_pImpl->m_sUser;
			if (0 != _pDataSource->m_pImpl->m_aPassword.getLength())
				sPassword = _pDataSource->m_pImpl->m_aPassword;
		}

		::connectivity::OConnectionWrapper::createUniqueId(url,aInfoCopy,nId.m_pBuffer,sUser,sPassword);
		TConnectionMap::iterator aIter = m_aConnections.find(nId);

		if ( m_aConnections.end() == aIter )
		{
			TConnectionHolder aHolder;
			aHolder.nALiveCount = 0; // will be incremented by addListener
			aHolder.xMasterConnection = _pDataSource->buildIsolatedConnection(user,password);
			aIter = m_aConnections.insert(TConnectionMap::value_type(nId,aHolder)).first;
		}

		Reference<XConnection> xRet;
		if ( aIter->second.xMasterConnection.is() )
		{
			Reference< XAggregation > xConProxy = m_xProxyFactory->createProxy(aIter->second.xMasterConnection.get());
			xRet = new OSharedConnection(xConProxy);
			m_aSharedConnection.insert(TSharedConnectionMap::value_type(xRet,aIter));
			addEventListener(xRet,aIter);
		}

		return xRet;
	}
	void OSharedConnectionManager::addEventListener(const Reference<XConnection>& _rxConnection,TConnectionMap::iterator& _rIter)
	{
		Reference<XComponent> xComp(_rxConnection,UNO_QUERY);
		xComp->addEventListener(this);
		OSL_ENSURE( m_aConnections.end() != _rIter , "Iterator is end!");
		osl_incrementInterlockedCount(&_rIter->second.nALiveCount);
	}

    //----------------------------------------------------------------------
	namespace
	{
        //------------------------------------------------------------------
        Sequence< PropertyValue > lcl_filterDriverProperties( const Reference< XDriver >& _xDriver, const ::rtl::OUString& _sUrl,
            const Sequence< PropertyValue >& _rDataSourceSettings, const AsciiPropertyValue* _pKnownSettings )
		{
			if ( _xDriver.is() )
			{
				Sequence< DriverPropertyInfo > aDriverInfo(_xDriver->getPropertyInfo(_sUrl,_rDataSourceSettings));

				const PropertyValue* pDataSourceSetting = _rDataSourceSettings.getConstArray();
				const PropertyValue* pEnd = pDataSourceSetting + _rDataSourceSettings.getLength();

				::std::vector< PropertyValue > aRet;

				for ( ; pDataSourceSetting != pEnd ; ++pDataSourceSetting )
				{
					sal_Bool bAllowSetting = sal_False;
                    const AsciiPropertyValue* pSetting = _pKnownSettings;
					for ( ; pSetting->AsciiName; ++pSetting )
					{
						if ( !pDataSourceSetting->Name.compareToAscii( pSetting->AsciiName ) )
						{   // the particular data source setting is known

                            const DriverPropertyInfo* pAllowedDriverSetting = aDriverInfo.getConstArray();
							const DriverPropertyInfo* pDriverSettingsEnd = pAllowedDriverSetting + aDriverInfo.getLength();
							for ( ; pAllowedDriverSetting != pDriverSettingsEnd; ++pAllowedDriverSetting )
							{
								if ( !pAllowedDriverSetting->Name.compareToAscii( pSetting->AsciiName ) )
								{   // the driver also allows this setting
									bAllowSetting = sal_True;
									break;
								}
							}
							break;
						}
					}
					if ( bAllowSetting || !pSetting->AsciiName )
					{   // if the driver allows this particular setting, or if the setting is completely unknown,
                        // we pass it to the driver
						aRet.push_back( *pDataSourceSetting );
					}
				}
				if ( !aRet.empty() )
					return Sequence< PropertyValue >(&(*aRet.begin()),aRet.size()); 
			}
			return Sequence< PropertyValue >();
		}

        //------------------------------------------------------------------
        typedef ::std::map< ::rtl::OUString, sal_Int32 > PropertyAttributeCache;

        //------------------------------------------------------------------
        struct IsDefaultAndNotRemoveable : public ::std::unary_function< PropertyValue, bool >
        {
        private:
            const PropertyAttributeCache& m_rAttribs;

        public:
            IsDefaultAndNotRemoveable( const PropertyAttributeCache& _rAttribs ) : m_rAttribs( _rAttribs ) { }

            bool operator()( const PropertyValue& _rProp )
            {
                if ( _rProp.State != PropertyState_DEFAULT_VALUE )
                    return false;

                bool bRemoveable = true;

                PropertyAttributeCache::const_iterator pos = m_rAttribs.find( _rProp.Name );
                OSL_ENSURE( pos != m_rAttribs.end(), "IsDefaultAndNotRemoveable: illegal property name!" );
                if ( pos != m_rAttribs.end() )
                    bRemoveable = ( ( pos->second & PropertyAttribute::REMOVEABLE ) != 0 );

                return !bRemoveable;
            }
        };
	}
//============================================================
//= ODatabaseContext
//============================================================
DBG_NAME(ODatabaseSource)
//--------------------------------------------------------------------------
extern "C" void SAL_CALL createRegistryInfo_ODatabaseSource()
{
	static ::dba::OAutoRegistration< ODatabaseSource > aAutoRegistration;
}

//--------------------------------------------------------------------------
ODatabaseSource::ODatabaseSource(const ::rtl::Reference<ODatabaseModelImpl>& _pImpl)
			:ModelDependentComponent( _pImpl )
            ,OSubComponent( getMutex(), Reference< XInterface >() )
			,OPropertySetHelper(OComponentHelper::rBHelper)
            ,m_aBookmarks( *this, getMutex() )
            ,m_aFlushListeners( getMutex() )
{
	// some kind of default
	DBG_CTOR(ODatabaseSource,NULL);
}

//--------------------------------------------------------------------------
ODatabaseSource::~ODatabaseSource()
{
	DBG_DTOR(ODatabaseSource,NULL);
	if ( !OComponentHelper::rBHelper.bInDispose && !OComponentHelper::rBHelper.bDisposed )
	{
		acquire();
		dispose();
	}
}

//--------------------------------------------------------------------------
void ODatabaseSource::setName( const Reference< XDocumentDataSource >& _rxDocument, const ::rtl::OUString& _rNewName, DBContextAccess )
{
    ODatabaseSource& rModelImpl = dynamic_cast< ODatabaseSource& >( *_rxDocument.get() );

    ::osl::MutexGuard aGuard( rModelImpl.m_aMutex );
    if ( rModelImpl.m_pImpl.is() )
        rModelImpl.m_pImpl->m_sName = _rNewName;
}

// com::sun::star::lang::XTypeProvider
//--------------------------------------------------------------------------
Sequence< Type > ODatabaseSource::getTypes() throw (RuntimeException)
{
	OTypeCollection aPropertyHelperTypes(	::getCppuType( (const Reference< XFastPropertySet > *)0 ),
											::getCppuType( (const Reference< XPropertySet > *)0 ),
											::getCppuType( (const Reference< XMultiPropertySet > *)0 ));

	return ::comphelper::concatSequences(
		::comphelper::concatSequences(
			OSubComponent::getTypes(),
			aPropertyHelperTypes.getTypes()
		),
		ODatabaseSource_Base::getTypes()
	);
}

//--------------------------------------------------------------------------
Sequence< sal_Int8 > ODatabaseSource::getImplementationId() throw (RuntimeException)
{
	static OImplementationId * pId = 0;
	if (! pId)
	{
		MutexGuard aGuard( Mutex::getGlobalMutex() );
		if (! pId)
		{
			static OImplementationId aId;
			pId = &aId;
		}
	}
	return pId->getImplementationId();
}

// com::sun::star::uno::XInterface
//--------------------------------------------------------------------------
Any ODatabaseSource::queryInterface( const Type & rType ) throw (RuntimeException)
{
	Any aIface = OSubComponent::queryInterface( rType );
	if (!aIface.hasValue())
	{
		aIface = ODatabaseSource_Base::queryInterface( rType );
		if ( !aIface.hasValue() )
		{
			aIface = ::cppu::queryInterface(
						rType,
						static_cast< XPropertySet* >( this ),
						static_cast< XFastPropertySet* >( this ),
						static_cast< XMultiPropertySet* >( this ));
	    }
	}
	return aIface;
}

//--------------------------------------------------------------------------
void ODatabaseSource::acquire() throw ()
{
	OSubComponent::acquire();
}

//--------------------------------------------------------------------------
void ODatabaseSource::release() throw ()
{
	OSubComponent::release();
}
// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::disposing( const ::com::sun::star::lang::EventObject& Source ) throw(RuntimeException)
{
	if ( m_pImpl.is() )
		m_pImpl->disposing(Source);
}
// XServiceInfo
//------------------------------------------------------------------------------
rtl::OUString ODatabaseSource::getImplementationName(  ) throw(RuntimeException)
{
	return getImplementationName_static();
}

//------------------------------------------------------------------------------
rtl::OUString ODatabaseSource::getImplementationName_static(  ) throw(RuntimeException)
{
	return rtl::OUString::createFromAscii("com.sun.star.comp.dba.ODatabaseSource");
}

//------------------------------------------------------------------------------
Sequence< ::rtl::OUString > ODatabaseSource::getSupportedServiceNames(  ) throw (RuntimeException)
{
	return getSupportedServiceNames_static();
}
//------------------------------------------------------------------------------
Reference< XInterface > ODatabaseSource::Create( const Reference< XComponentContext >& _rxContext )
{
    ::comphelper::ComponentContext aContext( _rxContext );
    Reference< XSingleServiceFactory > xDBContext( aContext.createComponent( (::rtl::OUString)SERVICE_SDB_DATABASECONTEXT ), UNO_QUERY_THROW );
    return xDBContext->createInstance();
}

//------------------------------------------------------------------------------
Sequence< ::rtl::OUString > ODatabaseSource::getSupportedServiceNames_static(  ) throw (RuntimeException)
{
	Sequence< ::rtl::OUString > aSNS( 2 );
	aSNS[0] = SERVICE_SDB_DATASOURCE;
    aSNS[1] = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.sdb.DocumentDataSource"));
	return aSNS;
}

//------------------------------------------------------------------------------
sal_Bool ODatabaseSource::supportsService( const ::rtl::OUString& _rServiceName ) throw (RuntimeException)
{
	return ::comphelper::findValue(getSupportedServiceNames(), _rServiceName, sal_True).getLength() != 0;
}
// OComponentHelper
//------------------------------------------------------------------------------
void ODatabaseSource::disposing()
{
	OSubComponent::disposing();
	OPropertySetHelper::disposing();

	EventObject aDisposeEvent(static_cast<XWeak*>(this));
	m_aFlushListeners.disposeAndClear( aDisposeEvent );

    ODatabaseDocument::clearObjectContainer(m_pImpl->m_xCommandDefinitions);
    ODatabaseDocument::clearObjectContainer(m_pImpl->m_xTableDefinitions);
	m_pImpl.clear();
}
//------------------------------------------------------------------------------
Reference< XConnection > ODatabaseSource::buildLowLevelConnection(const ::rtl::OUString& _rUid, const ::rtl::OUString& _rPwd)
{
	Reference< XConnection > xReturn;

    Reference< XDriverManager > xManager;
    if ( !m_pImpl->m_aContext.createComponent( "com.sun.star.sdbc.ConnectionPool", xManager ) )
        // no connection pool installed, fall back to driver manager
        m_pImpl->m_aContext.createComponent( "com.sun.star.sdbc.DriverManager", xManager );

	::rtl::OUString sUser(_rUid);
	::rtl::OUString sPwd(_rPwd);
	if ((0 == sUser.getLength()) && (0 == sPwd.getLength()) && (0 != m_pImpl->m_sUser.getLength()))
	{	// ease the usage of this method. data source which are intended to have a user automatically
		// fill in the user/password combination if the caller of this method does not specify otherwise
		// 86951 - 05/08/2001 - frank.schoenheit@germany.sun.com
		sUser = m_pImpl->m_sUser;
		if (0 != m_pImpl->m_aPassword.getLength())
			sPwd = m_pImpl->m_aPassword;
	}

	sal_uInt16 nExceptionMessageId = RID_STR_COULDNOTCONNECT_UNSPECIFIED;
	if (xManager.is())
	{
		sal_Int32 nAdditionalArgs(0);
		if (sUser.getLength()) ++nAdditionalArgs;
		if (sPwd.getLength()) ++nAdditionalArgs;

		Sequence< PropertyValue > aUserPwd(nAdditionalArgs);
		sal_Int32 nArgPos = 0;
		if (sUser.getLength())
		{
			aUserPwd[ nArgPos ].Name = ::rtl::OUString::createFromAscii("user");
			aUserPwd[ nArgPos ].Value <<= sUser;
			++nArgPos;
		}
		if (sPwd.getLength())
		{
			aUserPwd[ nArgPos ].Name = ::rtl::OUString::createFromAscii("password");
			aUserPwd[ nArgPos ].Value <<= sPwd;
		}
		Reference< XDriver > xDriver;
		try
		{
			Reference< XDriverAccess > xAccessDrivers( xManager, UNO_QUERY );
			if ( xAccessDrivers.is() )
				xDriver = xAccessDrivers->getDriverByURL( m_pImpl->m_sConnectURL );
		}
		catch( const Exception& )
		{
			DBG_ERROR( "ODatabaseSource::buildLowLevelConnection: got a strange exception while analyzing the error!" );
		}
		if ( !xDriver.is() )
			nExceptionMessageId = RID_STR_COULDNOTCONNECT_NODRIVER;
		else
		{
            Sequence< PropertyValue > aDriverInfo = lcl_filterDriverProperties(
                xDriver,
                m_pImpl->m_sConnectURL,
                m_pImpl->m_xSettings->getPropertyValues(),
                m_pImpl->getDefaultDataSourceSettings()
            );

            impl_insertJavaDriverClassPath_nothrow(aDriverInfo);

			if ( m_pImpl->isEmbeddedDatabase() )
			{
				sal_Int32 nCount = aDriverInfo.getLength();
				aDriverInfo.realloc(nCount + 2 );
				aDriverInfo[nCount].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("URL"));
				aDriverInfo[nCount++].Value <<= m_pImpl->getURL();
				aDriverInfo[nCount].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Storage"));
                Reference< css::document::XDocumentSubStorageSupplier> xDocSup( m_pImpl->getDocumentSubStorageSupplier() );
				aDriverInfo[nCount++].Value <<= xDocSup->getDocumentSubStorage(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("database")),ElementModes::READWRITE);
			}
			if (nAdditionalArgs)
				xReturn = xManager->getConnectionWithInfo(m_pImpl->m_sConnectURL, ::comphelper::concatSequences(aUserPwd,aDriverInfo));
			else
				xReturn = xManager->getConnectionWithInfo(m_pImpl->m_sConnectURL,aDriverInfo);

            if ( m_pImpl->isEmbeddedDatabase() )
            {
                // see ODatabaseSource::flushed for comment on why we register as FlushListener
                // at the connection
                Reference< XFlushable > xFlushable( xReturn, UNO_QUERY );
                if ( xFlushable.is() )
                    FlushNotificationAdapter::installAdapter( xFlushable, this );
            }
		}
	}
	else
		nExceptionMessageId = RID_STR_COULDNOTLOAD_MANAGER;

	if ( !xReturn.is() )
	{
#if defined USE_JAVA && defined MACOSX
		if ( !pApplication_canUseJava )
			pApplication_canUseJava = (Application_canUseJava_Type *)dlsym( RTLD_MAIN_ONLY, "Application_canUseJava" );
		if ( !pApplication_canUseJava || !pApplication_canUseJava() )
		{
			static ::rtl::OUString aEmbeddedHSQLDB( RTL_CONSTASCII_USTRINGPARAM( "sdbc:embedded:hsqldb" ) );
			static ::rtl::OUString aJDBC( RTL_CONSTASCII_USTRINGPARAM( "jdbc:" ) );
			static ::rtl::OUString aMySQLJDBC( RTL_CONSTASCII_USTRINGPARAM( "sdbc:mysql:jdbc:" ) );
			static ::rtl::OUString aOracleJDBC( RTL_CONSTASCII_USTRINGPARAM( "jdbc:oracle:thin:" ) );
			if ( ( aEmbeddedHSQLDB.getLength() && m_pImpl->m_sConnectURL.indexOf( aEmbeddedHSQLDB ) )
				|| ( aJDBC.getLength() && m_pImpl->m_sConnectURL.indexOf( aJDBC ) )
				|| ( aMySQLJDBC.getLength() && m_pImpl->m_sConnectURL.indexOf( aMySQLJDBC ) )
				|| ( aOracleJDBC.getLength() && m_pImpl->m_sConnectURL.indexOf( aOracleJDBC ) ) )
					nExceptionMessageId = RID_STR_COULDNOTCONNECT_NOJAVA;
		}
#endif	// USE_JAVA && MACOSX

		::rtl::OUString sMessage = DBACORE_RESSTRING( nExceptionMessageId );

		SQLContext aContext;
        aContext.Message = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "A connection for the following URL was requested: " ) );
            // TODO: resource
        aContext.Message += m_pImpl->m_sConnectURL;

		throwGenericSQLException( sMessage, static_cast< XDataSource* >( this ), makeAny( aContext ) );
	}

	return xReturn;
}

// OPropertySetHelper
//------------------------------------------------------------------------------
Reference< XPropertySetInfo >  ODatabaseSource::getPropertySetInfo() throw (RuntimeException)
{
	return createPropertySetInfo( getInfoHelper() ) ;
}

// comphelper::OPropertyArrayUsageHelper
//------------------------------------------------------------------------------
::cppu::IPropertyArrayHelper* ODatabaseSource::createArrayHelper( ) const
{
	BEGIN_PROPERTY_HELPER(13)
		DECL_PROP1(INFO,						Sequence< PropertyValue >,  BOUND);
		DECL_PROP1_BOOL(ISPASSWORDREQUIRED,									BOUND);
		DECL_PROP1_BOOL(ISREADONLY,											READONLY);
		DECL_PROP1(LAYOUTINFORMATION,			Sequence< PropertyValue >,  BOUND);
		DECL_PROP1(NAME,						::rtl::OUString,			READONLY);
		DECL_PROP2_IFACE(NUMBERFORMATSSUPPLIER,	XNumberFormatsSupplier,		READONLY, TRANSIENT);
		DECL_PROP1(PASSWORD,					::rtl::OUString,			TRANSIENT);
		DECL_PROP2_IFACE(SETTINGS,              XPropertySet,               BOUND, READONLY);
		DECL_PROP1_BOOL(SUPPRESSVERSIONCL,									BOUND);
		DECL_PROP1(TABLEFILTER,					Sequence< ::rtl::OUString >,BOUND);
		DECL_PROP1(TABLETYPEFILTER,				Sequence< ::rtl::OUString >,BOUND);
		DECL_PROP1(URL,							::rtl::OUString,			BOUND);
		DECL_PROP1(USER,						::rtl::OUString,			BOUND);
	END_PROPERTY_HELPER();
}

// cppu::OPropertySetHelper
//------------------------------------------------------------------------------
::cppu::IPropertyArrayHelper& ODatabaseSource::getInfoHelper()
{
	return *getArrayHelper();
}

//------------------------------------------------------------------------------
sal_Bool ODatabaseSource::convertFastPropertyValue(Any & rConvertedValue, Any & rOldValue, sal_Int32 nHandle, const Any& rValue ) throw( IllegalArgumentException  )
{
    sal_Bool bModified(sal_False);
    if ( m_pImpl.is() )
    {
	    switch (nHandle)
	    {
		    case PROPERTY_ID_TABLEFILTER:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_aTableFilter);
			    break;
		    case PROPERTY_ID_TABLETYPEFILTER:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_aTableTypeFilter);
			    break;
		    case PROPERTY_ID_USER:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_sUser);
			    break;
		    case PROPERTY_ID_PASSWORD:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_aPassword);
			    break;
		    case PROPERTY_ID_ISPASSWORDREQUIRED:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_bPasswordRequired);
			    break;
		    case PROPERTY_ID_SUPPRESSVERSIONCL:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_bSuppressVersionColumns);
			    break;
		    case PROPERTY_ID_LAYOUTINFORMATION:
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_aLayoutInformation);
			    break;
		    case PROPERTY_ID_URL:
		    {
			    bModified = ::comphelper::tryPropertyValue(rConvertedValue, rOldValue, rValue, m_pImpl->m_sConnectURL);
		    }	break;
		    case PROPERTY_ID_INFO:
		    {
			    Sequence<PropertyValue> aValues;
			    if (!(rValue >>= aValues))
				    throw IllegalArgumentException();

			    const PropertyValue* valueEnd = aValues.getConstArray() + aValues.getLength();
			    const PropertyValue* checkName = aValues.getConstArray();
			    for ( ;checkName != valueEnd; ++checkName )
			    {
				    if ( !checkName->Name.getLength() )
					    throw IllegalArgumentException();
			    }

                Sequence< PropertyValue > aSettings = m_pImpl->m_xSettings->getPropertyValues();
                bModified = aSettings.getLength() != aValues.getLength();
			    if ( !bModified )
			    {
                    const PropertyValue* pInfoIter = aSettings.getConstArray();
				    const PropertyValue* checkValue = aValues.getConstArray();
				    for ( ;!bModified && checkValue != valueEnd ; ++checkValue,++pInfoIter)
				    {
					    bModified = checkValue->Name != pInfoIter->Name;
					    if ( !bModified )
					    {
						    bModified = !::comphelper::compare(checkValue->Value,pInfoIter->Value);
					    }
				    }
			    }

			    rConvertedValue = rValue;
                rOldValue <<= aSettings;
            }
            break;
		    default:
                DBG_ERROR( "ODatabaseSource::convertFastPropertyValue: unknown or readonly Property!" );
	    }
    }
	return bModified;
}

//------------------------------------------------------------------------------
namespace
{
    struct SelectPropertyName : public ::std::unary_function< PropertyValue, ::rtl::OUString >
    {
    public:
        const ::rtl::OUString& operator()( const PropertyValue& _lhs )
        {
            return _lhs.Name;
        }
    };

    /** sets a new set of property values at a given property bag instance

        The methods takes a property bag, and a sequence of property values to set at this bag.
        Upon return, every property which is not part of the given sequence is
        <ul><li>removed from the bag, if it's a removeable property</li>
            <li><em>or</em>reset to its default value, if it's not a removeable property</li>
        </ul>.

        @param  _rxPropertyBag
            the property bag to operate on
        @param  _rAllNewPropertyValues
            the new property values to set at the bag
    */
    void lcl_setPropertyValues_resetOrRemoveOther( const Reference< XPropertyAccess >& _rxPropertyBag, const Sequence< PropertyValue >& _rAllNewPropertyValues )
    {
        // sequences are ugly to operate on
        typedef ::std::set< ::rtl::OUString >   StringSet;
        StringSet aToBeSetPropertyNames;
        ::std::transform(
            _rAllNewPropertyValues.getConstArray(),
            _rAllNewPropertyValues.getConstArray() + _rAllNewPropertyValues.getLength(),
            ::std::insert_iterator< StringSet >( aToBeSetPropertyNames, aToBeSetPropertyNames.end() ),
            SelectPropertyName()
        );

        try
        {
            // obtain all properties currently known at the bag
            Reference< XPropertySet > xPropertySet( _rxPropertyBag, UNO_QUERY_THROW );
            Reference< XPropertySetInfo > xPSI( xPropertySet->getPropertySetInfo(), UNO_QUERY_THROW );
            Sequence< Property > aAllExistentProperties( xPSI->getProperties() );

            Reference< XPropertyState > xPropertyState( _rxPropertyBag, UNO_QUERY_THROW );
            Reference< XPropertyContainer > xPropertyContainer( _rxPropertyBag, UNO_QUERY_THROW );

            // loop through them, and reset resp. default properties which are not to be set
            const Property* pExistentProperty( aAllExistentProperties.getConstArray() );
            const Property* pExistentPropertyEnd( aAllExistentProperties.getConstArray() + aAllExistentProperties.getLength() );
            for ( ; pExistentProperty != pExistentPropertyEnd; ++pExistentProperty )
            {
                if ( aToBeSetPropertyNames.find( pExistentProperty->Name ) != aToBeSetPropertyNames.end() )
                    continue;

                // this property is not to be set, but currently exists in the bag.
                // -> Remove, respectively default, it
                if ( ( pExistentProperty->Attributes & PropertyAttribute::REMOVEABLE ) != 0 )
                    xPropertyContainer->removeProperty( pExistentProperty->Name );
                else
                    xPropertyState->setPropertyToDefault( pExistentProperty->Name );
            }

            // finally, set the new property values
            _rxPropertyBag->setPropertyValues( _rAllNewPropertyValues );
        }
        catch( const Exception& )
        {
        	DBG_UNHANDLED_EXCEPTION();
        }
    }
}

//------------------------------------------------------------------------------
void ODatabaseSource::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const Any& rValue ) throw (Exception)
{
    if ( m_pImpl.is() )
    {
	    switch(nHandle)
	    {
		    case PROPERTY_ID_TABLEFILTER:
			    rValue >>= m_pImpl->m_aTableFilter;
			    break;
		    case PROPERTY_ID_TABLETYPEFILTER:
			    rValue >>= m_pImpl->m_aTableTypeFilter;
			    break;
		    case PROPERTY_ID_USER:
			    rValue >>= m_pImpl->m_sUser;
			    // if the user name changed, reset the password
			    m_pImpl->m_aPassword = ::rtl::OUString();
			    break;
		    case PROPERTY_ID_PASSWORD:
			    rValue >>= m_pImpl->m_aPassword;
			    break;
		    case PROPERTY_ID_ISPASSWORDREQUIRED:
			    m_pImpl->m_bPasswordRequired = any2bool(rValue);
			    break;
		    case PROPERTY_ID_SUPPRESSVERSIONCL:
			    m_pImpl->m_bSuppressVersionColumns = any2bool(rValue);
			    break;
		    case PROPERTY_ID_URL:
			    rValue >>= m_pImpl->m_sConnectURL;
			    break;
		    case PROPERTY_ID_INFO:
            {
                Sequence< PropertyValue > aInfo;
                OSL_VERIFY( rValue >>= aInfo );
                lcl_setPropertyValues_resetOrRemoveOther( m_pImpl->m_xSettings, aInfo );
            }
            break;
		    case PROPERTY_ID_LAYOUTINFORMATION:
			    rValue >>= m_pImpl->m_aLayoutInformation;
			    break;
	    }
	    m_pImpl->setModified(sal_True);
    }
}

//------------------------------------------------------------------------------
void ODatabaseSource::getFastPropertyValue( Any& rValue, sal_Int32 nHandle ) const
{
    if ( m_pImpl.is() )
    {
	    switch (nHandle)
	    {
		    case PROPERTY_ID_TABLEFILTER:
			    rValue <<= m_pImpl->m_aTableFilter;
			    break;
		    case PROPERTY_ID_TABLETYPEFILTER:
			    rValue <<= m_pImpl->m_aTableTypeFilter;
			    break;
		    case PROPERTY_ID_USER:
			    rValue <<= m_pImpl->m_sUser;
			    break;
		    case PROPERTY_ID_PASSWORD:
			    rValue <<= m_pImpl->m_aPassword;
			    break;
		    case PROPERTY_ID_ISPASSWORDREQUIRED:
			    rValue = bool2any(m_pImpl->m_bPasswordRequired);
			    break;
		    case PROPERTY_ID_SUPPRESSVERSIONCL:
			    rValue = bool2any(m_pImpl->m_bSuppressVersionColumns);
			    break;
		    case PROPERTY_ID_ISREADONLY:
			    rValue = bool2any(m_pImpl->m_bReadOnly);
			    break;
		    case PROPERTY_ID_INFO:
            {
                try
                {
                    // collect the property attributes of all current settings
                    Reference< XPropertySet > xSettingsAsProps( m_pImpl->m_xSettings, UNO_QUERY_THROW );
                    Reference< XPropertySetInfo > xPST( xSettingsAsProps->getPropertySetInfo(), UNO_QUERY_THROW );
                    Sequence< Property > aSettings( xPST->getProperties() );
                    ::std::map< ::rtl::OUString, sal_Int32 > aPropertyAttributes;
                    for (   const Property* pSettings = aSettings.getConstArray();
                            pSettings != aSettings.getConstArray() + aSettings.getLength();
                            ++pSettings
                        )
                    {
                        aPropertyAttributes[ pSettings->Name ] = pSettings->Attributes;
                    }

                    // get all current settings with their values
                    Sequence< PropertyValue > aValues( m_pImpl->m_xSettings->getPropertyValues() );

                    // transform them so that only property values which fulfill certain
                    // criterions survive
                    Sequence< PropertyValue > aNonDefaultOrUserDefined( aValues.getLength() );
                    const PropertyValue* pCopyEnd = ::std::remove_copy_if(
                        aValues.getConstArray(),
                        aValues.getConstArray() + aValues.getLength(),
                        aNonDefaultOrUserDefined.getArray(),
                        IsDefaultAndNotRemoveable( aPropertyAttributes )
                    );
                    aNonDefaultOrUserDefined.realloc( pCopyEnd - aNonDefaultOrUserDefined.getArray() );
                    rValue <<= aNonDefaultOrUserDefined;
                }
                catch( const Exception& )
                {
                	DBG_UNHANDLED_EXCEPTION();
                }
            }
            break;
            case PROPERTY_ID_SETTINGS:
                rValue <<= m_pImpl->m_xSettings;
			    break;
		    case PROPERTY_ID_URL:
			    rValue <<= m_pImpl->m_sConnectURL;
			    break;
		    case PROPERTY_ID_NUMBERFORMATSSUPPLIER:
			    rValue <<= m_pImpl->getNumberFormatsSupplier();
			    break;
		    case PROPERTY_ID_NAME:
			    rValue <<= m_pImpl->m_sName;
			    break;
		    case PROPERTY_ID_LAYOUTINFORMATION:
			    rValue <<= m_pImpl->m_aLayoutInformation;
			    break;
		    default:
			    DBG_ERROR("unknown Property");
	    }
    }
}

// XDataSource
//------------------------------------------------------------------------------
void ODatabaseSource::setLoginTimeout(sal_Int32 seconds) throw( SQLException, RuntimeException )
{
    ModelMethodGuard aGuard( *this );
	m_pImpl->m_nLoginTimeout = seconds;
}

//------------------------------------------------------------------------------
sal_Int32 ODatabaseSource::getLoginTimeout(void) throw( SQLException, RuntimeException )
{
    ModelMethodGuard aGuard( *this );
	return m_pImpl->m_nLoginTimeout;
}


// XCompletedConnection
//------------------------------------------------------------------------------
Reference< XConnection > SAL_CALL ODatabaseSource::connectWithCompletion( const Reference< XInteractionHandler >& _rxHandler ) throw(SQLException, RuntimeException)
{
	return connectWithCompletion(_rxHandler,sal_False);
}
// -----------------------------------------------------------------------------
Reference< XConnection > ODatabaseSource::getConnection(const rtl::OUString& user, const rtl::OUString& password) throw( SQLException, RuntimeException )
{
	return getConnection(user,password,sal_False);
}
// -----------------------------------------------------------------------------
Reference< XConnection > SAL_CALL ODatabaseSource::getIsolatedConnection( const ::rtl::OUString& user, const ::rtl::OUString& password ) throw(SQLException, RuntimeException)
{
	return getConnection(user,password,sal_True);
}
// -----------------------------------------------------------------------------
Reference< XConnection > SAL_CALL ODatabaseSource::getIsolatedConnectionWithCompletion( const Reference< XInteractionHandler >& _rxHandler ) throw(SQLException, RuntimeException)
{
	return connectWithCompletion(_rxHandler,sal_True);
}
// -----------------------------------------------------------------------------
Reference< XConnection > SAL_CALL ODatabaseSource::connectWithCompletion( const Reference< XInteractionHandler >& _rxHandler,sal_Bool _bIsolated ) throw(SQLException, RuntimeException)
{
    ModelMethodGuard aGuard( *this );

	if (!_rxHandler.is())
	{
		DBG_ERROR("ODatabaseSource::connectWithCompletion: invalid interaction handler!");
		return getConnection(m_pImpl->m_sUser, m_pImpl->m_aPassword,_bIsolated);
	}

	::rtl::OUString sUser(m_pImpl->m_sUser), sPassword(m_pImpl->m_aPassword);
	sal_Bool bNewPasswordGiven = sal_False;

	if (m_pImpl->m_bPasswordRequired && (0 == sPassword.getLength()))
	{	// we need a password, but don't have one yet.
		// -> ask the user

		// build an interaction request
		// two continuations (Ok and Cancel)
		OInteractionAbort* pAbort = new OInteractionAbort;
		OAuthenticationContinuation* pAuthenticate = new OAuthenticationContinuation;

        // the name which should be referred in the login dialog
        ::rtl::OUString sServerName( m_pImpl->m_sName );
        INetURLObject aURLCheck( sServerName );
        if ( aURLCheck.GetProtocol() != INET_PROT_NOT_VALID )
            sServerName = aURLCheck.getBase( INetURLObject::LAST_SEGMENT, true, INetURLObject::DECODE_UNAMBIGUOUS );

		// the request
		AuthenticationRequest aRequest;
		aRequest.ServerName = sServerName;
		aRequest.HasRealm = aRequest.HasAccount = sal_False;
		aRequest.HasUserName = aRequest.HasPassword = sal_True;
		aRequest.UserName = m_pImpl->m_sUser;
        aRequest.Password = m_pImpl->m_sFailedPassword.getLength() ? m_pImpl->m_sFailedPassword : m_pImpl->m_aPassword;
		OInteractionRequest* pRequest = new OInteractionRequest(makeAny(aRequest));
		Reference< XInteractionRequest > xRequest(pRequest);
		// some knittings
		pRequest->addContinuation(pAbort);
		pRequest->addContinuation(pAuthenticate);

		// handle the request
		try
		{
			MutexRelease aRelease( getMutex() );
				// release the mutex when calling the handler, it may need to lock the SolarMutex
			_rxHandler->handle(xRequest);
		}
		catch(Exception&)
		{
            DBG_UNHANDLED_EXCEPTION();
		}

		if (!pAuthenticate->wasSelected())
			return Reference< XConnection >();

		// get the result
		sUser = m_pImpl->m_sUser = pAuthenticate->getUser();
		sPassword = pAuthenticate->getPassword();

		if (pAuthenticate->getRememberPassword())
		{
			m_pImpl->m_aPassword = pAuthenticate->getPassword();
			bNewPasswordGiven = sal_True;
		}
        m_pImpl->m_sFailedPassword = ::rtl::OUString();
	}

	try
	{
		return getConnection(sUser, sPassword,_bIsolated);
	}
	catch(Exception&)
	{
		if (bNewPasswordGiven)
        {
            m_pImpl->m_sFailedPassword = m_pImpl->m_aPassword;
			// assume that we had an authentication problem. Without this we may, after an unsucessful connect, while
			// the user gave us a password an the order to remember it, never allow an password input again (at least
			// not without restarting the session)
			m_pImpl->m_aPassword = ::rtl::OUString();
        }
		throw;
	}
}

// -----------------------------------------------------------------------------
Reference< XConnection > ODatabaseSource::buildIsolatedConnection(const rtl::OUString& user, const rtl::OUString& password)
{
	Reference< XConnection > xConn;
	Reference< XConnection > xSdbcConn = buildLowLevelConnection(user, password);
	DBG_ASSERT( xSdbcConn.is(), "ODatabaseSource::buildIsolatedConnection: invalid return value of buildLowLevelConnection!" );
	// buildLowLevelConnection is expected to always succeed
	if ( xSdbcConn.is() )
	{
		// build a connection server and return it (no stubs)
		xConn = new OConnection(*this, xSdbcConn, m_pImpl->m_aContext.getLegacyServiceFactory());
	}
	return xConn;
}
//------------------------------------------------------------------------------
Reference< XConnection > ODatabaseSource::getConnection(const rtl::OUString& user, const rtl::OUString& password,sal_Bool _bIsolated) throw( SQLException, RuntimeException )
{
    ModelMethodGuard aGuard( *this );

	Reference< XConnection > xConn;
	if ( _bIsolated )
	{
		xConn = buildIsolatedConnection(user,password);
	}
	else
	{ // create a new proxy for the connection
		if ( !m_pImpl->m_xSharedConnectionManager.is() )
		{
			m_pImpl->m_pSharedConnectionManager = new OSharedConnectionManager( m_pImpl->m_aContext.getLegacyServiceFactory() );
			m_pImpl->m_xSharedConnectionManager = m_pImpl->m_pSharedConnectionManager;
		}
		xConn = m_pImpl->m_pSharedConnectionManager->getConnection(
            m_pImpl->m_sConnectURL, user, password, m_pImpl->m_xSettings->getPropertyValues(), this );
	}

	if ( xConn.is() )
	{
		Reference< XComponent> xComp(xConn,UNO_QUERY);
		if ( xComp.is() )
			xComp->addEventListener( static_cast< XContainerListener* >( this ) );
		m_pImpl->m_aConnections.push_back(OWeakConnection(xConn));
	}

	return xConn;
}

//------------------------------------------------------------------------------
Reference< XNameAccess > SAL_CALL ODatabaseSource::getBookmarks(  ) throw (RuntimeException)
{
    ModelMethodGuard aGuard( *this );
	return static_cast< XNameContainer* >(&m_aBookmarks);
}

//------------------------------------------------------------------------------
Reference< XNameAccess > SAL_CALL ODatabaseSource::getQueryDefinitions( ) throw(RuntimeException)
{
    ModelMethodGuard aGuard( *this );

    Reference< XNameAccess > xContainer = m_pImpl->m_xCommandDefinitions;
	if ( !xContainer.is() )
	{
        TContentPtr& rContainerData( m_pImpl->getObjectContainer( ODatabaseModelImpl::E_QUERY ) );
		xContainer = new OCommandContainer( m_pImpl->m_aContext.getLegacyServiceFactory(), *this, rContainerData, sal_False );
		m_pImpl->m_xCommandDefinitions = xContainer;
	}
	return xContainer;
}
//------------------------------------------------------------------------------
// XTablesSupplier
//------------------------------------------------------------------------------
Reference< XNameAccess >  ODatabaseSource::getTables() throw( RuntimeException )
{
    ModelMethodGuard aGuard( *this );

    Reference< XNameAccess > xContainer = m_pImpl->m_xTableDefinitions;
	if ( !xContainer.is() )
	{
        TContentPtr& rContainerData( m_pImpl->getObjectContainer( ODatabaseModelImpl::E_TABLE ) );
		xContainer = new OCommandContainer( m_pImpl->m_aContext.getLegacyServiceFactory(), *this, rContainerData, sal_True );
		m_pImpl->m_xTableDefinitions = xContainer;
	}
	return xContainer;
}
// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::flush(  ) throw (RuntimeException)
{
	try
	{
        // SYNCHRONIZED ->
        {
            ModelMethodGuard aGuard( *this );

            typedef ::utl::SharedUNOComponent< XModel, ::utl::CloseableComponent > SharedModel;
            SharedModel xModel( m_pImpl->getModel_noCreate(), SharedModel::NoTakeOwnership );

            if ( !xModel.is() )
                xModel.reset( m_pImpl->createNewModel_deliverOwnership( false ), SharedModel::TakeOwnership );

            Reference< css::frame::XStorable> xStorable( xModel, UNO_QUERY_THROW );
            xStorable->store();
        }
        // <- SYNCHRONIZED

        css::lang::EventObject aFlushedEvent(*this);
        m_aFlushListeners.notifyEach( &XFlushListener::flushed, aFlushedEvent );
	}
	catch( const Exception& )
	{
        DBG_UNHANDLED_EXCEPTION();
	}
}

// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::flushed( const EventObject& /*rEvent*/ ) throw (RuntimeException)
{
    ModelMethodGuard aGuard( *this );

    // Okay, this is some hack.
    //
    // In general, we have the problem that embedded databases write into their underlying storage, which
    // logically is one of our sub storage, and practically is a temporary file maintained by the
    // package implementation. As long as we did not commit this storage and our main storage,
    // the changes made by the embedded database engine are not really reflected in the database document
    // file. This is Bad (TM) for a "real" database application - imagine somebody entering some
    // data, and then crashing: For a database application, you would expect that the data still is present
    // when you connect to the database next time.
    //
    // Since this is a conceptual problem as long as we do use those ZIP packages (in fact, we *cannot*
    // provide the desired functionality as long as we do not have a package format which allows O(1) writes),
    // we cannot completely fix this. However, we can relax the problem by commiting more often - often
    // enough so that data loss is more seldom, and seldom enough so that there's no noticable performance
    // decrease.
    //
    // For this, we introduced a few places which XFlushable::flush their connections, and register as
    // XFlushListener at the embedded connection (which needs to provide the XFlushable functionality).
    // Then, when the connection is flushed, we commit both the database storage and our main storage.
    //
    // #i55274# / 2005-09-30 / frank.schoenheit@sun.com

    OSL_ENSURE( m_pImpl->isEmbeddedDatabase(), "ODatabaseSource::flushed: no embedded database?!" );
    sal_Bool bWasModified = m_pImpl->m_bModified;
    m_pImpl->commitEmbeddedStorage();
    m_pImpl->setModified( bWasModified );
}

// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::addFlushListener( const Reference< ::com::sun::star::util::XFlushListener >& _xListener ) throw (RuntimeException)
{
	m_aFlushListeners.addInterface(_xListener);
}
// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::removeFlushListener( const Reference< ::com::sun::star::util::XFlushListener >& _xListener ) throw (RuntimeException)
{
	m_aFlushListeners.removeInterface(_xListener);
}
// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::elementInserted( const ContainerEvent& /*Event*/ ) throw (RuntimeException)
{
    ModelMethodGuard aGuard( *this );
	if ( m_pImpl.is() )
		m_pImpl->setModified(sal_True);
}
// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::elementRemoved( const ContainerEvent& /*Event*/ ) throw (RuntimeException)
{
    ModelMethodGuard aGuard( *this );
	if ( m_pImpl.is() )
		m_pImpl->setModified(sal_True);
}
// -----------------------------------------------------------------------------
void SAL_CALL ODatabaseSource::elementReplaced( const ContainerEvent& /*Event*/ ) throw (RuntimeException)
{
    ModelMethodGuard aGuard( *this );
	if ( m_pImpl.is() )
		m_pImpl->setModified(sal_True);
}
// -----------------------------------------------------------------------------
// XDocumentDataSource
Reference< XOfficeDatabaseDocument > SAL_CALL ODatabaseSource::getDatabaseDocument() throw (RuntimeException)
{
    ModelMethodGuard aGuard( *this );

    Reference< XModel > xModel( m_pImpl->getModel_noCreate() );
    if ( !xModel.is() )
        xModel = m_pImpl->createNewModel_deliverOwnership( false );

    return Reference< XOfficeDatabaseDocument >( xModel, UNO_QUERY_THROW );
}
// -----------------------------------------------------------------------------
Reference< XInterface > ODatabaseSource::getThis() const
{
    return *const_cast< ODatabaseSource* >( this );
}
// -----------------------------------------------------------------------------
void ODatabaseSource::impl_insertJavaDriverClassPath_nothrow(Sequence< PropertyValue >& _rDriverInfo)
{
    Reference< XPropertySet > xPropertySet( m_pImpl->m_xSettings, UNO_QUERY_THROW );
    ::rtl::OUString sJavaDriverClass;
    xPropertySet->getPropertyValue(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("JavaDriverClass"))) >>= sJavaDriverClass;
    if ( sJavaDriverClass.getLength() )
    {
        static const ::rtl::OUString s_sNodeName(RTL_CONSTASCII_USTRINGPARAM("org.openoffice.Office.DataAccess/JDBC/DriverClassPaths"));
        ::utl::OConfigurationTreeRoot aNamesRoot = ::utl::OConfigurationTreeRoot::createWithServiceFactory(
            m_pImpl->m_aContext.getLegacyServiceFactory(), s_sNodeName, -1, ::utl::OConfigurationTreeRoot::CM_READONLY);
        if ( aNamesRoot.isValid() && aNamesRoot.hasByName( sJavaDriverClass ) )
        {
            ::utl::OConfigurationNode aRegisterObj = aNamesRoot.openNode( sJavaDriverClass );
            ::rtl::OUString sURL;
            OSL_VERIFY( aRegisterObj.getNodeValue( "Path" ) >>= sURL );

            ::comphelper::NamedValueCollection aDriverSettings( _rDriverInfo );
            aDriverSettings.put( "JavaDriverClassPath", sURL );
	        aDriverSettings >>= _rDriverInfo;
        }
    }
}
//........................................................................
}	// namespace dbaccess
//........................................................................

