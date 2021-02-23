#include "../../Common/Window.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "TutorialGame.h"
#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"
#include "../CSC8503Common/BehaviourNode.h"
#include "../CSC8503Common/BehaviourAction.h"
using namespace NCL;
using namespace CSC8503;

Window* w;
bool restart = false;
extern unsigned short players = USHRT_MAX;

void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;

	State* A = new State([&](float dt)->void
		{
			std::cout << "I'm in state A!" << std::endl;
			data++;
		}
	);

	State* B = new State([&](float dt)->void
		{
			std::cout << "I'm in state B!" << std::endl;
			data--;
		}
	);

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool
		{
			return data > 10;
		}
	);

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool
		{
			return data < 0;
		}
	);

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}
}

class PauseScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Press [U] to unpause game!" << std::endl;
	}
};

class GameScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		w->ShowConsole(true);
		while (w->UpdateWindow()) {
			pauseReminder -= dt;

			// reminding user of pause command
			if (pauseReminder < 0) {
				pauseReminder += 120.0f;
				std::cout << "Press [P] to pause the game!" << std::endl;
			}

			// losing condition
			if (g->gameOver) {
				w->ShowOSPointer(true);
				w->LockMouseToWindow(false);
				w->ShowConsole(true);
				std::string response;
				do {
					std::cout << "Would you like to try again? (Y/N)" << std::endl;
					std::cin >> response;

					if (response == "Y" || response == "y") {
						restart = true;
						return PushdownResult::Pop;
					}
					else if (response == "N" || response == "n") {
						return PushdownResult::Pop;
					}
				} while (response != "Y" || response != "y" || response != "N" || response != "n");
			}

			// exit game
			if (w->GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
				w->ShowOSPointer(true);
				w->LockMouseToWindow(false);
				w->ShowConsole(true);
				return PushdownResult::Pop;
			}

			// pause game
			if (w->GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
				w->ShowConsole(true);
				*newState = new PauseScreen();
				return PushdownResult::Push;
			}

			dt = w->GetTimer()->GetTimeDeltaSeconds();
			if (dt > 0.1f) {
				std::cout << "Skipping large time delta" << std::endl;
				continue;
			}

			if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
				w->ShowConsole(true);
			}
			if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
				w->ShowConsole(false);
			}
			if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
				w->SetWindowPosition(0, 0);
			}

			w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

			g->UpdateGame(dt);
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		w->UpdateWindow();
		w->ShowOSPointer(false);
		w->LockMouseToWindow(true);
		w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	};

protected:
	float pauseReminder = 1.0f;
	TutorialGame* g = new TutorialGame();
};

class IntroScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (w->GetKeyboard()->KeyPressed(KeyboardKeys::SPACE)) {
			do {
				std::cout << "How many CPUs would you like to play against? [1 ~ 2]";
				try {
					std::cin >> players;

					if (players >= 1 && players <= 2) {
						*newState = new GameScreen();
						return PushdownResult::Push;
					}
					else {
						throw (players);
					}
				}
				catch (unsigned short players) {
					std::cout << "Please select a value within the range of players!" << std::endl;
					players = USHRT_MAX;
				}

			} while (players < 1 || players > 4 || w->GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE));
		}
		if (w->GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}

	void OnAwake() override {
		std::cout << "Welcome to (not) Hall Guys: Total Wipeout!" << std::endl;
		std::cout << "Press \"Space\" to begin or \"Esc\" to quit!" << std::endl;
	}
};

int main() {
	w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);
	if (!w->HasInitialised()) {
		return -1;
	}
	srand(time(0));

	PushdownMachine machine(new IntroScreen());
	while (w->UpdateWindow()) {
		if (!machine.Update(w->GetTimer()->GetTimeDeltaSeconds())) {
			return -1;
		}
	}
	Window::DestroyGameWindow();
}