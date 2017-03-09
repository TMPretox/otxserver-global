/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2017  Mark Samman <mark.samman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "otpch.h"

#include <fstream>

#include "commands.h"
#include "player.h"
#include "game.h"
#include "iologindata.h"
#include "scheduler.h"

#include "pugicast.h"

extern Game g_game;

namespace {

void sellHouse(Player& player, const std::string& param)
{
	Player* tradePartner = g_game.getPlayerByName(param);
	if (!tradePartner || tradePartner == &player) {
		player.sendCancelMessage("Trade player not found.");
		return;
	}

	if (!Position::areInRange<2, 2, 0>(tradePartner->getPosition(), player.getPosition())) {
		player.sendCancelMessage("Trade player is too far away.");
		return;
	}

	if (!tradePartner->isPremium()) {
		player.sendCancelMessage("Trade player does not have a premium account.");
		return;
	}

	HouseTile* houseTile = dynamic_cast<HouseTile*>(player.getTile());
	if (!houseTile) {
		player.sendCancelMessage("You must stand in your house to initiate the trade.");
		return;
	}

	House* house = houseTile->getHouse();
	if (!house || house->getOwner() != player.getGUID()) {
		player.sendCancelMessage("You don't own this house.");
		return;
	}

	if (g_game.map.houses.getHouseByPlayerId(tradePartner->getGUID())) {
		player.sendCancelMessage("Trade player already owns a house.");
		return;
	}

	if (IOLoginData::hasBiddedOnHouse(tradePartner->getGUID())) {
		player.sendCancelMessage("Trade player is currently the highest bidder of an auctioned house.");
		return;
	}

	Item* transferItem = house->getTransferItem();
	if (!transferItem) {
		player.sendCancelMessage("You can not trade this house.");
		return;
	}

	transferItem->getParent()->setParent(&player);

	if (!g_game.internalStartTrade(&player, tradePartner, transferItem)) {
		house->resetTransferItem();
	}
}

std::map<std::string, CommandFunction> defined_commands = {
	// TODO: move all commands to talkactions

	// player commands
	{"!sellhouse", sellHouse}
};

}

Commands::Commands()
{
	// set up command map
	for (auto& command : defined_commands) {
		commandMap[command.first] = new Command(command.second, 1, ACCOUNT_TYPE_GOD, true);
	}
}

Commands::~Commands()
{
	for (const auto& it : commandMap) {
		delete it.second;
	}
}

bool Commands::loadFromXml()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/commands.xml");
	if (!result) {
		printXMLError("Error - Commands::loadFromXml", "data/XML/commands.xml", result);
		return false;
	}

	for (auto commandNode : doc.child("commands").children()) {
		pugi::xml_attribute cmdAttribute = commandNode.attribute("cmd");
		if (!cmdAttribute) {
			std::cout << "[Warning - Commands::loadFromXml] Missing cmd" << std::endl;
			continue;
		}

		auto it = commandMap.find(cmdAttribute.as_string());
		if (it == commandMap.end()) {
			std::cout << "[Warning - Commands::loadFromXml] Unknown command " << cmdAttribute.as_string() << std::endl;
			continue;
		}

		Command* command = it->second;

		pugi::xml_attribute groupAttribute = commandNode.attribute("group");
		if (groupAttribute) {
			command->groupId = pugi::cast<uint32_t>(groupAttribute.value());
		} else {
			std::cout << "[Warning - Commands::loadFromXml] Missing group for command " << it->first << std::endl;
		}

		pugi::xml_attribute acctypeAttribute = commandNode.attribute("acctype");
		if (acctypeAttribute) {
			command->accountType = static_cast<AccountType_t>(pugi::cast<uint32_t>(acctypeAttribute.value()));
		} else {
			std::cout << "[Warning - Commands::loadFromXml] Missing acctype for command " << it->first << std::endl;
		}

		pugi::xml_attribute logAttribute = commandNode.attribute("log");
		if (logAttribute) {
			command->log = booleanString(logAttribute.as_string());
		} else {
			std::cout << "[Warning - Commands::loadFromXml] Missing log for command " << it->first << std::endl;
		}
		g_game.addCommandTag(it->first.front());
	}
	return true;
}

bool Commands::reload()
{
	for (const auto& it : commandMap) {
		Command* command = it.second;
		command->groupId = 1;
		command->accountType = ACCOUNT_TYPE_GOD;
		command->log = true;
	}

	g_game.resetCommandTag();
	return loadFromXml();
}

bool Commands::exeCommand(Player& player, const std::string& cmd)
{
	std::string str_command;
	std::string str_param;

	std::string::size_type loc = cmd.find(' ', 0);
	if (loc != std::string::npos) {
		str_command = std::string(cmd, 0, loc);
		str_param = std::string(cmd, (loc + 1), cmd.size() - loc - 1);
	} else {
		str_command = cmd;
	}

	//find command
	auto it = commandMap.find(str_command);
	if (it == commandMap.end()) {
		return false;
	}

	Command* command = it->second;
	if (command->groupId > player.getGroup()->id || command->accountType > player.getAccountType()) {
		if (player.getGroup()->access) {
			player.sendTextMessage(MESSAGE_STATUS_SMALL, "You can not execute this command.");
		}

		return false;
	}

	//execute command
	CommandFunction cfunc = command->f;
	cfunc(player, str_param);

	if (command->log) {
		player.sendTextMessage(MESSAGE_STATUS_CONSOLE_RED, cmd);

		std::ostringstream logFile;
		logFile << "data/logs/" << player.getName() << " commands.log";
		std::ofstream out(logFile.str(), std::ios::app);
		if (out.is_open()) {
			time_t ticks = time(nullptr);
			const tm* now = localtime(&ticks);
			char buf[32];
			strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", now);

			out << '[' << buf << "] " << cmd << std::endl;
			out.close();
		}
	}
	return true;
}
