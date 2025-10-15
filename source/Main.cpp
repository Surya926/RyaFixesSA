#include "plugin.h"
#include "CStats.h"
#include "CTheZones.h"
#include "CTaskComplexKillPedOnFoot.h"
#include "CTaskComplexSequence.h"
#include "CGangWars.h"
#include "CZone.h"
#include "CZoneInfo.h"

using namespace plugin;

static void CopPistol();
static unsigned long loc_5DDCD9 = 0x005DDCD9;
static void IDidntDoItOfficer();
static unsigned long loc_620AEC = 0x00620AEC;

static void WhereYouFromProxy();
bool IsWhereYouFrom(CPed *ped);
static unsigned long loc_43B341 = 0x0043B341;
static unsigned long loc_43B346 = 0x0043B346;

static void DontGetOffendedProxy();
bool DontGetOffended(CPed *ped);
static unsigned long loc_43BF65 = 0x0043BF65;
static unsigned long loc_43C074 = 0x0043C074;

static void RecruitGangCheck1Proxy();
bool IsRecruitable(CPed* ped);
static unsigned long loc_60C8D5 = 0x0060C8D5; // Recruitable 1
static unsigned long loc_60C8B6 = 0x0060C8B6; // Not recruitable 1
static void RecruitGangCheck2Proxy();
static unsigned long loc_60D409 = 0x0060D409; // Recruitable 2
static unsigned long loc_60D400 = 0x0060D400; // Not recruitable 2
static void RecruitGangCheck3Proxy();
static unsigned long loc_60D4D2 = 0x0060D4D2; // Recruitable 3
static unsigned long loc_60D4DC = 0x0060D4DC; // Not recruitable 3
static void HelpAgainstCopsProxy();
static unsigned long loc_60D541 = 0x0060D541; // Help
static unsigned long loc_60D592 = 0x0060D592; // Not help

void GangTaunt(CPed *ped, CPed *victim);
void GangTauntAtk(CPed* ped, CPed* victim);

static void GangDriveBySpeech1();
static unsigned long loc_62D978 = 0x0062D978;
static void GangDriveBySpeech2();
static unsigned long loc_62D9A3 = 0x0062D9A3;

static void GangWarAtk();
static unsigned long loc_15690D6 = 0x015690D6;
static unsigned long loc_1568EAF = 0x01568EAF;

static void GroveHitBySprayPaintProxy();
static unsigned long loc_62064C = 0x0062064C;
void GroveHitBySprayPaint(CPed* attacker, CPed* victim);

static void WhereYouFromReply();
static void AttackCJProxy();
static unsigned long loc_43C0C2 = 0x0043C0C2;
void AttackCJ(CPed* attacker);

static void DisbandReply();
static unsigned long loc_5F8174 = 0x005F8174;
void DisbandGangReply(CPed* ped);

static void RecruitedPedBlip();
static unsigned long loc_60CB34 = 0x0060CB34;

void GangWarStuff();
void GangWarSwitches();

// I wanna make Silent's ExGangWars plugin dynamic, make the Rifa, DNB, and Italian Mafia territories acquireable
// only after completing certain missions - Rya
const size_t NUM_GANGS = 10;
class CZoneExtraInfo
{
public:
	unsigned char			m_nGangDensity[NUM_GANGS];
	unsigned char			DrugDealerCounter;
	CRGBA					ZoneColour;
	bool					unk1 : 1;
	bool					unk2 : 1;
	bool					unk3 : 1;
	bool					unk4 : 1;
	bool					unk5 : 1;
	bool					bUseColour : 1;
	bool					bInGangWar : 1;
	bool					bNoCops : 1;
	unsigned char			flags;
};

// Custom ExGangWars structure
struct tGangInfo
{
	bool					bCanFightWith : 1;
	bool					bShowOnMap	  : 1;
	unsigned char			bRed, bGreen, bBlue;
	unsigned int			nBlipIndex;
};

// SA global variables
unsigned int&			TotalNumberOfNavigationZones = **reinterpret_cast<unsigned int**>(0x443B06);
unsigned short&			TotalNumberOfZoneInfos = **reinterpret_cast<unsigned short**>(0x572200);
CZone* const		    NavigationZoneArray = *reinterpret_cast<CZone**>(0x443C1C);
CZoneExtraInfo* const	ZoneInfoArray = *reinterpret_cast<CZoneExtraInfo**>(0x443C51);

float&			        TerritoryUnderControlPercentage = **reinterpret_cast<float**>(0x443EDB);
unsigned int* const	    GangRatings = *reinterpret_cast<unsigned int**>(0x443F07);
unsigned int* const 	GangRatingStrength = *reinterpret_cast<unsigned int**>(0x443F0D);

auto CanPlayerStartAGangWarHere = reinterpret_cast<bool(*)(CZoneExtraInfo*)>(0x443F80);

// Custom ExGangWars variables
static tGangInfo		CustomGangInfo[NUM_GANGS];

// This function needs to remove the parameter from the stack by itself
int __stdcall PickDefensiveGang(CZoneInfo* pZone)
{
	// Attacking gang is the strongest 'fightable' gang at this territory
	int		nCurrentStrongestGang = -1;
	int		nStrongestGangStrength = -1;
    int     casinoHeist = CStats::GetStatValue(307);
    int     homeComing = CStats::GetStatValue(308); // actually Home in the Hills' flag, but you know what I mean.

    for (int i = 0; i < NUM_GANGS; i++)
    {
        if (pZone->m_nGangDensity[i] > nStrongestGangStrength)
        {
            nStrongestGangStrength = pZone->m_nGangDensity[i];
            nCurrentStrongestGang = i;
        }
    }

	// nCurrentStrongestGang should never be -1, but if it is, we'll default to Ballas
	return nCurrentStrongestGang != -1 ? nCurrentStrongestGang : 0;
}

// This function needs to remove the parameter from the stack by itself
bool __stdcall CanNotPedtypeBeProvoked(unsigned int nPedType)
{
	// Returns true if this pedtype can NOT be provoked to start a gangwar
	// Obviously, the pedtype has to be a gang first...

    // I wanna limit this only to fightable gangs - Rya
    unsigned int casinoHeist = CStats::GetStatValue(307);
    unsigned int homeComing = CStats::GetStatValue(308); // actually Home in the Hills' flag, but you know what I mean.

    switch(nPedType){
    case PED_TYPE_GANG1: // Ballas
    case PED_TYPE_GANG3: // Vagos
    case PED_TYPE_GANG9: // Russians
        return false;
    case PED_TYPE_GANG4: // Rifas
    case PED_TYPE_GANG5: // DNB
        if (homeComing)
            return false;
        else
            return true;
    case PED_TYPE_GANG6: // Mafia
        if (casinoHeist)
            return false;
        else
            return true;
    default:
        return true;
    }
}

