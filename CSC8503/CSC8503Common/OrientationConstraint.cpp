#include "GameObject.h"
#include "OrientationConstraint.h"

using namespace NCL::CSC8503;

/*
void OrientationConstraint::UpdateConstraint(float dt) {
	Quaternion relativePos = objectA->GetTransform().GetOrientation() - objectB->GetTransform().GetOrientation();
	Vector3 offset = rotation.ToEuler() - relativePos.ToEuler();

	if (abs(offset.x) > rotation.x || abs(offset.y) > rotation.y || abs(offset.z) > rotation.z) {
		Vector3 offsetDir = relativePos.ToEuler().Normalised();

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();
		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

		if (constraintMass > 0.0f) {
			// how much of the relative force is affecting the constraint
			float velocityDot = Vector3::Dot(relativeVelocity, offsetDir);
			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;

			float lambda = -(velocityDot + bias) / constraintMass;

			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;

			physA->ApplyLinearImpulse(aImpulse); // multiplied by mass here
			physB->ApplyLinearImpulse(bImpulse); // multiplied by mass here
		}
	}
}
*/