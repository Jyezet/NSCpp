#pragma once

// Why nationstates, why
enum class DispatchCategory {
	Factbook = 1,
	Bulletin = 3,
	Account = 5,
	Meta = 8
};

enum class DispatchSubcategory {
	Overview = 100,
	History = 101,
	Geography = 102,
	Culture = 103,
	Politics = 104,
	Legislation = 105,
	Religion = 106,
	FactMilitary = 107,
	Economy = 108,
	International = 109,
	Trivia = 110,
	Miscellaneous = 111,
	Policy = 305,
	News = 315,
	Opinion = 325,
	Campaign = 385,
	AccMilitary = 505,
	Trade = 515,
	Sport = 525,
	Drama = 535,
	Diplomacy = 545,
	Science = 555,
	Culture = 565,
	Other = 595,
	Gameplay = 835,
	Reference = 845
};

// Why not
namespace APIType {
	std::string WORLD = "world", NATION = "nation", REGION = "region";
}

namespace DispatchAction {
	std::string ADD = "add", EDIT = "edit", REMOVE = "remove";
}