#include "AgentMgr.h"

#include "GWAPIMgr.h"


BYTE* GWAPI::AgentMgr::dialog_log_ret_ = NULL;
DWORD GWAPI::AgentMgr::last_dialog_id_ = 0;


GWAPI::GW::AgentArray GWAPI::AgentMgr::GetAgentArray() {
	return *(GW::AgentArray*)MemoryMgr::agArrayPtr;
}

std::vector<GWAPI::GW::Agent*> * GWAPI::AgentMgr::GetParty() {
	std::vector<GW::Agent*>* party = new std::vector<GW::Agent*>(GetPartySize());
	GW::AgentArray agents = GetAgentArray();

	for (size_t i = 0; i < agents.size(); ++i) {
		if (agents[i]->Allegiance == 1
			&& (agents[i]->TypeMap & 0x20000)) {

			party->push_back(agents[i]);
		}
	}

	return party;
}

size_t GWAPI::AgentMgr::GetPartySize() {
	size_t ret = 0;
	size_t* retptr = NULL;
	for (BYTE i = 0; i < 3; ++i) {
		retptr = MemoryMgr::ReadPtrChain<size_t*>(MemoryMgr::GetContextPtr(), 3, 0x4C, 0x54, 0x0C + 0x10 * i);
		if (retptr == NULL)
			return NULL;
		else
			ret += *retptr;
	}
	return ret;
}

DWORD GWAPI::AgentMgr::GetDistance(GW::Agent* a, GW::Agent* b) {
	return (DWORD)sqrtl((DWORD)(a->X - b->X) * (DWORD)(a->X - b->X) + (DWORD)(a->Y - b->Y) * (DWORD)(a->Y - b->Y));
}

DWORD GWAPI::AgentMgr::GetSqrDistance(GW::Agent* a, GW::Agent* b) {
	return (DWORD)(a->X - b->X) * (DWORD)(a->X - b->X) + (DWORD)(a->Y - b->Y) * (DWORD)(a->Y - b->Y);
}

GWAPI::AgentMgr::AgentMgr(GWAPIMgr& api) : GWCAManager(api) {
	change_target_ = (ChangeTarget_t)MemoryMgr::ChangeTargetFunction;
	move_ = (Move_t)MemoryMgr::MoveFunction;
	dialog_log_ret_ = (BYTE*)hk_dialog_log_.Detour(MemoryMgr::DialogFunc, (BYTE*)AgentMgr::detourDialogLog, 9);
}

void GWAPI::AgentMgr::RestoreHooks() {
	hk_dialog_log_.Retour();
}

void GWAPI::AgentMgr::ChangeTarget(GW::Agent* Agent)
{
	api().Gamethread().Enqueue(change_target_, Agent->Id,0);
}

void GWAPI::AgentMgr::Move(float X, float Y, DWORD ZPlane /*= 0*/)
{
	static GW::GamePos* buf = new GW::GamePos();

	buf->x = X;
	buf->y = Y;
	buf->zplane = ZPlane;

	api().Gamethread().Enqueue(move_, buf);
}

void GWAPI::AgentMgr::Move(const GW::GamePos& pos)
{
	static GW::GamePos* buf = new GW::GamePos();

	*buf = pos;

	api().Gamethread().Enqueue(move_, buf);
}

void GWAPI::AgentMgr::Dialog(DWORD id)
{
	api().CtoS().SendPacket(0x8, 0x35, id);
}

GWAPI::GW::PartyMemberArray GWAPI::AgentMgr::GetPartyMemberArray()
{
	return *MemoryMgr::ReadPtrChain<GW::PartyMemberArray*>(MemoryMgr::GetContextPtr(), 3, 0x4C, 0x54, 0x4);
}

bool GWAPI::AgentMgr::GetIsPartyLoaded()
{
	if (api().Map().GetInstanceType() == GwConstants::InstanceType::Loading) return false;

	GW::PartyMemberArray party = GetPartyMemberArray();
	if (!party.valid()) return false;

	for (DWORD i = 0; i < party.size(); i++){
		if ((party[i].state & 1) == 0) return false;
	}

	return true;
}

GWAPI::GW::MapAgentArray GWAPI::AgentMgr::GetMapAgentArray()
{
	return *MemoryMgr::ReadPtrChain<GW::MapAgentArray*>(MemoryMgr::GetContextPtr(), 2, 0x2C, 0x7C);
}

