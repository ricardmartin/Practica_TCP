#include "Client.h"
#include <Timer.cpp>

Client::Client()
{
	tcpSocket = new TCPSocket();
	listener = new TCPListener();
	selector = new TCPSocketSelector();
	status = new TCPStatus();
	timer = new Timer();
}

Client::~Client()
{
}


void Client::LineCout() {
	std::cout << std::endl<<"-------------------------------------------------------------" << std::endl;

}

//Conseguimos un random float para la perdida de paquetes
static float GetRandomFloat() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}

//Comprueba si los jugadores estan ready, cuando todos estan ready empieza el game
void Client::CheckPlayersReady2() {
	LineCout();
	bool ready = true;
	system("cls");
	if (!playerReady) {
		ready = false;
	}
	for (auto it : clients) {
		if (it->GetReady()) {
			std::cout << "El jugador " << it->GetLocalPort() << " esta ready"<<std::endl;
		}
		else {
			std::cout << "El jugador " << it->GetLocalPort() << " no esta ready" << std::endl;
			ready = false;
		}
	}
	if (ready) {
		LineCout();
		std::cout << "            TODOS ESTAN READY";
		LineCout();
		Sleep(3000);
		gameBegin = true;

	}




}

//Crea, busca o se une en un game
void Client::JoinOrCreateRoom() {//Envia los paquetes
	createdOrJoinAGame = false;
	while (createdOrJoinAGame==false)
	{

			sf::Packet packet;
		sf::IpAddress ip = sf::IpAddress::LocalHost;
		unsigned short port = SERVER_PORT;//Modificar este magic number
		std::string message;
		std::string nombreSala;
		std::string contraseņa;
		std::string numeroJugadores;
		std::cout << std::endl << "Quieres crear, buscar o unirte a partida?(c/b/u): ";
		std::cin >> message;
		packet << EnumToString(LISTENER::CREAR_PARTIDA);
		packet << std::to_string(tcpSocket->GetLocalPort());
		if (message == "c") {
			packet << message;
			std::cout << std::endl << "Indica el nombre de la sala: ";
			std::cin >> nombreSala;
			packet << nombreSala;
			std::cout << std::endl << "Indica numero de jugadores:(3-6) ";
			std::cin >> numeroJugadores;
			packet << numeroJugadores;
			std::cout << std::endl << "Quieres poner contraseņa?(s/n): ";
			std::cin >> message;
			if (message == "s") {
				packet << message;
				std::cout << std::endl << "Indica la contraseņa?: ";
				std::cin >> contraseņa;
				packet << contraseņa;
				tcpSocket->Send(packet);
				createdOrJoinAGame = true;
			}
			else {
				packet << "none";
				tcpSocket->Send(packet);
				createdOrJoinAGame = true;

			}
			tamaņoSala = std::stoi(numeroJugadores);
		}
		else if (message == "b") {
			std::string numOfPlayers;
			std::string withPassword;
			std::cout << std::endl << "Con cuantos jugadores quieres la sala?(3-6, n para no poner filtro. ";
			std::cin >> numOfPlayers;
			
			std::cout << std::endl << "Quieres mostrar partidas con contraseņa?(s/n) ";
			std::cin >> withPassword;
			packet.clear();
			packet << EnumToString(LISTENER::BUSCAR_PARTIDA);
			packet << std::to_string(tcpSocket->GetLocalPort());
			packet << numOfPlayers;
			packet << withPassword;
			tcpSocket->Send(packet);

		}
		else if (message == "u") {
			std::string nameOfRoom;
			std::string password;
			std::cout << std::endl << "A que sala te quieres unir? ";
			std::cin >> nameOfRoom;
			std::cout << std::endl << "Escribe la contraseņa: (si no tiene escribe none)";
			std::cin >> password;
			packet.clear();
			packet << EnumToString(LISTENER::UNIRSE_PARTIDA);
			packet << std::to_string(tcpSocket->GetLocalPort());
			packet << nameOfRoom;
			packet << password;
			createdOrJoinAGame = true;
			tcpSocket->Send(packet);

		}
	} 
}

//Seteamos los players a ready
void Client::ManageReady(sf::Packet &packet, TCPSocket* _tcpSocket) {
	for (auto it : clients) {
		if (it->GetRemotePort() == _tcpSocket->GetRemotePort()) {
			it->SetReady(true);
		}
	}
	CheckPlayersReady2();
}

//Muestra por pantalla si esta ready
void Client::checkReady() {
	if (!playerReady) {
		std::string message;
		std::cout << "Escribe ready para empezar la partida: ";
		std::cin >> message;
		if (message == "ready") {
			std::cout << "Estas listo para empezar la partida, hay que comprobar el resto de jugadores. " << std::endl;
			playerReady = true;
			CheckPlayersReady2();
			sf::Packet packet;
				for (auto it : clients) {

					packet << EnumToString(LISTENER::READY);
					status->SetStatus(it->Send(packet));
					if (status->GetStatus() == sf::Socket::Done)
					{
						std::cout << "El paquete se ha enviado correctamente\n";
						packet.clear();
					}
					else {
						std::cout << "El paquete no se ha podido enviar\n";
					}
				}

			
		}
	}

}

