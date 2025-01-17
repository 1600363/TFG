#pragma once

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>

using namespace std;

class CWifiAccessPoint;

void GuardarEnCSV(const std::list<CWifiAccessPoint>& listaWifis, string aula, const string& filename);

list<CWifiAccessPoint> ObtenerWifis();

list<CWifiAccessPoint> EliminarWifisInnecesarios(list<CWifiAccessPoint> listaWifis);

void RecogerDatos();

void CargarDiccionarios(unordered_map<wstring, int>& dict_ssid, unordered_map<wstring, int>& dict_bssid);

std::wstring DecodificarAula(int64_t valor);

void TransformarValoresWifis(list<CWifiAccessPoint>& listaWifis, const unordered_map<wstring, int>& dict_ssid, const unordered_map<wstring, int>& dict_bssid);

void CrearVectorSSID_BSSID(list<CWifiAccessPoint> listaWifis, vector<float>& intensidadesWifi);

int64_t CargarRandomForest(vector<float> datosWifi);

int IdentificarAula();