unsigned int GetRivalGangsTotalDensity(unsigned int nZoneExtraInfoID)
{
	// We need to get a total strength of all gangs player can fight with
	// so the game can decide whether to start a defensive gang war there
	unsigned int		nTotalStrength = 0;
    unsigned int        casinoHeist = CStats::GetStatValue(307);
    unsigned int        homeComing = CStats::GetStatValue(308); // actually Home in the Hills' flag, but you know what I mean.

    for ( unsigned int i = 0; i < NUM_GANGS; i++ )
	{
        switch(i){
        case 0: // GANG1 - Ballas
        case 2: // GANG3 - Vagos
        case 8: // GANG9 - Russians
			nTotalStrength += ZoneInfoArray[nZoneExtraInfoID].m_nGangDensity[i];
            break;
        case 3: // GANG4 - Rifas
        case 4: // GANG5 - DNB
		    if ( homeComing )
			    nTotalStrength += ZoneInfoArray[nZoneExtraInfoID].m_nGangDensity[i];
            break;
        case 5: // GANG6 - Mafia
		    if ( casinoHeist )
			    nTotalStrength += ZoneInfoArray[nZoneExtraInfoID].m_nGangDensity[i];
            break;
        default: // no no no, you can't take over Grove's, Triads', Aztecas' territories! :p
            break;
        }
	}
	return nTotalStrength;
}

void FillZonesWithGangColours(bool bDontColour) // All gangs remained have colours on the radar and map, I won't change a thing here from Silent's code. - Rya
{
	for ( int16_t i = 0; i < TotalNumberOfZoneInfos; i++ )
	{
		uint32_t	nTotalDensity = 0;
		uint32_t	bRedToPick = 0, bGreenToPick = 0, bBlueToPick = 0;

		for ( int j = 0; j < NUM_GANGS; j++ )
		{
			if ( CustomGangInfo[j].bShowOnMap )
			{
				nTotalDensity += ZoneInfoArray[i].m_nGangDensity[j];
				bRedToPick += CustomGangInfo[j].bRed * ZoneInfoArray[i].m_nGangDensity[j];
				bGreenToPick += CustomGangInfo[j].bGreen * ZoneInfoArray[i].m_nGangDensity[j];
				bBlueToPick += CustomGangInfo[j].bBlue * ZoneInfoArray[i].m_nGangDensity[j];
			}
		}

		ZoneInfoArray[i].bUseColour = nTotalDensity != 0 && !bDontColour && CanPlayerStartAGangWarHere(&ZoneInfoArray[i]);
		ZoneInfoArray[i].bInGangWar = false;

		ZoneInfoArray[i].ZoneColour.a = static_cast<uint8_t>(std::min<uint32_t>(120, 3 * nTotalDensity));

		if ( nTotalDensity != 0 )
			ZoneInfoArray[i].ZoneColour.a = std::max<uint8_t>(55, ZoneInfoArray[i].ZoneColour.a);
		else
			nTotalDensity = 1;

		// The result is a simple weighted arithmetic average
		// each gang's RGB having the weight of gang's density in this area
		ZoneInfoArray[i].ZoneColour.r = static_cast<uint8_t>(bRedToPick / nTotalDensity);
		ZoneInfoArray[i].ZoneColour.g = static_cast<uint8_t>(bGreenToPick / nTotalDensity);
		ZoneInfoArray[i].ZoneColour.b = static_cast<uint8_t>(bBlueToPick / nTotalDensity);
	}
}

void UpdateTerritoryUnderControlPercentage()
{
	std::pair<unsigned char,unsigned int>	vecZonesForGang[NUM_GANGS];
	unsigned int		nTotalTerritories = 0;
    unsigned int        casinoHeist = CStats::GetStatValue(307);
    unsigned int        homeComing = CStats::GetStatValue(308); // actually Home in the Hills' flag, but you know what I mean.

	// Initialise the array
	{
		unsigned char index = 0;
		for ( auto& it : vecZonesForGang )
		{
			it.first = index++;
		}
	}

	// Count the turfs belonging to each gang
	for ( unsigned int i = 0; i < TotalNumberOfNavigationZones; i++ )
	{
		const unsigned int		nZoneInfoIndex = NavigationZoneArray[i].m_nZoneExtraIndexInfo;
		if ( nZoneInfoIndex != 0 )
		{
			// Should we even count this territory?
			bool	bCountMe = false;

			for ( unsigned int j = 0; j < NUM_GANGS; j++ )
			{
				if ( (j == 1 || j == 6 || j == 7 || CustomGangInfo[j].bCanFightWith) && ZoneInfoArray[nZoneInfoIndex].m_nGangDensity[j] != 0 )
				{
					bCountMe = true;
					break;
				}
			}

			if ( bCountMe )
			{
				// Instantiate a very temporary array to find which fightable gang has the most influence in this area
				unsigned char		vecGangPopularity[NUM_GANGS];

				for ( unsigned char j = 0; j < NUM_GANGS; j++ )
				{
					vecGangPopularity[j] = j == 1 || j == 6 || j == 7 || CustomGangInfo[j].bCanFightWith ? ZoneInfoArray[nZoneInfoIndex].m_nGangDensity[j] : 0;
				}

				auto it = std::max_element( std::begin(vecGangPopularity), std::end(vecGangPopularity) );

				// Add to gang's territory counter
				vecZonesForGang[std::distance(std::begin(vecGangPopularity), it)].second++;
				nTotalTerritories++;
			}
		}
	}

	// Update the stats
    if (homeComing){
        CStats::SetStatValue(236, (static_cast<float>(vecZonesForGang[1].second) + static_cast<float>(vecZonesForGang[6].second) + static_cast<float>(vecZonesForGang[7].second)));                                         // NUMBER_TERRITORIES_HELD
	    CStats::SetStatValue(237, std::max((static_cast<float>(vecZonesForGang[1].second) + static_cast<float>(vecZonesForGang[6].second) + static_cast<float>(vecZonesForGang[7].second)), CStats::GetStatValue(237)));	// HIGHEST_NUMBER_TERRITORIES_HELD
    }
    else {
        CStats::SetStatValue(236, static_cast<float>(vecZonesForGang[1].second));								        // NUMBER_TERRITORIES_HELD
	    CStats::SetStatValue(237, std::max(static_cast<float>(vecZonesForGang[1].second), CStats::GetStatValue(237)));	// HIGHEST_NUMBER_TERRITORIES_HELD
    }

	if ( nTotalTerritories != 0 )
	{
        if (homeComing)
		    TerritoryUnderControlPercentage = (static_cast<float>(vecZonesForGang[1].second) + static_cast<float>(vecZonesForGang[6].second) + static_cast<float>(vecZonesForGang[7].second)) / nTotalTerritories;
        else
		    TerritoryUnderControlPercentage = static_cast<float>(vecZonesForGang[1].second) / nTotalTerritories;

		// Sort the array to find top 3 gangs
		std::partial_sort(std::begin(vecZonesForGang), std::begin(vecZonesForGang) + 3, std::end(vecZonesForGang), [] (const auto& Left, const auto& Right)
		{ 
			if ( Right.second < Left.second ) return true;
			if ( Left.second < Right.second ) return false;

			// In case of a tie, game favours GSF, then Ballas, then Vagos
			// So we sort by gang ID, with the exception that ID 1 (GSF) always comes first
			if ( Left.first == 1 ) return true;
			if ( Right.first == 1 ) return false;

			return Left.first < Right.first;
		});

		GangRatings[0] = vecZonesForGang[0].first;
		GangRatings[1] = vecZonesForGang[1].first;
		GangRatings[2] = vecZonesForGang[2].first;

		GangRatingStrength[0] = vecZonesForGang[0].second;
		GangRatingStrength[1] = vecZonesForGang[1].second;
		GangRatingStrength[2] = vecZonesForGang[2].second;
	}
	else
		TerritoryUnderControlPercentage = 0.0f;
}

