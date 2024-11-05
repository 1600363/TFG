#include <stdio.h>
#include <objbase.h>
#include <wtypes.h>
#include <wlanapi.h>
#include <list>
#include <string>
#include <stdexcept>
#include <iostream>
#include <codecvt>  // Para std::wstring_convert
#include <iomanip>  // Para std::setfill y std::setw
#include <sstream>
#include <fstream> //para guardar en csv
#include <ctime> //para guardar la hora
#include <chrono> //para obtener la hora actual

using namespace std;

int CONST PuntosPorClase = 5;
string CONST nombreFichero = "BDwifis.csv";

// ==============================================================================
// POSICIONAMIENTO DEL PORTATIL =================================================
// ==============================================================================

struct CWifiAccessPoint {
    wstring m_SSID;
    wstring m_BSSID;
    int m_Quality;
    int m_Intensity;

    CWifiAccessPoint(const wstring& ssid, const wstring& bssid, int quality, int intensity)
        : m_SSID(ssid), m_BSSID(bssid), m_Quality(quality), m_Intensity(intensity) {}
};

wstring ANSIToUTF16(const string& str) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

wstring formatBSSID(const UCHAR* pMac) {
    wstringstream ss;
    ss <<hex <<setfill(L'0')
        <<setw(2) << (int)pMac[0] << L":"
        <<setw(2) << (int)pMac[1] << L":"
        <<setw(2) << (int)pMac[2] << L":"
        <<setw(2) << (int)pMac[3] << L":"
        <<setw(2) << (int)pMac[4] << L":"
        <<setw(2) << (int)pMac[5];
    return ss.str();
}

static bool FinScan = false;

//WLAN_NOTIFICATION_CALLBACK WlanNotificationCallback;

void WlanNotificationCallback (PWLAN_NOTIFICATION_DATA unnamedParam1, PVOID unnamedParam2)
{
    //cout << "Notificacion " << unnamedParam1->NotificationCode << endl;
    if (unnamedParam1->NotificationCode == wlan_notification_acm_scan_complete) {
        //cout << "FIN SCAN" << endl;
        FinScan = true;
    }
}

string GuardarHora() {

    auto now = chrono::system_clock::now();
    time_t time_now = chrono::system_clock::to_time_t(now);


    tm time_info;
    localtime_s(&time_info, &time_now);
    ostringstream timeStream;
    timeStream << put_time(&time_info, "%H:%M:%S");

    return timeStream.str();
}

void GuardarEnCSV(const list<CWifiAccessPoint>& listaWifis, string aula, int posicionAula, const string& filename) {
    
    ifstream fileRead(filename); //primero abrimos el fichero en modo lectura para ver si existe y en tal caso si esta vacio

    bool fileEmpty = true;

    if (fileRead.is_open()) { //miramos si existe
         fileEmpty = fileRead.peek() == ifstream::traits_type::eof(); //miramos si esta vacio
         fileRead.close();
    }
    else
        cout << "\n" << "El archivo no existe, se creara." << "\n";

    ofstream file; //abrimos archivo
    file.open(filename, ios_base::app); //app (append) para no sobreescribir

    if (fileEmpty) { //si el archivo esta vacio ponemos los nombres a las columnas
        file << "Aula,Posición Aula,SSID,BSSID,Intensity,Quality,Hora\n";
    }

    //string horaActual = GuardarHora(); //funcion obtener hora

    wstring_convert<codecvt_utf8_utf16<wchar_t>> converter; //para pasar los valores de wstring a string

    for (const auto& lw : listaWifis) { //recorremos las wifis encontradas
        
        string ssid_utf8 = converter.to_bytes(lw.m_SSID);
        string bssid_utf8 = converter.to_bytes(lw.m_BSSID);

        if (ssid_utf8.length() == 0)
            ssid_utf8 = "RedOculta";

        //añadimos al documento
        file << aula << "," << ssid_utf8 << "," << bssid_utf8 << "," << lw.m_Intensity << "," << lw.m_Quality << "\n";
        //file << aula << "," << posicionAula << "," << ssid_utf8 << "," << bssid_utf8 << "," << lw.m_Intensity << "," << lw.m_Quality << "," << horaActual << "\n";
    }

    file.close();
}

