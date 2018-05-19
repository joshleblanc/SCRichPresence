// SCRichPresence.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "discord_rpc.h"
#include "time.h"
#include <windows.h> 
#include <iostream>
#include <cpprest/http_client.h>


using namespace utility;
using namespace web::http;
using namespace web::http::client;

static DiscordRichPresence initDiscordPresence() {
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.largeImageKey = "logo";
	return discordPresence;
}

static DiscordRichPresence gamePresence(web::json::value json) {
	wchar_t buffer[256];
	DiscordRichPresence presence = initDiscordPresence();
	presence.startTimestamp = time(0);
	web::json::array players = json.at(L"players").as_array();
	string_t p1name = players.at(0).at(L"name").as_string().c_str();
	wchar_t p1race = players.at(0).at(L"race").as_string().c_str()[0];
	string_t p2name = players.at(1).at(L"name").as_string().c_str();
	wchar_t p2race = players.at(1).at(L"race").as_string().c_str()[0];
	swprintf_s(buffer, (size_t)(sizeof(p1name) + sizeof(p2name) + 2), L"(%c) %s vs (%c) %s", p1race, &p1name[0u], p2race, &p2name[0u]);
	size_t size = 257 * 2;
	size_t convertedChars = 0;
	char *tmp = (char*)malloc(size);
	wcstombs_s(&convertedChars, tmp, size, buffer, size);
	presence.details = tmp;
	return presence;
}

static void inMenu() {
	DiscordRichPresence presence = initDiscordPresence();
	presence.state = "In menu";
	presence.details = "Idle";
	Discord_UpdatePresence(&presence);
}

static void inGame(web::json::value json) {
	DiscordRichPresence presence = gamePresence(json);
	presence.state = "In game";
	Discord_UpdatePresence(&presence);

}

static void inReplay(web::json::value json) {
	DiscordRichPresence presence = gamePresence(json);
	presence.state = "Watching a replay";
	Discord_UpdatePresence(&presence);
}

static void watchStarcraft() {
	http_client client(L"http://localhost:6119");
	int state = -1;
	while (true) {
		http_response screensRequest = client.request(methods::GET, uri_builder(U("/ui")).to_string()).get();
		web::json::value screens = screensRequest.extract_json().get();
		bool in_game = screens.at(L"activeScreens").as_array().size() == 0;
		if (in_game) {
			http_response gameRequest = client.request(methods::GET, uri_builder(U("/game")).to_string()).get();
			web::json::value game = gameRequest.extract_json().get();
			bool is_replay = game.at(L"isReplay").as_bool();
			if (is_replay) {
				if (state != 2) {
					state = 2;
					inReplay(game);
					printf("Entered replay\n");
				}
			}
			else {
				if (state != 1) {
					state = 1;
					inGame(game);
					printf("Entered game\n");
				}
			}
		}
		else {
			if (state != 0) {
				state = 0;
				inMenu();
				printf("Entered menu\n");
			}
		}
		Sleep(1000);
	}
}

static void handleDiscordReady(const DiscordUser* connectedUser)
{
	printf("\nDiscord: connected to user %s#%s - %s\n",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId);
}

static void handleDiscordError(int errcode, const char* message)
{
	printf("\nDiscord: error (%d: %s)\n", errcode, message);
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
	printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

void initDiscord() {
	printf("Initializing Discord...\n");
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.errored = handleDiscordError;
	handlers.disconnected = handleDiscordDisconnected;
	Discord_Initialize("447390334758944770", &handlers, 1, NULL);
}

int main()
{
	printf("Starting...\n");
	try {
		initDiscord();
		watchStarcraft();
	}
	catch (std::exception &e) {
		printf("Starcraft isn't running.\nPlease launch Starcraft and try again.\n\nClosing in 3 seconds.");
		Sleep(3000);
	}
	return 0;
}