#pragma once
#include <string>

enum class BehaviourState {
	Initialise,
	Failure,
	Success,
	Ongoing
};

class BehaviourNode
{
public:
	virtual ~BehaviourNode() {}
	virtual BehaviourState Execute(float dt) = 0;
	virtual void Reset() = 0;

protected:
	BehaviourState currentState;
	std::string name;

	BehaviourNode(const std::string& nodeName) {
		currentState = BehaviourState::Initialise;
		name = nodeName;
	}
};