//Asignamos la baraja
void Client::AssignDeck()
{
	system("cls");
	deck = new Deck();
	if (idPlayer == 0) {
		seed = tcpSocket->GetLocalPort();
		deck->MixDeck(seed);
	}
	else {
		for (int i = 0;i < clients.size();i++) {
			if (clients[i]->GetID() == 0) {
				seed = clients[i]->GetRemotePort();
				deck->MixDeck(seed);
			
			}
		}
	}
	std::cout << "El cliente con id " << idPlayer << " La seed es la siguiente: " << seed << std::endl;

}

//Asignamos turnos
void Client::AsignTurns()
{
	//ESTE 5 SERAN LOS NUMEROS DE JUGADORES QUE HAY EN LA PARTIDA
	for (int i = 0;i < tamaņoSala;i++) {
		playerCards[i] = new PlayerCards();
		playerCards[i]->actualTurn = 0;
		playerCards[i]->isPlaying = true;
	
	}

	for (int i = 0; i < deck->deck.size();i++) {
		if (tamaņoSala == 3) {
			if (i <= 14)playerCards[0]->giveCard(*deck->deck[i]);
			else if (i > 14 && i <= 28)playerCards[1]->giveCard(*deck->deck[i]);
			else playerCards[2]->giveCard(*deck->deck[i]);
		
		}
		else if (tamaņoSala == 4) {
			if (i <= 10)playerCards[0]->giveCard(*deck->deck[i]);
			else if (i > 10 && i <= 20)playerCards[1]->giveCard(*deck->deck[i]);
			else if (i > 20 && i <= 30)playerCards[2]->giveCard(*deck->deck[i]);
			else playerCards[3]->giveCard(*deck->deck[i]);
		}

		else if (tamaņoSala == 5) {
			if (i <= 7)playerCards[0]->giveCard(*deck->deck[i]);
			else if (i > 7 && i <= 14)playerCards[1]->giveCard(*deck->deck[i]);
			else if (i > 14 && i <= 21)playerCards[2]->giveCard(*deck->deck[i]);
			else if (i > 21 && i <= 28)playerCards[3]->giveCard(*deck->deck[i]);
			else playerCards[4]->giveCard(*deck->deck[i]);
		}

		else if (tamaņoSala == 6) {
			if (i <= 7)playerCards[0]->giveCard(*deck->deck[i]);
			else if (i > 7 && i <= 14)playerCards[1]->giveCard(*deck->deck[i]);
			else if (i > 14 && i <= 21)playerCards[2]->giveCard(*deck->deck[i]);
			else if (i > 21 && i <= 28)playerCards[3]->giveCard(*deck->deck[i]);
			else if (i > 28 && i <= 35)playerCards[4]->giveCard(*deck->deck[i]);

			else playerCards[5]->giveCard(*deck->deck[i]);
		}
	}
	std::thread chat(&Client::InterfazChat, this);
	chat.detach();
	std::thread timer(&Client::TimerTurn, this);
	timer.detach();
}

//Esperamos los jugadores
void Client::Waiting4Players() {
	Sleep(7000);
		while (clients.size() < tamaņoSala - 1 ) {
			if (selector->Wait()) {
				if (selector->isReady(&listener->GetListener())) {
					TCPSocket* client = new TCPSocket();
					if (listener->Accept(client->GetSocket()) == sf::Socket::Done)
					{
						std::cout << "Connexion recibia de: " << client->GetRemotePort() << std::endl;
						selector->Add(client->GetSocket());
						client->SetID(clients.size() + 1);
						clientMutex.lock();
						clients.push_back(client);
						clientMutex.unlock();
					
						std::cout << "Hay " << clients.size() << " clientes conectados a este cliente." << std::endl;
					}
					else {
						delete client;
						std::cout << "Error al recibir un player." << std::endl;
						exit(0);
					}
				}

			}
		}
		game = true;

	
	
}

