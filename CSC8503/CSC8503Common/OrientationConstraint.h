#pragma once
#include "Constraint.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class OrientationConstraint : public Constraint {
		public:
			OrientationConstraint(GameObject* a, GameObject* b, Quaternion r) {
				objectA = a;
				objectB = b;
				rotation = r;
			}
			~OrientationConstraint() {}

			void UpdateConstraint(float dt) override;
		protected:
			GameObject* objectA;
			GameObject* objectB;
			Quaternion rotation;
		};
	}
}