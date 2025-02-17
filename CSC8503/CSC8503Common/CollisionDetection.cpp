#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "../../Common/Vector2.h"
#include "../../Common/Window.h"
#include "../../Common/Maths.h"
#include "Debug.h"

#include <list>

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray&r, const Plane&p, RayCollision& collisions) {
	float ln = Vector3::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}
	
	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector3::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r, GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume	= object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
		case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume, collision); break;
		case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume, collision); break;
		case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume, collision); break;
		case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray&r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1.0f, -1.0f, -1.0f);

	for (int i = 0; i < 3; ++i) { // get best 3 intersections
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}

	float bestT = tVals.GetMaxElement();
	if (bestT < 0.0f) {
		return false; // no backwards rays
	}

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f;

	for (int i = 0; i < 3; ++i) {
		if (intersection[i] + epsilon < boxMin[i] || intersection[i] - epsilon > boxMax[i]) {
			return false;
		}
	}

	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	// Debug::DrawLine(r.GetPosition(), intersection, Debug::RED, 10.0f);
	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray&r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray&r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());
	Vector3 localRayPos = r.GetPosition() - position;

	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());

	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);

	if (collided) {
		collision.collidedAt = transform * collision.collidedAt + position;
	}
	return false;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	Vector3 capsulePos = worldTransform.GetPosition();

	// Get the direction between the ray origin and the capsule origin.
	Vector3 dir = (capsulePos - r.GetPosition());

	// Then project the capsule's origin onto our ray direction vector.
	float capsuleProj = Vector3::Dot(dir, r.GetDirection());

	if (capsuleProj < 0.0f)
		return false; // did not hit ray

	// Get closest point on ray line to capsule
	Vector3 collisionPoint = r.GetPosition() + (r.GetDirection() * capsuleProj);
	float currentMousePos = (collisionPoint - capsulePos).Length();
	float bottomPos = (collisionPoint - Vector3(capsulePos.x, capsulePos.y - volume.GetRadius(), capsulePos.z)).Length();
	float topPos = (collisionPoint - Vector3(capsulePos.x, capsulePos.y + volume.GetRadius(), capsulePos.z)).Length();
	AABBVolume a = AABBVolume(Vector3(volume.GetRadius(), volume.GetHalfHeight() / 2, volume.GetRadius()));
	bool inAABB = currentMousePos > a.GetHalfDimensions().x && currentMousePos > a.GetHalfDimensions().y && currentMousePos > a.GetHalfDimensions().z;
	if (bottomPos > volume.GetRadius() && topPos > volume.GetRadius() && inAABB) {
		return false;
	}

	collision.rayDistance = capsuleProj;
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);
	return true;
}

bool CollisionDetection::RaySphereIntersection(const Ray&r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	// Get the direction between the ray origin and the sphere origin.
	Vector3 dir = (spherePos - r.GetPosition());
	
	// Then project the sphere's origin onto our ray direction vector.
	float sphereProj = Vector3::Dot(dir, r.GetDirection());

	if (sphereProj < 0.0f) {
		return false; // point is behind the ray!
	}

	// Get closest point on ray line to sphere
	Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);

	float sphereDist = (point - spherePos).Length();

	if (sphereDist > sphereRadius) {
		return false;
	}

	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));

	collision.rayDistance = sphereProj - offset;
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);
	return true;
}