//Thread que recibe paquetes del server
void Client::RecievingThread() {
	while (!this->getClients) {
		sf::Packet packet;
		status->SetStatus(tcpSocket->Receive(packet));
		if (status->GetStatus() != sf::Socket::Done)
		{
			std::cout << "Error al recibir el paquete\n";
		}
		else {
			std::cout << "Se ha recibido un paquete\n";
			std::string stringTag;
			packet >> stringTag;
			LISTENER tag = StringToEnum(stringTag);
			std::string size;
			int sizeInt;
			std::string nombreSala;
			std::string numeroJugadoresActuales;
			std::string numeroJugadoresPartida;
			std::string stringPort;
			std::string numOfPlayers;
			std::string auxiliar;
			int sala;
			int auxiliarNumOfPlayers;
			switch (tag)
			{
		
			case BUSCAR_PARTIDA:
				packet >> size;
				sizeInt = std::stoi(size);

				if (sizeInt != 0) {
					for (int i = 0;i < sizeInt;i++ ) {
						packet >> nombreSala;
						packet >> numeroJugadoresActuales;
						packet >> numeroJugadoresPartida;
						std::cout << nombreSala << "   (" << numeroJugadoresActuales << "/" << numeroJugadoresPartida << ")" << std::endl;
					}
				}
				break;
				case ENVIAR_CLIENTESACTUALES:
					createdOrJoinAGame = true;
					if (tamaņoSala == 0) {

						packet >> auxiliar;
						sala = std::stoi(auxiliar);
						tamaņoSala = sala;
					}
				packet >> numOfPlayers;
				auxiliarNumOfPlayers = std::stoi(numOfPlayers);
			
				int port;
				for (int i = 0;i < auxiliarNumOfPlayers;i++) {

					packet >> stringPort;
					port = std::stoi(stringPort);
					TCPSocket* client = new TCPSocket;

					status->SetStatus(client->Connect("localhost", port, sf::milliseconds(15.f)));
					if (status->GetStatus() == sf::Socket::Done) {
						client->SetID(i);
						std::cout << "Se ha conectado con el cliente " << port << std::endl;
						clients.push_back(client);
						selector->Add(client->GetSocket());
						
						getClients = true;

					}
					else {
						std::cout << "Error al conectarse con el jugador" << std::endl;
						delete client;
					}

				}
				idPlayer = auxiliarNumOfPlayers;
				
			default:
				break;
			}

			}
		}
	}

//Devolvemos el tag del paquete
LISTENER Client::GetTag(sf::Packet& packet) {
	std::string auxiliar;
	packet >> auxiliar;
	return StringToEnum(auxiliar);
}

//Controlamos el cambio de carta
void Client::ManageCambioCarta(sf::Packet &packet) {
	int idPlayerThief;
	int idPlayer2Steal;
	int cultura;
	int familia;
	
	CULTURA _cultura;
	MIEMBRO_FAMILIA _familia;
	std::string auxiliar;
	std::string mensajeId;
	packet >> mensajeId;
	idPlayerThief = std::stoi(mensajeId);
	auxiliar.clear();
	packet >> auxiliar;
	idPlayer2Steal = std::stoi(auxiliar);
	auxiliar.clear();

	packet >> auxiliar;

	cultura = std::stoi(auxiliar);
	_cultura = (CULTURA)cultura;

	packet >> auxiliar;
	familia = std::stoi(auxiliar);
	_familia = (MIEMBRO_FAMILIA)familia;
	if (idPlayer2Steal == idPlayer) {
		std::string a = "El jugador ";
		std::string b = " te pide ";
		std::string c = " " ;

		aMensajes.push_back(a+mensajeId+b+ CulturaToString(_cultura) +c+ FamiliaToString(_familia));

		ChangeCardsBetweenPlayers(idPlayerThief, idPlayer2Steal, _cultura, _familia);
		a = "El jugador " + std::to_string(idPlayerThief);
		aMensajes.push_back(a + "te ha robado la carta");


			
	}
	else{
		std::string a = "El jugador " + idPlayerThief;
		std::string b = "le ha robado una carta a " + idPlayer2Steal;
		aMensajes.push_back(a+b);

		ChangeCardsBetweenPlayers(idPlayerThief, idPlayer2Steal, _cultura, _familia);
		

	}
}

//Controlamos el recibimiento de un mensaje
void Client::ManageMessage(sf::Packet& packet) {
	std::string message;
	packet >> message;
	aMensajes.push_back(message);
}

//Controlamos si un jugador ha dicho que se acaba el game
void Client::ManageFinish(sf::Packet& packet) {
	std::string id, points;
	packet >> id;
	packet >> points;
	aMensajes.push_back("El jugador " + id + " ha ganado con " + points);
	Sleep(5000);
	gameBegin = false;
	game = false;
	exit(0);
}

//Thread que recibe paquetes de los peers
void Client::ClientsListener() {
	while (true) {
		for (auto it : clients) {
			if (selector->Wait()) {
				if (selector->isReady(it->GetSocket()))
				{
					sf::Packet packet;
					status->SetStatus(it->Receive(packet));
					if (status->GetStatus() == sf::Socket::Done) {
						LISTENER tag = GetTag(packet);
						switch (tag)
						{
						case READY:
							ManageReady(packet, it);
							break;

						case PASAR_TURNO:
							PasarTurno();
							break;
						case CAMBIO_CARTA:
							ManageCambioCarta(packet);
							break;
						case MESSAGE:
							ManageMessage(packet);
							break;
						case FINISH:
							ManageFinish(packet);
							break;
						}



					}
					else if (status->GetStatus()==sf::Socket::Disconnected) {
						selector->Remove(it->GetSocket());
						int aux = -1;
						for (int i = 0;i < clients.size();i++) {
							if (clients[i]->GetRemotePort() == it->GetRemotePort()) {
								aux = i;
							}
						}
						clients.erase(clients.begin() + aux);
					}
				}
			}
		}
	}
}

