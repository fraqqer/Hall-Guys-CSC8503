#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourSequence : public BehaviourNodeWithChildren {
public:
	BehaviourSequence(const std::string& nodeName) : BehaviourNodeWithChildren(nodeName) {

	}
	~BehaviourSequence() {}
	BehaviourState Execute(float dt) override {
		// std::cout << "Executing sequence: " << name << std::endl;
		for (auto& i : childNodes) {
			BehaviourState nodeState = i->Execute(dt);

			switch (nodeState) {
			case BehaviourState::Failure:
				continue;
			case BehaviourState::Success:
			case BehaviourState::Ongoing:
			{
				currentState = nodeState;
				return currentState;
			}
			}
		}
		return BehaviourState::Success;
	}
};