Matrix4 GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const Camera& cam) {
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	float aspect	= screenSize.x / screenSize.y;
	float fov		= cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane  = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const Camera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2 screenSize	= Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	//std::cout << "Ray Direction:" << c << std::endl;

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov*PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f*(nearPlane*farPlane) / neg_depth;

	m.array[0]  = aspect / h;
	m.array[5]  = tan(fov*PI_OVER_360);

	m.array[10] = 0.0f;
	m.array[11] = 1.0f / d;

	m.array[14] = 1.0f / e;

	m.array[15] = -c / (d*e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
Matrix4::Translation(position) *
Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
Matrix4::Rotation(pitch, Vector3(1, 0, 0));

return iview;
}

/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const Camera &c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) return false;

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	switch (pairType)
	{
		case VolumeType::AABB:
			return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
			break;
		case VolumeType::Sphere:
			return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
			break;
		case VolumeType::OBB:
			return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
			break;
		case VolumeType::Capsule:
			return CapsuleIntersection((CapsuleVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
			break;
	}

	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		std::swap(collisionInfo.a, collisionInfo.b);
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		std::swap(collisionInfo.a, collisionInfo.b);
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	return false;
}

/// <summary>Checks whether 2 objects are intersecting on every axis.</summary>
/// <param name='posA'>The world position of object A.</param>
/// <param name='posB'>The world position of object B.</param>
/// <param name='boxSizeA'>The size of object A.</param>
/// <param name='boxSizeB'>The size of object B.</param>
/// <returns>Boolean which returns whether objects A and B are intersecting.</returns>
bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& boxSizeA, const Vector3& boxSizeB) {
	Vector3 delta = posB - posA;
	Vector3 totalSize = boxSizeA + boxSizeB;
	if (abs(delta.x) < totalSize.x && abs(delta.y) < totalSize.y && abs(delta.z) < totalSize.z) {
		return true;
	}
	return false;
}

/// <summary>Detects and performs an axis-aligned bounding box (AABB) test against two objects with AABB volumes.</summary>
/// <param name='volumeA'>The AABB bounding volume of object A.</param>
/// <param name='worldTransformA'>The world transform of object A.</param>
/// <param name='volumeB'>The AABB bounding volume of object B.</param>
/// <param name='worldTransformB'>The world transform of object B.</param>
/// <param name='collisionInfo'>Struct containing data about the collision. At this stage, it should only contain both objects being tested.</param>
/// <returns>Boolean which returns whether objects A and B are intersecting, adds more information about the collision to <c>collisionInfo</c></returns>
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();

	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);

	if (overlap) {
		static const Vector3 faces[6] = {
			Vector3(-1, 0, 0), Vector3(1, 0, 0), Vector3(0, -1, 0), Vector3(0, 1, 0), Vector3(0, 0, -1), Vector3(0, 0, 1)
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;
		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] = {
			(maxB.x - minA.x),
			(maxA.x - minB.x),
			(maxB.y - minA.y),
			(maxA.y - minB.y),
			(maxB.z - minA.z),
			(maxA.z - minB.z)
		};

		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; i++) {
			if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		return true;
	}
	return false;
}

/// <summary>Detects whether two objects with sphere volumes are intersecting.</summary>
/// <param name='volumeA'>The sphere bounding volume of object A.</param>
/// <param name='worldTransformA'>The world transform of object A.</param>
/// <param name='volumeB'>The sphere bounding volume of object B.</param>
/// <param name='worldTransformB'>The world transform of object B.</param>
/// <param name='collisionInfo'>Struct containing data about the collision. At this stage, it should only contain both objects being tested.</param>
/// <returns>Boolean which returns whether objects A and B are intersecting, adds more information about the collision to <c>collisionInfo</c></returns>
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

	float deltaLength = delta.Length();

	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);
		Vector3 normal = delta.Normalised();
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true; // we're colliding
	}
	return false;
}

/// <summary>Detects whether an object with an axis-aligned bounding box (AABB) is colliding with a sphere.</summary>
/// <param name='volumeA'>The AABB bounding volume of object A.</param>
/// <param name='worldTransformA'>The world transform of object A.</param>
/// <param name='volumeB'>The sphere bounding volume of object B.</param>
/// <param name='worldTransformB'>The world transform of object B.</param>
/// <param name='collisionInfo'>Struct containing data about the collision. At this stage, it should only contain both objects being tested.</param>
/// <returns>Boolean which returns whether objects A and B are intersecting, adds more information about the collision to <c>collisionInfo</c></returns>
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxSize = volumeA.GetHalfDimensions();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	
	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();

	if (distance < volumeB.GetRadius()) {
		Vector3 collisionNormal = localPoint.Normalised();
		float penetration = (volumeB.GetRadius() - distance);

		Vector3 localA = Vector3();
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}
	return false;
}

