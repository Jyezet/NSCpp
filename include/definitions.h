#pragma once
#include <iostream>

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
	FactCulture = 103,
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
	AccCulture = 565,
	Other = 595,
	Gameplay = 835,
	Reference = 845
};

const std::string validNationShards[] = { "ADMIRABLE", "ADMIRABLES", "ANIMAL", "ANIMALTRAIT", "ANSWERED", "BANNER", "BANNERS", "CAPITAL", "CATEGORY", "CENSUS", "CRIME", "CURRENCY", "CUSTOMLEADER", "CUSTOMCAPITAL", "CUSTOMRELIGION", "DBID", "DEATHS", "DEMONYM", "DEMONYM2", "DEMONYM2PLURAL", "DISPATCHES", "DISPATCHLIST", "ENDORSEMENTS", "FACTBOOKS", "FACTBOOKLIST", "FIRSTLOGIN", "FLAG", "FOUNDED", "FOUNDEDTIME", "FREEDOM", "FULLNAME", "GAVOTE", "GDP", "GOVT", "GOVTDESC", "GOVTPRIORITY", "HAPPENINGS", "INCOME", "INDUSTRYDESC", "INFLUENCE", "LASTACTIVITY", "LASTLOGIN", "LEADER", "LEGISLATION", "MAJORINDUSTRY", "MOTTO", "NAME", "NOTABLE", "NOTABLES", "POLICIES", "POOREST", "POPULATION", "PUBLICSECTOR", "RCENSUS", "REGION", "RELIGION", "RICHEST", "SCVOTE", "SECTORS", "SENSIBILITIES", "TAX", "TGCANRECRUIT", "TGCANCAMPAIGN", "TYPE", "WA", "WABADGES", "WCENSUS", "ZOMBIE" };
const std::string privateShards[] = { "DOSSIER", "ISSUES", "ISSUESUMMARY", "NEXTISSUE", "NEXTISSUETIME", "NOTICES", "PACKS", "PING", "RDOSSIER", "UNREAD" };