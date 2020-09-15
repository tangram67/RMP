/*
 * upnpservices.h
 *
 *  Created on: 09.02.2019
 *      Author: dirk
 */

#ifndef APP_UPNPSERVICES_H_
#define APP_UPNPSERVICES_H_

#include "upnptypes.h"

namespace upnp {

class TUPnPServices {
private:
	TUPnPServiceMap srvmap;
	TUPnPService defSrv;

public:
	const TUPnPLogo* getLogos() const;
	const TUPnPService* getServices() const;

	const TUPnPServiceDescription& getContendDirectoryService() const;
	const TUPnPServiceDescription& getConnectionManagerService() const;
	const TUPnPServiceDescription& getMediaRegistrarService() const;

	const TUPnPService& getService(const EUPnPServiceType type) const;
	const TUPnPService& operator [] (const EUPnPServiceType type) const;

	TUPnPServices();
	virtual ~TUPnPServices();
};

} /* namespace upnp */

#endif /* APP_UPNPSERVICES_H_ */