class GangFixes {
public:
    GangFixes() {
        Events::gameProcessEvent += [] { // live processed stuff
            
            // Stat related stuff for gangs
            float missionRelationshipStat = CStats::GetStatValue(119);
            if (missionRelationshipStat >= 2.0f) { // After "Mountain Cloud Boys"
                patch::SetUChar(0x0043DF17, 2); // Triads - lost respect
                patch::SetUChar(0x0043D2E2, 1); // Triads - listed as friendly gang killed
                if (missionRelationshipStat >= 5.0f) { // After "Los Desperados"
                    patch::SetUChar(0x0043DF18, 2); // Aztecas - lost respect
                    patch::SetUChar(0x0043D2E3, 1); // Aztecas - listed as friendly gang killed
                }
                else {
                    patch::SetUChar(0x0043DF18, 1); // Aztecas - gain respect
                    patch::SetUChar(0x0043D2E3, 0); // Aztecas - listed as enemy gang killed
                }
            }
            else {
                // Adjustments to respect stats - 0: gain average respect (Cop, Drug Dealers), 1: gain good respect (Gangs), 2: lost respect; when you kill them.
                patch::SetUChar(0x0043DF17, 1); // Triads
                patch::SetUChar(0x0043DF18, 1); // Aztecas
                // set these as friendly/enemy gang members killed in stats menu - 0: enemies, 1: friendly
                patch::SetUChar(0x0043D2E2, 0); // Triads
                patch::SetUChar(0x0043D2E3, 0); // Aztecas
            }

            // Cops based on city you're in
            switch (CTheZones::m_CurrLevel) {
            case 1:
                patch::SetUInt(0x005DDD86, 284); // LS Biker Cop
                patch::SetUInt(0x008A5AB0, 284); // LS Biker Cop
                break;
            case 2:
                patch::SetUInt(0x005DDD86, 272); // SF Biker Cop
                patch::SetUInt(0x008A5AB0, 272); // SF Biker Cop
                break;
            case 3:
                patch::SetUInt(0x005DDD86, 273); // LV Biker Cop
                patch::SetUInt(0x008A5AB0, 273); // LV Biker Cop
                break;
            default:
                patch::SetUInt(0x005DDD86, 283); // Countryside Cop
                patch::SetUInt(0x008A5AB0, 283); // Countryside Cop
            }

            // Gang wars stuff
            GangWarStuff();
            GangWarSwitches();

            // Make the Triads', Aztecas', and Bikers' turfs completely unavailable to take (not even via territory glitch!)
            patch::Nop(0x0044619C, 49); // Triads
            patch::Nop(0x004461CD, 49); // Aztecas
            patch::Nop(0x0044622F, 49); // Bikers

            // Makes territory gained as Triads' in LV
            switch (CTheZones::m_CurrLevel) {
            case 3:
                // Makes territory gained as Triads' not Grove's
                patch::SetUChar(0x00445E8E, 6); // Gang ID - Triads
                patch::SetUChar(0x00445EB2, 6);
                patch::SetUChar(0x00445EEC, 6);
                patch::SetUChar(0x00445EF3, 6);
                patch::SetUChar(0x00445F13, 6);
                // Enemy version
                patch::SetUChar(0x00445FD8, 6);
                patch::SetUChar(0x00446016, 6);
                patch::SetUChar(0x00446019, 6);
                patch::SetUChar(0x00446026, 6);
                // Strengthen the zone!
                patch::SetUChar(0x01565098, 6);
                patch::SetUChar(0x015650CF, 6);
                patch::SetUChar(0x015650DE, 6);
                // Enemies attack zones occupied by this gang!
                patch::SetUChar(0x0443C0C, 0xF6);
                break;
            default:
                // Makes territory gained as Grove's
                patch::SetUChar(0x00445E8E, 1); // Gang ID - Grove
                patch::SetUChar(0x00445EB2, 1);
                patch::SetUChar(0x00445EEC, 1);
                patch::SetUChar(0x00445EF3, 1);
                patch::SetUChar(0x00445F13, 1);
                // Enemy version
                patch::SetUChar(0x00445FD8, 1);
                patch::SetUChar(0x00446016, 1);
                patch::SetUChar(0x00446019, 1);
                patch::SetUChar(0x00446026, 1);
                // Strengthen the zone!
                patch::SetUChar(0x01565098, 1);
                patch::SetUChar(0x015650CF, 1);
                patch::SetUChar(0x015650DE, 1);
                // Enemies attack zones occupied by this gang!
                patch::SetUChar(0x0443C0C, 0xF1);
            }
        };

        // Trick the cops to think that they're holding pistol by default, so they'll more responsive to gun crimes caused by NPCs
		patch::RedirectJump(0x005DDCD3, &CopPistol);

        // Remove the unneccessary "give pistol" to peds that sometimes overwrite their current weapons
        // (Imagine already having an AK, yet unused and you use Pistol to kill others instead)
        patch::Nop(0x0068E40C, 27); // Ped responses to the cops
        patch::Nop(0x0062E1BD, 23); // Gang responses to another car hit their car
        // Fight like a man, bitches!
/*      patch::SetUChar(0x0062B3B9, 1);
        patch::SetUChar(0x0062B3C2, 1);
        //
        patch::SetUChar(0x0062B5C6, 1);
        patch::SetUChar(0x0062B5CF, 1);
        //
        patch::SetUChar(0x005E8B97, 1);
        patch::SetUChar(0x005E8BA2, 1);
        //
        patch::SetUChar(0x005E8B19, 1);
        patch::SetUChar(0x005E8B24, 1);
        //
        patch::SetUChar(0x005E8B58, 10);
        patch::SetUChar(0x005E8B63, 10);*/

        // Make peds who got hit by cops say "I didn't do it, Officer!"
        patch::RedirectJump(0x00620AD3, &IDidntDoItOfficer);

        // Fix CJ's missing voice when his bike got hit
        patch::SetUShort(0x008C6E8E, 5); // Crash bike speech ID (66)

		// Where you from stuff
		patch::RedirectJump(0x0043B332, &WhereYouFromProxy); // "where you from" quotes pedtype check
        patch::SetUChar(0x0043B347, 9); // fix the "where you from" RNG from 8-10 to 8-9 as the option 10 does not exist!
		patch::RedirectJump(0x0043BF56, &DontGetOffendedProxy); // make friendly gangs not get offended when CJ responded negatively to their insult
        // replies - topic ID: 8
        patch::SetUChar(0x0043BC05, 0xA1);
        patch::SetUInt(0x0043BC06, 0x9691C0); // mov, eax ds:CPedToPlayerConversations::m_pPed
        patch::SetUChar(0x0043BC0A, 0xE9);
        patch::SetUInt(0x0043BC0B, 0x2BE); // jmp loc_43BECD
        patch::Nop(0x0043BC0F, 1);
        // replies - topic ID: 9
        patch::SetUChar(0x0043BD0B, 0xA1);
        patch::SetUInt(0x0043BD0C, 0x9691C0); // mov, eax ds:CPedToPlayerConversations::m_pPed
        patch::SetUChar(0x0043BD10, 0xE9);
        patch::SetUInt(0x0043BD11, 0x1B8); // jmp loc_43BECD
        patch::Nop(0x0043BD15, 1);
        // redirect topic ID 8 and 9 to this segment
        patch::SetUChar(0x0043C0B3, 0x15);
        patch::SetUChar(0x0043C0B8, 0xA1);
        patch::SetUInt(0x0043C0B9, 0x9691BC); // mov, eax ds:CPedToPlayerConversations::m_nTopic
        patch::RedirectJump(0x0043C0BD, &WhereYouFromReply); // "where you from" reply from the asking gang member
        patch::SetUChar(0x0043C0C2, 0xA1);
        patch::SetUInt(0x0043C0C3, 0x9691C0); // mov, eax ds:CPedToPlayerConversations::m_pPed
        patch::RedirectJump(0x0043C0C7, &AttackCJProxy); // attack CJ after above fiasco
        patch::Nop(0x0043C0CC, 2);


		// Gang taunt stuff
        patch::RedirectCall(0x00660248, &GangTaunt); // CTaskGangHasslePed::ControlSubTask
        patch::RedirectCall(0x0066069E, &GangTaunt); // CTaskComplexStareAtPed::ControlSubTask
        // Fix the SF and LV gang taunts (the gangs mentioned below are the "victim" not the taunting one)
        patch::SetUShort(0x008C6AC0, 0x43); // Triads speech ID (5)
        //patch::SetUShort(0x008C6AD0, 0xFFFF); // Mafia speech ID (6) - still unknown (no gang taunts the Mafia at all, not even the Triads!)
        patch::SetUShort(0x008C6AE0, 0x42); // Rifa speech ID (7)
        patch::SetUShort(0x008C6AF0, 0x6D); // Da Nang speech ID (8)

		// Gang taunt while attacking stuff
        patch::RedirectCall(0x00626A5B, &GangTauntAtk); // CTaskComplexKillPedOnFoot::ControlSubTask
        // Patches the unused speech IDs for SF and LV gang taunts while attacking  (the gangs mentioned below are the "victim" not the taunting one)
        //patch::SetUShort(0x008C6ED0, 0xFFFF); // Unknown - now Triads speech ID (70)
        //patch::SetUShort(0x008C6EE0, 0xFFFF); // Unknown - now Mafia speech ID (71)
        //patch::SetUShort(0x008C6EF0, 0xFFFF); // Unknown - now Rifa speech ID (72)
        patch::SetUShort(0x008C6F00, 0x6E); // Unknown - now Da Nang speech ID (73)

        // Grove's replies when CJ's group got disbanded
        patch::RedirectJump(0x005F8168, &DisbandReply);

        // Make Triads and Aztecas recruitable as the story goes
        patch::RedirectJump(0x0060C8AD, &RecruitGangCheck1Proxy); // recruitable gang pedtype check
        patch::RedirectJump(0x0060D3F7, &RecruitGangCheck2Proxy); // recruitable gang pedtype check when you pressed DPAD-UP or G
        patch::RedirectJump(0x0060D4C9, &RecruitGangCheck3Proxy); // recruitable gang pedtype check when you pressed DPAD-DOWN or H

        // Make Triads and Aztecas help you fight cops as the story goes
        patch::RedirectJump(0x0060D538, &HelpAgainstCopsProxy); // pedtype check

        // Make these gangs don't attack CJ when he started a gang war (default only Grove, now expanded to all non-gangwarable gangs)
        patch::RedirectJump(0x01568E94, &GangWarAtk); // pedtype check

        // Make other gangs say something when drive-by
        patch::RedirectJump(0x0062D960, &GangDriveBySpeech1);
        patch::RedirectJump(0x0062D98D, &GangDriveBySpeech2);

        // Russian Mafia and Bikers blip colours
        patch::SetUChar(0x008D134C, 0xC8); // Russian Mafia R
        patch::SetUChar(0x008D1358, 0); // Russian Mafia G
        patch::SetUChar(0x008D1364, 0); // Russian Mafia B
        //
        patch::SetUChar(0x008D134D, 0x46); // Bikers R
        patch::SetUChar(0x008D1359, 0x46); // Bikers G
        patch::SetUChar(0x008D1365, 0x46); // Bikers B

        // Make the Grove react to spraying paint over their faces
        patch::RedirectJump(0x00620633, &GroveHitBySprayPaintProxy);
        patch::SetUShort(0x008C7AB0, 0x2F); // Unknown - now Grove hit by spray paint reaction speech ID (260)

        // Recruited peds blip change based on pedtypes
        patch::RedirectJump(0x0060CB1F, &RecruitedPedBlip);

///////////////////////////////////////////////////////////////////
////////// Silent's ExGangWars plugin!/////////////////////////////
///////////////////////////////////////////////////////////////////
        patch::RedirectJump(0x572440, FillZonesWithGangColours);
	    //InjectHook(0x44665D, UpdateTerritoryUnderControlPercentage);
        patch::RedirectJump(0x443DE0, UpdateTerritoryUnderControlPercentage);

	    // cmp eax, 1 \ jz     loc_4463EC
	    patch::Nop(0x446272, 4);
	    patch::SetUChar(0x446278, 1);
	    patch::SetUChar(0x44627A, 0x84);

	    // push edx \ call CanNotPedtypeBeProvoked \ test al, al
	    patch::SetUChar(0x443968, 0x52);
	    patch::SetUShort(0x44396E, 0xC084);
	    patch::RedirectCall(0x443969, CanNotPedtypeBeProvoked);

	    // Allow defensive gang wars in entire state
	    patch::SetUShort(0x443B9D, 0x65EB);

	    // push edx \ push ebx \ call GetRivalGangsTotalDensity \ add esp, 4 \ pop edx \ imul ebx, 11h \ cmp eax, 14h
	    patch::SetUShort(0x443B55, 0x5352);
	    patch::RedirectCall(0x443B55 + 2, GetRivalGangsTotalDensity);
	    patch::SetUInt(0x443B55 + 7, 0x5A04C483);
	    patch::SetUInt(0x443B55 + 11, 0x8311DB6B);
	    patch::SetUInt(0x443B55 + 15, 0x909014F8);
	    patch::Nop(0x443B55 + 19, 3);

	    // lea edx, CTheZones::ZoneInfoArray[ebx] \ push edx \ call PickDefensiveGang
	    patch::SetUShort(0x443C3B, 0x938D);
	    patch::SetUChar(0x443C65, 0x52);
	    patch::Nop(0x443C6B, 2);
	    patch::RedirectCall(0x443C66, PickDefensiveGang);

	    // mov ecx, CustomGangInfo.nBlipIndex[ecx*8] \ push ecx
	    patch::SetUShort(0x4443EB, 0x0C8B);
	    patch::SetUChar(0x4443ED, 0xCD);
	    patch::SetPointer(0x4443EE, &CustomGangInfo->nBlipIndex);
	    patch::SetUChar(0x4443F2, 0x51);
	    patch::SetUShort(0x4443F3, 0x0DEB);

	    // mov edx, CustomGangInfo.nColour[eax*8]
	    patch::Nop(0x44438C, 6);
	    patch::Nop(0x4443B0, 3);
	    patch::SetUShort(0x444399, 0x148B);
	    patch::SetUChar(0x44439B, 0xC5);
	    patch::SetPointer(0x44439C, &CustomGangInfo->bRed);
	    patch::SetUShort(0x4443A0, 0x09EB);
    };
} GangFixesPlugin;

