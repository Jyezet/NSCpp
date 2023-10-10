#pragma once
#include <iostream>

#include <chrono>

namespace NSCpp {
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

	enum class censusTypes {
		civilRights,
		Economy,
		PoliticalFreedoms,
		Population,
		WealthGaps,
		DeathRate,
		Compassion,
		EcoFriendliness,
		SocialConservatism,
		Nudity,
		IndustryAutomobileManufacturing,
		IndustryCheeseExports,
		IndustryBasketWeaving,
		IndustryInformtionTechnology,
		IndustryPizzaDelivery,
		IndustryTroutFishing,
		IndustryArmsManufacturing,
		SectorAgriculture,
		IndustryBeverageSales,
		IndustryTimberWoodchipping,
		IndustryMining,
		IndustryInsurance,
		IndustryFurnitureRestoration,
		IndustryRetail,
		IndustryBookPublishing,
		IndustryGambling,
		sectorManufacturing,
		GovernmentSize,
		Welfare,
		PublicHealthcare,
		LawEnforcement,
		BusinessSubsidization,
		Religiousness,
		IncomeEquality,
		Niceness,
		Rudeness,
		Intelligence,
		Ignorance,
		PoliticalApathy,
		Health,
		Cheerfulness,
		Weather,
		Compliance,
		Safety,
		Lifespan,
		IdeologicalRadicality,
		DefenseForces,
		Pacifism,
		EconomicFreedom,
		Taxation,
		FreedomFromTaxation,
		Corruption,
		Integrity,
		Authoritarianism,
		YouthRebelliousness,
		Culture,
		Employment,
		PublicTransport,
		Tourism,
		Weaponization,
		RecreationalDrugUse,
		Obesity,
		Secularism,
		EnvironmentalBeauty,
		Charmlessness,
		Influence,
		WorldAssemblyEndorsements,
		Averageness,
		HumanDevelopmentIndex,
		Primitiveness,
		ScientificAdvancement,
		Inclusiveness,
		AverageIncome,
		AverageIncomeOfPoor,
		AverageIncomeOfRich,
		PublicEducation,
		EconomicOutput,
		Crime,
		ForeignAid,
		BlackMarket,
		Residency,
		Survivors,
		Zombies,
		Dead,
		PercentageZombies,
		AverageDisposableIncome,
		InternationalArtwork,
		Patriotism,
		FoodQuality
	};
	const std::string validNationShards[] = { "ADMIRABLE", "ADMIRABLES", "ANIMAL", "ANIMALTRAIT", "ANSWERED", "BANNER", "BANNERS", "CAPITAL", "CATEGORY", "CENSUS", "CRIME", "CURRENCY", "CUSTOMLEADER", "CUSTOMCAPITAL", "CUSTOMRELIGION", "DBID", "DEATHS", "DEMONYM", "DEMONYM2", "DEMONYM2PLURAL", "DISPATCHES", "DISPATCHLIST", "ENDORSEMENTS", "FACTBOOKS", "FACTBOOKLIST", "FIRSTLOGIN", "FLAG", "FOUNDED", "FOUNDEDTIME", "FREEDOM", "FULLNAME", "GAVOTE", "GDP", "GOVT", "GOVTDESC", "GOVTPRIORITY", "HAPPENINGS", "INCOME", "INDUSTRYDESC", "INFLUENCE", "LASTACTIVITY", "LASTLOGIN", "LEADER", "LEGISLATION", "MAJORINDUSTRY", "MOTTO", "NAME", "NOTABLE", "NOTABLES", "POLICIES", "POOREST", "POPULATION", "PUBLICSECTOR", "RCENSUS", "REGION", "RELIGION", "RICHEST", "SCVOTE", "SECTORS", "SENSIBILITIES", "TAX", "TGCANRECRUIT", "TGCANCAMPAIGN", "TYPE", "WA", "WABADGES", "WCENSUS", "ZOMBIE", "DOSSIER", "ISSUES", "ISSUESUMMARY", "NEXTISSUE", "NEXTISSUETIME", "NOTICES", "PACKS", "PING", "RDOSSIER", "UNREAD" };
	const std::string validRegionShards[] = { "BANLIST", "BANNER", "BANNERBY", "BANNERURL", "CENSUS", "CENSUSRANKS", "DBID", "DELEGATE", "DELEGATEAUTH", "DELEGATEVOTES", "DISPATCHES", "EMBASSIES", "EMBASSYRMB", "FACTBOOK", "FLAG", "FOUNDED", "FOUNDEDTIME", "FOUNDER", "FRONTIER", "GAVOTE", "HAPPENINGS", "HISTORY", "LASTUPDATE", "LASTMAJORUPDATE", "LASTMINORUPDATE", "MESSAGES", "NAME", "NATIONS", "NUMNATIONS", "WANATIONS", "NUMWANATIONS", "OFFICERS", "POLL", "POWER", "SCVOTE", "TAGS", "WABADGES", "ZOMBIE" };
	const std::string validWorldShards[] = { "BANNER", "CENSUS", "CENSUSID", "CENSUSDESC", "CENSUSNAME", "CENSUSRANKS", "CENSUSSCALE", "CENSUSTITLE", "DISPATCH", "DISPATCHLIST", "FACTION", "FACTIONS", "FEATUREDREGION", "HAPPENINGS", "LASTEVENTID", "NATIONS", "NEWNATIONS", "NEWNATIONDETAILS", "NUMNATIONS", "NUMREGIONS", "POLL", "REGIONS", "REGIONSBYTAG", "TGQUEUE" };
	const std::string validWAShards[] = { "NUMNATIONS", "NUMDELEGATES", "DELEGATES", "MEMBERS", "HAPPENINGS", "PROPOSALS", "RESOLUTION", "VOTERS", "VOTETRACK", "DELLOG", "DELVOTES", "LASTRESOLUTION" };
	const std::string privateShards[] = { "DOSSIER", "ISSUES", "ISSUESUMMARY", "NEXTISSUE", "NEXTISSUETIME", "NOTICES", "PACKS", "PING", "RDOSSIER", "UNREAD" };

	const std::string authErr = "Please supply valid credentials.";

	// Ignore this class, it's for benchmarking (Code rightfully stolen from https://www.youtube.com/watch?v=YG4jexlSAjc)
	class Timer {
		std::chrono::time_point<std::chrono::high_resolution_clock> startTimepoint;
		std::string timerName;
	public:
		Timer(std::string name) {
			this->startTimepoint = std::chrono::high_resolution_clock::now();
			this->timerName = name;
		}

		~Timer() {
			auto endTimepoint = std::chrono::high_resolution_clock::now();
			auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(startTimepoint).time_since_epoch().count();
			auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(endTimepoint).time_since_epoch().count();
			auto duration = end - start;
			std::cout << "\n" << this->timerName << " took " << duration << " ms\n";
		}
	};
}