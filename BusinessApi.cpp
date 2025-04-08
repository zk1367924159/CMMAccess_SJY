#include "BusinessApi.h"
#include "TowerConfig.h"
#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include "Poco/File.h"
#include "CLog.h"
#include "SysCommon.h"
#include "../../NetComm/CDaemonComm.h"
#include "../../ExtAppIpc/ExtAppIpcApi.h"


//#include "NetModule.h"
//#include "../../SdkAdaptor/HwSdkAdaptor.h"
//#include "../../UpgradeModule/UpgradeApi.h"

#include <stdio.h>

#include <sys/socket.h>  
#include <netinet/in.h>  
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>

BusinessApi::BusinessApi(/* args */)
{
}

BusinessApi::~BusinessApi()
{
}

BusinessApi* BusinessApi::Instance()
{
    static BusinessApi *m_Instance = NULL;
    static Poco::FastMutex	m_sMutex;
    if (NULL == m_Instance)
    {
        Poco::FastMutex::ScopedLock lock(m_sMutex);
        if (NULL == m_Instance)
        {
            m_Instance = new BusinessApi;

        }
    }
    return m_Instance;
}

int BusinessApi::AddLinuxSysUser(CData user, CData passwd, CData dir)
{	
	//system("mount -o remount,rw /");
	//Poco::Thread::sleep(500);
	
	FILE   *shellFile; 
	int ret = 0;
	CData cmd = "adduser " + user; 
	if (dir.size()>0)
	{
		cmd += " -h " + dir;
	}
	if (passwd.empty())
	{
		cmd += " -D";
	}
	
	if ((shellFile = popen(cmd.c_str(), "w") ) == nullptr) 
	{ 
		perror("popen");
		LogError("popen error:"<<strerror(errno));
		ret = -1; 
	} 
	else
	{
		LogInfo("Shell cmd: "<<cmd);

		if (passwd.size() > 0)
		{
			Poco::Thread::sleep(500);
			passwd += "\r";
			LogInfo("=====>shell write the passwd:"<<passwd);
			fwrite(passwd.c_str(), 1, passwd.size(), shellFile);
			
			Poco::Thread::sleep(500);
			LogInfo("=====>shell write the passwd again");
			fwrite(passwd.c_str(), 1, passwd.size(), shellFile);
		}
		
		if ((ret = pclose(shellFile)) == -1) 
		{ 
			LogError("close popen error, cmd:"<<cmd<<" ret:"<<ret);
			ret = -2;
		} 
	}

	//system("mount -o remount,ro /");
	//Poco::Thread::sleep(500);
	
	return ret;
}

int BusinessApi::ModifyLinuxSysPasswd(CData user, CData passwd)
{	
	//system("mount -o remount,rw /");
	//Poco::Thread::sleep(500);
	
	FILE   *shellFile; 
	int ret = 0;

	CData cmd = "passwd " + user; 
	
	if ((shellFile = popen(cmd.c_str(), "w") ) == nullptr) 
	{ 
		perror("popen");
		LogError("popen error:"<<strerror(errno));
		ret = -1; 
	} 
	else
	{
		LogInfo("Shell cmd: "<<cmd);
		
		Poco::Thread::sleep(500);
		passwd += "\r";
		LogInfo("=====>modify passwd:"<<passwd);
		fwrite(passwd.c_str(), 1, passwd.size(), shellFile);
		
		Poco::Thread::sleep(500);
		LogInfo("=====>modify passwd again");
		fwrite(passwd.c_str(), 1, passwd.size(), shellFile);
		
		if ((ret = pclose(shellFile)) == -1) 
		{ 
			LogError("close popen error, cmd:"<<cmd<<" ret:"<<ret);
			ret = -2;
		} 
	}

	//system("mount -o remount,ro /");
	//Poco::Thread::sleep(500);
	
	return ret;
}

CData BusinessApi::GetIfcIp(CData ifcName)
{		
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifcName.c_str());
	
	struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
	addr->sin_family = AF_INET;
	
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	CData ipaddr;
	if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
	{
		ipaddr = inet_ntoa(addr->sin_addr);
	}
	close(fd);
	return ipaddr;
}

bool BusinessApi::Is4GOk()
{
	if (GetIfcIp("ppp0").empty())
	{
		return false;
	}
	return true;
}



CData BusinessApi::getMacId() 
{
	std::map<CData,CData> netCfg;
	APPAPI::GetMainAppNetCfg(netCfg,3000);
	return netCfg["mac"];
}

bool BusinessApi::IsVpnOk()
{
	if (GetIfcIp("ppp1").empty())
	{
		return false;
	}
	return true;
}


CData BusinessApi::GetNetIfcParam(CData ifconfig, CData key)
{
	CData val;
	CData content = ifconfig;		
	CData pattern = key;
	
	int startPos = content.find(pattern);
	if (CDATA_NPOS != startPos)
	{
		int endPos =  content.find(" ", startPos+pattern.size());
		if (CDATA_NPOS != endPos)
		{
			val = content.substring(startPos+pattern.size(), endPos);
		}
	}
	return val;
}



void BusinessApi::RebootApp()
{
	APPAPI::RebootApp();
}

void BusinessApi::RebootSys()
{
	APPAPI::RebootSys();
}

void BusinessApi::AddRoute(CData destIp, CData gateWay)
{
	
	if (gateWay == "eth0Gw")
	{
		std::map<CData,CData> netCfg;
		APPAPI::GetMainAppNetCfg(netCfg);
		gateWay = netCfg["localGw"];
	}
	if (gateWay == "ppp0Gw")
	{
		gateWay = GetNetIfcParam(ISFIT::Shell("ifconfig ppp0"), "P-t-P:");
	}
	else if (gateWay == "ppp1Gw")
	{
		gateWay = GetNetIfcParam(ISFIT::Shell("ifconfig ppp1"), "P-t-P:");
	}

	if (gateWay.size() > 0)
	{
		//CData  delppp1Route = "route del default ppp1";
		//LogInfo("===> "<<delppp1Route);
		//ISFIT::Shell(delppp1Route);
	
		CData delRoute = "route del " + destIp;
		LogInfo("===>AddRoute() delRoute: "<<delRoute);
		ISFIT::Shell(delRoute);
				
		CData addRoute = "route add -net " + destIp + " netmask 255.255.255.255 gw " + gateWay;
		LogInfo("===>addRoute: "<<addRoute);
		ISFIT::Shell(addRoute);
	}

}



