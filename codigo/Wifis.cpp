#include "Wifis.h"
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
#include <unordered_map>
#include <locale>
#include <vector>
#include <array>

//includes cargar random forest
#include <onnxruntime_cxx_api.h>
#include <cpu_provider_factory.h>

using namespace std;
using namespace Ort;

//ruta guardar CSV
string CONST nombreFichero = "BDwifisV2.csv";
int CONST PuntosPorClase = 1;

//rutas diccionarios
wstring CONST rutaDiccionarioSSID = L"diccionarios/diccionario_ssid.txt";
wstring CONST rutaDiccionarioBSSID = L"diccionarios/diccionario_bssid.txt";
wstring CONST rutaDiccionarioAulas = L"diccionarios/diccionario_aulas.txt";
wstring CONST rutaDiccionarioNombresColumnas = L"diccionario_nombres_columnas.txt";

//ruta modelo
const wchar_t* CONST model_path = L"D:\\TFG\\random_forest_model3.onnx";


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


void GuardarEnCSV(const list<CWifiAccessPoint>& listaWifis, string aula, const string& filename) {
    
    ifstream fileRead(filename); //primero abrimos el fichero en modo lectura para ver si existe y en tal caso si esta vacio

    bool fileEmpty = true;

    if (fileRead.is_open()) { //miramos si existe
         fileEmpty = fileRead.peek() == ifstream::traits_type::eof(); //miramos si esta vacio
         fileRead.close();
    }
    else {
        cout << "\n" << "El archivo no existe, se creara." << "\n";
    }
        

    ofstream file; //abrimos archivo
    file.open(filename, ios_base::app); //app (append) para no sobreescribir

    if (fileEmpty) { //si el archivo esta vacio ponemos los nombres a las columnas
        file << "Aula,SSID,BSSID,Intensity,Quality\n";
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

list<CWifiAccessPoint> EliminarWifisInnecesarios(list<CWifiAccessPoint> listaWifis) {

    //eliminamos los wifis que no sean UAB o eduroam
    listaWifis.remove_if([](const CWifiAccessPoint& wifi) {
        return (wifi.m_SSID != L"UAB" && wifi.m_SSID != L"eduroam");
        //return (wifi.m_SSID != L"MOVISTAR_PLUS_E920" && wifi.m_SSID != L"MOVISTAR_E920");
        }
    );

    return listaWifis;
}

void RecogerDatos() {

    string aula;
    list<CWifiAccessPoint> listaWifis;

    cout << "En que clase estas?" << "\n";
    cin >> aula;
    cout << "\n";

    cin.ignore();

    //bucle que se repitre para cada punto que queremoms registrar
    for (int i = 1; i < PuntosPorClase + 1; i++) {

        cout << "Estas en el punto " << i << "?" << "\n";
        cout << "Pulsa Enter para registrar las redes cercanas.";

        cin.get();
        //system("pause");

        //obtenemos las redes que detectamos
        cout << "Registrando wifis" << "\n";
        listaWifis.splice(listaWifis.end(), ObtenerWifis());

        cout << "\n";
    }

    cout << "Arreglando datos recogidos" << "\n";
    listaWifis = EliminarWifisInnecesarios(listaWifis);


    cout << "Guardando las redes" << "\n";
    //GuardarEnCSV(listaWifis, aula, i, nombreFichero); //caso guardar posicion aula
    GuardarEnCSV(listaWifis, aula, nombreFichero); //caso no guardar posicion de la aula

    cout << "Se ha registrado todo correctamente";

}

void CargarDiccionarios(unordered_map<wstring, int>& dict_ssid, unordered_map<wstring, int>& dict_bssid) {

    //cargamos diccionario ssid

    wifstream file(rutaDiccionarioSSID);
    if (!file.is_open()) {
        cerr << L"error abrir diccionario ssid\n";
        return;
    }

    wstring line;
    while (getline(file, line)) {
        wstringstream ss(line);
        wstring key;
        int value;

        if (getline(ss, key, L'-') && ss >> value) {
            dict_ssid[key] = value;
        }
    }
    file.close();


    //cargamos diccionario bssid

    wifstream file2(rutaDiccionarioBSSID);
    if (!file2.is_open()) {
        wcerr << L"error abrir diccionario bssid\n";
        return;
    }
    file.imbue(locale("en_US.UTF-8")); //linea para poder trabajar con wstring
    wstring line2;
    while (getline(file2, line2)) {
        wstringstream ss(line2);
        wstring key;
        int value;

        if (getline(ss, key, L'-') && ss >> value) {
            dict_bssid[key] = value;
        }
    }
    file.close();
}

wstring DecodificarAula(int64_t valor) {

    wstring aula;

    wifstream file(rutaDiccionarioAulas);
    if (!file.is_open()) {
        wcerr << "Error al abrir diccionario aulas: \n";
        return L"Error al abrir diccionario aula";
    }

    wstring line;
    while (getline(file, line)) {
        // Buscar la posición del carácter ':'
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            // Extraer la parte antes y después de ':'
            wstring key = line.substr(0, colonPos);       // Parte antes de ':'
            int value = stoi(line.substr(colonPos + 1)); // Parte después de ':'

            // Comparar el valor con el objetivo
            if (value == valor) {
                aula = key;
                break;
            }
        }
    }
    file.close();

    return aula;

}

void TransformarValoresWifis(list<CWifiAccessPoint>& listaWifis, const unordered_map<wstring, int>& dict_ssid, const unordered_map<wstring, int>& dict_bssid) {

    
    //recorremos lista objetos
    for (auto& objeto : listaWifis) {
        
        //cambiamos eduroam y uab a valor correspondiente
        if (dict_ssid.find(objeto.m_SSID) != dict_ssid.end()) {
            objeto.m_SSID = to_wstring(dict_ssid.at(objeto.m_SSID));
        }
        else {
            objeto.m_SSID = -1; // Valor si ssid no en el diccionario
        }

        //cambiamos bssid a valor correspondiente
        if (dict_bssid.find(objeto.m_BSSID) != dict_bssid.end()) {
            objeto.m_BSSID = to_wstring(dict_bssid.at(objeto.m_BSSID));
        }
        else {
            objeto.m_BSSID = -1; // Valor si ssid no en el diccionario
        }
    }
}

void CrearVectorSSID_BSSID(list<CWifiAccessPoint> listaWifis, vector<float>& intensidadesWifi) {

    unordered_map<wstring, float> tabla_datos; //creamos unordered map, que sera nuestra tabla de datos
    vector<wstring> orden_columnas;

    //primero setearemos la tabla con los nombres de las columnas y todos los valores a -200

    wifstream file(rutaDiccionarioNombresColumnas);
    if (!file.is_open()) 
    {
        wcerr << L"error abrir diccionario nombres columnas\n";
        return;
    }

    wstring columna;
    while (getline(file, columna)){
        if (!columna.empty()) {
            if (tabla_datos.find(columna) == tabla_datos.end()) {
                tabla_datos[columna] = -200;
                orden_columnas.push_back(columna);
            }
        }
    }
    file.close();


    //ahora seteamos los valores de las intensidades, en funcion de la listaWifis que hemos recogido previamente
    for (const auto& wifi : listaWifis) {
        wstring ssid_bssid = wifi.m_SSID + L"_" + wifi.m_BSSID;
        auto iterator = tabla_datos.find(ssid_bssid); // Buscar clave en el mapa
        if (iterator != tabla_datos.end()) {
            iterator->second = wifi.m_Intensity; // Actualizar el valor en el mapa
        }
    }

    //tabla_datos[L"1_133"] = -20;
    //tabla_datos[L"0_0"] = -10;
    //ahora guardamos en el vector los datos de las intensidades

    for (const auto& columna : orden_columnas) {
        auto it = tabla_datos.find(columna);
        if (it != tabla_datos.end())
            intensidadesWifi.push_back(it->second);
    }

}

int64_t CargarRandomForest( vector<float> datosWifi) {

    // Inicializar ONNX Runtime
    Env env;

    //cargamos la session
    Session session(env, model_path, SessionOptions{});

    ifstream file_check(model_path);
    if (!file_check) {
        cerr << "Error: El archivo ONNX no existe en la ruta proporcionada." << endl;
        return 0;
    }
    else
        cout << "Modelo cargado correctamente" << endl;

    cout << "numero inputs " << session.GetInputCount() << endl;
    cout << "numero outputs " << session.GetOutputCount() << endl;

    AllocatorWithDefaultOptions allocator;
    
    AllocatedStringPtr inputName = session.GetInputNameAllocated(0, allocator);
    cout << "input name: " << inputName << endl;

    AllocatedStringPtr outputName = session.GetOutputNameAllocated(0, allocator);
    cout << "output name: " << outputName << endl;

    vector<int64_t> inputShape = {1, 176};

    // Configurar memoria para la entrada 
    auto memory_info = MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    auto inputOnnxTensor = Value::CreateTensor<float>(
        memory_info,
        datosWifi.data(),
        datosWifi.size(),
        inputShape.data(), 
        inputShape.size()
        );

    // Ejecutar la prediccion
    vector <const char*> inputNames = { "float_input" };
    vector <const char*> outputNames = { "output_label", "output_probability" };

    try {
        auto outputValues = session.Run(
            RunOptions{ nullptr },
            inputNames.data(),
            &inputOnnxTensor,
            1,
            outputNames.data(),
            2
        );

        // Mostrar resultados
        auto outputLabelTensor = outputValues[0].GetTensorMutableData<int64_t>();
        std::cout << "Predicción de la etiqueta: " << outputLabelTensor[0] << std::endl;

        const OrtValue* probabilitiesMap = outputValues[1];

        return outputLabelTensor[0];
    }
    catch (const Exception& e) {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }
}

int IdentificarAula() {

    list<CWifiAccessPoint> listaWifis;

    cout << "Recogiendo Wifis" << "\n\n";

    listaWifis = ObtenerWifis(); //recogemos wifis

    cout << "Wifis recogidos" << "\n\n";

    listaWifis = EliminarWifisInnecesarios(listaWifis); //eliminamos innecesarios

    //si lista Wifis esta vacia, significa que ningun wifi era UAB o eduroam
    if (listaWifis.empty()) {
        cout << "Los wifis detectados no pertenecen a la facultad";
        return 0;
    }

    unordered_map<wstring, int> dict_ssid, dict_bssid; //creamos diccionarios para los datos

    CargarDiccionarios(dict_ssid, dict_bssid); //cargamos diccionarios ssid y bssid

    TransformarValoresWifis(listaWifis, dict_ssid, dict_bssid); //cambiamos valores en funcion de los diccionarios

    vector<float> intensidadesWifi; //vector donde guardaremos las intensidades
    CrearVectorSSID_BSSID(listaWifis, intensidadesWifi);//funcion transforma en vector

    cout << "Ejecutando Random Forest" << "\n\n";

    //por ultimo llamamos al random forest
    int64_t valor_prediccion = CargarRandomForest(intensidadesWifi);

    wstring prediccion = DecodificarAula(valor_prediccion);
    wcout << "La prediccion es aula: " << prediccion << "\n\n";

    return 0;

}

int main() {
    
    //RecogerDatos();

    IdentificarAula();

    //ultimaPrueba();

    return 0;
}