list<CWifiAccessPoint> ObtenerWifis()
{
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;   //    
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;
    int iRet = 0;

    WCHAR GuidString[40] = { 0 };

    int i;

    /* variables used for WlanEnumInterfaces  */

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_INTERFACE_INFO pIfInfo = NULL;
    list<CWifiAccessPoint> accessPoints;
    try {
        dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
        if (dwResult != ERROR_SUCCESS) {
            throw runtime_error("WlanOpenHandle failed with error: %u" + dwResult);
        }

        dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwResult != ERROR_SUCCESS) {
            throw runtime_error("WlanEnumInterfaces failed with error: %u" + dwResult);
        }
        for (i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
            pIfInfo = (WLAN_INTERFACE_INFO*)&pIfList->InterfaceInfo[i];
            iRet = StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR)&GuidString, 39);
            // For c rather than C++ source code, the above line needs to be
            // iRet = StringFromGUID2(&pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString, 39); 
            if (iRet == 0) throw runtime_error("StringFromGUID2 failed");
            if (pIfInfo->isState == wlan_interface_state_connected) {
                HANDLE hClientHandle;
                DWORD version;
                DWORD err = WlanOpenHandle(
                    2,
                    NULL,
                    &version,
                    &hClientHandle
                );
                if (err != ERROR_SUCCESS) {
                    throw runtime_error("Error WlanOpenHandle %u" + err);
                }
                // Forzar scan
                err = WlanRegisterNotification(
                    hClientHandle,
                    WLAN_NOTIFICATION_SOURCE_ACM,
                    false,
                    (WLAN_NOTIFICATION_CALLBACK) WlanNotificationCallback,
                    NULL,
                    NULL,
                    NULL
                );
                if (err != ERROR_SUCCESS) {
                    throw runtime_error("Error WlanRegisterNotification %u" + err);
                }
                FinScan = false;
                err=WlanScan(hClientHandle,
                    &pIfInfo->InterfaceGuid,
                    NULL,
                    NULL,
                    NULL);
                if (err != ERROR_SUCCESS) {
                    throw runtime_error("Error WlanScan %u" + err);
                }
                int time = 0;
                while (!FinScan) {
                    //cout << "wait" << endl;
                    Sleep(100);
                    ++time;
                    if (time > 100) throw runtime_error("WiFi Scan out of time");
                }
                err = WlanRegisterNotification(
                    hClientHandle,
                    WLAN_NOTIFICATION_SOURCE_ACM,
                    false,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
                FinScan = false;


                PWLAN_BSS_LIST WlanBssList;
                err = WlanGetNetworkBssList(
                    hClientHandle,
                    &pIfInfo->InterfaceGuid,
                    NULL,
                    dot11_BSS_type_any,
                    false,
                    NULL,
                    &WlanBssList
                );
                if (err != ERROR_SUCCESS) {
                    throw runtime_error("Error WlanGetNetworkBssList %u" + err);
                }
                for (int i = 0; i < WlanBssList->dwNumberOfItems; ++i) {
                    char ssid[DOT11_SSID_MAX_LENGTH + 1];
                    for (int j = 0; j < WlanBssList->wlanBssEntries[i].dot11Ssid.uSSIDLength; ++j) {
                        ssid[j] = WlanBssList->wlanBssEntries[i].dot11Ssid.ucSSID[j];

                    }
                    ssid[WlanBssList->wlanBssEntries[i].dot11Ssid.uSSIDLength] = 0;
                    //cout << "SSID " << ssid << " ";
                    UCHAR* pMac = WlanBssList->wlanBssEntries[i].dot11Bssid;
                    //cout << "BSSID " << (int)pMac[0];
                    //for (int j = 1; j < 6; ++j) cout << ":" << (int)pMac[j];
                    //cout << endl;
                    accessPoints.push_back(CWifiAccessPoint(
                        ANSIToUTF16(ssid),
                        formatBSSID(WlanBssList->wlanBssEntries[i].dot11Bssid),
                        WlanBssList->wlanBssEntries[i].uLinkQuality,
                        WlanBssList->wlanBssEntries[i].lRssi
                    ));
                }
                accessPoints.sort([](CWifiAccessPoint& a, CWifiAccessPoint& b) { return a.m_Quality > b.m_Quality;  });
                break;
            }
            pIfInfo = NULL;
        }
        if (!pIfInfo) throw runtime_error("Wifi disabled");
        if (pIfList != NULL) {
            WlanFreeMemory(pIfList);
            pIfList = NULL;
        }
    }
    catch (exception& ex) {
        if (pIfList != NULL) {
            WlanFreeMemory(pIfList);
        }
        throw;
    }
    return accessPoints;
}

int main() {
    
    string aula;

    cout << "En que clase estas?" << "\n";
    cin >> aula;
    cout << "\n";

    cin.ignore();

    for (int i = 1; i < PuntosPorClase + 1; i++) { //bucle que se repitre para cada punto que queremoms registrar
        
        cout << "Estas en el punto " << i << "?" << "\n";
        cout << "Pulsa Enter para registrar las redes cercanas.";

        
        cin.get();
        //system("pause");

        //obtenemos las redes que detectamos
        cout << "Registrando wifis" << "\n";
        list<CWifiAccessPoint> listaWifis = ObtenerWifis();

        cout << "Guardando las redes" << "\n";
        GuardarEnCSV(listaWifis, aula, i, nombreFichero);

        cout << "\n";
    }

    cout << "Se ha registrado todo correctamente";

    return 0;
}