//Chequeamos si la partida ha terminado
void Client::CheckFinish() {
	int playersPlaying = 0;
	int cultureCompleted = 0;
	for (auto it : playerCards) {
		if (it.second->isPlaying) {
			playersPlaying++;
		}
		cultureCompleted += it.second->puntuacion;
	}
	if (playersPlaying <= 2) {
		aMensajes.push_back("Han quedado dos jugadores.");
		int _id = 0;
		int winnerPoints = 0;
		for (auto it : playerCards) {
			if (it.second->puntuacion > winnerPoints) {
				_id = it.first;
				winnerPoints = it.second->puntuacion;
			}
		}
		std::string aux = "El ganador es" + std::to_string(_id);
		std::string aux2 = " con " + std::to_string(winnerPoints);
		aMensajes.push_back(aux + aux2);
		sf::Packet packet;
		for (auto it : clients) {

			packet << EnumToString(LISTENER::FINISH);
			packet << std::to_string(_id);
			packet << std::to_string(winnerPoints);
			status->SetStatus(it->Send(packet));
			if (status->GetStatus() == sf::Socket::Done)
			{
				std::cout << "El paquete se ha enviado correctamente\n";
				packet.clear();
			}
			else {
				std::cout << "El paquete no se ha podido enviar\n";
			}
		}

		Sleep(7000);
		gameBegin = false;
		game = false;
		exit(0);
	}
	else if (cultureCompleted == 7) {
		aMensajes.push_back("Se han completado todas las familias.");
		int _id = 0;
		int winnerPoints = 0;
		for (auto it : playerCards) {
			if (it.second->puntuacion > winnerPoints) {
				_id = it.first;
				winnerPoints = it.second->puntuacion;
			}
		}
		std::string aux = "El ganador es"+ std::to_string(_id) ;
		std::string aux2 = " con " + std::to_string(winnerPoints);
		aMensajes.push_back(aux + aux2);
		sf::Packet packet;
		for (auto it : clients) {

			packet << EnumToString(LISTENER::FINISH);
			packet << std::to_string(_id);
			packet << std::to_string(winnerPoints);
			status->SetStatus(it->Send(packet));
			if (status->GetStatus() == sf::Socket::Done)
			{
				std::cout << "El paquete se ha enviado correctamente\n";
				packet.clear();
			}
			else {
				std::cout << "El paquete no se ha podido enviar\n";
			}
		}
	
		Sleep(7000);
		gameBegin = false;
		game = false;
	}
}

//Cambio de cartas entre dos players
void Client::ChangeCardsBetweenPlayers(int _actualPlayer, int _changePlayer, CULTURA _culture, MIEMBRO_FAMILIA _familia) {
	Card* card = new Card(_culture, _familia);
	playerCards[_actualPlayer]->giveCard(*card);
	playerCards[_changePlayer]->eraseCard(*card);


}

//El jugador cambia el turno localmente
void Client::PasarTurno() {
	int actualTurn = 0;

	for(auto it : playerCards){
		actualTurn = it.second->actualTurn;
		if (actualTurn == tamaņoSala - 1) {
			it.second->actualTurn = 0;
			actualTurn = 0;
		}

		else {
			it.second->actualTurn = it.second->actualTurn + 1;
				actualTurn = it.second->actualTurn + 1;
			}
		}
	 
}