void GangWarSwitches()
{
            // Make the Rifas and DNB turf completely unattackable before Home Coming (not even via territory glitch!)
            unsigned int homeComing = CStats::GetStatValue(308); // actually Home in the Hills' flag, but you know what I mean.
            if (homeComing){
                patch::SetUChar(0x0044610C, 3); // Rifas
                patch::SetUInt(0x00446119, 3);
                patch::SetUChar(0x00446127, 3);
                patch::SetUInt(0x00446136, 3);
                //
                patch::SetUChar(0x0044613D, 4); // DNB
                patch::SetUInt(0x0044614A, 4);
                patch::SetUChar(0x00446158, 4);
                patch::SetUInt(0x00446167, 4);
            }
            else{
                patch::SetUChar(0x0044610C, 0); // fall back to Ballas
                patch::SetUInt(0x00446119, 0);
                patch::SetUChar(0x00446127, 0);
                patch::SetUInt(0x00446136, 0);
                //
                patch::SetUChar(0x0044613D, 0); // fall back to Ballas
                patch::SetUInt(0x0044614A, 0);
                patch::SetUChar(0x00446158, 0);
                patch::SetUInt(0x00446167, 0);
            }

            // Make the Mafia turf completely unattackable before the casino heist (not even via territory glitch!)
            unsigned int casinoHeist = CStats::GetStatValue(307);
            if (casinoHeist){
                patch::SetUChar(0x0044616E, 5); // Mafia
                patch::SetUInt(0x0044617B, 5);
                patch::SetUChar(0x00446189, 5);
                patch::SetUInt(0x00446198, 5);
            }
            else{
                patch::SetUChar(0x0044616E, 0); // fall back to Ballas
                patch::SetUInt(0x0044617B, 0);
                patch::SetUChar(0x00446189, 0);
                patch::SetUInt(0x00446198, 0);
            }
}

