#include "GameState.h"

GameState::GameState() : mQuest(0), mQuestMask(0), mUnlockMask(0) {
}

bool GameState::AdvanceQuest() {
	if (mQuest < gQuestCount) {
		mQuest++;
		Save();
		return true;
	}
	return false;
}

bool GameState::IsActive(const TriggerData& trigger) const {
	return (
		(trigger.questBegin == 0xff || trigger.questBegin <= mQuest) &&
		(trigger.questEnd == 0xff || trigger.questEnd >= mQuest) &&
		(
			trigger.flagId == 0 || 
			(trigger.flagId <= 32 && (mQuestMask & (1<<(trigger.flagId-1))) == 0) || 
			((mUnlockMask & (1<<(trigger.flagId-33))) == 0)
		)
	);
}

bool GameState::IsActive(uint8_t questId, uint8_t flagId) const {
	return (
		(questId == 0xff || questId == mQuest) &&
		(
			flagId == 0 ||
			(flagId <= 32 && (mQuestMask & (1<<(flagId-1))) == 0) || 
			((mUnlockMask & (1<<(flagId-33))) == 0)
		)
	);
}

bool GameState::FlagTrigger(const TriggerData& trigger) {
	if (IsActive(trigger) && trigger.flagId) {
		if (trigger.flagId <= 32) {
			mQuestMask |= (1 << (trigger.flagId-1));
		} else {
			mUnlockMask |= (1 << (trigger.flagId-33));
		}
		Save();
		return true;
	}
	return false;
}

bool GameState::Flag(uint8_t questId, uint8_t flagId) {
	if (IsActive(questId, flagId)) {
		if (flagId <= 32) {
			mQuestMask |= (1 << (flagId-1));
		} else {
			mUnlockMask |= (1 << (flagId-33));
		}
		Save();
		return true;
	}
	return false;
}

void GameState::Save() {
	// TODO
}

void GameState::Load() {
	// TODO
}