//Interfaz para el chat
void Client::InterfazChat() {

	sf::Vector2i screenDimensions(800, 600);

	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("courbd.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}

	sf::String mensaje = " >";

	sf::Text chattingText(mensaje, font, 14);
	chattingText.setFillColor(sf::Color(0, 160, 0));
	chattingText.setStyle(sf::Text::Bold);


	sf::Text text(mensaje, font, 14);
	text.setFillColor(sf::Color(0, 160, 0));
	text.setStyle(sf::Text::Bold);
	text.setPosition(0, 560);

	sf::RectangleShape separator(sf::Vector2f(800, 5));
	separator.setFillColor(sf::Color(200, 200, 200, 255));
	separator.setPosition(0, 550);

	while (window.isOpen())
	{
		sf::Event evento;
		if (!indicaciones) {
			indicaciones = true;
			std::string id = std::to_string(idPlayer);
			id = "Eres el id " + id;
			aMensajes.push_back(id);
			aMensajes.push_back("Q - BUENOS DIAS A TODOS  |  W - VAIS A PERDER  |  E - GG EZ");

		}

		while (window.pollEvent(evento))
		{
			switch (evento.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				if (evento.key.code == sf::Keyboard::Escape)
					window.close();
				else if (evento.key.code == sf::Keyboard::Return)
				{
					aMensajes.push_back(mensaje);
					if (aMensajes.size() > 25)
					{
						aMensajes.erase(aMensajes.begin(), aMensajes.begin() + 1);
					}
					mensaje = ">";
				}
				else if (evento.key.code == sf::Keyboard::Q)
				{
					aMensajes.push_back("BUENOS DIAS(T para todos o marca el id del jugador");
					waitingAnswer1 = true;
					waitingAnswer2 = false;
					waitingAnswer3 = false;

				}
				else if (evento.key.code == sf::Keyboard::W)
				{
					aMensajes.push_back("VAS A PERDER(T para todos o marca el id del jugador");
					waitingAnswer1 = false;
					waitingAnswer2 = true;
					waitingAnswer3 = false;

				}
				else if (evento.key.code == sf::Keyboard::E)
				{
					aMensajes.push_back("GG EZ(T para todos o marca el id del jugador");
					waitingAnswer1 = false;
					waitingAnswer2 = false;
					waitingAnswer3 = true;
				}
				else if (evento.key.code == sf::Keyboard::T) {
					if (waitingAnswer1 == true) {
						sf::Packet packet;
						aMensajes.push_back("Se ha enviado BUENOS DIAS a todos");

						for (auto it : clients) {

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "BUENOS DIAS";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}

					}
					else if (waitingAnswer2 == true) {
						sf::Packet packet;
						aMensajes.push_back("Se ha enviado VAS A PERDER a todos");

						for (auto it : clients) {

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "VAS A PERDER";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
					else if (waitingAnswer3 == true) {
						sf::Packet packet;
						aMensajes.push_back("Se ha enviado GG EZ a todos");

						for (auto it : clients) {

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "GG EZ";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
					waitingAnswer1 = false;
					waitingAnswer2 = false;
					waitingAnswer3 = false;
				}
				else if (evento.key.code == sf::Keyboard::Num0&&idPlayer!=0) {
					if (waitingAnswer1 == true) {
						sf::Packet packet;

						for (auto it : clients) {
							if (it->GetID() == 0) {
								aMensajes.push_back("Se ha enviado BUENOS DIAS a 0");

								packet << EnumToString(LISTENER::MESSAGE);
								packet << "BUENOS DIAS";
								status->SetStatus(it->Send(packet));
								if (status->GetStatus() == sf::Socket::Done)
								{
									std::cout << "El paquete se ha enviado correctamente\n";
									packet.clear();
								}
								else {
									std::cout << "El paquete no se ha podido enviar\n";
								}
							}
						}

					}
					else if (waitingAnswer2 == true) {
						sf::Packet packet;

						for (auto it : clients) {
							if (it->GetID() == 0) {
								aMensajes.push_back("Se ha enviado VAS A PERDER al jugador 0");

								packet << EnumToString(LISTENER::MESSAGE);
								packet << "VAS A PERDER";
								status->SetStatus(it->Send(packet));
								if (status->GetStatus() == sf::Socket::Done)
								{
									std::cout << "El paquete se ha enviado correctamente\n";
									packet.clear();
								}
								else {
									std::cout << "El paquete no se ha podido enviar\n";
								}
							}
						}
					}
					else if (waitingAnswer3 == true) {
						sf::Packet packet;

						for (auto it : clients) {
							if (it->GetID() == 0) {
								aMensajes.push_back("Se ha enviado GG EZ al jugador 0");

								packet << EnumToString(LISTENER::MESSAGE);
								packet << "GG EZ";
								status->SetStatus(it->Send(packet));
								if (status->GetStatus() == sf::Socket::Done)
								{
									std::cout << "El paquete se ha enviado correctamente\n";
									packet.clear();
								}
								else {
									std::cout << "El paquete no se ha podido enviar\n";
								}
							}
						}
					}
					waitingAnswer1 = false;
					waitingAnswer2 = false;
					waitingAnswer3 = false;
				}
				else if (evento.key.code == sf::Keyboard::Num1 && idPlayer != 1) {
				if (waitingAnswer1 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 1) {
							aMensajes.push_back("Se ha enviado BUENOS DIAS a 1");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "BUENOS DIAS";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}

				}
				else if (waitingAnswer2 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 1) {
							aMensajes.push_back("Se ha enviado VAS A PERDER al jugador 1");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "VAS A PERDER";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				else if (waitingAnswer3 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 1) {
							aMensajes.push_back("Se ha enviado GG EZ al jugador 1");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "GG EZ";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				waitingAnswer1 = false;
				waitingAnswer2 = false;
				waitingAnswer3 = false;
				}
				else if (evento.key.code == sf::Keyboard::Num2 && idPlayer != 2) {
				if (waitingAnswer1 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 2) {
							aMensajes.push_back("Se ha enviado BUENOS DIAS a 2");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "BUENOS DIAS";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}

				}
				else if (waitingAnswer2 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 2) {
							aMensajes.push_back("Se ha enviado VAS A PERDER al jugador 2");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "VAS A PERDER";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				else if (waitingAnswer3 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 2) {
							aMensajes.push_back("Se ha enviado GG EZ al jugador 2");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "GG EZ";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				waitingAnswer1 = false;
				waitingAnswer2 = false;
				waitingAnswer3 = false;
				}
				else if (evento.key.code == sf::Keyboard::Num3 && tamaņoSala >=4 && idPlayer != 3) {
				if (waitingAnswer1 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 3) {
							aMensajes.push_back("Se ha enviado BUENOS DIAS a 3");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "BUENOS DIAS";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}

				}
				else if (waitingAnswer2 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 3) {
							aMensajes.push_back("Se ha enviado VAS A PERDER al jugador 3");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "VAS A PERDER";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				else if (waitingAnswer3 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 3) {
							aMensajes.push_back("Se ha enviado GG EZ al jugador 3");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "GG EZ";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				waitingAnswer1 = false;
				waitingAnswer2 = false;
				waitingAnswer3 = false;
				}
				else if (evento.key.code == sf::Keyboard::Num4 && tamaņoSala >= 5 && idPlayer != 4) {
				if (waitingAnswer1 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 4) {
							aMensajes.push_back("Se ha enviado BUENOS DIAS a 4");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "BUENOS DIAS";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}

				}
				else if (waitingAnswer2 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 4) {
							aMensajes.push_back("Se ha enviado VAS A PERDER al jugador 4");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "VAS A PERDER";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				else if (waitingAnswer3 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 4) {
							aMensajes.push_back("Se ha enviado GG EZ al jugador 4");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "GG EZ";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				waitingAnswer1 = false;
				waitingAnswer2 = false;
				waitingAnswer3 = false;
				}
				else if (evento.key.code == sf::Keyboard::Num5 && tamaņoSala >= 6 && idPlayer != 5) {
				if (waitingAnswer1 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 5) {
							aMensajes.push_back("Se ha enviado BUENOS DIAS a 5");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "BUENOS DIAS";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}

				}
				else if (waitingAnswer2 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 5) {
							aMensajes.push_back("Se ha enviado VAS A PERDER al jugador 5");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "VAS A PERDER";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				else if (waitingAnswer3 == true) {
					sf::Packet packet;

					for (auto it : clients) {
						if (it->GetID() == 5) {
							aMensajes.push_back("Se ha enviado GG EZ al jugador 5");

							packet << EnumToString(LISTENER::MESSAGE);
							packet << "GG EZ";
							status->SetStatus(it->Send(packet));
							if (status->GetStatus() == sf::Socket::Done)
							{
								std::cout << "El paquete se ha enviado correctamente\n";
								packet.clear();
							}
							else {
								std::cout << "El paquete no se ha podido enviar\n";
							}
						}
					}
				}
				waitingAnswer1 = false;
				waitingAnswer2 = false;
				waitingAnswer3 = false;
				}
				break;
			case sf::Event::TextEntered:
				if (evento.text.unicode >= 32 && evento.text.unicode <= 126)
					mensaje += (char)evento.text.unicode;
				else if (evento.text.unicode == 8 && mensaje.getSize() > 0)
					mensaje.erase(mensaje.getSize() - 1, mensaje.getSize());
				break;
			}
		}
		window.draw(separator);
		for (size_t i = 0; i < aMensajes.size(); i++)
		{
			std::string chatting = aMensajes[i];
			chattingText.setPosition(sf::Vector2f(0, 20 * i));
			chattingText.setString(chatting);
			window.draw(chattingText);
		}
		std::string mensaje_ = mensaje + "_";
		text.setString(mensaje_);
		window.draw(text);


		window.display();
		window.clear();
	}


}

//Enviamos a los jugadores que tienen que cambiar los turnos
void Client::SendPasarTurno() {
	sf::Packet packet;
	for (auto it : clients) {

		packet << EnumToString(LISTENER::PASAR_TURNO);
		status->SetStatus(it->Send(packet));
		if (status->GetStatus() == sf::Socket::Done)
		{
			std::cout << "El paquete se ha enviado correctamente\n";
			packet.clear();
		}
		else {
			std::cout << "El paquete no se ha podido enviar\n";
		}
	}

}

//Elige la carta que cambiar
void Client::ChooseCard() {
	do
	{
		std::cout << "Elige jugador al que robar:(Numero) ";
		std::cin >> player2Steal;
		std::cout << std::endl;
	} while (player2Steal >6&&idPlayer==player2Steal);
	int auxCultura;
	do
	{
		std::cout << "Elige la cultura:  0: ARABE   1: BANTU   2: CHINA   3: ESQUIMAL   4: INDIA   5: MEXICANA   6: TIROLESA " << std::endl;
		std::cin >> auxCultura;
		cultura = (CULTURA)auxCultura;

	} while (auxCultura > 6);
	int auxFamilia;
	do
	{
		std::cout << "Elige la familia:  0: ABUELO   1: ABUELA   2: PADRE   3: MADRE   4: HIJO   5: HIJA" << std::endl;
		std::cin >> auxFamilia;
		familia = (MIEMBRO_FAMILIA)auxFamilia;

	} while (auxFamilia > 5);
	if (CheckCard(player2Steal, cultura, familia)&&isYourTurn) {
		std::cout << "El jugador tiene la carta";
		//Sleep(3000);

		ChangeCardsBetweenPlayers(idPlayer, player2Steal, cultura, familia);
		SendCambioCarta(idPlayer, player2Steal, cultura, familia);
		ManageGame();
		CheckFinish();

	
	
	}
	else {
		std::cout << "El jugador no tiene la carta o se te ha acabado el tiempo";
		isYourTurn = false;
		Sleep(3000);
		//SendCambioCarta(idPlayer, player2Steal, cultura, familia);
		PasarTurno();
		SendPasarTurno();
		CheckFinish();
		//ManageGame();

	}

}


//Aqui nos conectamos al bss y tambien abrimos el listener
void Client::ConnectServer() {
	status->SetStatus(tcpSocket->Connect("localhost", 50000, sf::milliseconds(15.f)));
	if (status->GetStatus() != sf::Socket::Done)
	{
		std::cout << "Error al establecer conexion\n";
		//exit(0);
	}
	else
	{
		std::cout << "Se ha establecido conexion\n";
		status->SetStatus(listener->Listen(tcpSocket->GetLocalPort(), sf::IpAddress::LocalHost));
		if (status->GetStatus() == sf::Socket::Done) {
			std::cout << "Se ha abierto el listener\n";
			selector->Add(&listener->GetListener());
		}
		else {
			std::cout << "Error al abrir el listener\n";
			//exit(0);

		}
	}
}

//Comprobamos las cartas
bool Client::CheckCard(int _id, CULTURA _cultura, MIEMBRO_FAMILIA _familia) {
	Card* card = new Card(_cultura, _familia);
	return playerCards[_id]->checkCard(*card);
}

void Client::SendCambioCarta(int _id,int playerToChange, CULTURA _cultura, MIEMBRO_FAMILIA _familia) {
	sf::Packet packet;
	for (auto it : clients) {

		packet << EnumToString(LISTENER::CAMBIO_CARTA);
		packet << std::to_string(_id);
		packet << std::to_string(playerToChange);
		packet << std::to_string((int)_cultura);
		packet << std::to_string((int)_familia);
		status->SetStatus(it->Send(packet));
		if (status->GetStatus() == sf::Socket::Done)
		{
			std::cout << "El paquete se ha enviado correctamente\n";
			packet.clear();
		}
		else {
			std::cout << "El paquete no se ha podido enviar\n";
		}
	}
}

void Client::ManageGame() {

	if (idPlayer == playerCards[idPlayer]->actualTurn)
	{
		if (playerCards[idPlayer]->isPlaying) {
			timer->ResetTimer();

			isYourTurn = true;
			system("cls");
			std::cout << "          " << idPlayer << " - JUGANDO           ";

			std::cout << "Estas son tus cartas: " << std::endl;
			
			playerCards[idPlayer]->PrintHand();
		
			ChooseCard();
		}
	}
	else {
		system("cls");
		if (playerCards[idPlayer]->isPlaying) {
			LineCout();

			std::cout << "          "<<   idPlayer<<" - ESPERANDO TURNO           ";
			LineCout();
			std::cout << "Tu puntuacion" << playerCards[idPlayer]->puntuacion<<std::endl;
			std::cout << "Esta jugando el jugador: " << playerCards[idPlayer]->actualTurn << std::endl;
			playerCards[idPlayer]->PrintHand();

			Sleep(7000);
		}
		else {
			LineCout();
			std::cout << "          HAS PERDIDO: MODO ESPECTADOR           ";
			LineCout();
			std::cout << "Esta jugando el jugador: " << playerCards[idPlayer]->actualTurn << std::endl;
			//playerCards[idPlayer]->PrintHand();
			if (playerCards[0]->actualTurn == idPlayer) {
				PasarTurno();
				SendPasarTurno();
			}
		}
		
	}
}

void Client::TimerTurn() {
	while (true) {
		if (isYourTurn) {
			std::cout << std::endl<< "Tiempo: ("<<timer->GetDuration() << "/" << TURN_DURATION <<")"<< std::endl;
			
		}
		if (timer->GetDuration() > TURN_DURATION) {
			isYourTurn = false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	}
}

void Client::ClientLoop()
{

	ConnectServer();
	std::thread recievingThread(&Client::RecievingThread, this);
	recievingThread.detach();
	JoinOrCreateRoom();
	//GetConnectedPlayers();
	
	Waiting4Players();

	AssignDeck();
	AsignTurns();
	std::thread clientsSelector(&Client::ClientsListener, this);
	clientsSelector.detach();

	while (game) {
		checkReady();

		while (gameBegin) {
			ManageGame();
		}
	}
}

std::string Client::EnumToString(LISTENER _listener) {
	if (_listener == ENVIAR_CLIENTESACTUALES) {
		return "ENVIAR_CLIENTESACTUALES";
	}
	else if (_listener == ENVIAR_NUEVOCLIENTE) {
		return "ENVIAR_NUEVOCLIENTE";

	}
	else if (_listener == READY) {
		return "READY";

	}
	else if (_listener == DATOS_PARTIDA) {
		return "DATOS_PARTIDA";

	}
	else if (_listener == BUSCAR_PARTIDA) {
		return "BUSCAR_PARTIDA";

	}
	else if (_listener == CREAR_PARTIDA) {
		return "CREAR_PARTIDA";

	}
	else if (_listener == UNIRSE_PARTIDA) {
		return "UNIRSE_PARTIDA";

	}
	else if (_listener == PASAR_TURNO) {
		return "PASAR_TURNO";

	}
	else if (_listener == CAMBIO_CARTA) {
		return "CAMBIO_CARTA";

	}
	else if (_listener == MESSAGE) {
		return "MESSAGE";

	}
	else if (_listener == FINISH) {
		return "FINISH";

	}
}
LISTENER Client::StringToEnum(std::string _string) {
	if (_string == "ENVIAR_CLIENTESACTUALES") {
		return LISTENER::ENVIAR_CLIENTESACTUALES;
	}
	else if (_string == "ENVIAR_NUEVOCLIENTE") {
		return LISTENER::ENVIAR_NUEVOCLIENTE;
	}
	else if (_string == "READY") {
		return LISTENER::READY;
	}
	else if (_string == "DATOS_PARTIDA") {
		return LISTENER::DATOS_PARTIDA;
	}
	else if (_string == "BUSCAR_PARTIDA") {
		return LISTENER::BUSCAR_PARTIDA;
	}
	else if (_string == "CREAR_PARTIDA") {
		return LISTENER::CREAR_PARTIDA;
	}
	else if (_string == "UNIRSE_PARTIDA") {
		return LISTENER::UNIRSE_PARTIDA;
	}
	else if (_string == "PASAR_TURNO") {
		return LISTENER::PASAR_TURNO;
	}
	else if (_string == "CAMBIO_CARTA") {
		return LISTENER::CAMBIO_CARTA;
	}
	else if (_string == "MESSAGE") {
		return LISTENER::MESSAGE;
	}
	else if (_string == "FINISH") {
		return LISTENER::FINISH;
	}
}

CULTURA Client::StringToCultura(std::string _string) {
	if (_string == "ARABE") {
		return CULTURA::ARABE;
	}
	else if (_string == "BANTU") {
		return CULTURA::BANTU;
	}
	else if (_string == "CHINA") {
		return CULTURA::CHINA;
	}
	else if (_string == "ESQUIMAL") {
		return CULTURA::ESQUIMAL;
	}
	else if (_string == "INDIA") {
		return CULTURA::INDIA;
	}
	else if (_string == "MEXICANA") {
		return CULTURA::MEXICANA;
	}
	else if (_string == "TIROLESA") {
		return CULTURA::TIROLESA;
	}
	
}

std::string Client::CulturaToString(CULTURA _cultura) {
	if (_cultura == CULTURA::ARABE) {
		return "ARABE";
	}
	else if (_cultura == CULTURA::BANTU) {
		return "BANTU";
	}
	else if (_cultura == CULTURA::CHINA) {
		return "CHINA";
	}
	else if (_cultura == CULTURA::ESQUIMAL) {
		return "ESQUIMAL";
	}
	else if (_cultura == CULTURA::INDIA) {
		return "INDIA";
	}
	else if (_cultura == CULTURA::MEXICANA) {
		return "MEXICANA";
	}
	else if (_cultura == CULTURA::TIROLESA) {
		return "TIROLESA";
	}

}


MIEMBRO_FAMILIA Client::StringToFamilia(std::string _string) {
	if (_string == "ABUELO") {
		return MIEMBRO_FAMILIA::ABUELO;
	}
	else if (_string == "ABUELA") {
		return MIEMBRO_FAMILIA::ABUELA;
	}
	else if (_string == "PADRE") {
		return MIEMBRO_FAMILIA::PADRE;
	}
	else if (_string == "MADRE") {
		return MIEMBRO_FAMILIA::MADRE;
	}
	else if (_string == "HIJO") {
		return MIEMBRO_FAMILIA::HIJO;
	}
	else if (_string == "HIJA") {
		return MIEMBRO_FAMILIA::HIJA;
	}
	

}

std::string Client::FamiliaToString(MIEMBRO_FAMILIA _familia) {
	if (_familia == MIEMBRO_FAMILIA::ABUELO) {
		return "ABUELO";
	}
	else if (_familia == MIEMBRO_FAMILIA::ABUELA) {
		return "ABUELA";
	}
	else if (_familia == MIEMBRO_FAMILIA::PADRE) {
		return "PADRE";
	}
	else if (_familia == MIEMBRO_FAMILIA::MADRE) {
		return "MADRE";
	}
	else if (_familia == MIEMBRO_FAMILIA::HIJO) {
		return "HIJO";
	}
	else if (_familia == MIEMBRO_FAMILIA::HIJA) {
		return "HIJA";
	}


}