void __declspec(naked) RecruitedPedBlip()
{
    __asm
    {
        cmp [edi+598h], 7
        jz blipBallas
        cmp [edi+598h], 8
        jz blipGrove
        cmp [edi+598h], 9
        jz blipVagos
        cmp [edi+598h], 10
        jz blipRifas
        cmp [edi+598h], 11
        jz blipDaNang
        cmp [edi+598h], 12
        jz blipMafia
        cmp [edi+598h], 13
        jz blipTriads
        cmp [edi+598h], 14
        jz blipAztecas
        cmp [edi+598h], 15
        jz blipRussians
        cmp [edi+598h], 16
        jz blipBikers
        mov esi, 0 // default ped blip colors
        mov eax, 0
        mov ecx, 7Fh
        jmp loc_60CB34
    blipBallas:
        mov esi, 0C8h
        mov eax, 0
        mov ecx, 0C8h
        jmp loc_60CB34
    blipGrove:
        mov esi, 46h
        mov eax, 0C8h
        mov ecx, 0
        jmp loc_60CB34
    blipVagos:
        mov esi, 0FFh
        mov eax, 0C8h
        mov ecx, 0
        jmp loc_60CB34
    blipRifas:
        mov esi, 0
        mov eax, 0
        mov ecx, 0C8h
        jmp loc_60CB34
    blipDaNang:
        mov esi, 0FFh
        mov eax, 0DCh
        mov ecx, 0BEh
        jmp loc_60CB34
    blipMafia:
        mov esi, 0C8h
        mov eax, 0C8h
        mov ecx, 0C8h
        jmp loc_60CB34
    blipTriads:
        mov esi, 0F0h
        mov eax, 8Ch
        mov ecx, 0F0h
        jmp loc_60CB34
    blipAztecas:
        mov esi, 0
        mov eax, 0C8h
        mov ecx, 0FFh
        jmp loc_60CB34
    blipRussians:
        mov esi, 0C8h
        mov eax, 0
        mov ecx, 0
        jmp loc_60CB34
    blipBikers:
        mov esi, 46h
        mov eax, 46h
        mov ecx, 46h
        jmp loc_60CB34
    }
}

