#pragma once
#include "..\CSC8503Common\GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class StateGameObject : public GameObject {
		public:
			StateGameObject();
			~StateGameObject();

			virtual void Update(float dt);

		protected:
			void MoveLeft(float dt);
			void MoveRight(float dt);

			StateGameObject* AddStateObjectToWorld(const Vector3& position);
			StateGameObject* testStateObject;

			StateMachine* stateMachine;
			float counter;
		};
	}
}
