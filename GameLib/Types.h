#pragma once
const int SERVER_PORT = 50000;
const int NUM_CULTURA = 7;
const int NUM_FAMILIA = 6;
enum class CULTURA
{
	ARABE,
	BANTU,
	CHINA,
	ESQUIMAL,
	INDIA,
	MEXICANA,
	TIROLESA
};

enum class MIEMBRO_FAMILIA
{
	ABUELO,
	ABUELA,
	PADRE,
	MADRE,
	HIJO,
	HIJA

};

enum LISTENER
{
	ENVIAR_CLIENTESACTUALES,
	ENVIAR_NUEVOCLIENTE,
	READY,
	DATOS_PARTIDA,
	BUSCAR_PARTIDA,
	CREAR_PARTIDA,
	UNIRSE_PARTIDA,
	PASAR_TURNO,
	CAMBIO_CARTA,
};