void GangWarStuff()
{
        // Ballas
        CustomGangInfo[0].bCanFightWith = true;
		CustomGangInfo[0].bRed = 0xC8;
		CustomGangInfo[0].bGreen = 0;
		CustomGangInfo[0].bBlue = 0xC8;
		CustomGangInfo[0].bShowOnMap = true;
	    CustomGangInfo[0].nBlipIndex = 59;

        // Grove
        CustomGangInfo[1].bCanFightWith = false;
		CustomGangInfo[1].bRed = 0x46;
		CustomGangInfo[1].bGreen = 0xC8;
		CustomGangInfo[1].bBlue = 0;
		CustomGangInfo[1].bShowOnMap = true;
		CustomGangInfo[1].nBlipIndex = 62;

        // Vagos
        CustomGangInfo[2].bCanFightWith = true;
		CustomGangInfo[2].bRed = 0xFF;
		CustomGangInfo[2].bGreen = 0xC8;
		CustomGangInfo[2].bBlue = 0;
		CustomGangInfo[2].bShowOnMap = true;
		CustomGangInfo[2].nBlipIndex = 60;

        // Rifas
        CustomGangInfo[3].bCanFightWith = true;
		CustomGangInfo[3].bRed = 0;
		CustomGangInfo[3].bGreen = 0;
		CustomGangInfo[3].bBlue = 0xC8;
		CustomGangInfo[3].bShowOnMap = true;
		CustomGangInfo[3].nBlipIndex = 61;

        // DNB
        CustomGangInfo[4].bCanFightWith = true;
		CustomGangInfo[4].bRed = 0xFF;
		CustomGangInfo[4].bGreen = 0xDC;
		CustomGangInfo[4].bBlue = 0xBE;
		CustomGangInfo[4].bShowOnMap = true;
		CustomGangInfo[4].nBlipIndex = 26;

        // Mafia
        CustomGangInfo[5].bCanFightWith = true;
		CustomGangInfo[5].bRed = 0xC8;
		CustomGangInfo[5].bGreen = 0xC8;
		CustomGangInfo[5].bBlue = 0xC8;
		CustomGangInfo[5].bShowOnMap = true;
        CustomGangInfo[5].nBlipIndex = 58; //25;

        // Triads
        CustomGangInfo[6].bCanFightWith = false;
		CustomGangInfo[6].bRed = 0xF0;
		CustomGangInfo[6].bGreen = 0x8C;
		CustomGangInfo[6].bBlue = 0xF0;
		CustomGangInfo[6].bShowOnMap = true;
		CustomGangInfo[6].nBlipIndex = 43;

        // Aztecas
        CustomGangInfo[7].bCanFightWith = false;
		CustomGangInfo[7].bRed = 0;
		CustomGangInfo[7].bGreen = 0xC8;
		CustomGangInfo[7].bBlue = 0xFF;
		CustomGangInfo[7].bShowOnMap = true;
        CustomGangInfo[7].nBlipIndex = 13; //58;

        // Russians
        CustomGangInfo[8].bCanFightWith = true;
		CustomGangInfo[8].bRed = 0xC8;
		CustomGangInfo[8].bGreen = 0;
		CustomGangInfo[8].bBlue = 0;
		CustomGangInfo[8].bShowOnMap = true;
		CustomGangInfo[8].nBlipIndex = 19;

        // Bikers
        CustomGangInfo[9].bCanFightWith = false;
		CustomGangInfo[9].bRed = 0x46;
		CustomGangInfo[9].bGreen = 0x46;
		CustomGangInfo[9].bBlue = 0x46;
		CustomGangInfo[9].bShowOnMap = true;
		CustomGangInfo[9].nBlipIndex = 53;
}

void __declspec(naked) DisbandReply()
{
    __asm
    {
        mov eax, [esi]
        or dword ptr[eax + 470h], 100000h
        push [esi]
        call DisbandGangReply
        pop ecx
        jmp loc_5F8174
    }
}

void DisbandGangReply(CPed* ped)
{
    ped->Say(168, 0, 1.0f, 1, 0, 0); // Grove's responses to CJ disbanding his group
}

void __declspec(naked) WhereYouFromReply()
{
    __asm
    {
        cmp eax, 8
        jge gangTopicReply
        push 0
        push 0
        push 1
        push 3F800000h
        push 0
        push 133 // Ped's response to CJ's negative reply to their compliments
        mov ecx, edx
        call CPed::Say
        jmp loc_43C074
    gangTopicReply:
        push 0
        push 0
        push 1
        push 3F800000h
        push 0
        push 253 // Gang's reply after CJ's reply to "Where you from"
        mov ecx, edx
        call CPed::Say
        jmp loc_43C0C2
    }
}

void __declspec(naked) AttackCJProxy()
{
    __asm
    {
        push eax
        call AttackCJ
        pop eax
        jmp loc_43C074
    }
}

void AttackCJ(CPed* attacker)
{
    CPed* player = FindPlayerPed();
    if (player && player->IsAlive() && attacker && attacker->IsAlive()) {
        CTask* task = new CTaskComplexKillPedOnFoot(player, -1, 0, 0, 0, 0);
        attacker->m_pIntelligence->m_TaskMgr.SetTask(task, 3, false);
    }
}

void __declspec(naked) GroveHitBySprayPaintProxy()
{
    __asm
    {
        push ebx
        push [esi+8]
        call GroveHitBySprayPaint
        pop ecx
        pop edx
        jmp loc_62064C
    }
}

void GroveHitBySprayPaint(CPed* attacker, CPed* victim)
{
    if (attacker && attacker->IsPlayer() && victim->m_nPedType == PED_TYPE_GANG2 && victim->m_nLastWeaponDamage == WEAPONTYPE_SPRAYCAN)
        victim->Say(260, 0, 1.0f, 0, 0, 0);
    else
        victim->Say(340, 0, 1.0f, 0, 0, 0);
}