/// <summary>Detects whether an object with a capsule bounding volume is colliding with another capsule bounding volume. Borrowed from: https://wickedengine.net/2020/04/26/capsule-collision-detection/</summary>
/// <param name='volumeA'>The capsule bounding volume of object A.</param>
/// <param name='worldTransformA'>The world transform of object A.</param>
/// <param name='volumeB'>The capsule bounding volume of object B.</param>
/// <param name='worldTransformB'>The world transform of object B.</param>
/// <param name='collisionInfo'>Struct containing data about the collision. At this stage, it should only contain both objects being tested.</param>
/// <returns>Boolean which returns whether objects A and B are intersecting, adds more information about the collision to <c>collisionInfo</c></returns>
bool CollisionDetection::CapsuleIntersection(const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 volATip = Vector3(worldTransformA.GetPosition().x,
		worldTransformA.GetPosition().y + volumeA.GetHalfHeight() + volumeA.GetRadius(),
		worldTransformA.GetPosition().z);
	Vector3 volABase = Vector3(worldTransformA.GetPosition().x,
		worldTransformA.GetPosition().y - volumeA.GetHalfHeight() - volumeA.GetRadius(),
		worldTransformA.GetPosition().z);
	Vector3 volBTip = Vector3(worldTransformB.GetPosition().x,
		worldTransformB.GetPosition().y + volumeB.GetHalfHeight() + volumeB.GetRadius(),
		worldTransformB.GetPosition().z);
	Vector3 volBBase = Vector3(worldTransformB.GetPosition().x,
		worldTransformB.GetPosition().y - volumeB.GetHalfHeight() - volumeB.GetRadius(),
		worldTransformB.GetPosition().z);

	// Capsule A
	Vector3 aNormal = Vector3(volATip - volABase).Normalised();
	Vector3 aLineEndOffset = aNormal * volumeA.GetRadius();
	Vector3 aEndpointA = volABase + aLineEndOffset;
	Vector3 aEndpointB = volATip - aLineEndOffset;

	// Capsule B
	Vector3 bNormal = Vector3(volBTip - volBBase).Normalised();
	Vector3 bLineEndOffset = bNormal * volumeB.GetRadius();
	Vector3 bEndpointA = volBBase + bLineEndOffset;
	Vector3 bEndpointB = volBTip - bLineEndOffset;

	// vectors between line endpoints
	Vector3 v0 = bEndpointA - aEndpointA;
	Vector3 v1 = bEndpointB - aEndpointA;
	Vector3 v2 = bEndpointA - aEndpointB;
	Vector3 v3 = bEndpointB - aEndpointB;

	// squared distances
	float d0 = Vector3::Dot(v0, v0);
	float d1 = Vector3::Dot(v1, v1);
	float d2 = Vector3::Dot(v2, v2);
	float d3 = Vector3::Dot(v3, v3);

	// Select best endpoint on capsule A
	Vector3 bestA;
	(d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) ? bestA = aEndpointB : bestA = aEndpointA;
	
	Vector3 bestB = ClosestPointOnLine(bEndpointA, bEndpointB, bestA);
	bestA = ClosestPointOnLine(aEndpointA, aEndpointB, bestB);

	Vector3 normal = bestB - bestA;
	float length = normal.Length();
	normal = normal.Normalised();
    float penetration = volumeA.GetRadius() + volumeB.GetRadius() - length;

	if (penetration > 0) {
		bestA = normal * volumeA.GetRadius();
		bestB = -normal * volumeB.GetRadius();
		collisionInfo.AddContactPoint(bestA, bestB, normal, penetration);
		return true;
	}
	return false;
}

// Capsule - Capsule Collision (https://wickedengine.net/2020/04/26/capsule-collision-detection/)
Vector3 CollisionDetection::ClosestPointOnLine(const Vector3& capsuleBase, const Vector3& capsuleTip, const Vector3& point) {
	Vector3 capsuleHeight = capsuleTip - capsuleBase;
	float t = Vector3::Dot(point - capsuleBase, capsuleHeight) / Vector3::Dot(capsuleHeight, capsuleHeight);
	float maxt = max(t, 0);
	return capsuleBase + capsuleHeight * min(maxt, 1);
}

bool CollisionDetection::OBBIntersection(
	const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
}

bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	return false;
}