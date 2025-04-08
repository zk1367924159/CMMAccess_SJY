/*
 * @Author: your name
 * @Date: 2021-07-26 11:30:26
 * @LastEditTime: 2021-08-05 10:54:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /TowerAccess/BusinessApi.h
 */
#ifndef __BUSINESSAPI_H__
#define __BUSINESSAPI_H__

#include "Data.h"

class BusinessApi
{
public:
    BusinessApi(/* args */);
    ~BusinessApi();

    static BusinessApi* Instance();

public:
    int AddLinuxSysUser(CData user, CData passwd, CData dir);
    int ModifyLinuxSysPasswd(CData user, CData passwd);
    CData GetIfcIp(CData ifcName);
    bool Is4GOk();
    void Start4G();
    void Stop4G();

    CData getMacId();
    bool IsVpnOk();
    void StartVpn();
    void StopVpn();
    CData GetNetIfcParam(CData ifconfig, CData key);
    void SoftResetWirlessModule();
    void HwResetWirlessModule();
    void RebootApp();
    void RebootSys();
    void AddRoute(CData destIp, CData gateWay);
};




#endif // __BUSINESSAPI_H__