void __declspec(naked) GangWarAtk()
{
    __asm
    {
        cmp eax, 15  // PEDTYPE_GANG9 - Russian Mafia
        jz isAtk     // jump if above is true
        cmp eax, 7  // PEDTYPE_GANG1 - Ballas
        jl isNotAtk // jump if less than above
        cmp eax, 12 // PEDTYPE_GANG6 - Mafia
        jg isNotAtk // jump if greater than above
        cmp eax, 8  // PEDTYPE_GANG2 - Grove Street Families
        jz isNotAtk // jump if above is true
    isAtk:
        jmp loc_1568EAF
    isNotAtk:
        jmp loc_15690D6
    }
}
void __declspec(naked) GangDriveBySpeech1()
{
    __asm
    {
        push 0
        fstp st
        push 0
        push 0
        push 3F800000h
        push 0
        push 31
        mov ecx, ebp
        call CPed::Say
        push 0
        push 0
        push 0
        push 3F800000h
        push 0
        push 184
        mov ecx, ebp
        call CPed::Say
        jmp loc_62D978
    }
}

void __declspec(naked) GangDriveBySpeech2()
{
    __asm
    {
        push 0
        push 0
        push 0
        push 3F800000h
        push 0
        push 30
        mov ecx, ebp
        call CPed::Say
        push 0
        push 0
        push 0
        push 3F800000h
        push 0
        push 184
        mov ecx, ebp
        call CPed::Say
        jmp loc_62D9A3
    }
}

void __declspec(naked) RecruitGangCheck1Proxy()
{
    __asm
    {
        push edi
        call IsRecruitable
        pop ecx
        test al, al
        jnz isRecruitable
        jmp loc_60C8B6
    isRecruitable:
        jmp loc_60C8D5
    }
}

void __declspec(naked) RecruitGangCheck2Proxy()
{
    __asm
    {
        push ebp
        call IsRecruitable
        pop ecx
        test al, al
        jnz isRecruitable
        jmp loc_60D400
    isRecruitable:
        jmp loc_60D409
    }
}

void __declspec(naked) RecruitGangCheck3Proxy()
{
    __asm
    {
        push ebp
        call IsRecruitable
        pop ecx
        test al, al
        jnz isRecruitable
        jmp loc_60D4DC
    isRecruitable:
        jmp loc_60D4D2
    }
}

void __declspec(naked) HelpAgainstCopsProxy()
{
    __asm
    {
        push esi
        call IsRecruitable
        pop ecx
        test al, al
        jnz willingHelp
        jmp loc_60D592
    willingHelp:
        jmp loc_60D541
    }
}

bool IsRecruitable(CPed* ped)
{
    ePedType pedType = (ePedType)ped->m_nPedType;
    float missionRelationshipStat = CStats::GetStatValue(119);
    switch (pedType) {
    case PED_TYPE_GANG2: // Grove, they're always recruitable
        return true;
    case PED_TYPE_GANG7: // Triads, recruitable after "Fish in a Barrel"
        if (missionRelationshipStat < 4.0f)
            return false;
        else
            return true;
    case PED_TYPE_GANG8: // Aztecas, recruitable after "Los Desperados"
        if (missionRelationshipStat < 5.0f)
            return false;
        else
            return true;
    default:
        return false;
    }
}

void GangTaunt(CPed* ped, CPed* victim)
{
    ePedType pedType = (ePedType)ped->m_nPedType;
    ePedType victimPedType = (ePedType)victim->m_nPedType;
    if (pedType == PED_TYPE_GANG1 && victim->IsPlayer()) // Dunno why the Ballas does not have specific taunt quotes for CJ while other gangs have
        ped->Say(4, 0, 1.0f, 0, 0, 0); // So, I'll generalize him with the Grove instead. Even then, only the BALLAS3 model has such quotes...
    else {
        switch (victimPedType) {
        case PED_TYPE_PLAYER1: // Victim: CJ - Custom one
            ped->Say(20, 0, 1.0f, 0, 0, 0);
            break;
        case PED_TYPE_GANG1: // Victim: Ballas
            ped->Say(1, 0, 1.0f, 0, 0, 0);
            break;
        case PED_TYPE_GANG2: // Victim: Grove
            ped->Say(4, 0, 1.0f, 0, 0, 0);
            break;
        case PED_TYPE_GANG3: // Victim: Vagos - This part of the code is missing in the original game
            ped->Say(2, 0, 1.0f, 0, 0, 0); // So that's why you don't hear any taunting quotes against the Vagos
            break;
        case PED_TYPE_GANG4: // Victim: Rifas - This part of the code is actually exist in the original game
            ped->Say(7, 0, 1.0f, 0, 0, 0); // The problem was something related to bank lookup table, So that's why you don't hear any taunting quotes against the Rifas
            break;
        case PED_TYPE_GANG5: // Victim: Da Nang - This part of the code is actually exist in the original game
            ped->Say(8, 0, 1.0f, 0, 0, 0); // The problem was something related to bank lookup table, So that's why you don't hear any taunting quotes against the Da Nang
            break;
        case PED_TYPE_GANG6: // Victim: Mafia - This part of the code is actually exist in the original game
            ped->Say(6, 0, 1.0f, 0, 0, 0); // The problem is outstandingly something else: No gangs actually dared to taunt them! Not even their sworn enemies, the Triads!
            break;
        case PED_TYPE_GANG7: // Victim: Triads - This part of the code is actually exist in the original game
            ped->Say(5, 0, 1.0f, 0, 0, 0); // The problem was something related to bank lookup table, So that's why you don't hear any taunting quotes against the Triads
            break;
        case PED_TYPE_GANG8: // Victim: Aztecas
            ped->Say(3, 0, 1.0f, 0, 0, 0); // Despite all that shit about previous gangs, this one is surprisingly normal
            break;
        case PED_TYPE_GANG9: // Victim: Russians
//          ped->Say(0, 0, 1.0f, 0, 0, 0); // Nope, nothing about them sadly
            break;
        case PED_TYPE_GANG10: // Victim: Bikers
//          ped->Say(0, 0, 1.0f, 0, 0, 0); // Nope, nothing about them sadly
            break;
        }
    }
}