GWAPI::GW::Agent* GWAPI::AgentMgr::GetPlayer() {
	GW::AgentArray agents = GetAgentArray();
	int id = GetPlayerId();
	if (agents.valid() && id > 0) {
		return agents[id];
	} else {
		return nullptr;
	}
}

GWAPI::GW::Agent* GWAPI::AgentMgr::GetTarget() {
	GW::AgentArray agents = GetAgentArray();
	int id = GetTargetId();
	if (agents.valid() && id > 0) {
		return agents[id];
	} else {
		return nullptr;
	}
}

void GWAPI::AgentMgr::GoNPC(GW::Agent* Agent, DWORD CallTarget /*= 0*/)
{
	api().CtoS().SendPacket(0xC, 0x33, Agent->Id, CallTarget);
}

void GWAPI::AgentMgr::GoPlayer(GW::Agent* Agent)
{
	api().CtoS().SendPacket(0x8, 0x2D, Agent->Id);
}

void GWAPI::AgentMgr::GoSignpost(GW::Agent* Agent, BOOL CallTarget /*= 0*/)
{
	api().CtoS().SendPacket(0xC, 0x4B, Agent->Id, CallTarget);
}

void GWAPI::AgentMgr::CallTarget(GW::Agent* Agent)
{
	api().CtoS().SendPacket(0xC, 0x1C, 0xA, Agent->Id);
}

void __declspec(naked) GWAPI::AgentMgr::detourDialogLog()
{
	_asm MOV GWAPI::AgentMgr::last_dialog_id_, ESI
	_asm JMP GWAPI::AgentMgr::dialog_log_ret_
}

DWORD GWAPI::AgentMgr::GetAmountOfPlayersInInstance()
{
	// -1 because the 1st array element is nil
	return MemoryMgr::ReadPtrChain<DWORD>(MemoryMgr::GetContextPtr(), 3, 0x2C, 0x814, 0) - 1;
}

wchar_t* GWAPI::AgentMgr::GetPlayerNameByLoginNumber(DWORD loginnumber)
{
	return MemoryMgr::ReadPtrChain<wchar_t*>(MemoryMgr::GetContextPtr(), 4, 0x2C, 0x80C, 0x28 + 0x4C * loginnumber, 0);
}

DWORD GWAPI::AgentMgr::GetAgentIdByLoginNumber(DWORD loginnumber)
{
	return MemoryMgr::ReadPtrChain<DWORD>(MemoryMgr::GetContextPtr(), 4, 0x2C, 0x80C, 0x4C * loginnumber, 0);
}

bool GWAPI::AgentMgr::GetPartyTicked() {
	GW::PartyMemberArray party = GetPartyMemberArray();
	if (party.valid()) {
		for (size_t i = 0; i < party.size(); ++i) {
			if ((party[i].state & 2) == 0) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
}

bool GWAPI::AgentMgr::GetTicked(DWORD index) {
	GW::PartyMemberArray party = GetPartyMemberArray();
	if (party.valid()) {
		return (party[index].state & 2) != 0;
	} else {
		return false;
	}
}

bool GWAPI::AgentMgr::GetTicked() {
	GW::PartyMemberArray party = GetPartyMemberArray();
	GW::Agent* me = GetPlayer();
	if (party.valid() && me) {
		for (DWORD i = 0; i < party.size();i++){
			if (party[i].loginnumber == me->LoginNumber){
				return (party[i].state & 2) != 0;
			}
		}
		return false;
	} else {
		return false;
	}
}

void GWAPI::AgentMgr::Tick(bool flag)
{
	api().CtoS().SendPacket(0x8, 0xA9, flag);
}


const char* GWAPI::AgentMgr::GetProfessionAcronym(GwConstants::Profession profession) {
	switch (profession) {
	case GwConstants::Profession::Warrior: return "W";
	case GwConstants::Profession::Ranger: return "R";
	case GwConstants::Profession::Monk: return "Mo";
	case GwConstants::Profession::Necromancer: return "N";
	case GwConstants::Profession::Mesmer: return "Me";
	case GwConstants::Profession::Elementalist: return "E";
	case GwConstants::Profession::Assassin: return "A";
	case GwConstants::Profession::Ritualist: return "Rt";
	case GwConstants::Profession::Paragon: return "P";
	case GwConstants::Profession::Dervish: return "D";
	default: return "";
	}
}

GWAPI::GW::PlayerArray GWAPI::AgentMgr::GetPlayerArray()
{
	return *MemoryMgr::ReadPtrChain<GW::PlayerArray*>(MemoryMgr::GetContextPtr(), 2, 0x2C, 0x80C);
}