void GangTauntAtk(CPed* ped, CPed* victim)
{
    ePedType pedType = (ePedType)ped->m_nPedType;
    ePedType victimPedType = (ePedType)victim->m_nPedType;
    float missionRelationshipStat = CStats::GetStatValue(119);
    switch (pedType) {
    case PED_TYPE_GANG2: // Grove, the one who shoot
        switch (victimPedType) {
        case PED_TYPE_GANG1: // Ballas, the victim
            ped->Say(17, 0, 1.0f, 0, 0, 0); // Taunt quotes against the Ballas
            ped->Say(183, 0, 1.0f, 0, 0, 0); // Shootout quotes against the Ballas
            break;
        case PED_TYPE_GANG3: // Vagos, the victim
            ped->Say(18, 0, 1.0f, 0, 0, 0); // Taunt quotes against the Vagos
            ped->Say(185, 0, 1.0f, 0, 0, 0); // Shootout quotes against the Vagos
            break;
        case PED_TYPE_GANG8: // Aztecas, the victim
            ped->Say(19, 0, 1.0f, 0, 0, 0); // Taunt quotes against the Aztecas
            ped->Say(186, 0, 1.0f, 0, 0, 0); // Shootout quotes against the Aztecas
            break;
        default: // Anyone else the victim
            ped->Say(184, 0, 1.0f, 0, 0, 0); // Default shootout quotes
            break;
        }
        break;
    case PED_TYPE_GANG7: // Triads, the one who shoot
        switch (victimPedType) {
        case PED_TYPE_GANG5: // Da Nang, the victim
            ped->Say(73, 0, 1.0f, 0, 0, 0); // Taunt quotes against the Da Nang
            ped->Say(184, 0, 1.0f, 0, 0, 0); // Default shootout quotes
            break;
        default: // Anyone else the victim
            ped->Say(184, 0, 1.0f, 0, 0, 0); // Default shootout quotes
            break;
        }
        break;
    default: // Anyone else who shoot
        switch (victimPedType) {
        case PED_TYPE_PLAYER1: // CJ, the victim
            if ((pedType == PED_TYPE_GANG4 && missionRelationshipStat < 3.0f) || // Rifa - Before "Outrider"
                (pedType == PED_TYPE_GANG5 && missionRelationshipStat < 2.0f)) // Da Nang - Before "Mountain Cloud Boys"
                ped->Say(184, 0, 1.0f, 0, 0, 0); // Default shootout quotes
            else {
                ped->Say(20, 0, 1.0f, 0, 0, 0); // Taunt quotes against CJ
                ped->Say(187, 0, 1.0f, 0, 0, 0); // Shootout quotes against the Grove
            }
            break;
        case PED_TYPE_GANG2: // Grove, the victim
            ped->Say(187, 0, 1.0f, 0, 0, 0); // Shootout quotes against the Grove
            break;
        default: // Anyone else the victim
            ped->Say(184, 0, 1.0f, 0, 0, 0); // Default shootout quotes
            break;
        }
        break;
    }
}

void __declspec(naked) DontGetOffendedProxy()
{
	__asm
	{
		push eax
		call DontGetOffended
		pop ecx
		test al, al
		jnz dontGetOffended
		jmp loc_43BF65 // has high chance of being offended, then attack CJ
	dontGetOffended:
		jmp loc_43C074 // don't get offended
	}
}

bool DontGetOffended(CPed *ped)
{
	CPedAcquaintance pedAcquaintance = ped->m_acquaintance;
	ePedType pedType = (ePedType)ped->m_nPedType;
    float missionRelationshipStat = CStats::GetStatValue(119);
    switch (pedType) {
	case PED_TYPE_GANG2: // Grove Street 4 Life
		return true;
    // these below should be something else, due to them having friendly conversations with CJ
	case PED_TYPE_GANG3: // Vagos gang, ese!
	case PED_TYPE_GANG4: // You can't see the Rifa!
	case PED_TYPE_GANG5: // Kiss my Da Nang ass!
	case PED_TYPE_GANG10: // You wanna beating, huh? HUH?!
        return false;
    case PED_TYPE_GANG7: // Fuckhead!
        if (missionRelationshipStat < 2.0f) // After "Mountain Cloud Boys"
            return false;
        else
            return true;
    case PED_TYPE_GANG8: // VLA right here, homie
        if (missionRelationshipStat < 5.0f) // After "Los Desperados"
            return false;
        else
            return true;
    default:
		return false;
	}
}

void __declspec(naked) WhereYouFromProxy()
{
	__asm
	{
		push esi
		call IsWhereYouFrom
		pop ecx
		test al, al
		jnz whereYouFrom
		jmp loc_43B341 // 0, 7
	whereYouFrom:
		jmp loc_43B346 // 8, 10
	}
}

bool IsWhereYouFrom(CPed *ped)
{
	CPedAcquaintance pedAcquaintance = ped->m_acquaintance;
	ePedType pedType = (ePedType)ped->m_nPedType;
    float missionRelationshipStat = CStats::GetStatValue(119);
    switch (pedType) {
	case PED_TYPE_GANG1: // Balla, fool
		return true;
    // these below should be something else, due to them having friendly conversations with CJ
	case PED_TYPE_GANG3: // Vagos gang, ese!
        return true;
	case PED_TYPE_GANG4: // You can't see the Rifa!
        if (missionRelationshipStat < 3.0f) // After "Outrider"
			return false;
		else
			return true;
	case PED_TYPE_GANG5: // Kiss my Da Nang ass!
        if (missionRelationshipStat < 2.0f) // After "Mountain Cloud Boys"
			return false;
		else
			return true;
	case PED_TYPE_GANG8: // VLA right here, homie
        if (missionRelationshipStat < 1.0f) // After "Cesar Vialpando"
			return true;
		else
			return false;
	default:
		return false;
	}
}

void __declspec(naked) CopPistol()
{
    __asm
    {
        mov[esi + 718h], 2
        jmp loc_5DDCD9
    }
}

void __declspec(naked) IDidntDoItOfficer()
{
    __asm
    {
        mov ecx, [edi+8]
        test ecx, ecx
        jz defaultPain
        mov ecx, [edi+8]
        cmp [ecx+598h], 6
        jnz defaultPain
        cmp [esi+598h], 6
        jz defaultPain
        push 1
        mov ecx, esi
        call CPed::DisablePedSpeech
        mov ecx, esi
        call CPed::EnablePedSpeech
        push 0
        push 0
        push 0
        push 3F800000h
        push 3E8h
        push 69
        mov ecx, esi
        call CPed::Say
        test ax, ax
        jge subSomething
    defaultPain:
        push 0
        push 0
        push 0
        push 3F800000h
        push 0
        mov ecx, esi
        push 159h
        call CPed::Say
    subSomething:
        jmp loc_620AEC
    }
}